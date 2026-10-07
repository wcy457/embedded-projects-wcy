#include "can_handle.h"
#include "main.h"
#include "canif.h"
#include "can_handle.h"
#include "usart.h"
#include <stdio.h>

ui_Typedef ui_rpm;                // 发动机转速缓存结构体
ui_Typedef ui_water_temp;         // 水温缓存结构体
ui_Typedef ui_safty_belt;         // 安全带状态缓存结构体
ui_Typedef ui_light;              // 灯光状态缓存结构体
ui_Typedef ui_turn_light;         // 转向灯状态缓存结构体
ui_Typedef ui_date;               // 日期缓存结构体（打包32bit）
ui_Typedef ui_time;               // 时间缓存结构体（打包32bit）

static uint8_t tp_ota_flag = 0;   // OTA升级标志，静态全局，0=无OTA指令，1=收到OTA启动指令

/**
 * @brief 获取OTA升级标志
 * @retval tp_ota_flag：1 需要进入OTA升级流程；0 正常仪表运行
 */
uint8_t get_ota_flag(void)
{
    return tp_ota_flag;            // 返回OTA标志给上层调用者
}

/**
 * @brief 读取发动机转速
 * @param rpm_val [输出] 用于存放读取到的转速数值
 * @retval 0：读取成功，本次拿到新CAN数据；1：没有新数据
 * @note 读取成功后自动清零ui_freshflag，保证一帧CAN数据只被UI读取一次，避免画面重复刷新旧值
 */
uint8_t ui_get_rpm_value(uint32_t *rpm_val)
{
    if(ui_rpm.ui_freshflag == 1)  // 判断是否存在未读取的新转速数据
    {
        ui_rpm.ui_freshflag = 0;   // 清除新数据标记，表示数据已经被上层读取
        *rpm_val = ui_rpm.ui_value;// 将缓存的转速赋值到输出指针变量
        return 0;                  // 返回0，代表读取成功，拿到新数据
    }
    else
    {
        return 1;                  // 返回1，没有未读取的新数据
    }
}

/**
 * @brief 读取水温
 * @param water_temp_val [输出] 水温数值
 * @retval 0：读取成功，有新数据；1：无新数据
 */
uint8_t ui_get_water_temp_value(uint32_t *water_temp_val)
{
    if(ui_water_temp.ui_freshflag == 1) // 判断是否存在未读取的新水温数据
    {
        ui_water_temp.ui_freshflag = 0; // 清除新数据标记
        *water_temp_val = ui_water_temp.ui_value; // 将缓存水温赋值给输出变量
        return 0;                        // 读取成功
    }
    else
    {
        return 1;                        // 无新数据
    }
}

/**
 * @brief 读取安全带状态
 * @param safty_belt_val [输出] 安全带状态值，0未系，1已系（协议自定义）
 * @retval 0：读取成功，有新数据；1：无新数据
 */
uint8_t ui_get_safty_belt_value(uint32_t *safty_belt_val)
{
    if(ui_safty_belt.ui_freshflag == 1) // 判断安全带是否有未读新数据
    {
        ui_safty_belt.ui_freshflag = 0;  // 清除刷新标志
        *safty_belt_val = ui_safty_belt.ui_value; // 输出状态值
        return 0;                         // 读取成功
    }
    else
    {
        return 1;                         // 无新数据
    }
}

/**
 * @brief 读取灯光状态
 * @param light_val [输出] 灯光状态值（近光/远光/示宽灯，协议自定义编码）
 * @retval 0：读取成功，有新数据；1：无新数据
 */
uint8_t ui_get_light_value(uint32_t *light_val)
{
    if(ui_light.ui_freshflag == 1)       // 判断灯光是否存在未读新数据
    {
        ui_light.ui_freshflag = 0;        // 清除刷新标志
        *light_val = ui_light.ui_value;    // 输出灯光状态
        return 0;                          // 读取成功
    }
    else
    {
        return 1;                          // 无新数据
    }
}

/**
 * @brief 读取转向灯状态
 * @param turn_light_val [输出] 转向灯状态，左转向/右转向/双闪（协议自定义编码）
 * @retval 0：读取成功，有新数据；1：无新数据
 */
uint8_t ui_get_turn_light_value(uint32_t *turn_light_val)
{
    if(ui_turn_light.ui_freshflag == 1)  // 判断转向灯是否有未读新数据
    {
        ui_turn_light.ui_freshflag = 0;   // 清除刷新标志
        *turn_light_val = ui_turn_light.ui_value; // 输出转向灯状态
        return 0;                         // 读取成功
    }
    else
    {
        return 1;                         // 无新数据
    }
}

/**
 * @brief 读取打包后的日期数据
 * @param date_val [输出] 拼接好的32位日期
 * @retval 0：读取成功，有新数据；1：无新数据
 * @note CAN报文中rx_data[2]~rx_data[5]共4字节，拼接为uint32_t，存放年月日信息
 */
uint8_t ui_get_date_value(uint32_t *date_val)
{
    if(ui_date.ui_freshflag == 1)        // 判断日期是否存在未读新数据
    {
        ui_date.ui_freshflag = 0;         // 清除刷新标志
        *date_val = ui_date.ui_value;     // 输出打包后的32位日期
        return 0;                         // 读取成功
    }
    else
    {
        return 1;                         // 无新数据
    }
}

