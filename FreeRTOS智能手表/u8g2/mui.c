/*

  mui.c
  
  Monochrome minimal user interface: Core library.

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2021, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

  

  "mui.c" is a graphical user interface, developed as part of u8g2.
  However "mui.c" is independent of u8g2 and can be used without u8g2 code.
  The glue code between "mui.c" and u8g2 is located in "mui_u8g2.c"

  c: cmd
  i:  ID0
  j: ID1
  xy: Position (x and y)
  /text/: some text. The text can start with any delimiter (except 0 and |), but also has to end with the same delimiter
  a: Single char argument
  u: Single char argument with the user interface form number

  "Uu" the interface                                                    --> no ID
  
  Manual ID:
  "Fijxy"  Generic field: Places field with id ii at x/y        --> ID=ij
  "Bijxy/text/"   Generic field (Button) with Text   --> ID=ij
  "Tiixya/text/"  Generic field with argument and text --> ID = ij
  "Aiixya"
  
  Fixed ID:
  "Si" the style                                                        --> ID=@i
  "Lxy/labeltext/"  Places a text at the specified position, field with   -     -> ID=.L, .l
  "Gxyu/menutext/"  Go to the specified menu without placing the user interface form on the stack       --> ID=.G, .g
  
  
  cijxy
  cijxy/text/
  cijxya/text/
  
  cxy/text/
  cxya/text/

  文件: mui.c
  功能: MUI（菜单用户界面）核心库实现
        MUI是一个轻量级的单色图形用户界面框架，专为嵌入式系统设计。
        虽然是u8g2库的一部分，但mui.c本身不依赖u8g2，可以独立使用。
        与u8g2的连接代码位于mui_u8g2.c中。

  主要功能:
    - 管理菜单表单（Form）的创建、进入、离开和切换
    - 解析FDS（Field Definition String）字段定义字符串
    - 处理光标导航（上一个/下一个字段）
    - 支持字段的绘制、选择、值增减等交互操作
    - 提供可滚动的菜单支持

  FDS字段定义字符串格式说明:
    'U' - 用户表单定义
    'S' - 样式定义
    'F' - 通用字段（无参数无文本）
    'B' - 带文本的字段（如按钮）
    'T' - 带参数和文本的字段
    'A' - 带参数的字段（无文本）
    'L' - 文本标签（固定ID）
    'G' - 跳转按钮（固定ID）
    'D' - 数据字段
    'Z' - 零字段（无位置参数）
*/

#include "mui.h"





//#define mui_get_fds_char(s) ((uint8_t)(*s))

//#include <stdio.h>
//#define MUI_DEBUG(...) printf(__VA_ARGS__)
#define MUI_DEBUG(...)

/*
  函数: mui_get_fds_char
  功能: 从FDS（字段定义字符串）中读取一个字节
        使用mui_pgm_read宏来支持从程序存储器（如AVR的PROGMEM）读取数据
  参数:
    s - FDS字符串指针
  返回值: 读取到的字节值
*/
uint8_t mui_get_fds_char(fds_t *s)
{
  return (uint8_t)mui_pgm_read(s);  // 从程序存储器读取一个字节
}


/*
  函数: mui_fds_get_cmd_size_without_text (内部函数)
  功能: 获取FDS命令的大小（不包括文本部分）
        根据命令类型返回不同的字节数：
          'U' = 2字节 (命令+表单ID)
          'S' = 2字节 (命令+样式ID)
          'D' = 3字节 (命令+2字节ID)
          'Z' = 3字节 (命令+2字节ID)
          'F' = 5字节 (命令+2字节ID+X+Y)
          'B' = 5字节 (命令+2字节ID+X+Y，不含文本)
          'T' = 6字节 (命令+2字节ID+X+Y+参数，不含文本)
          'A' = 6字节 (命令+2字节ID+X+Y+参数)
          'L' = 3字节 (命令+X+Y)
          'G' = 4字节 (命令+X+Y+参数)
  参数:
    s - FDS字符串指针，必须指向有效的命令起始位置
  返回值: 命令的字节大小（不包括文本部分）
*/
static size_t mui_fds_get_cmd_size_without_text(fds_t *s) MUI_NOINLINE;
static size_t mui_fds_get_cmd_size_without_text(fds_t *s)
{
  uint8_t c = mui_get_fds_char(s);
  c &= 0xdf; /* consider upper and lower case */
  switch(c)
  {
    case 'U': return 2;         // User Form: CMD  (1 Byte), Form-Id (1 Byte)
    case 'S': return 2;         // Style: CMD (1 Byte), Style Id (1 Byte)
    case 'D': return 3;         // Data within Text: CMD (1 Byte), ID (2 Bytes), Text (does not count here)
    case 'Z': return 3;         // Zero field without x, y, arg & text: CMD (1 Byte), ID (2 Bytes)
    case 'F': return 5;         // Field without arg & text: CMD (1 Byte), ID (2 Bytes), X, Y
    case 'B': return 5;         // Field with text: CMD (1 Byte), ID (2 Bytes), X, Y, Text (does not count here)
    case 'T': return 6;         // Field with arg & text: CMD (1 Byte), ID (2 Bytes), X, Y, Arg, Text (does not count here)
    case 'A': return 6;         // Field with arg (no text): CMD (1 Byte), ID (2 Bytes), X, Y, Arg, Text
    case 'L': return 3;          // Text Label: CMD (1 Byte), X, Y (same as 'B' but with fixed ID '.L', MUIF_LABEL, MUI_LABEL)
    case 'G': return 4;         // Goto Btutton: CMD (1Byte), X, Y, Arg, Text  (same as 'T' but with fixed ID '.G', MUIF_GOTO, MUI_GOTO)
    case 0: return 0;
  }
  return 1;
}



