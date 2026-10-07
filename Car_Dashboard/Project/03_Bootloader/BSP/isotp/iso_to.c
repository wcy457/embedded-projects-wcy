#include "iso_to.h"
#include "canif.h"
#include "boot_manager.h"
#include "can.h"

/* ISO-TP接收状态机枚举 */
typedef enum {
    RX_STATE_IDLE = 0,         // 空闲状态，等待新传输
    RX_STATE_WAIT_CF,          // 收到首帧，等待接收后续连续帧
} rx_state_t;

static rx_state_t rx_state = RX_STATE_IDLE;    // 当前接收状态，初始为空闲
static uint16_t rx_total_len = 0;              // 本次ISO-TP传输的固件总字节长度
static uint16_t rx_recv_len = 0;               // 当前已经收到的数据字节数
static uint8_t  rx_seq = 0;                    // 连续帧序号(0~15循环)，用于校验分包顺序
static uint8_t  rx_buf[4096];                 // ISO-TP接收缓冲区，最大4KB，适配Flash一页大小
static uint8_t  ota_finished = 0;              // OTA传输完成标记，1=固件接收完毕


/**
 * @brief ISO-TP协议栈初始化，复位所有接收状态、缓存变量
 */
void iso_tp_init(void)
{
    rx_state = RX_STATE_IDLE;
    rx_total_len = 0;
    rx_recv_len = 0;
    rx_seq = 0;
    ota_finished = 0;
}

/**
 * @brief 发送ISO-TP流控帧FC，通知发送端发送策略
 * @param flow_status 流控状态
 *                    0:FC_CONTINUE 继续发送
 *                    1:FC_WAIT     请等待
 *                    2:FC_ABORT    溢出，终止本次传输
 */
static void send_fc(uint8_t flow_status)
{
	uint8_t fc[8] = {0};
	fc[0] = ISO_TP_FC | flow_status;  // 高4bit=FC帧类型，低4bit为流控状态
	fc[1] = 0x00;                     // block size:0 代表不限制连续帧块数量，一次性发完
	fc[2] = 0x00;                     // STmin:最小帧间隔0ms，发送方不延时
	// CAN发送：参数顺序 id, len, buf
	Can_Send_Msg(CAN_ID_OTA_RECV, 8, fc);
}

/**
 * @brief 处理ISO-TP单帧SF，数据很短，一包CAN报文传完
 * @param data CAN报文数据指针
 * @param dlc  CAN报文有效数据长度
 */
static void handle_sf(uint8_t *data, uint8_t dlc)
{
	
	uint8_t sf_len = data[0] & 0x0F; // 单帧：第一个字节低4bit=有效数据长度
	// 合法性校验：长度不能超过7，也不能超过CAN报文剩余字节
	if (sf_len > 7 || sf_len > dlc - 1) return;
	// 取出有效数据，调用上层回调
	iso_tp_on_receive(&data[1], sf_len);
}

/**
 * @brief 处理ISO-TP首帧FF，长分包传输的第一包，携带总长度
 * @param data CAN报文数据指针
 */
static void handle_ff(uint8_t *data)
{
    // 首帧：低4bit + data[1] 拼接出本次传输总长度
    rx_total_len = ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
    // 判断总长度是否超出接收缓冲区大小，溢出则发送FC终止
    if (rx_total_len > sizeof(rx_buf)) {
        send_fc(2);
        rx_state = RX_STATE_IDLE;
        return;
    }
    rx_recv_len = 0;
    // 拷贝首帧携带的6字节有效数据到接收缓存
    for (uint8_t i = 0; i < 6; i++) {
        rx_buf[rx_recv_len++] = data[2 + i];
    }
    rx_seq = 1;                 // 下一个连续帧序号必须为1
    rx_state = RX_STATE_WAIT_CF;// 切换状态：等待连续帧CF
    send_fc(0);                 // 发送流控帧，允许发送方继续发连续帧
}

/**
 * @brief 处理ISO-TP连续帧CF，分包后续数据包
 * @param data CAN报文数据指针
 * @param dlc CAN报文有效数据长度
 */
static void handle_cf(uint8_t *data, uint8_t dlc)
{
    // 状态校验：不在等待CF状态直接丢弃报文
    if (rx_state != RX_STATE_WAIT_CF) return;
    uint8_t seq = data[0] & 0x0F; // 获取当前连续帧序号
    // 序号不匹配，分包乱序，直接丢弃，回到空闲状态
    if (seq != rx_seq) {
        rx_state = RX_STATE_IDLE;
        return;
    }
    rx_seq = (rx_seq + 1) & 0x0F; // 序号+1，0~15循环
    uint8_t data_len = dlc - 1;   // 除去第1字节帧头，剩余为有效载荷

    // 防止拷贝超出总长度
    if (rx_recv_len + data_len > rx_total_len) {
        data_len = rx_total_len - rx_recv_len;
    }
    // 将连续帧内有效数据拷贝到接收缓冲区
    for (uint8_t i = 0; i < data_len; i++) {
        rx_buf[rx_recv_len++] = data[1 + i];
    }

    // 全部数据接收完成
    if (rx_recv_len >= rx_total_len) {
        rx_state = RX_STATE_IDLE;
        iso_tp_on_receive(rx_buf, rx_total_len); // 回调上层，处理完整固件包
    }
}

/**
 * @brief ISO-TP服务主函数，轮询调用，读取CAN报文并分发处理
 */
void iso_tp_server(void)
{
    uint32_t recv_id;
    uint8_t  rxdata[8];
    uint8_t  recv_len;
    recv_len = Can_Recv_Msg(&recv_id, rxdata); // 读取一帧CAN报文
    if (recv_len == 0) return;                 // 没有收到报文直接返回

    /* 检测OTA传输结束指令报文 ID=CAN_ID_CALL_OTA，魔数BEADBEEF + 02 */
    if (recv_id == CAN_ID_CALL_OTA) {
        if (rxdata[0] == 0xBE && rxdata[1] == 0xAD &&
            rxdata[2] == 0xBE && rxdata[3] == 0xEF &&
            rxdata[4] == 0x02) {
            ota_finished = 1;  // 标记OTA全部固件传输完成
            return;
        }
    }

    /* 只处理OTA固件ISO-TP报文，其他CAN报文直接过滤 */
    if (recv_id != CAN_ID_OTA_SEND) return;

    // 取出报文首字节高4bit，判断帧类型 SF/FF/CF
    uint8_t frame_type = rxdata[0] & 0xF0;
    switch (frame_type) {
    case ISO_TP_SF: handle_sf(rxdata, recv_len); break;  // 单帧处理
    case ISO_TP_FF: handle_ff(rxdata);           break;  // 首帧处理
    case ISO_TP_CF: handle_cf(rxdata, recv_len); break;  // 连续帧处理
    default: break; // 未知帧类型直接丢弃
    }
}

/**
 * @brief 获取OTA传输完成标志
 * @retval 1:传输完成；0:传输中/未开始
 */
uint8_t iso_tp_is_ota_finished(void)
{
    return ota_finished;
}

