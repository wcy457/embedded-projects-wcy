/**
 * ============================================================
 *  智能门锁完整项目 - 基于STM32F103C8T6
 * ============================================================
 *  硬件资源(依据智能锁原理图V2.0):
 *    触摸按键 BS8116A  - I2C(PB6-SCL, PB7-SDA), 中断(PB8)
 *    指纹模块 MG200    - USART2, 电源(PB9), 检测(PC13)
 *    RFID模块 MFRC522  - SPI2, CS(PA6), RES(PA7)
 *    OLED显示          - 模拟SPI, DC(PA11), RES(PA12), DS(PA15), CLK(PB13), MOSI(PB15)
 *    语音模块          - DATA(PB0), BUSY(PB1)
 *    电机/门锁         - LOCK+(PA4), LOCK-(PA5)
 *    蓝牙              - USART3, PWR(PB12), KEY(PA8)
 *    EEPROM AT24C04    - I2C(PB6-SCL, PB7-SDA)
 *    LED指示灯         - LED1(PB3), LED2(PB2)
 *    RTC               - 内部RTC
 * ============================================================
 */

#include "sys.h"
#include "delay.h"
#include "led.h"
#include "OLED.h"
#include "usart.h"
#include "bluetooth.h"
#include "at24cxx.h"
#include "stdio.h"
#include "rtc.h"
#include "BS8116A.h"
#include "MG200.h"
#include "spi.h"
#include "MFRC522.h"
#include "motor.h"
#include "voic.h"

/* ==================== EEPROM地址规划 ==================== */
#define EEPROM_ADMIN_PASS_ADDR    0x00   // 管理员密码存储地址(6字节)
#define EEPROM_DOOR_PASS_ADDR     0x10   // 开门密码存储地址(6字节)
#define EEPROM_CARD_ADDR          0x20   // 门禁卡号存储地址(4字节x5张=20字节)
#define EEPROM_CARD_NUM_ADDR      0x40   // 已注册卡数量(1字节)
#define EEPROM_VOLUME_ADDR        0x41   // 音量设置(1字节)
#define EEPROM_MAGIC_ADDR         0x50   // 初始化标志(2字节)
#define EEPROM_MAGIC_VALUE        0xA55A // 初始化标志值

/* ==================== 系统参数 ==================== */
#define PASSWORD_LEN              6      // 密码长度
#define MAX_CARD_NUM              5      // 最大注册卡数
#define MAX_ERROR_COUNT           3      // 最大密码错误次数
#define LOCK_OPEN_TIME            5      // 开锁时间(秒)

/* ==================== 系统状态定义 ==================== */
typedef enum {
    STATE_IDLE,              // 空闲/待机状态
    STATE_MAIN_MENU,         // 主菜单
    STATE_INPUT_PASSWORD,    // 输入密码
    STATE_ADMIN_MENU,        // 管理员菜单
    STATE_CHANGE_ADMIN_PASS, // 修改管理员密码
    STATE_SET_DOOR_PASS,     // 设置开门密码
    STATE_REGISTER_FINGER,   // 登记指纹
    STATE_REGISTER_CARD,     // 登记门禁卡
    STATE_DELETE_CARD,        // 删除门禁卡
    STATE_DELETE_FINGER,      // 删除指纹
    STATE_CHANGE_TIME,       // 修改时间
    STATE_FINGER_MATCH,      // 指纹比对
    STATE_CARD_MATCH,        // 门禁卡比对
} SystemState;

/* ==================== 全局变量 ==================== */
SystemState g_state = STATE_IDLE;       // 当前系统状态
u8 g_admin_password[PASSWORD_LEN] = {'1','2','3','4','5','6'};  // 默认管理员密码
u8 g_door_password[PASSWORD_LEN]  = {'0','0','0','0','0','0'};  // 默认开门密码
u8 g_input_buffer[PASSWORD_LEN];        // 输入缓冲区
u8 g_input_index = 0;                   // 输入索引
u8 g_error_count = 0;                   // 密码错误计数
u8 g_is_admin_mode = 0;                 // 是否为管理员模式
u8 g_menu_index = 0;                    // 菜单索引
u32 g_idle_timer = 0;                   // 空闲计时器
u8 g_card_count = 0;                    // 已注册卡数量
RTC_DATES g_rtc_date;                   // RTC日期时间

