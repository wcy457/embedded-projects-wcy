#include "lcd.h"
#include "lcd_font.h"   /* ��ģ����ͷ�ļ������ASCII�ַ����� */
#include "spi.h"


/**
 * @brief Ӳ��SPI1����1���ֽ�
 * @param data: ��Ҫ���͵�8bit����
 * @retval ��
 */
static void LCD_SPI_WriteByte(uint8_t data)
{
    // ʹ��HAL��SPI�������ͣ���ʱ100ms
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
}

/**
 * @brief ��ILI9341д������
 * @param cmd: ILI9341�Ĵ���������
 * @retval ��
 */
void LCD_WriteCmd(uint8_t cmd)
{
    LCD_CS(0);         /* Ƭѡ���ͣ�ѡ��LCD�豸 */
    LCD_DC(0);         /* DC=0����ʾ���δ��������� */
    LCD_SPI_WriteByte(cmd);
    LCD_CS(1);         /* ������ɣ��ͷ�Ƭѡ */
}

/**
 * @brief ��ILI9341д��1�ֽڼĴ�������
 * @param data: 8bit�Ĵ�������
 * @retval ��
 */
void LCD_WriteData(uint8_t data)
{
    LCD_CS(0);
    LCD_DC(1);         /* DC=1����ʾ���δ�������ʾ����/�Ĵ������� */
    LCD_SPI_WriteByte(data);
    LCD_CS(1);
}

/**
 * @brief ���Դ�д��2�ֽ�RGB565��ɫ����
 * @param data: RGB565 16λ��ɫֵ
 * @retval ��
 * @note ILI9341 SPI����Ҫ�󣺸��ֽ����ȷ���
 */
void LCD_WriteData16(uint16_t data)
{
    LCD_CS(0);
    LCD_DC(1);
    LCD_SPI_WriteByte(data >> 8);     /* �ȷ��͸�8λ */
    LCD_SPI_WriteByte(data & 0xFF);   /* �ٷ��͵�8λ */
    LCD_CS(1);
}

/**
 * @brief ����LCD��д���ڣ��޶������Դ�д������
 * @param x1,y1: �������Ͻ�����
 * @param x2,y2: �������½�����
 * @retval ��
 * @note ִ����0x2C����֮�������������ݾͻ��Զ�����������
 */
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* �е�ַ�������� (0x2A)������X�����ұ߽� */
    LCD_WriteCmd(0x2A);
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1 & 0xFF);
    LCD_WriteData(x2 >> 8);
    LCD_WriteData(x2 & 0xFF);

    /* �е�ַ�������� (0x2B)������Y�����±߽� */
    LCD_WriteCmd(0x2B);
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1 & 0xFF);
    LCD_WriteData(y2 >> 8);
    LCD_WriteData(y2 & 0xFF);

    /* д�Դ����� (0x2C)�����ʹ�����󣬺���SPI����ֱ��д����Ļ�Դ� */
    LCD_WriteCmd(0x2C);
}


/**
 * @brief ����������Ϊָ����ɫ
 * @param x1,y1: �������Ͻ�
 * @param x2,y2: �������½�
 * @param color: RGB565��ɫ
 * @retval ��
 */
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    // ������Ҫ���������ظ���
    uint32_t total = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    // ������䴰��
    LCD_SetWindow(x1, y1, x2, y2);
    LCD_CS(0);
    LCD_DC(1);
    // ѭ��д��ÿһ�����ص�RGB565��ɫ
    for (uint32_t i = 0; i < total; i++) {
        LCD_SPI_WriteByte(color >> 8);
        LCD_SPI_WriteByte(color & 0xFF);
    }
    LCD_CS(1);
}

/**
 * @brief ��ָ�����껭�������ص�
 * @param x,y: ��������
 * @param color: RGB565��ɫ
 * @retval ��
 */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    // ��������Ϊ�������أ�д����ɫ
    LCD_SetWindow(x, y, x, y);
    LCD_WriteData16(color);
}

/**
 * @brief ��ʾ����ASCII�ַ�
 * @param x,y    �ַ����Ͻ���ʼ����
 * @param ch     ��Ҫ��ʾ��ASCII�ַ�
 * @param color  �ַ�ǰ��ɫ
 * @param bg     �ַ�����ɫ
 * @param size   �ֺ� 16=8*16���壻32=16*32����
 * @retval ��
 */
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg, uint8_t size)
{
    uint8_t i, j;
    uint8_t font_w = size / 2;         /* �ַ����ȣ�8x16��8���أ�16x32��16���� */
    uint8_t font_h = size;             /* �ַ��߶ȣ�8x16��16���أ�16x32��32���� */
    uint8_t font_bytes = font_h / 8;   /* ÿ��ռ���ֽ�����8bit=1�ֽ� */

    /* �����ַ�����ģ�����е�ƫ������ASCII�ӿո�' '��0x20����ʼ��� */
    uint32_t offset = (ch - ' ') * font_w * font_bytes;

    // �����ַ���ʾ����
    LCD_SetWindow(x, y, x + font_w - 1, y + font_h - 1);
    LCD_CS(0);
    LCD_DC(1);

    // ������ģ����
    for (i = 0; i < font_w; i++) {
        for (j = 0; j < font_bytes; j++) {
            uint8_t line = lcd_font[offset + i * font_bytes + j];
            // ��bit�жϣ�bit=1����ǰ��ɫ��bit=0���Ʊ���ɫ
            for (uint8_t k = 0; k < 8; k++) {
                if (line & (0x80 >> k)) {
                    LCD_SPI_WriteByte(color >> 8);
                    LCD_SPI_WriteByte(color & 0xFF);
                } else {
                    LCD_SPI_WriteByte(bg >> 8);
                    LCD_SPI_WriteByte(bg & 0xFF);
                }
            }
        }
    }
    LCD_CS(1);
}

