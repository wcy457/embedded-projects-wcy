#ifndef __CAN_HANDLE_H__
#define __CAN_HANDLE_H__
#include "main.h"


/**
 * @brief UI变量结构体，保存仪表数据+更新标志位
 * @param ui_value: 存储对应的传感器/状态数值
 * @param ui_freshflag: 数据刷新标志 1=收到新CAN报文，数据已更新；0=无新数据
 */
typedef struct
{
		uint32_t ui_value;
		uint8_t ui_freshflag;
} ui_Typedef;

/* CAN报文ID定义 */
#define CAN_ID_UI        0x0B4   // 仪表UI数据报文ID
#define CAN_MSG_HEAD     0xAF    // UI报文帧头校验字节
#define CAN_ID_CALL_OTA  0x0B3   // OTA升级指令报文ID

/**
 * @brief UI数据类型枚举，标识CAN报文中携带的数据类别
 */
typedef enum
{
		E_UI_TYPE_RPM = 0x1,            // 发动机转速
    E_UI_TYPE_WATER_TEMP,           // 水温
    E_UI_TYPE_SAFTY_BELT,           // 安全带状态
    E_UI_TYPE_LIGHT,                // 灯光状态
    E_UI_TYPE_TURN_LIGHT,           // 转向灯状态
    E_UI_TYPE_TURN_DATE,            // 日期数据
    E_UI_TYPE_TURN_TIME,            // 时间数据		
} E_Ui_Type;

uint8_t get_ota_flag(void);
uint8_t ui_get_rpm_value(uint32_t *rpm_val);
uint8_t ui_get_water_temp_value(uint32_t *water_temp_val);
uint8_t ui_get_safty_belt_value(uint32_t *safty_belt_val);
uint8_t ui_get_light_value(uint32_t *light_val);
uint8_t ui_get_turn_light_value(uint32_t *turn_light_val);
uint8_t ui_get_date_value(uint32_t *date_val);
uint8_t ui_get_time_value(uint32_t *time_val);

void Can_Handle(void);

#endif