/* ==================== 函数声明 ==================== */
void System_Init(void);
void EEPROM_LoadConfig(void);
void EEPROM_SaveAdminPassword(void);
void EEPROM_SaveDoorPassword(void);
void EEPROM_SaveCardCount(void);
void Show_IdleScreen(void);
void Show_PasswordInput(u8 is_admin);
void Show_MainMenu(void);
void Show_AdminMenu(void);
void Show_Message(u8 *line1, u8 *line2, u16 delay_ms_val);
u8   Password_InputProcess(u8 key);
void Door_Open(void);
void Admin_MenuProcess(u8 key);
void Main_MenuProcess(u8 key);
u8   Card_Read(u8 *card_id);
u8   Card_CheckRegistered(u8 *card_id);
void Card_Register(u8 *card_id);
void Bluetooth_Process(void);
void Alarm_Trigger(void);

/*****************************************************
 * 函数名称: System_Init
 * 功能描述: 系统所有外设初始化
 * 参数: void
 * 返回值: void
 *****************************************************/
void System_Init(void)
{
    /* 系统滴答定时器初始化 */
    SysTick_Init();

    /* 串口1初始化 - 用于调试 */
    USART1_Init(115200);
    printf("===== Smart Lock System V2.0 =====\r\n");

    /* LED初始化 */
    LED_Init();
    LED1_OFF;
    LED2_OFF;

    /* OLED初始化 */
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, 16, (u8*)"Smart Lock V2.0");
    OLED_ShowString(0, 2, 16, (u8*)"Initializing...");

    /* 语音模块初始化 */
    Voic_Init();

    /* 电机/门锁初始化 */
    Motor_Init();

    /* 触摸按键初始化 */
    BS81xx_Init1();

    /* SPI2初始化(用于RFID) */
    SPI2_Init();

    /* RFID模块初始化 */
    MFRC_Init();

    /* 指纹模块初始化 */
    MG200_Init();

    /* 蓝牙初始化 */
    Bluetooth_Init();
    BLUE_PWR_ON;    // 开启蓝牙电源

    /* AT24C04 EEPROM初始化 */
    AT24CXX_Init();

    /* RTC初始化 - 默认时间 */
    g_rtc_date.year  = 2026;
    g_rtc_date.month = 6;
    g_rtc_date.day   = 25;
    g_rtc_date.hour  = 12;
    g_rtc_date.min   = 0;
    g_rtc_date.sec   = 0;
    MyRTC_Init(g_rtc_date);

    /* 从EEPROM加载配置 */
    EEPROM_LoadConfig();

    /* 播放欢迎语音 */
    Voic_SendData(CONNECT_SUCCESS);
    while(VO_BUSY_HL);

    printf("System Init OK!\r\n");
}

/*****************************************************
 * 函数名称: EEPROM_LoadConfig
 * 功能描述: 从EEPROM加载保存的配置数据
 * 参数: void
 * 返回值: void
 *****************************************************/
void EEPROM_LoadConfig(void)
{
    u16 magic;
    u8 buf[6];
    u8 temp;

    /* 读取初始化标志 */
    AT24CXX_Read_Bytes(EEPROM_MAGIC_ADDR, 2, (u8*)&magic);

    if(magic == EEPROM_MAGIC_VALUE)
    {
        /* 已初始化过,读取保存的密码 */
        AT24CXX_Read_Bytes(EEPROM_ADMIN_PASS_ADDR, PASSWORD_LEN, g_admin_password);
        AT24CXX_Read_Bytes(EEPROM_DOOR_PASS_ADDR, PASSWORD_LEN, g_door_password);
        AT24CXX_Read_Byte(EEPROM_CARD_NUM_ADDR, &g_card_count);
        printf("Config loaded from EEPROM\r\n");
    }
    else
    {
        /* 首次运行,保存默认配置 */
        EEPROM_SaveAdminPassword();
        EEPROM_SaveDoorPassword();
        g_card_count = 0;
        EEPROM_SaveCardCount();
        magic = EEPROM_MAGIC_VALUE;
        AT24CXX_Write_Bytes(EEPROM_MAGIC_ADDR, 2, (u8*)&magic);
        printf("First run, saved default config\r\n");
    }
}

/*****************************************************
 * 函数名称: EEPROM_SaveAdminPassword
 * 功能描述: 保存管理员密码到EEPROM
 *****************************************************/
void EEPROM_SaveAdminPassword(void)
{
    AT24CXX_Write_Bytes(EEPROM_ADMIN_PASS_ADDR, PASSWORD_LEN, g_admin_password);
}