/**
 * @brief ����Ļ����ʾ�ַ���
 * @param x,y    �ַ������Ͻ���ʼ����
 * @param str    �ַ���ָ��
 * @param color  ����ǰ��ɫ
 * @param bg     ���ֱ���ɫ
 * @param size   �ֺ� 16 / 32
 * @retval ��
 */
void LCD_ShowString(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg, uint8_t size)
{
    // ѭ��ֱ���ַ���������'\0'
    while (*str != '\0') {
        LCD_ShowChar(x, y, *str, color, bg, size);
        x += size / 2;  /* X��������һ���ַ����ȣ�׼����ӡ��һ���ַ� */
        str++;
    }
}

/**
 * @brief LCD ILI9341��ʼ������
 *
 * ��ʼ��˳��: ��λ -> ������� -> ��ʾ���� -> ٤��У�� -> �˳����� -> ����ʾ
 * @retval ��
 */
void LCD_Init(void)
{
    /* ����GPIOA����ʱ�� */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // ��ʼ��LCD�������ţ�RES DC CS BLK
    GPIO_InitStruct.Pin = LCD_SCL_PIN | LCD_SDA_PIN | LCD_RES_PIN |
                          LCD_DC_PIN  | LCD_CS_PIN  | LCD_BLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    //�������
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  //����
    HAL_GPIO_Init(LCD_PORT, &GPIO_InitStruct);

    /* Ӳ����λILI9341��Ļ */
    LCD_RES(0);
    HAL_Delay(20);    //���ͱ���20ms
    LCD_RES(1);
    HAL_Delay(20);    //���ߵȴ��ȶ�

    /* ===== ILI9341 �Ĵ�����ʼ������ ===== */
    LCD_WriteCmd(0xCF);   //��Դ����B
    LCD_WriteData(0x00);
    LCD_WriteData(0xD9);
    LCD_WriteData(0x30);

    LCD_WriteCmd(0xED);   //�ϵ�ʱ�����
    LCD_WriteData(0x64);
    LCD_WriteData(0x03);
    LCD_WriteData(0x12);
    LCD_WriteData(0x81);

    LCD_WriteCmd(0xE8);   //����ʱ�����A
    LCD_WriteData(0x85);
    LCD_WriteData(0x00);
    LCD_WriteData(0x78);

    LCD_WriteCmd(0xCB);   //��Դ����A
    LCD_WriteData(0x39);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x00);
    LCD_WriteData(0x34);
    LCD_WriteData(0x02);

    LCD_WriteCmd(0xF7);   //�ñȿ���
    LCD_WriteData(0x20);

    LCD_WriteCmd(0xEA);   //����ʱ�����B
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);

    LCD_WriteCmd(0xC0);   //��Դ����1 (VRH)
    LCD_WriteData(0x23);

    LCD_WriteCmd(0xC1);   //��Դ����2
    LCD_WriteData(0x10);

    LCD_WriteCmd(0xC5);   //VCOM����1
    LCD_WriteData(0x3E);
    LCD_WriteData(0x28);

    LCD_WriteCmd(0xC7);   //VCOM����2
    LCD_WriteData(0x86);

    LCD_WriteCmd(0x36);   //�ڴ���ʿ��ƣ���Ļɨ�跽��
    LCD_WriteData(0x48);  /* MY=0, MX=1, MV=0, ML=0, BGR=1 */

    LCD_WriteCmd(0x3A);   //���ظ�ʽ����
    LCD_WriteData(0x55);  //0x55=16bit RGB565

    LCD_WriteCmd(0xB1);   //֡���ʿ���
    LCD_WriteData(0x00);
    LCD_WriteData(0x18);

    LCD_WriteCmd(0xB6);   //��ʾ���ܿ���
    LCD_WriteData(0x08);
    LCD_WriteData(0x82);
    LCD_WriteData(0x27);

    LCD_WriteCmd(0xF2);   //3D٤���ر�
    LCD_WriteData(0x00);

    LCD_WriteCmd(0x26);   //ѡ��٤������
    LCD_WriteData(0x01);

    /* ������٤��У������ */
    LCD_WriteCmd(0xE0);
    uint8_t gamma_p[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,
                         0x37,0x07,0x10,0x03,0x0E,0x09,0x00};
    for (uint8_t i = 0; i < 15; i++) LCD_WriteData(gamma_p[i]);

    /* ������٤��У������ */
    LCD_WriteCmd(0xE1);
    uint8_t gamma_n[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,
                         0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F};
    for (uint8_t i = 0; i < 15; i++) LCD_WriteData(gamma_n[i]);

    LCD_WriteCmd(0x11);   //�˳�����ģʽ
    HAL_Delay(120);       //�ȴ��ڲ���ѹ�ȶ���������ʱ

    LCD_WriteCmd(0x29);   //������Ļ��ʾ

    LCD_BLK(1);           //�򿪱���

    /* ��ʼ����ɣ�����Ϊ��ɫ */
    LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, LCD_COLOR_BLACK);
}
