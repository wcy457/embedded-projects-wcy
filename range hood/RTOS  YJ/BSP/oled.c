#include "oled.h"
#include <string.h>

#if PROTEUS_SIM

/* ================= Proteus 仿真：无 SSD1306 模型，OLED 接口空实现（UI 改走串口打印） ================= */
void oled_init(void) {}
void oled_clear(void) {}
void oled_clear_page(uint8_t page) { (void)page; }
void oled_draw_string(uint8_t page, uint8_t col, const char *str) { (void)page; (void)col; (void)str; }
void oled_draw_int(uint8_t page, uint8_t col, int32_t val) { (void)page; (void)col; (void)val; }

#else

#define OLED_I2C_ADDR       (0x3C << 1)   /* SSD1306 7bit 0x3C → 8bit 0x78 */
#define OLED_PAGE_MAX       8             /* 64 像素 = 8 页 */

/* 一页的显存缓冲（128 字节），用于局部刷新，不做全屏 1KB 缓存 */
static uint8_t s_page[128];
/* I2C 发送缓冲：控制字节 + 一页数据 */
static uint8_t s_txbuf[129];

/* ---------------- 字体（5x7，每列一个字节，bit0=顶行） ---------------- */
/* 本字体为最小可用字集，可按需替换成完整 ASCII 字库 */
static const uint8_t FONT_DIGITS[10][5] = {
    {0x3E,0x41,0x41,0x41,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
};

static const uint8_t FONT_UPPER[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
};

static const uint8_t FONT_SPACE[5]   = {0x00,0x00,0x00,0x00,0x00};
static const uint8_t FONT_COLON[5]   = {0x00,0x36,0x36,0x00,0x00};
static const uint8_t FONT_DOT[5]     = {0x00,0x60,0x60,0x00,0x00};
static const uint8_t FONT_DASH[5]    = {0x08,0x08,0x08,0x08,0x08};
static const uint8_t FONT_PERCENT[5] = {0x63,0x13,0x08,0x64,0x62};
static const uint8_t FONT_SLASH[5]   = {0x40,0x20,0x10,0x08,0x04};

static const uint8_t *font_glyph(char c)
{
    if (c >= '0' && c <= '9') return FONT_DIGITS[c - '0'];
    if (c >= 'A' && c <= 'Z') return FONT_UPPER[c - 'A'];
    if (c >= 'a' && c <= 'z') return FONT_UPPER[c - 'a'];
    switch (c) {
        case ' ': return FONT_SPACE;
        case ':': return FONT_COLON;
        case '.': return FONT_DOT;
        case '-': return FONT_DASH;
        case '%': return FONT_PERCENT;
        case '/': return FONT_SLASH;
        default:  return FONT_SPACE;
    }
}

/* ---------------- 底层 I2C ---------------- */
static void oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};       /* 0x00 = 命令 */
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, 2, 100);
}

/* 写某一页的某一列段（局部刷新核心） */
static void oled_write_page_seg(uint8_t page, uint8_t col, const uint8_t *d, uint16_t len)
{
    oled_cmd(0xB0 | page);                        /* 页地址 */
    oled_cmd(0x00 | (col & 0x0F));                /* 列地址低 4 位 */
    oled_cmd(0x10 | ((col >> 4) & 0x0F));         /* 列地址高 4 位 */

    s_txbuf[0] = 0x40;                            /* 0x40 = 数据 */
    memcpy(&s_txbuf[1], d, len);
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_txbuf, len + 1, 100);
}

/* ---------------- 公开接口 ---------------- */
void oled_init(void)
{
    HAL_Delay(100);
    oled_cmd(0xAE);   /* 显示关 */
    oled_cmd(0x20); oled_cmd(0x00);   /* 内存寻址：水平模式 */
    oled_cmd(0xB0);                   /* 起始页 */
    oled_cmd(0xC8);                   /* 扫描方向 */
    oled_cmd(0x00); oled_cmd(0x10);   /* 起始列 */
    oled_cmd(0x40);                   /* 起始行 */
    oled_cmd(0x81); oled_cmd(0xCF);   /* 对比度 */
    oled_cmd(0xA1);                   /* 段重映射 */
    oled_cmd(0xA6);                   /* 正常显示 */
    oled_cmd(0xA8); oled_cmd(0x3F);   /* 复用率 1/64 */
    oled_cmd(0xA4);                   /* 显示跟随 RAM */
    oled_cmd(0xD3); oled_cmd(0x00);   /* 显示偏移 */
    oled_cmd(0xD5); oled_cmd(0x80);   /* 时钟 */
    oled_cmd(0xD9); oled_cmd(0xF1);   /* 预充电 */
    oled_cmd(0xDA); oled_cmd(0x12);   /* COM 引脚 */
    oled_cmd(0xDB); oled_cmd(0x40);   /* VCOM 检测 */
    oled_cmd(0x8D); oled_cmd(0x14);   /* 电荷泵 */
    oled_cmd(0xAF);                   /* 显示开 */
    oled_clear();
}

void oled_clear(void)
{
    uint8_t p;
    memset(&s_txbuf[1], 0, 128);
    s_txbuf[0] = 0x40;
    for (p = 0; p < OLED_PAGE_MAX; p++) {
        oled_cmd(0xB0 | p);
        oled_cmd(0x00);
        oled_cmd(0x10);
        HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_txbuf, 129, 100);
    }
}

void oled_clear_page(uint8_t page)
{
    memset(&s_txbuf[1], 0, 128);
    s_txbuf[0] = 0x40;
    oled_cmd(0xB0 | page);
    oled_cmd(0x00);
    oled_cmd(0x10);
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_txbuf, 129, 100);
}

void oled_draw_string(uint8_t page, uint8_t col, const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    uint16_t w = len * 6;                 /* 5 列字形 + 1 列间距 */
    uint16_t i;
    int c;

    if (w > 128 - col) w = 128 - col;
    memset(s_page, 0, w);

    for (i = 0; i < len && (i * 6) < w; i++) {
        const uint8_t *g = font_glyph(str[i]);
        for (c = 0; c < 5; c++) {
            s_page[i * 6 + c] = g[c];
        }
    }
    oled_write_page_seg(page, col, s_page, w);
}

void oled_draw_int(uint8_t page, uint8_t col, int32_t val)
{
    char buf[12];
    char tmp[10];
    int i = 0, j = 0;
    uint32_t u;

    if (val < 0) { buf[i++] = '-'; u = (uint32_t)(-val); }
    else         { u = (uint32_t)val; }

    do { tmp[j++] = (char)('0' + (u % 10)); u /= 10; } while (u && j < 9);
    while (j > 0) buf[i++] = tmp[--j];
    buf[i] = '\0';

    oled_draw_string(page, col, buf);
}

#endif /* !PROTEUS_SIM */