/*****************************************************
 * 函数名称: EEPROM_SaveDoorPassword
 * 功能描述: 保存开门密码到EEPROM
 *****************************************************/
void EEPROM_SaveDoorPassword(void)
{
    AT24CXX_Write_Bytes(EEPROM_DOOR_PASS_ADDR, PASSWORD_LEN, g_door_password);
}

/*****************************************************
 * 函数名称: EEPROM_SaveCardCount
 * 功能描述: 保存已注册卡数量到EEPROM
 *****************************************************/
void EEPROM_SaveCardCount(void)
{
    AT24CXX_Write_Byte(EEPROM_CARD_NUM_ADDR, g_card_count);
}

/*****************************************************
 * 函数名称: Show_IdleScreen
 * 功能描述: 显示空闲待机界面(OLED显示时间和日期)
 * 参数: void
 * 返回值: void
 *****************************************************/
void Show_IdleScreen(void)
{
    u8 time_str[20];
    u8 date_str[20];

    g_rtc_date = RTC_Get();

    /* 第1行: 标题 */
    OLED_ShowString(0, 0, 16, (u8*)"  Smart Lock  ");

    /* 第2行: 日期 */
    sprintf((char*)date_str, "2024-%02d-%02d",
            g_rtc_date.month, g_rtc_date.day);
    OLED_ShowString(0, 2, 16, date_str);

    /* 第3行: 时间 */
    sprintf((char*)time_str, "  %02d:%02d:%02d  ",
            g_rtc_date.hour, g_rtc_date.min, g_rtc_date.sec);
    OLED_ShowString(0, 4, 16, time_str);

    /* 第4行: 提示信息 */
    OLED_ShowString(0, 6, 16, (u8*)"Key:# Enter");
}

/*****************************************************
 * 函数名称: Show_PasswordInput
 * 功能描述: 显示密码输入界面
 * 参数: is_admin - 1:管理员密码 0:开门密码
 * 返回值: void
 *****************************************************/
void Show_PasswordInput(u8 is_admin)
{
    u8 i;
    u8 display[17];

    OLED_Clear();
    if(is_admin)
        OLED_ShowString(0, 0, 16, (u8*)"Admin Password:");
    else
        OLED_ShowString(0, 0, 16, (u8*)"Door Password:");

    /* 显示已输入的密码(用*号) */
    for(i = 0; i < PASSWORD_LEN; i++)
    {
        if(i < g_input_index)
            display[i] = '*';
        else
            display[i] = '_';
    }
    display[PASSWORD_LEN] = '\0';
    OLED_ShowString(16, 3, 16, display);

    OLED_ShowString(0, 6, 16, (u8*)"#OK *Back");
}

/*****************************************************
 * 函数名称: Show_MainMenu
 * 功能描述: 显示主菜单
 * 参数: void
 * 返回值: void
 *****************************************************/
void Show_MainMenu(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, 16, (u8*)"=== Main Menu ===");

    switch(g_menu_index % 4)
    {
        case 0:
            OLED_ShowString(0, 2, 16, (u8*)">1.Input Password");
            OLED_ShowString(0, 4, 16, (u8*)" 2.Finger Match");
            OLED_ShowString(0, 6, 16, (u8*)" 3.Card Match");
            break;
        case 1:
            OLED_ShowString(0, 2, 16, (u8*)" 1.Input Password");
            OLED_ShowString(0, 4, 16, (u8*)">2.Finger Match");
            OLED_ShowString(0, 6, 16, (u8*)" 3.Card Match");
            break;
        case 2:
            OLED_ShowString(0, 2, 16, (u8*)" 1.Input Password");
            OLED_ShowString(0, 4, 16, (u8*)" 2.Finger Match");
            OLED_ShowString(0, 6, 16, (u8*)">3.Card Match");
            break;
        case 3:
            OLED_ShowString(0, 2, 16, (u8*)" 1.Input Password");
            OLED_ShowString(0, 4, 16, (u8*)" 2.Finger Match");
            OLED_ShowString(0, 6, 16, (u8*)">4.Admin Login");
            break;
    }
}

/*****************************************************
 * 函数名称: Show_AdminMenu
 * 功能描述: 显示管理员菜单
 *****************************************************/