/*
  函数: mui_fds_parse_text (内部函数)
  功能: 解析FDS中的文本字段
        文本字段由分隔符包围，例如"B00ab/ok/"中的"/ok/"。
        分隔符实际上是0xff，可以是任意非0和非'|'的字符。
        函数将文本内容复制到ui->text缓冲区中。
  参数:
    ui - MUI用户界面结构体指针
    s  - FDS字符串指针，必须指向文本分隔符的起始位置（如第一个'/'）
  返回值: 文本字段的总大小（包括分隔符）
  副作用: 将文本内容复制到ui->text中
*/
static size_t mui_fds_parse_text(mui_t *ui, fds_t *s)
{
  uint8_t c;
  uint8_t i = 0;
  fds_t *t = s;
  ui->delimiter = mui_get_fds_char(s);   // 读取文本分隔符（如'/'）

#ifdef MUI_CHECK_EOFDS
  if ( ui->delimiter == 0 )              // 如果遇到字符串结束符，返回0
    return 0;
#endif
  t++;                                    // 跳过分隔符，指向文本内容的第一个字符
  for( ;; )
  {
    c = mui_get_fds_char(t);             // 读取当前字符
#ifdef MUI_CHECK_EOFDS
    if ( c == 0 )                        // 字符串结束
      break;
#endif
    if ( c == ui->delimiter )            // 遇到结束分隔符
    {
      t++;                               // 跳过结束分隔符
      break;
    }
    if ( i < MUI_MAX_TEXT_LEN )          // 防止缓冲区溢出
    {
      ui->text[i++] = c;                // 将字符复制到文本缓冲区
    }
    t++;
  }
  ui->text[i] = '\0' ;                  // 添加字符串结束符
  return t-s;                            // 返回文本字段的总大小
}

/*
  get the first token within a text argument.
  The text argument may look like this:
    "B00ab/banana|apple|peach|cherry/"
  The outer delimiter "/" is not fixed and can be any char except "|" and "\0"
  The inner delimiter "|" is fixed. It must be the pipe symbol.
  This function will place "banana" into ui->text if the result is not 0

  if ( mui_fds_first_token(ui) )
  {
    do 
    {
      // handle token in ui->text
    } while ( mui_fds_next_token(ui) )
  }

*/

/*
  函数: mui_fds_first_token
  功能: 获取文本参数中的第一个令牌（token）
        文本参数可能包含用'|'分隔的多个选项，例如"banana|apple|peach|cherry"。
        外部分隔符（如'/'）不是固定的，但内部分隔符必须是'|'字符。
        结果存储在ui->text中。
  参数:
    ui - MUI用户界面结构体指针
  返回值: 1=找到令牌（存储在ui->text中），0=没有找到令牌
*/
uint8_t mui_fds_first_token(mui_t *ui)
{
  ui->token = ui->fds;                                        // 定位到当前字段
  ui->token += mui_fds_get_cmd_size_without_text(ui->fds);   // 跳过命令头，指向文本部分
  ui->delimiter = mui_get_fds_char(ui->token);                // 读取外部分隔符
  ui->token++;                                                 // 跳过分隔符，指向第一个字符
  return mui_fds_next_token(ui);                               // 获取第一个令牌
}

/*
  函数: mui_fds_next_token
  功能: 获取文本参数中的下一个令牌
        令牌之间用'|'（管道符号）分隔。当遇到外部分隔符或'|'时停止。
        结果存储在ui->text中。
  参数:
    ui - MUI用户界面结构体指针
  返回值: 1=找到下一个令牌，0=没有更多令牌
*/
uint8_t mui_fds_next_token(mui_t *ui)
{
  uint8_t c;
  uint8_t i = 0;
  for( ;; )
  {
    c = mui_get_fds_char(ui->token);    // 读取当前字符
#ifdef MUI_CHECK_EOFDS
    if ( c == 0 )                        // 字符串结束
      break;
#endif
    if ( c == ui->delimiter )            // 遇到外部分隔符，令牌结束
      break;
    if ( c == '|'  )                     // 遇到内部分隔符'|'
    {
      ui->token++;                       // 跳过'|'，指向下一个令牌的起始位置
      break;
    }

    if ( i < MUI_MAX_TEXT_LEN )          // 防止缓冲区溢出
    {
      ui->text[i++] = c;                // 将字符复制到文本缓冲区
    }

    ui->token++;
  }
  ui->text[i] = '\0' ;                  // 添加字符串结束符
  if ( i == 0 )
    return 0;                            // 没有找到更多令牌
  return 1;                              // 令牌已存储在ui->text中
}