/**
 * @brief 读取打包后的时间数据
 * @param time_val [输出] 拼接好的32位时间
 * @retval 0：读取成功，有新数据；1：无新数据
 * @note CAN报文中rx_data[2]~rx_data[4]共3字节，拼接进uint32_t，存放时分信息
 */
uint8_t ui_get_time_value(uint32_t *time_val)
{
    if(ui_time.ui_freshflag == 1)         // 判断时间是否存在未读新数据
    {
        ui_time.ui_freshflag = 0;         // 清除刷新标志
        *time_val = ui_time.ui_value;      // 输出打包后的32位时间
        return 0;                          // 读取成功
    }
    else
    {
        return 1;                          // 无新数据
    }
}

/**
 * @brief CAN报文业务处理入口函数
 * @note 在主循环周期性调用，调用底层canif接口查询接收报文，解析报文数据更新UI缓存、OTA标志
 * @attention 当前是查询式接收，不是中断接收，必须放在while(1)循环里反复调用；报文量大时有丢包风险
 */
void Can_Handle(void)
{
    uint32_t recv_len;                    // 接收报文有效数据长度DLC，0~8，0代表无报文
    uint32_t recv_id;                     // 存储收到CAN报文的标准ID
    uint8_t rx_data[8] = {0};             // CAN接收缓冲区，CAN2.0单帧最多8字节，初始化为0

    recv_len = Can_Recv_Msg(&recv_id, rx_data); //调用底层接口查询CAN，读取报文，返回数据长度

    if(recv_len > 0)                      // recv_len>0，成功收到一帧CAN报文，开始解析
    {
        if(recv_id == CAN_ID_UI)           // 判断报文ID，是否为仪表UI报文(0x0B4)
        {
            if(rx_data[0] == CAN_MSG_HEAD) // 校验帧头0xAF，过滤无效杂报文
            {
                if(rx_data[1] == E_UI_TYPE_RPM) // 判断数据类型：发动机转速
                {
                    ui_rpm.ui_value = rx_data[2];        // 将第2字节赋值给转速缓存
                    ui_rpm.ui_freshflag = 1;             // 置新数据标志，通知UI刷新
                }
                else if(rx_data[1] == E_UI_TYPE_WATER_TEMP) // 判断数据类型：水温
                {
                    ui_water_temp.ui_value = rx_data[2];   // 读取水温单字节
                    ui_water_temp.ui_freshflag = 1;        // 标记新数据
                }
                else if(rx_data[1] == E_UI_TYPE_SAFTY_BELT) // 判断数据类型：安全带
                {
                    ui_safty_belt.ui_value = rx_data[2];    // 读取安全带状态
                    ui_safty_belt.ui_freshflag = 1;        // 标记新数据
                }
                else if(rx_data[1] == E_UI_TYPE_LIGHT)      // 判断数据类型：灯光
                {
                    ui_light.ui_value = rx_data[2];         // 读取灯光状态
                    ui_light.ui_freshflag = 1;              // 标记新数据
                }
                else if(rx_data[1] == E_UI_TYPE_TURN_LIGHT) // 判断数据类型：转向灯
                {
                    ui_turn_light.ui_value = rx_data[2];    // 读取转向灯状态
                    ui_turn_light.ui_freshflag = 1;         // 标记新数据
                }
                else if(rx_data[1] == E_UI_TYPE_TURN_DATE)  // 判断数据类型：日期
                {
                    // rx_data[2]最高字节，rx_data[5]最低字节，大端拼接32位日期
                    ui_date.ui_value = ((uint32_t)rx_data[2] << 24) | ((uint32_t)rx_data[3] << 16) | ((uint32_t)rx_data[4] << 8) | ((uint32_t)rx_data[5] << 0);
                    ui_date.ui_freshflag = 1;                // 标记新日期数据
                }
                else if(rx_data[1] == E_UI_TYPE_TURN_TIME)  // 判断数据类型：时间
                {
                    // rx_data[2]~rx_data[4]拼接32bit时间，最低字节补0
                    ui_time.ui_value = ((uint32_t)rx_data[2] << 24) | ((uint32_t)rx_data[3] << 16) | ((uint32_t)rx_data[4] << 8) ;
                    ui_time.ui_freshflag = 1;                // 标记新时间数据
                }
            }
        }
        else if (recv_id == CAN_ID_CALL_OTA)                 // 判断报文ID，是否为OTA指令报文(0x0B3)
        {
            // OTA魔术字校验，连续4字节 0xBE,0xAD,0xBE,0xEF，防止误触发升级
            if ((rx_data[0] == 0xBE) &&
                (rx_data[1] == 0xAD) &&
                (rx_data[2] == 0xBE) &&
                (rx_data[3] == 0xEF))
            {
                if (rx_data[4] == 0x01)                     // 判断OTA启动指令
                {
                    tp_ota_flag = 1;                         // 设置OTA标志，上层检测后跳转Bootloader
                    printf("[tp server] Get ota flag\r\n");  // 串口打印收到OTA指令
                }
            }
        }
    }
}