void Show_AdminMenu(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, 16, (u8*)"== Admin Menu ==");

    switch(g_menu_index % 6)
    {
        case 0:
            OLED_ShowString(0, 2, 16, (u8*)">1.Chg Admin Pass");
            OLED_ShowString(0, 4, 16, (u8*)" 2.Set Door Pass");
            OLED_ShowString(0, 6, 16, (u8*)" 3.Reg Finger");
            break;
        case 1:
            OLED_ShowString(0, 2, 16, (u8*)" 1.Chg Admin Pass");
            OLED_ShowString(0, 4, 16, (u8*)">2.Set Door Pass");
            OLED_ShowString(0, 6, 16, (u8*)" 3.Reg Finger");
            break;
        case 2:
            OLED_ShowString(0, 2, 16, (u8*)" 1.Chg Admin Pass");
            OLED_ShowString(0, 4, 16, (u8*)" 2.Set Door Pass");
            OLED_ShowString(0, 6, 16, (u8*)">3.Reg Finger");
            break;
        case 3:
            OLED_ShowString(0, 2, 16, (u8*)">4.Reg Card");
            OLED_ShowString(0, 4, 16, (u8*)" 5.Del Finger");
            OLED_ShowString(0, 6, 16, (u8*)" 6.Change Time");
            break;
        case 4:
            OLED_ShowString(0, 2, 16, (u8*)" 4.Reg Card");
            OLED_ShowString(0, 4, 16, (u8*)">5.Del Finger");
            OLED_ShowString(0, 6, 16, (u8*)" 6.Change Time");
            break;
        case 5:
            OLED_ShowString(0, 2, 16, (u8*)" 4.Reg Card");
            OLED_ShowString(0, 4, 16, (u8*)" 5.Del Finger");
            OLED_ShowString(0, 6, 16, (u8*)">6.Change Time");
            break;
    }
}

/*****************************************************
 * 函数名称: Show_Message
 * 功能描述: 显示提示信息
 * 参数: line1-第1行, line2-第2行, delay_ms_val-延时
 *****************************************************/
void Show_Message(u8 *line1, u8 *line2, u16 delay_ms_val)
{
    OLED_Clear();
    OLED_ShowString(0, 2, 16, line1);
    if(line2 != (u8*)0)
        OLED_ShowString(0, 4, 16, line2);
    if(delay_ms_val > 0)
        delay_ms(delay_ms_val);
}

/*****************************************************
 * 函数名称: Door_Open
 * 功能描述: 开门操作 - 语音提示+电机开门+LED指示
 *****************************************************/
void Door_Open(void)
{
    printf("Door Opening...\r\n");

    /* 语音: 欢迎回家 */
    Voic_SendData(DOOROPEN_SUCCESS);
    while(VO_BUSY_HL);

    /* LED指示 */
    LED1_ON;

    /* 显示开门成功 */
    Show_Message((u8*)"  Door Open!", (u8*)" Welcome Home!", 0);

    /* 电机开门 */
    Motor_DoorOpen(LOCK_OPEN_TIME);

    /* 恢复状态 */
    LED1_OFF;
    g_error_count = 0;
    g_state = STATE_IDLE;
    OLED_Clear();
}

/*****************************************************
 * 函数名称: Alarm_Trigger
 * 功能描述: 报警触发 - 密码错误超过3次
 *****************************************************/
void Alarm_Trigger(void)
{
    u8 i;

    printf("ALARM! Too many errors!\r\n");

    /* 语音报警 */
    Voic_SendData(Warm);
    while(VO_BUSY_HL);

    /* LED闪烁报警 */
    for(i = 0; i < 10; i++)
    {
        LED1_ON;
        LED2_ON;
        delay_ms(200);
        LED1_OFF;
        LED2_OFF;
        delay_ms(200);
    }

    /* 蓝牙发送报警信息 */
    USART3_Send_Str((u8*)"ALARM:Password error 3 times!\r\n");

    Show_Message((u8*)"!!! ALARM !!!", (u8*)"Locked 30s", 0);
    delay_ms(30000);  // 锁定30秒

    g_error_count = 0;
    g_state = STATE_IDLE;
    OLED_Clear();
}

/*****************************************************
 * 函数名称: Password_InputProcess
 * 功能描述: 密码输入处理
 * 参数: key - 按键值
 * 返回值: 1-密码验证完成 0-还在输入中
 *****************************************************/