/*
  函数: mui_fds_get_nth_token
  功能: 获取文本参数中第n个令牌（从0开始计数）
        令牌之间用'|'分隔。如果n超过令牌总数，返回0。
        结果存储在ui->text中。
  参数:
    ui - MUI用户界面结构体指针
    n  - 令牌索引（从0开始）
  返回值: 1=找到第n个令牌，0=索引超出范围
*/
uint8_t mui_fds_get_nth_token(mui_t *ui, uint8_t n)
{
  if ( mui_fds_first_token(ui) )         // 获取第一个令牌
  {
    do
    {
      if ( n == 0 )                       // 找到第n个令牌
      {
        return 1;
      }
      n--;                                // 继续查找下一个
    } while ( mui_fds_next_token(ui) );   // 获取后续令牌
  }
  return 0;                               // 索引超出范围
}

/*
  函数: mui_fds_get_token_cnt
  功能: 获取文本参数中令牌的总数（以'|'分隔的选项数量）
  参数:
    ui - MUI用户界面结构体指针
  返回值: 令牌的总数
*/
uint8_t mui_fds_get_token_cnt(mui_t *ui)
{
  uint8_t n = 0;
  if ( mui_fds_first_token(ui) )         // 获取第一个令牌
  {
    do
    {
      n++;                                // 计数
    } while ( mui_fds_next_token(ui) );   // 遍历所有令牌
  }
  return n;                               // 返回令牌总数
}

/* 判断命令是否包含文本参数的宏 */
#define mui_fds_is_text(c) ( (c) == 'U' || (c) == 'S' || (c) == 'F' || (c) == 'A' || (c) == 'Z' ? 0 : 1 )

/*
  函数: mui_fds_get_cmd_size (内部函数)
  功能: 获取FDS命令的完整大小（包括文本部分）
        如果命令包含文本参数，则同时将文本内容复制到ui->text中。
        如果命令不包含文本参数，则将ui->text设置为空字符串。
  参数:
    ui - MUI用户界面结构体指针
    s  - FDS字符串指针，必须指向有效的命令起始位置
  返回值: 命令的完整字节大小（包括文本部分）
  副作用: 更新ui->text的内容
*/
static size_t mui_fds_get_cmd_size(mui_t *ui, fds_t *s) MUI_NOINLINE;
static size_t mui_fds_get_cmd_size(mui_t *ui, fds_t *s)
{
  size_t l = mui_fds_get_cmd_size_without_text(s);  // 获取命令头大小（不含文本）
  uint8_t c = mui_get_fds_char(s);                   // 读取命令字符
 ui->text[0] = '\0' ;                                // 总是先清空文本缓冲区
 if ( mui_fds_is_text(c) )                           // 如果该命令包含文本参数
  {
    l += mui_fds_parse_text(ui, s+l);                // 解析文本并累加大小
  }
  return l;                                           // 返回命令的完整大小
}


/*
  mui_Init() will setup the menu system but will not activate or display anything.
  Use mui_GotoForm() after this command, then use mui_Draw() to draw the menu on a display.
*/
/*
  函数: mui_Init
  功能: 初始化MUI菜单系统
        设置菜单系统的各项参数，但不会激活或显示任何内容。
        初始化后需要调用mui_GotoForm()进入表单，然后调用mui_Draw()绘制菜单。
  参数:
    ui             - MUI用户界面结构体指针（由调用者分配内存）
    graphics_data  - 图形设备数据指针（如u8g2_t*），存储在ui->graphics_data中
    fds            - FDS字段定义字符串数组的起始地址
    muif_tlist     - MUI字段（muif_t）数组的起始地址
    muif_tcnt      - MUI字段数组的元素数量
  返回值: 无
*/
void mui_Init(mui_t *ui, void *graphics_data, fds_t *fds, muif_t *muif_tlist, size_t muif_tcnt)
{
  memset(ui, 0, sizeof(mui_t));          // 将整个结构体清零
  ui->root_fds = fds;                     // 设置FDS根指针
  ui->muif_tlist = muif_tlist;            // 设置字段定义列表
  ui->muif_tcnt = muif_tcnt;              // 设置字段定义数量
  ui->graphics_data = graphics_data;      // 设置图形设备数据指针
}

