#include "canif.h"
#include "can.h"
#include "usart.h"
/**
 * @brief CAN过滤器初始化函数
 * @retval 无
 * @note 使用掩码模式，16位过滤器，FilterBank0，全部接收(0x0 + 0x0掩码)
 *       SlaveStartFilterBank =14 双CAN模式(CAN1+CAN2)，CAN2从14号过滤器bank开始
 */
void Can_Filter_Init(void)
{
	CAN_FilterTypeDef Can_FilterConfig;	// 定义CAN过滤器配置结构体
	
	Can_FilterConfig.FilterBank = 0;                     // 使用0号过滤器bank
	Can_FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 过滤器模式：ID掩码模式
	Can_FilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;// 过滤器位宽：使用2个16bit过滤器
	Can_FilterConfig.FilterIdHigh = 0x0;                // 过滤器ID高16位
	Can_FilterConfig.FilterIdLow = 0x0;                 // 过滤器ID低16位
	Can_FilterConfig.FilterMaskIdHigh = 0x0;            // 掩码高16位，0代表不校验对应位
	Can_FilterConfig.FilterMaskIdLow = 0x0;             // 掩码低16位，0代表不校验对应位
	/* ID和掩码全部置0，所有CAN报文直接通过过滤器，不筛选ID */
	Can_FilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0; // 收到报文存入FIFO0
	Can_FilterConfig.FilterActivation = CAN_FILTER_ENABLE;    // 使能该过滤器
	Can_FilterConfig.SlaveStartFilterBank = 14;              // 双CAN模式，CAN2起始过滤器bank编号为14
	                                                         // 仅对CAN2生效，不影响CAN1的过滤器配置
	// 将过滤器配置写入CAN外设寄存器
	if(HAL_CAN_ConfigFilter(&hcan, &Can_FilterConfig) != HAL_OK)
	{
		Error_Handler();	// 过滤器配置失败，进入错误处理函数
	}
	
	HAL_CAN_Start(&hcan);	// 启动CAN外设，开启CAN收发功能
}

/**
 * @brief CAN发送函数，发送标准数据帧，阻塞等待发送完成
 * @param id    标准帧ID (11位)
 * @param len   数据长度DLC，范围0~8
 * @param buf   待发送数据缓冲区指针
 * @retval 0发送成功，1发送失败
 * @note 阻塞等待3个发送邮箱全部空闲后才返回
 */
uint32_t Can_Send_Msg(uint32_t id, uint32_t len,uint8_t *buf)
{
    CAN_TxHeaderTypeDef TxMessage;  // CAN发送头部结构体，存放报文控制信息
    uint32_t Tx_Mail = CAN_TX_MAILBOX0; // 选中使用的发送邮箱
    
    TxMessage.StdId = id;                // 设置标准帧ID(11bit)
    TxMessage.IDE = CAN_ID_STD;          // IDE=0：标准帧；IDE=1：扩展帧
    TxMessage.DLC = len;                 // DLC 数据长度，取值范围0~8
    TxMessage.RTR  = CAN_RTR_DATA;       // RTR=0：数据帧；RTR=1：远程请求帧
    
    // 将报文写入CAN发送邮箱，加入发送队列
    if(HAL_CAN_AddTxMessage(&hcan, &TxMessage,buf, &Tx_Mail) != HAL_OK)
    {
        return 1; // 添加发送报文失败，返回1
    }
    // 循环阻塞等待，直到3个发送邮箱全部空闲，代表发送完成
    while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) !=3);
    return 0; // 发送成功返回0
}

/**
 * @brief 查询方式读取CAN报文，从FIFO0读取
 * @param id     输出参数，保存读到的标准ID
 * @param buf    输出缓冲区，存放读到的CAN数据
 * @retval >0 收到报文，返回DLC(数据长度1~8)；0：FIFO无数据
 * @note 查询FIFO判断有无报文，读到数据后自动释放FIFO
 */
uint32_t Can_Recv_Msg(uint32_t *id, uint8_t *buf)
{
    CAN_RxHeaderTypeDef RxMessage;  // CAN接收头部结构体，存放接收报文信息
    // 判断FIFO0是否存在报文；FIFO填充级别=0代表没有收到报文
    if(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_FILTER_FIFO0) == 0)
        return 0;
    // 读取FIFO0中的报文，读取后自动释放FIFO空间
    if (HAL_CAN_GetRxMessage(&hcan, CAN_FILTER_FIFO0, &RxMessage, buf) != HAL_OK)
        return 0;
    *id = RxMessage.StdId;  // 将读到的标准ID赋值给外部传入的id指针
    return RxMessage.DLC;   // 返回报文数据长度DLC
}