u8 Password_InputProcess(u8 key)
{
    u8 *target_password;
    u8 i;
    u8 match = 1;

    if(key >= '0' && key <= '9')
    {
        if(g_input_index < PASSWORD_LEN)
        {
            g_input_buffer[g_input_index] = key;
            g_input_index++;

            /* 播放按键音 */
            Voic_SendData(Di);
            while(VO_BUSY_HL);

            /* 更新显示 */
            Show_PasswordInput(g_is_admin_mode);
        }
    }
    else if(key == '#')  // 确认键
    {
        if(g_input_index == PASSWORD_LEN)
        {
            /* 选择比对的密码 */
            if(g_is_admin_mode)
                target_password = g_admin_password;
            else
                target_password = g_door_password;

            /* 比对密码 */
            for(i = 0; i < PASSWORD_LEN; i++)
            {
                if(g_input_buffer[i] != target_password[i])
                {
                    match = 0;
                    break;
                }
            }

            if(match)
            {
                /* 密码正确 */
                if(g_is_admin_mode)
                {
                    printf("Admin password OK!\r\n");
                    Voic_SendData(SETTING_SUCCESS);
                    while(VO_BUSY_HL);
                    g_state = STATE_ADMIN_MENU;
                    g_menu_index = 0;
                    Show_AdminMenu();
                }
                else
                {
                    printf("Door password OK!\r\n");
                    Door_Open();
                }
                g_input_index = 0;
                return 1;
            }
            else
            {
                /* 密码错误 */
                g_error_count++;
                printf("Password Error! Count=%d\r\n", g_error_count);

                Voic_SendData(PASSWORD_ERROR);
                while(VO_BUSY_HL);

                Show_Message((u8*)"Password Error!", (u8*)"Try Again", 1500);

                g_input_index = 0;

                if(g_error_count >= MAX_ERROR_COUNT)
                {
                    Alarm_Trigger();
                    return 1;
                }

                Show_PasswordInput(g_is_admin_mode);
            }
        }
    }
    else if(key == '*')  // 取消/返回键
    {
        g_input_index = 0;
        g_state = STATE_IDLE;
        OLED_Clear();
        return 1;
    }

    return 0;
}

/*****************************************************
 * 函数名称: Main_MenuProcess
 * 功能描述: 主菜单按键处理
 *****************************************************/
void Main_MenuProcess(u8 key)
{
    if(key == '2')  // 上移
    {
        if(g_menu_index > 0) g_menu_index--;
        Show_MainMenu();
    }
    else if(key == '8')  // 下移
    {
        if(g_menu_index < 3) g_menu_index++;
        Show_MainMenu();
    }
    else if(key == '#')  // 确认
    {
        switch(g_menu_index % 4)
        {
            case 0: // 密码输入
                g_state = STATE_INPUT_PASSWORD;
                g_is_admin_mode = 0;
                g_input_index = 0;
                Show_PasswordInput(0);
                break;
            case 1: // 指纹比对
                g_state = STATE_FINGER_MATCH;
                break;
            case 2: // 门禁卡比对
                g_state = STATE_CARD_MATCH;
                break;
            case 3: // 管理员登录
                g_state = STATE_INPUT_PASSWORD;
                g_is_admin_mode = 1;
                g_input_index = 0;
                Show_PasswordInput(1);
                break;
        }
    }
    else if(key == '*')  // 返回
    {
        g_state = STATE_IDLE;
        OLED_Clear();
    }
}

/*****************************************************
 * 函数名称: Admin_MenuProcess
 * 功能描述: 管理员菜单按键处理
 *****************************************************/
void Admin_MenuProcess(u8 key)
{
    if(key == '2')  // 上移
    {
        if(g_menu_index > 0) g_menu_index--;
        Show_AdminMenu();
    }
    else if(key == '8')  // 下移
    {
        if(g_menu_index < 5) g_menu_index++;
        Show_AdminMenu();
    }
    else if(key == '#')  // 确认
    {
        switch(g_menu_index % 6)
        {
            case 0: // 修改管理员密码
                g_state = STATE_CHANGE_ADMIN_PASS;
                g_is_admin_mode = 1;
                g_input_index = 0;
                Show_Message((u8*)"Input New Admin", (u8*)"Password:", 1000);
                Show_PasswordInput(1);
                break;
            case 1: // 设置开门密码
                g_state = STATE_SET_DOOR_PASS;
                g_is_admin_mode = 0;
                g_input_index = 0;
                Show_Message((u8*)"Input New Door", (u8*)"Password:", 1000);
                Show_PasswordInput(0);
                break;
            case 2: // 登记指纹
                g_state = STATE_REGISTER_FINGER;
                break;
            case 3: // 登记门禁卡
                g_state = STATE_REGISTER_CARD;
                break;
            case 4: // 删除指纹
                g_state = STATE_DELETE_FINGER;
                break;
            case 5: // 修改时间
                g_state = STATE_CHANGE_TIME;
                break;
        }
    }
    else if(key == '*')  // 返回
    {
        g_state = STATE_IDLE;
        OLED_Clear();
    }
}