/*
  函数: mui_find_uif
  功能: 在MUI字段定义列表中查找匹配的字段
        根据id0和id1两个标识符在muif_tlist中查找对应的字段定义。
  参数:
    ui  - MUI用户界面结构体指针
    id0 - 第一个标识符（通常是字段类型标识，如'S'、'.'等）
    id1 - 第二个标识符（通常是字段编号或命令字符）
  返回值: 找到的字段在列表中的索引，未找到返回-1
*/
int mui_find_uif(mui_t *ui, uint8_t id0, uint8_t id1)
{
  size_t i;
  for( i = 0; i < ui->muif_tcnt; i++ )   // 遍历所有字段定义
  {
      if ( muif_get_id0(ui->muif_tlist+i) == id0 )   // 比较第一个标识符
        if ( muif_get_id1(ui->muif_tlist+i) == id1 ) // 比较第二个标识符
          return i;                                    // 找到匹配，返回索引
  }
  return -1;  // 未找到匹配的字段
}


/*
  函数: mui_prepare_current_field (内部函数)
  功能: 准备当前字段的解析
        假设ui->fds指向有效的位置，解析并计算字段的所有成员变量。
        一些字段（如ui->cmd和ui->len）总是会被计算，
        其他成员变量仅在返回值为1时才有效。
  参数:
    ui - MUI用户界面结构体指针
  返回值: 1=字段ID在uif列表中找到，0=未找到或ui->fds不是字段
  副作用: 更新ui->cmd, ui->len, ui->id0, ui->id1, ui->x, ui->y,
          ui->arg, ui->uif, ui->dflags, ui->text等成员变量
*/
static uint8_t mui_prepare_current_field(mui_t *ui) MUI_NOINLINE;
static uint8_t mui_prepare_current_field(mui_t *ui)
{
  int muif_tidx;

  ui->uif = NULL;          // 清空字段定义指针
  ui->dflags = 0;          // 清空动态标志
  ui->id0 = 0;             // 清空ID0
  ui->id1 = 0;             // 清空ID1
  ui->arg = 0;             // 清空参数

  /* 计算命令长度并复制文本参数 */
  /* 如果没有文本参数，也会清空ui->text */
  ui->len = mui_fds_get_cmd_size(ui, ui->fds);

  /* 获取命令字符 */
  ui->cmd = mui_get_fds_char(ui->fds);

  /* 将命令字符也复制到id1，某些命令会覆盖这个值 */
  ui->id1 = ui->cmd;

  /* 将命令转为大写，使大小写都能被识别 */
  ui->cmd &= 0xdf;  // 清除第5位，将小写转为大写

  if ( ui->cmd == 'U' || ui->cmd == 0 )  // 遇到表单开始或结束标记
    return 0;

  /* 计算动态标志 */
  if ( ui->fds == ui->cursor_focus_fds )           // 当前字段是否拥有光标焦点
    ui->dflags |= MUIF_DFLAG_IS_CURSOR_FOCUS;
  if ( ui->fds == ui->touch_focus_fds )            // 当前字段是否拥有触摸焦点
    ui->dflags |= MUIF_DFLAG_IS_TOUCH_FOCUS;
  

  /* 根据命令类型获取id0、id1和其他参数 */
  if  ( ui->cmd == 'F' || ui->cmd == 'B' || ui->cmd == 'T' || ui->cmd == 'A' )
  {
      /* F/B/T/A类型：命令+2字节ID+X+Y[+参数] */
      ui->id0 = mui_get_fds_char(ui->fds+1);   // 读取ID0
      ui->id1 = mui_get_fds_char(ui->fds+2);   // 读取ID1
      ui->x = mui_get_fds_char(ui->fds+3);     // 读取X坐标
      ui->y = mui_get_fds_char(ui->fds+4);     // 读取Y坐标
      if ( ui->cmd == 'A' || ui->cmd == 'T' )   // A和T类型还有额外参数
      {
        ui->arg = mui_get_fds_char(ui->fds+5);  // 读取参数
      }
  }
  else if ( ui->cmd == 'D' || ui->cmd == 'Z' )
  {
      /* D/Z类型：命令+2字节ID */
      ui->id0 = mui_get_fds_char(ui->fds+1);   // 读取ID0
      ui->id1 = mui_get_fds_char(ui->fds+2);   // 读取ID1
  }
  else if ( ui->cmd == 'S' )
  {
      /* S类型：命令+样式ID */
      ui->id0 = 'S';                            // 固定id0为'S'
      ui->id1 = mui_get_fds_char(ui->fds+1);   // 读取样式ID
  }
  else
  {
      /* L/G/M等固定ID类型 */
      ui->id0 = '.';                            // 固定id0为'.'
      /* 注意：ui->id1已包含原始命令字符 */
      ui->x = mui_get_fds_char(ui->fds+1);     // 读取X坐标
      ui->y = mui_get_fds_char(ui->fds+2);     // 读取Y坐标
      if ( ui->cmd == 'G' || ui->cmd == 'M' )   // G和M类型有额外参数
      {
        ui->arg = mui_get_fds_char(ui->fds+3);  // 读取参数（通常是目标表单ID）
      }
  }

  //MUI_DEBUG("mui_prepare_current_field cmd='%c' len=%d arg=%d\n", ui->cmd, ui->len, ui->arg);

  
  /* 在字段定义列表中查找匹配的字段 */
  muif_tidx = mui_find_uif(ui, ui->id0, ui->id1);
  if ( muif_tidx >= 0 )                    // 找到匹配的字段定义
  {
    ui->uif = ui->muif_tlist + muif_tidx;  // 设置字段定义指针
    return 1;
  }
  return 0;  // 未找到匹配的字段
}

