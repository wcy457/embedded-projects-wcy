#include "boot_manager.h"
#include "inter_flashif.h"

/**
 * @brief ��ȡFlash��������У��ħ����checksum����ȡOTA���
 * @retval ota_flag(0/1)��0xFF ������������/��Ч
 */
uint8_t inter_flash_cfg_get_ota_flag(void)
{
	  flash_cfg_param_t param;  // �ֲ���������Ŷ����Ĳ���
    uint8_t calc = 0;         // ���ڼ����ۼ�У���
    uint8_t *p = (uint8_t *)&param;  // �ѽṹ��ת���ֽ�ָ�룬���ֽ��ۼ�
    uint8_t i;

    // ��Flash��������ȡȫ��12�ֽڵ����ؽṹ��
    inter_flashif_read(INTER_FLASH_PARAM_ADDR, (uint8_t *)&param, sizeof(param));

    // �ж�ħ����ħ�����Դ���������δ��ʼ������������
    if (param.magic[0] != FLASH_CFG_MAGIC_0 ||
        param.magic[1] != FLASH_CFG_MAGIC_1 ||
        param.magic[2] != FLASH_CFG_MAGIC_2 ||
        param.magic[3] != FLASH_CFG_MAGIC_3) {
        return 0xFF;
    }
				
		/*
     * У�����ֻ�ۼ�ǰ9�ֽ�
     * sizeof(param)=12����ȥĩβ3�ֽڣ�checksum1 + format2��=9�ֽ�
     * magic(4)+ota_bin_version(4)+ota_flag(1) =9�ֽ�
     */
    for (i = 0; i < sizeof(param) - 3; i++) {
        calc += p[i];
    }
    if(calc != param.checksum)
    {
        return 0xFF;
    }

    return param.ota_flag;
}

/**
 * @brief ����OTA������ǣ����²����������¼���У���
 * @param flag ������� 0���� /1������
 * @note ���ħ����Ч���Զ����Ĭ�ϳ�����������д��Flash
 */
void inter_flash_cfg_set_app_update_flag(uint8_t flag)
{
	flash_cfg_param_t param;
	uint8_t *p = (uint8_t *)&param;
	uint8_t i;

	// �ȶ���Flash��ԭ�еĲ���
	inter_flashif_read(INTER_FLASH_PARAM_ADDR, (uint8_t *)&param, sizeof(param));

	// ���ħ����ƥ�䣬����������δ��ʼ�������Ĭ�ϳ�����Ϣ
	if (param.magic[0] != FLASH_CFG_MAGIC_0) {
			param.magic[0] = FLASH_CFG_MAGIC_0;
			param.magic[1] = FLASH_CFG_MAGIC_1;
			param.magic[2] = FLASH_CFG_MAGIC_2;
			param.magic[3] = FLASH_CFG_MAGIC_3;
			param.ota_bin_version = 0; // Ĭ�ϰ汾0
			param.format[0] = 0;
			param.format[1] = 0;
	}
	
	param.ota_flag = flag;  // ����OTA���

	param.checksum = 0;     // �����У��ͣ������¼���
	// �ٴ��ۼ�ǰ9�ֽڣ������µ�У���
	for (i = 0; i < sizeof(param) - 3; i++) {
			param.checksum += p[i];
	}

	// Flashд��ǰ�������Ȳ�����ҳ
	inter_flashif_erase_page(INTER_FLASH_PARAM_ADDR);

	/*
	 * д������ṹ��
	 * sizeof(param)=12�ֽڣ�12/4=3��32bit��
	 * +1��д1���֣�ע��˵������ҳ�Ѿ�ȫ����Ϊ0xFF��ջ��������Ӱ����Ч����
	 */
	inter_flashif_write_page(INTER_FLASH_PARAM_ADDR,
													 (uint32_t *)&param, sizeof(param) / 4 + 1);
}

/**
 * @brief APP��תǰ�Ϸ��Լ�飬���ջ������λ������ַ�Ƿ�Ϸ�
 * @param app_addr APP������ʼ��ַ
 * @retval 1 APP����Ƿ���0У��ͨ����ִ����ת
 */
uint8_t boot_check_stack2jump_app(uint32_t app_addr)
{
    // APP�����׵�ַ��ų�ʼջ��MSP
    uint32_t app_stack = *(volatile uint32_t *)app_addr;
    // app_addr+4��Ÿ�λ�ж�������ַ����λ��ں�����
    uint32_t app_reset = *(volatile uint32_t *)(app_addr + 4);

    /*
     * STM32F103ZET6 RAM��Χ��0x20000000 ~ 0x2000FFFF
     * ջ����������RAM���䣻���APPδ��¼���ô���ȡΪ0xFFFFFFFFֱ���ж�ʧ��
     */
    if (app_stack < 0x20000000u || app_stack > 0x20010000u) {
        return 1;
    }

    // ��λ������������APP Flash�����ڣ���ֹ��ת���Ƿ���ַ
    if (app_reset < INTER_FLASH_APP_ADDR || app_reset >= INTER_FLASH_APP_END) {
        return 1;
    }

    // У��ȫ��ͨ����ִ����ת
    boot_jump_to_app(app_addr);
    return 0;   /* ��ת�ɹ��Ļ������д�����Զ�������� */
}

/**
 * @brief �ײ���ת��������ת��APP��λ���
 * @note 5��˳���ܵ�����˳�������������/�쳣�ж�
 */
void boot_jump_to_app(uint32_t app_addr)
{
	// ���庯��ָ�����ͣ��޲����޷��أ���Ӧ��λ����
    typedef void (*app_func_t)(void);
    app_func_t app_reset_handler;

    __disable_irq();                 // 1.�ر�ȫ���жϣ���ת���̽�ֹ�жϴ�������ֹ��������

    SysTick->CTRL = 0;               // 2.�ر�SysTick��ʱ��
    SysTick->LOAD = 0;               // �������ֵ
    SysTick->VAL  = 0;               // ��յ�ǰ����ֵ
    // ������SysTick����ת��APP��δ��ʼ����SysTick�жϻ���ʾ�������������HardFault

    SCB->VTOR = app_addr;            // 3.�ض�λ����������ַ���л���APP���ж�������

    // 4.������ջָ��MSPΪAPP�����һ���֣�APP�Լ���ջ����
    __set_MSP(*(volatile uint32_t *)app_addr);

    // ȡ��APP��λ������ַ
    app_reset_handler = (app_func_t)(*(volatile uint32_t *)(app_addr + 4));

    app_reset_handler();             // 5.����APP��λ��ڣ���ת�ɹ������ٷ���Bootloader
}