/*****************************************************
 * 函数名称: Card_Read
 * 功能描述: 读取RFID门禁卡号
 * 参数: card_id - 输出4字节卡号
 * 返回值: 1-读取成功 0-无卡
 *****************************************************/
u8 Card_Read(u8 *card_id)
{
    u8 CT[2];

    if(MFRC_PcdRequest(PICC_REQALL, CT) == MI_OK)
    {
        if(MFRC_PcdAnticoll(card_id) == MI_OK)
        {
            if(MFRC_PcdSelect(card_id) == MI_OK)
            {
                return 1;
            }
        }
    }
    return 0;
}

/*****************************************************
 * 函数名称: Card_CheckRegistered
 * 功能描述: 检查卡号是否已注册
 * 参数: card_id - 4字节卡号
 * 返回值: 1-已注册 0-未注册
 *****************************************************/
u8 Card_CheckRegistered(u8 *card_id)
{
    u8 i, j;
    u8 stored_card[4];
    u8 match;

    for(i = 0; i < g_card_count; i++)
    {
        AT24CXX_Read_Bytes(EEPROM_CARD_ADDR + i * 4, 4, stored_card);
        match = 1;
        for(j = 0; j < 4; j++)
        {
            if(card_id[j] != stored_card[j])
            {
                match = 0;
                break;
            }
        }
        if(match) return 1;
    }
    return 0;
}

/*****************************************************
 * 函数名称: Card_Register
 * 功能描述: 注册新门禁卡到EEPROM
 * 参数: card_id - 4字节卡号
 *****************************************************/
void Card_Register(u8 *card_id)
{
    if(g_card_count >= MAX_CARD_NUM)
    {
        Show_Message((u8*)"Card Full!", (u8*)"Max 5 Cards", 2000);
        return;
    }

    /* 检查是否重复 */
    if(Card_CheckRegistered(card_id))
    {
        Voic_SendData(CARD_REGISTERED);
        while(VO_BUSY_HL);
        Show_Message((u8*)"Card Registered!", (u8*)"Duplicate Card", 2000);
        return;
    }

    /* 保存卡号 */
    AT24CXX_Write_Bytes(EEPROM_CARD_ADDR + g_card_count * 4, 4, card_id);
    g_card_count++;
    EEPROM_SaveCardCount();

    Voic_SendData(SETTING_SUCCESS);
    while(VO_BUSY_HL);

    printf("Card Registered! Total=%d\r\n", g_card_count);
    Show_Message((u8*)"Register Success!", (u8*)"Card Saved", 2000);
}

/*****************************************************
 * 函数名称: Bluetooth_Process
 * 功能描述: 蓝牙数据处理 - 接收蓝牙指令进行远程控制
 *****************************************************/
void Bluetooth_Process(void)
{
    u8 buf[32];
    u8 len;

    /* 检查蓝牙是否收到数据 */
    /* 注意: 此处需要根据实际串口接收方式调整 */
    /* 简单实现: 通过USART3接收蓝牙数据 */
    /* 实际项目中建议使用中断接收 */

    /* 蓝牙开锁指令示例: 收到 "OPEN" 则开锁 */
    /* 蓝牙查询指令示例: 收到 "STATUS" 则返回状态 */
}

/*****************************************************
 * 函数名称: main
 * 功能描述: 主函数 - 智能门锁主循环
 * 参数: void
 * 返回值: int
 *****************************************************/