/*
  函数: mui_inner_loop_over_form (内部函数)
  功能: 遍历当前表单中的所有字段，并对每个字段调用task回调函数
        跳过第一个条目（总是'U'命令），遍历后续所有字段直到遇到下一个'U'或结束标记。
        通常不直接调用此函数，而是使用mui_loop_over_form。
  参数:
    ui   - MUI用户界面结构体指针
    task - 回调函数指针，对每个字段调用此函数。
           回调函数返回0继续遍历，返回非0则停止遍历。
  返回值: 无
*/
static void mui_inner_loop_over_form(mui_t *ui, uint8_t (*task)(mui_t *ui)) MUI_NOINLINE;
static void mui_inner_loop_over_form(mui_t *ui, uint8_t (*task)(mui_t *ui))
{
  uint8_t cmd;

  ui->fds += mui_fds_get_cmd_size(ui, ui->fds);   // 跳过第一个条目（总是'U'表单标记）
  for(;;)
  {
    cmd = mui_get_fds_char(ui->fds);               // 获取当前命令字符
    if ( cmd == 'U' || cmd == 0 )                  // 遇到下一个表单或结束标记
      break;
    if ( mui_prepare_current_field(ui) )           // 准备当前字段（副作用：计算ui->len）
      if ( task(ui) )                              // 调用任务回调函数
      {
        break;                                     // 回调返回非0，停止遍历
      }
    ui->fds += ui->len;                            // 移动到下一个字段
  }
}

/*
  函数: mui_loop_over_form (内部函数)
  功能: 遍历当前活动表单的所有字段
        首先检查表单是否活动，然后设置初始状态并调用mui_inner_loop_over_form。
  参数:
    ui   - MUI用户界面结构体指针
    task - 回调函数指针
  返回值: 无
*/
static void mui_loop_over_form(mui_t *ui, uint8_t (*task)(mui_t *ui)) MUI_NOINLINE;
static void mui_loop_over_form(mui_t *ui, uint8_t (*task)(mui_t *ui))
{
  if ( mui_IsFormActive(ui) == 0 )   // 检查是否有活动表单
    return;

  ui->fds = ui->current_form_fds;   // 设置FDS指针到当前表单起始位置
  ui->target_fds = NULL;             // 清空目标FDS
  ui->tmp_fds = NULL;                // 清空临时FDS

  mui_inner_loop_over_form(ui, task); // 执行遍历
}

/*
  函数: mui_find_form
  功能: 在FDS中查找指定编号的表单
        从FDS起始位置开始遍历，查找'U'命令后跟指定编号的表单。
  参数:
    ui - MUI用户界面结构体指针
    n  - 要查找的表单编号
  返回值: 找到的表单FDS指针，未找到返回NULL
*/
fds_t *mui_find_form(mui_t *ui, uint8_t n)
{
  fds_t *fds = ui->root_fds;
  uint8_t cmd;
  
  for( ;; )
  {
    cmd = mui_get_fds_char(fds);
    if ( cmd == 0 )
      break;
    if ( cmd == 'U'  )
    {
      if (   mui_get_fds_char(fds+1) == n )
      {
        return fds;
      }
      /* not found, just coninue */
    }
    
    fds += mui_fds_get_cmd_size(ui, fds);
  }
  return NULL;
}

/* === task procedures (arguments for mui_loop_over_form) === */
/* ui->fds contains the current field */

uint8_t mui_task_draw(mui_t *ui)
{
  //printf("mui_task_draw fds=%p uif=%p text=%s\n", ui->fds, ui->uif, ui->text);
  muif_get_cb(ui->uif)(ui, MUIF_MSG_DRAW);
  return 0;     /* continue with the loop */
}

uint8_t mui_task_form_start(mui_t *ui)
{
  muif_get_cb(ui->uif)(ui, MUIF_MSG_FORM_START);
  return 0;     /* continue with the loop */
}

uint8_t mui_task_form_end(mui_t *ui)
{
  muif_get_cb(ui->uif)(ui, MUIF_MSG_FORM_END);
  return 0;     /* continue with the loop */
}

static uint8_t mui_uif_is_cursor_selectable(mui_t *ui) MUI_NOINLINE;
static uint8_t mui_uif_is_cursor_selectable(mui_t *ui)
{
  if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  {
    return 1;
  }
  return 0;
}

uint8_t mui_task_find_prev_cursor_uif(mui_t *ui)
{
  //if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  if ( mui_uif_is_cursor_selectable(ui) )
  {
    if ( ui->fds == ui->cursor_focus_fds )
    {
      ui->target_fds = ui->tmp_fds;
      return 1;         /* stop looping */
    }
    ui->tmp_fds = ui->fds;
  }
  return 0;     /* continue with the loop */
}

uint8_t mui_task_find_first_cursor_uif(mui_t *ui)
{
  //if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  if ( mui_uif_is_cursor_selectable(ui) )
  {
    // if ( ui->target_fds == NULL )
    // {
      ui->target_fds = ui->fds;
      return 1;         /* stop looping */
    // }
  }
  return 0;     /* continue with the loop */
}

uint8_t mui_task_find_last_cursor_uif(mui_t *ui)
{
  //if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  if ( mui_uif_is_cursor_selectable(ui) )
  {
    //ui->cursor_focus_position++;
    ui->target_fds = ui->fds;
  }
  return 0;     /* continue with the loop */
}

uint8_t mui_task_find_next_cursor_uif(mui_t *ui)
{
  //if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  if ( mui_uif_is_cursor_selectable(ui) )
  {
    if ( ui->tmp_fds != NULL )
    {
      ui->target_fds = ui->fds;        
      ui->tmp_fds = NULL;
      return 1;         /* stop looping */
    }
    if ( ui->fds == ui->cursor_focus_fds )
    {
      ui->tmp_fds = ui->fds;
    }
  }
  return 0;     /* continue with the loop */
}

uint8_t mui_task_get_current_cursor_focus_position(mui_t *ui)
{
  //if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  if ( mui_uif_is_cursor_selectable(ui) )
  {
    if ( ui->fds == ui->cursor_focus_fds )
      return 1;         /* stop looping */
    ui->tmp8++;
  }
  return 0;     /* continue with the loop */
}

uint8_t mui_task_read_nth_selectable_field(mui_t *ui)
{
  //if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_CURSOR_SELECTABLE )
  if ( mui_uif_is_cursor_selectable(ui) )
  {
    if ( ui->tmp8 == 0 )
      return 1;         /* stop looping */
    ui->tmp8--;
  }
  return 0;     /* continue with the loop */
}

uint8_t mui_task_find_execute_on_select_field(mui_t *ui)
{
  if ( muif_get_cflags(ui->uif) & MUIF_CFLAG_IS_EXECUTE_ON_SELECT )
  {
      ui->target_fds = ui->fds;
      return 1;         /* stop looping */
  }
  return 0;     /* continue with the loop */
}


/* === utility functions for the user API === */

static uint8_t mui_send_cursor_msg(mui_t *ui, uint8_t msg) MUI_NOINLINE;
static uint8_t mui_send_cursor_msg(mui_t *ui, uint8_t msg)
{
  if ( ui->cursor_focus_fds )
  {
    ui->fds = ui->cursor_focus_fds;
    if ( mui_prepare_current_field(ui) )
      return muif_get_cb(ui->uif)(ui, msg);
  }
  return 0; /* not called, msg not handled */
}

/* === user API === */

/* 
  returns the field pos which has the current focus 
  If the first selectable field has the focus, then 0 will be returned
  Unselectable fields (for example labels) are skipped by this count.
  If no fields are selectable, then 0 is returned

  The return value can be used as last argument for mui_EnterForm or mui_GotoForm

  WARNING: This function will destroy current fds and field information.
*/
uint8_t mui_GetCurrentCursorFocusPosition(mui_t *ui)
{
  //fds_t *fds = ui->fds;
  ui->tmp8 = 0;  
  mui_loop_over_form(ui, mui_task_get_current_cursor_focus_position);
  //ui->fds = fds;
  return ui->tmp8;
}


void mui_Draw(mui_t *ui)
{
  mui_loop_over_form(ui, mui_task_draw);
}

void mui_next_field(mui_t *ui)
{
  mui_loop_over_form(ui, mui_task_find_next_cursor_uif);
  // ui->cursor_focus_position++;
  ui->cursor_focus_fds = ui->target_fds;      // NULL is ok  
  if ( ui->target_fds == NULL )
  {
    mui_loop_over_form(ui, mui_task_find_first_cursor_uif);
    ui->cursor_focus_fds = ui->target_fds;      // NULL is ok  
    // ui->cursor_focus_position = 0;
  }
}


/*
  this function will overwrite the ui field related member variables
  nth_token can be 0 if the fiel text is not a option list
  the result is stored in ui->text
  
  token delimiter is '|' (pipe symbol)
  
  fds:  The start of a field (MUI_DATA)
  nth_token: The position of the token, which should be returned
*/
uint8_t mui_GetSelectableFieldTextOption(mui_t *ui, fds_t *fds, uint8_t nth_token)
{
  fds_t *fds_backup = ui->fds;                                // backup the current fds, so that this function can be called inside a task loop 
  int len = ui->len;          // backup length of the current command, 26 sep 2021: probably this is not required any more
  uint8_t is_found;
  
  ui->fds = fds;
  // at this point ui->fds contains the field which contains the tokens  
  // now get the opion string out of the text field. nth_token can be 0 if this is no opion string
  is_found = mui_fds_get_nth_token(ui, nth_token);          // return value is ignored here
  
  ui->fds = fds_backup;                        // restore the previous fds position
  ui->len = len;
  // result is stored in ui->text
  return is_found;
}