int main(void)
{
    u8 key;             // 触摸按键值
    u8 card_id[4];      // 门禁卡号
    u8 finger_id;       // 指纹ID
    u8 card_buf[20];    // 卡号显示缓冲区

    /* 系统初始化 */
    System_Init();

    /* 显示空闲界面 */
    Show_IdleScreen();

    /* ==================== 主循环 ==================== */
    while(1)
    {
        /* 读取触摸按键 */
        key = BS81xx_Key();

        /* 获取当前RTC时间(用于空闲界面更新) */
        g_rtc_date = RTC_Get();

        /* ==================== 状态机处理 ==================== */
        switch(g_state)
        {
            /* ---------- 空闲待机状态 ---------- */
            case STATE_IDLE:
                /* 更新时间显示(每秒) */
                Show_IdleScreen();

                /* 按#键进入主菜单 */
                if(key == '#')
                {
                    g_state = STATE_MAIN_MENU;
                    g_menu_index = 0;
                    Voic_SendData(TIPS);
                    while(VO_BUSY_HL);
                    Show_MainMenu();
                }

                /* 指纹检测 - 快速指纹开门 */
                if(MG200_DETECT)
                {
                    delay_ms(50);  // 消抖
                    if(MG200_DETECT)
                    {
                        printf("Finger detected, matching...\r\n");
                        OLED_Clear();
                        OLED_ShowString(0, 2, 16, (u8*)"Finger Matching..");

                        finger_id = MG200_Match1Nl();
                        if(finger_id != 0)
                        {
                            printf("Finger Match OK! ID=%d\r\n", finger_id);
                            Door_Open();
                        }
                        else
                        {
                            printf("Finger Match Failed!\r\n");
                            Voic_SendData(PASSWORD_ERROR);
                            while(VO_BUSY_HL);
                            Show_Message((u8*)"Finger Failed!", (u8*)"", 1500);
                            OLED_Clear();
                        }
                    }
                }

                /* RFID卡检测 - 快速刷卡开门 */
                if(Card_Read(card_id))
                {
                    printf("Card detected!\r\n");
                    if(Card_CheckRegistered(card_id))
                    {
                        printf("Card Valid! Opening door...\r\n");
                        Door_Open();
                    }
                    else
                    {
                        printf("Card Not Registered!\r\n");
                        Voic_SendData(PASSWORD_ERROR);
                        while(VO_BUSY_HL);
                        Show_Message((u8*)"Card Invalid!", (u8*)"Not Registered", 1500);
                        OLED_Clear();
                    }
                }

                /* 蓝牙处理 */
                Bluetooth_Process();
                break;

            /* ---------- 主菜单状态 ---------- */
            case STATE_MAIN_MENU:
                if(key != 0)
                    Main_MenuProcess(key);
                break;

            /* ---------- 密码输入状态 ---------- */
            case STATE_INPUT_PASSWORD:
                if(key != 0)
                    Password_InputProcess(key);
                break;

            /* ---------- 管理员菜单状态 ---------- */
            case STATE_ADMIN_MENU:
                if(key != 0)
                    Admin_MenuProcess(key);
                break;

            /* ---------- 修改管理员密码 ---------- */
            case STATE_CHANGE_ADMIN_PASS:
                if(key != 0)
                {
                    if(Password_InputProcess(key))
                    {
                        if(g_state == STATE_CHANGE_ADMIN_PASS)
                        {
                            /* 保存新密码 */
                            memcpy(g_admin_password, g_input_buffer, PASSWORD_LEN);
                            EEPROM_SaveAdminPassword();
                            printf("Admin password changed!\r\n");
                            Voic_SendData(SETTING_SUCCESS);
                            while(VO_BUSY_HL);
                            Show_Message((u8*)"Password Changed!", (u8*)"", 2000);
                            g_state = STATE_ADMIN_MENU;
                            Show_AdminMenu();
                        }
                    }
                }
                break;

            /* ---------- 设置开门密码 ---------- */
            case STATE_SET_DOOR_PASS:
                if(key != 0)
                {
                    if(Password_InputProcess(key))
                    {
                        if(g_state == STATE_SET_DOOR_PASS)
                        {
                            /* 保存新密码 */
                            memcpy(g_door_password, g_input_buffer, PASSWORD_LEN);
                            EEPROM_SaveDoorPassword();
                            printf("Door password changed!\r\n");
                            Voic_SendData(SETTING_SUCCESS);
                            while(VO_BUSY_HL);
                            Show_Message((u8*)"Password Changed!", (u8*)"", 2000);
                            g_state = STATE_ADMIN_MENU;
                            Show_AdminMenu();
                        }
                    }
                }
                break;

            /* ---------- 登记指纹 ---------- */
            case STATE_REGISTER_FINGER:
                Show_Message((u8*)"Place Finger", (u8*)"3 Times Needed", 0);
                Voic_SendData(REGISTER_FINGER);
                while(VO_BUSY_HL);

                if(MG200_Enroll() == ENROLL_OK)
                {
                    Voic_SendData(SETTING_SUCCESS);
                    while(VO_BUSY_HL);
                    Show_Message((u8*)"Finger Enrolled!", (u8*)"", 2000);
                }
                else
                {
                    Show_Message((u8*)"Enroll Failed!", (u8*)"Try Again", 2000);
                }
                g_state = STATE_ADMIN_MENU;
                Show_AdminMenu();
                break;

            /* ---------- 登记门禁卡 ---------- */
            case STATE_REGISTER_CARD:
                Show_Message((u8*)"Place Card", (u8*)"Near Reader", 0);
                Voic_SendData(PUTCARD);
                while(VO_BUSY_HL);

                /* 等待刷卡(带超时) */
                {
                    u16 timeout = 500;  // 5秒超时
                    u8 card_found = 0;
                    while(timeout > 0)
                    {
                        if(Card_Read(card_id))
                        {
                            card_found = 1;
                            break;
                        }
                        delay_ms(10);
                        timeout--;
                    }

                    if(card_found)
                    {
                        sprintf((char*)card_buf, "ID:%02X%02X%02X%02X",
                                card_id[0], card_id[1], card_id[2], card_id[3]);
                        OLED_Clear();
                        OLED_ShowString(0, 2, 16, (u8*)"Card Found:");
                        OLED_ShowString(0, 4, 16, card_buf);
                        delay_ms(1000);

                        Card_Register(card_id);
                    }
                    else
                    {
                        Show_Message((u8*)"No Card Found!", (u8*)"Timeout", 2000);
                    }
                }
                g_state = STATE_ADMIN_MENU;
                Show_AdminMenu();
                break;

            /* ---------- 删除指纹 ---------- */
            case STATE_DELETE_FINGER:
                Show_Message((u8*)"Delete All?", (u8*)"#Yes *No", 0);
                Voic_SendData(DELETE_ALLFINGER);
                while(VO_BUSY_HL);

                /* 等待确认 */
                while(1)
                {
                    key = BS81xx_Key();
                    if(key == '#')
                    {
                        MG200_EraseAll();
                        Voic_SendData(SETTING_SUCCESS);
                        while(VO_BUSY_HL);
                        Show_Message((u8*)"All Fingers", (u8*)"Deleted!", 2000);
                        break;
                    }
                    else if(key == '*')
                    {
                        break;
                    }
                }
                g_state = STATE_ADMIN_MENU;
                Show_AdminMenu();
                break;

            /* ---------- 修改时间 ---------- */
            case STATE_CHANGE_TIME:
                Show_Message((u8*)"Set Time via", (u8*)"Bluetooth", 0);
                delay_ms(2000);
                /* 时间修改通过蓝牙或串口实现 */
                g_state = STATE_ADMIN_MENU;
                Show_AdminMenu();
                break;

            /* ---------- 指纹比对(菜单中) ---------- */
            case STATE_FINGER_MATCH:
                Show_Message((u8*)"Place Finger", (u8*)"on Sensor", 0);
                Voic_SendData(REGISTER_FINGER);
                while(VO_BUSY_HL);

                finger_id = MG200_Match1Nl();
                if(finger_id != 0)
                {
                    printf("Finger Match OK! ID=%d\r\n", finger_id);
                    Door_Open();
                }
                else
                {
                    printf("Finger Match Failed!\r\n");
                    Voic_SendData(DOOROPEN_FAIL);
                    while(VO_BUSY_HL);
                    Show_Message((u8*)"Match Failed!", (u8*)"", 2000);
                }
                g_state = STATE_IDLE;
                OLED_Clear();
                break;

            /* ---------- 门禁卡比对(菜单中) ---------- */
            case STATE_CARD_MATCH:
                Show_Message((u8*)"Place Card", (u8*)"Near Reader", 0);
                Voic_SendData(PUTCARD);
                while(VO_BUSY_HL);

                {
                    u16 timeout = 500;
                    u8 card_found = 0;
                    while(timeout > 0)
                    {
                        if(Card_Read(card_id))
                        {
                            card_found = 1;
                            break;
                        }
                        delay_ms(10);
                        timeout--;
                    }

                    if(card_found)
                    {
                        if(Card_CheckRegistered(card_id))
                        {
                            Door_Open();
                        }
                        else
                        {
                            Voic_SendData(DOOROPEN_FAIL);
                            while(VO_BUSY_HL);
                            Show_Message((u8*)"Card Invalid!", (u8*)"", 2000);
                        }
                    }
                    else
                    {
                        Show_Message((u8*)"No Card Found!", (u8*)"Timeout", 2000);
                    }
                }
                g_state = STATE_IDLE;
                OLED_Clear();
                break;

            default:
                g_state = STATE_IDLE;
                break;
        }

        /* 主循环延时 */
        delay_ms(100);
    }
}