uint8_t mui_GetSelectableFieldOptionCnt(mui_t *ui, fds_t *fds)
{
  fds_t *fds_backup = ui->fds;                                // backup the current fds, so that this function can be called inside a task loop 
  int len = ui->len;          // backup length of the current command   26 sep 2021: probably this is not required any more
  uint8_t cnt = 0;
  
  ui->fds = fds;
  // at this point ui->fds contains the field which contains the tokens  
  // now get the opion string out of the text field. nth_token can be 0 if this is no opion string
  cnt = mui_fds_get_token_cnt(ui); 
  
  ui->fds = fds_backup;                        // restore the previous fds position
  ui->len = len;
  // result is stored in ui->text
  return cnt;
}



//static void mui_send_cursor_enter_msg(mui_t *ui) MUI_NOINLINE;
static uint8_t mui_send_cursor_enter_msg(mui_t *ui)
{
  ui->is_mud = 0;
  return mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_ENTER);
}

/* 
  if called from a field function, then the current field variables are destroyed, so that call should be the last call in the field callback.
  mui_EnterForm is similar to mui_GotoForm and differes only in the second argument (which is the form id instead of the fds pointer)
*/
void mui_EnterForm(mui_t *ui, fds_t *fds, uint8_t initial_cursor_position)
{
  /* exit any previous form, will not do anything if there is no current form */
  mui_LeaveForm(ui);
  
  /* clean focus fields */
  ui->touch_focus_fds = NULL;
  ui->cursor_focus_fds = NULL;
  
  /* reset all the scoll values */
  ui->form_scroll_top = 0;
  ui->form_scroll_visible = 0;
  ui->form_scroll_total = 0;
  
  /* assign the form, which should be entered */
  ui->current_form_fds = fds;
  
  /* inform all fields that we start a new form */
  MUI_DEBUG("mui_EnterForm: form_start, initial_cursor_position=%d\n", initial_cursor_position);
  mui_loop_over_form(ui, mui_task_form_start);
  
  /* assign initional cursor focus */
  MUI_DEBUG("mui_EnterForm: find_first_cursor_uif\n");
  mui_loop_over_form(ui, mui_task_find_first_cursor_uif);  
  ui->cursor_focus_fds = ui->target_fds;      // NULL is ok  
  MUI_DEBUG("mui_EnterForm: find_first_cursor_uif target_fds=%p\n", ui->target_fds);
  
  while( initial_cursor_position > 0 )
  {
    mui_NextField(ui);          // mui_next_field(ui) is not sufficient in case of scrolling
    initial_cursor_position--;
  }
  
  while( mui_send_cursor_enter_msg(ui) == 255 )
  {
    mui_NextField(ui);          // mui_next_field(ui) is not sufficient in case of scrolling
  }
}

/* input: current_form_fds */
/*
  if called from a field function, then the current field variables are destroyed, so that call should be the last call in the field callback.
*/
void mui_LeaveForm(mui_t *ui)
{
  if ( mui_IsFormActive(ui) == 0 )
    return;
  
  mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_LEAVE);
  ui->cursor_focus_fds = NULL;
  
  /* inform all fields that we leave the form */
  MUI_DEBUG("mui_LeaveForm: form_end\n");
  mui_loop_over_form(ui, mui_task_form_end);  
  ui->current_form_fds = NULL;
}

/* 0: error, form not found */
/*
  if called from a field function, then the current field variables are destroyed, so that call should be the last call in the field callback.
*/
uint8_t mui_GotoForm(mui_t *ui, uint8_t form_id, uint8_t initial_cursor_position)
{
  fds_t *fds = mui_find_form(ui, form_id);
  if ( fds == NULL )
    return 0;
  /* EnterForm will also leave any previous form */
  mui_EnterForm(ui, fds, initial_cursor_position);
  return 1;
}

void mui_SaveForm(mui_t *ui)
{
  if ( mui_IsFormActive(ui) == 0 )
    return;
  
  ui->last_form_fds = ui->cursor_focus_fds;
  ui->last_form_id = mui_get_fds_char(ui->current_form_fds+1);
  ui->last_form_cursor_focus_position = mui_GetCurrentCursorFocusPosition(ui);
}

/*
  if called from a field function, then the current field variables are destroyed, so that call should be the last call in the field callback.
*/
void mui_RestoreForm(mui_t *ui)
{
  mui_GotoForm(ui, ui->last_form_id, ui->last_form_cursor_focus_position);
}

/*
  Save a cursor position for mui_GotoFormAutoCursorPosition command
  Two such positions is stored.
*/
void mui_SaveCursorPosition(mui_t *ui, uint8_t cursor_position)
{
  uint8_t form_id = mui_get_fds_char(ui->current_form_fds+1);
  MUI_DEBUG("mui_SaveCursorPosition form_id=%d cursor_position=%d\n", form_id, cursor_position);
  
  if ( form_id == ui->menu_form_id[0] )
    ui->menu_form_last_added = 0;
  else if ( form_id == ui->menu_form_id[1] )
    ui->menu_form_last_added = 1;
  else 
    ui->menu_form_last_added ^= 1;
  ui->menu_form_id[ui->menu_form_last_added] = form_id;
  ui->menu_form_cursor_focus_position[ui->menu_form_last_added] = cursor_position;
  MUI_DEBUG("mui_SaveCursorPosition ui->menu_form_last_added=%d \n", ui->menu_form_last_added);
}

/*
  Similar to mui_GotoForm, but will jump to previously stored cursor location (mui_SaveCursorPosition) or 0 if the cursor position was not saved.
*/
uint8_t mui_GotoFormAutoCursorPosition(mui_t *ui, uint8_t form_id)
{
  uint8_t cursor_position = 0;
  if ( form_id == ui->menu_form_id[0] )
    cursor_position = ui->menu_form_cursor_focus_position[0];
  if ( form_id == ui->menu_form_id[1] )
    cursor_position = ui->menu_form_cursor_focus_position[1];
  MUI_DEBUG("mui_GotoFormAutoCursorPosition form_id=%d cursor_position=%d\n", form_id, cursor_position);
  return mui_GotoForm(ui, form_id, cursor_position);
}

/*
  return current form id or -1 if the menu system is inactive
*/
int mui_GetCurrentFormId(mui_t *ui)
{
  if ( mui_IsFormActive(ui) == 0 )
    return -1;
  return mui_get_fds_char(ui->current_form_fds+1);
}

/*
  updates "ui->cursor_focus_fds"
*/
/*
  if called from a field function, then the current field variables are destroyed, so that call should be the last call in the field callback.
*/
void mui_NextField(mui_t *ui)
{
  do 
  {
    if ( mui_send_cursor_msg(ui, MUIF_MSG_EVENT_NEXT) )
      return;
    mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_LEAVE);
    mui_next_field(ui);
  } while ( mui_send_cursor_enter_msg(ui) == 255 );
}

/*
  updates "ui->cursor_focus_fds"
*/
/*
  if called from a field function, then the current field variables are destroyed, so that call should be the last call in the field callback.
*/
void mui_PrevField(mui_t *ui)
{
  do
  {
    if ( mui_send_cursor_msg(ui, MUIF_MSG_EVENT_PREV) )
      return;
    mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_LEAVE);
 
    mui_loop_over_form(ui, mui_task_find_prev_cursor_uif);
    ui->cursor_focus_fds = ui->target_fds;      // NULL is ok  
    if ( ui->target_fds == NULL )
    {
      //ui->cursor_focus_position = 0;
      mui_loop_over_form(ui, mui_task_find_last_cursor_uif);
      ui->cursor_focus_fds = ui->target_fds;      // NULL is ok  
    }
  } while( mui_send_cursor_enter_msg(ui) == 255 );
}


void mui_SendSelect(mui_t *ui)
{
  mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_SELECT);  
}

/*
  Same as mui_SendSelect(), but will try to find a field, which is marked as "execute on select" (MUIF_EXECUTE_ON_SELECT_BUTTON).
  If such a field exists, then this field is executed, otherwise the current field will receive the select message.

  MUIF_EXECUTE_ON_SELECT_BUTTON is set by muif macro MUIF_EXECUTE_ON_SELECT_BUTTON
  
  used by MUIInputVersatileRotaryEncoder.ino example
*/
void mui_SendSelectWithExecuteOnSelectFieldSearch(mui_t *ui)
{
  mui_loop_over_form(ui, mui_task_find_execute_on_select_field);  /* Is there a exec on select field? */
  if ( ui->target_fds != NULL )       /* yes, found, ui->fds already points to the field */
  {
    fds_t *exec_on_select_field = ui->target_fds;
    mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_LEAVE);
    ui->cursor_focus_fds = exec_on_select_field;    /* more cursor on the "exec on select" field */
    mui_send_cursor_enter_msg(ui);      
    mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_SELECT);  
  }
  else
  {
    /* no "exec on select" field found, just send the select message to the field */
    mui_send_cursor_msg(ui, MUIF_MSG_CURSOR_SELECT);  
  }
}


void mui_SendValueIncrement(mui_t *ui)
{
  mui_send_cursor_msg(ui, MUIF_MSG_VALUE_INCREMENT);  
}

void mui_SendValueDecrement(mui_t *ui)
{
  mui_send_cursor_msg(ui, MUIF_MSG_VALUE_DECREMENT);  
}
