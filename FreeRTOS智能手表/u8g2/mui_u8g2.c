/*

  mui_u8g2.c

  Monochrome minimal user interface: Glue code between mui and u8g2.

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

*/

/*
  文件: mui_u8g2.c
  功能: MUI（菜单用户界面）与u8g2图形库的胶水代码（连接层）
        将MUI核心库（mui.c）的抽象菜单功能与u8g2的具体绘图功能连接起来，
        提供各种预定义的菜单字段（Field）函数，用于在OLED/LCD上绘制和交互。

  命名规则: mui_u8g2_[动作]_[宽度]_[编辑模式]_[样式]
    动作:
      draw_text        - 绘制文本标签
      btn_goto         - 跳转按钮（跳转到指定表单）
      btn_exit         - 退出按钮（退出菜单系统）
      u8_value_0_9     - 0-9数值输入
      u8_min_max       - 带最小/最大值的数值输入
      u8_bar           - 带进度条的数值输入
      u8_chkbox        - 复选框
      u8_radio         - 单选按钮
      u8_opt_line      - 单行选项选择
      u8_opt_parent    - 选项父菜单
      u8_opt_child     - 选项子菜单
      u8_char          - 单字符输入
      u16_list         - 16位列表选择

    宽度:
      wm  - 最小宽度（根据内容自动调整）
      wa  - 宽度由FDS参数指定
      w1  - 全屏宽度
      w2  - 半屏宽度

    编辑模式:
      mse - 选择模式：选择事件直接增加值或激活字段
      mud - 上下模式：选择进入编辑模式，下一个/上一个事件增/减值

    样式:
      pi  - 未选中=普通，选中=反转，编辑=反转+边框
      fi  - 未选中=边框，选中=反转+边框，编辑=边框
      pf  - 未选中=普通，选中=边框，编辑=反转+边框
      if  - 未选中=反转，选中=边框，编辑=反转+边框
*/

/*

  field function naming convention

    action
      draw_text:                        (rename from draw label)
      draw_str:                      
      btn_jmp   button jump to:                      a button which jumps to a specific form
      btn_exit          button leave:                     a button which leaves the form and places an exit code into a uint8 variable
      u8_value_0_9      
      u8_chkbox
      u8_radio
      u8_opt_line       edit value options in the same line
      u8_opt_parent       edit value options parent
      u8_opt_child       edit value options child
    
    
    field width (not for draw text/str)
      wm                minimum width
      wa                width can be provided via FDS argument
      w1                full display width
      w2                half display size (minus some pixel)
      w3                one/third of the dispay width (minus some pixel)

    edit mode  (not for draw text/str, buttons and checkbox)                  
      mse       select: select event will increment the value or activate the field (buttons)
      mud      up/down:  select will enter the up/down edit mode. Next/prev event will increment/decrement the value
      
    styles (not for draw text/str)
      unselected                selected                        up/down edit                            postfix         Use for
      plain                          invers                             invers + gap + frame            pi                      input elements
      frame                         invers+frame                frame                                       fi                  buttons
      
      plain                          frame                              invers + frame                         pf               input elements
      invers                        frame                               invers + frame                          if              buttons
      
      
    mui_u8g2_[action]_[field_width]_[edit_mode]_[style]

  mui _label_u8g2 --> mui_u8g2_draw_text
  mui _goto_frame_button_invers_select_u8g2                              --> mui_u8g2_btn_goto_wm_fi
  mui _goto_half_width_frame_button_invers_select_u8g2           --> mui_u8g2_btn_goto_w2_fi
  mui _goto_line_button_invers_select_u8g2 -->  mui_u8g2_btn_goto_w1_fi
  mui _leave_menu_frame_button_invers_select_u8g2 --> mui_u8g2_btn_exit_wm_fi
  
  mui _input_uint8_invers_select_u8g2 --> mui_u8g2_u8_value_0_9_wm_mse_pi
  mui _single_line_option_invers_select_u8g2     --> mui_u8g2_u8_opt_line_wa_mse_pi
  mui _select_options_parent_invers_select_u8g2  --> mui_u8g2_u8_opt_parent_wa_mse_pi
  mui _select_options_child_invers_select_u8g2  --> mui_u8g2_u8_opt_child_wm_pi

  mui _checkbox_invers_select_u8g2 --> mui_u8g2_u8_chkbox_wm_pi
  mui _radio_invers_select_u8g2 --> mui_u8g2_u8_radio_wm_pi

  mui _input_char_invers_select_u8g2 --> mui_u8g2_u8_char_wm_mud_pi



  2 Buttons
    Only use "mse", don't use "mud"
  
    Button      Call                            Description
    1                mui_SendSelect()    Activate elements & change values
    2                mui_NextField()      Goto next field
    
  3 Buttons
    Use "mse" or "mud"
    Button      Call                            Description
    1                mui_SendSelect()    Activate elements / change values (mse) / enter "mud" mode (mud)
    2                mui_NextField()      Goto next field, increment value (mud)
    3                mui_PrevField()      Goto prev field, decrement value (mud)
    
  4 Buttons
    Prefer "mse"
    Button      Call                                            Description
    1                mui_SendValueIncrement()    Activate elements / increment values (mse)
    2                mui_SendValueDecrement()   Activate elements / decrement values (mse)
    3                mui_NextField()                       Goto next field
    4                mui_PrevField()                        Goto prev field

  5 Buttons
    Prefer "mse", use the MUIF_EXECUTE_ON_SELECT_BUTTON on forms to finish the form with the "form select" button 5
    Button      Call                                                                                            Description
    1                mui_SendValueIncrement()                                                           Activate elements / increment values (mse)
    2                mui_SendValueDecrement()                                                         Activate elements / decrement values (mse)
    3                mui_NextField()                                                                            Goto next field
    4                mui_PrevField()                                                                     Goto prev field
    5                mui_SendSelectWithExecuteOnSelectFieldSearch()             Execute the MUIF_EXECUTE_ON_SELECT_BUTTON button or activate the current element if there is no EOS button
    
  rotary encoder, push&release
    Prefer "mud"
    Button      Call                            Description
    encoder button                 mui_SendSelect()    Activate elements / change values (mse) / enter "mud" mode (mud)
    encoder CW                      mui_NextField()      Goto next field, increment value (mud)
    encoder CCW                    mui_PrevField()      Goto prev field, decrement value (mud)
  
  rotary encoder, push&rotate
    Prefer "mse"
    Button                                      Call                                            Description
    encoder CW                                  mui_SendValueIncrement()    Activate elements / increment values (mse)
    encoder CCW                                 mui_SendValueDecrement()   Activate elements / decrement values (mse)
    encoder CW+button press                mui_NextField()                       Goto next field
    encoder CCW+button press                mui_PrevField()                        Goto prev field


*/



#include "mui.h"
#include "u8g2.h"
#include "mui_u8g2.h"

/*

uint8_t mui_template(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  //ui->dflags                          MUIF_DFLAG_IS_CURSOR_FOCUS       MUIF_DFLAG_IS_TOUCH_FOCUS
  //muif_get_cflags(ui->uif)       MUIF_CFLAG_IS_CURSOR_SELECTABLE
  //muif_get_data(ui->uif)
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_VALUE_INCREMENT:
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}
*/

/*=========================================================================*/
#define MUI_U8G2_V_PADDING 1  /* 垂直内边距（像素） */

/*=========================================================================*/
/* 额外的u8g2绘图辅助函数 */

/*
  函数: u8g2_DrawCheckbox (内部函数)
  功能: 绘制一个复选框
        先绘制外框，如果is_checked为真则在内部绘制填充方块表示选中状态
  参数:
    u8g2       - u8g2图形设备指针
    x          - 复选框左上角X坐标
    y          - 复选框基线Y坐标
    w          - 复选框的宽度和高度（正方形）
    is_checked - 是否选中（1=选中，0=未选中）
  返回值: 无
*/
static void u8g2_DrawCheckbox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t is_checked) MUI_NOINLINE;
static void u8g2_DrawCheckbox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t is_checked)
{
  u8g2_DrawFrame(u8g2, x, y-w, w, w);  /* 绘制复选框外框 */
  if ( is_checked )
  {
    w-=4;                               /* 内部填充方块比外框小4像素（每边缩进2像素） */
    u8g2_DrawBox(u8g2, x+2, y-w-2, w, w);  /* 绘制填充方块表示选中 */
  }
}

/*
  函数: u8g2_DrawValueMark (内部函数)
  功能: 绘制一个值标记（实心方块），用于单选按钮等场景表示当前选中项
  参数:
    u8g2 - u8g2图形设备指针
    x    - 方块左上角X坐标
    y    - 方块基线Y坐标
    w    - 方块的宽度和高度
  返回值: 无
*/
static void u8g2_DrawValueMark(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w)
{
  u8g2_DrawBox(u8g2, x, y-w, w, w);    /* 绘制实心方块标记 */
}


/*=========================================================================*/
/* 辅助函数 */

/*
  函数: mui_get_x
  功能: 获取当前字段的X坐标
        如果显示器宽度>=255像素，则将坐标除以2（因为FDS中坐标以字符为单位）
  参数:
    ui - MUI用户界面结构体指针
  返回值: 转换后的X像素坐标
*/
u8g2_uint_t mui_get_x(mui_t *ui) MUI_NOINLINE;
u8g2_uint_t mui_get_x(mui_t *ui)
{
  if ( u8g2_GetDisplayWidth(mui_get_U8g2(ui)) >= 255 )
      return ui->x / 2;  /* 大屏：坐标除以2 */
  return ui->x;           /* 小屏：直接使用原始坐标 */
}

/*
  函数: mui_get_y
  功能: 获取当前字段的Y坐标
  参数:
    ui - MUI用户界面结构体指针
  返回值: Y像素坐标
*/
u8g2_uint_t mui_get_y(mui_t *ui)
{
  return ui->y;
}

/*
  函数: mui_get_U8g2
  功能: 从MUI结构体中获取u8g2图形设备指针
  参数:
    ui - MUI用户界面结构体指针
  返回值: u8g2图形设备指针
*/
u8g2_t *mui_get_U8g2(mui_t *ui)
{
  return (u8g2_t *)(ui->graphics_data);
}

/*
  函数: mui_u8g2_draw_button_utf
  功能: 绘制UTF8文本按钮的通用函数
        所有按钮绘制函数最终都调用此函数来完成实际绘制
  参数:
    ui        - MUI用户界面结构体指针
    flags     - 按钮标志（如U8G2_BTN_INV反转、U8G2_BTN_XFRAME边框等）
    width     - 按钮宽度（0表示自动适应文本宽度）
    padding_h - 水平内边距（像素）
    padding_v - 垂直内边距（像素）
    text      - 按钮文本内容
  返回值: 无
*/
//void u8g2_DrawButtonUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t flags, u8g2_uint_t width, u8g2_uint_t padding_h, u8g2_uint_t padding_v, const char *text);
void mui_u8g2_draw_button_utf(mui_t *ui, u8g2_uint_t flags, u8g2_uint_t width, u8g2_uint_t padding_h, u8g2_uint_t padding_v, const char *text)
{
  if ( text==NULL)
    text = "";  /* 文本为空时使用空字符串，防止空指针 */
  u8g2_DrawButtonUTF8(mui_get_U8g2(ui), mui_get_x(ui), mui_get_y(ui), flags, width, padding_h, padding_v, text);
}

/*
  函数: mui_u8g2_get_pi_flags (内部函数)
  功能: 获取"pi"样式的按钮标志
        pi样式: 未选中=普通，选中=反转，编辑模式(mud)=反转+额外边框
  参数:
    ui - MUI用户界面结构体指针
  返回值: u8g2按钮标志位组合
*/
u8g2_uint_t mui_u8g2_get_pi_flags(mui_t *ui)
{
  u8g2_uint_t flags = 0;
  if ( mui_IsCursorFocus(ui) )
  {
    flags |= U8G2_BTN_INV;          /* 有光标焦点时反转显示 */
    if ( ui->is_mud )
    {
      flags |= U8G2_BTN_XFRAME;     /* 编辑模式下添加额外边框 */
    }
  }
  return flags;
}

/*
  函数: mui_u8g2_draw_button_pi
  功能: 使用"pi"样式绘制按钮
        pi样式适用于输入类元素
  参数:
    ui        - MUI用户界面结构体指针
    width     - 按钮宽度
    padding_h - 水平内边距
    text      - 按钮文本
  返回值: 无
*/
void mui_u8g2_draw_button_pi(mui_t *ui, u8g2_uint_t width, u8g2_uint_t padding_h, const char *text)
{
  mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), width, padding_h , MUI_U8G2_V_PADDING, text);
}

/*
  函数: mui_u8g2_get_fi_flags (内部函数)
  功能: 获取"fi"样式的按钮标志
        fi样式: 未选中=边框，选中=反转+边框，编辑模式(mud)=仅边框（取消反转）
  参数:
    ui - MUI用户界面结构体指针
  返回值: u8g2按钮标志位组合
*/
u8g2_uint_t mui_u8g2_get_fi_flags(mui_t *ui)
{
  u8g2_uint_t flags = 1;             /* 默认带边框（bit0=1） */
  if ( mui_IsCursorFocus(ui) )
  {
    flags |= U8G2_BTN_INV;          /* 有光标焦点时反转显示 */
    if ( ui->is_mud )
    {
      flags = 1;                     /* 编辑模式下取消反转，仅保留边框 */
    }
  }
  return flags;
}

/*
  函数: mui_u8g2_draw_button_fi
  功能: 使用"fi"样式绘制按钮
        fi样式适用于按钮类元素
  参数:
    ui        - MUI用户界面结构体指针
    width     - 按钮宽度
    padding_h - 水平内边距
    text      - 按钮文本
  返回值: 无
*/
void mui_u8g2_draw_button_fi(mui_t *ui, u8g2_uint_t width, u8g2_uint_t padding_h, const char *text)
{
  mui_u8g2_draw_button_utf(ui, mui_u8g2_get_fi_flags(ui), width, padding_h , MUI_U8G2_V_PADDING, text);
}

/*
  函数: mui_u8g2_get_pf_flags (内部函数)
  功能: 获取"pf"样式的按钮标志
        pf样式: 未选中=普通，选中=边框，编辑模式(mud)=反转+边框
  参数:
    ui - MUI用户界面结构体指针
  返回值: u8g2按钮标志位组合
*/
u8g2_uint_t mui_u8g2_get_pf_flags(mui_t *ui)
{
  u8g2_uint_t flags = 0;
  if ( mui_IsCursorFocus(ui) )
  {
    flags |= 1;                      /* 有光标焦点时显示边框 */
    if ( ui->is_mud )
    {
      flags |= U8G2_BTN_INV;        /* 编辑模式下反转显示 */
    }
  }
  return flags;
}

/*
  函数: mui_u8g2_draw_button_pf
  功能: 使用"pf"样式绘制按钮
        pf样式适用于输入类元素（替代pi的另一种视觉风格）
  参数:
    ui        - MUI用户界面结构体指针
    width     - 按钮宽度
    padding_h - 水平内边距
    text      - 按钮文本
  返回值: 无
*/
void mui_u8g2_draw_button_pf(mui_t *ui, u8g2_uint_t width, u8g2_uint_t padding_h, const char *text)
{
  mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pf_flags(ui), width, padding_h , MUI_U8G2_V_PADDING, text);
}

/*
  函数: mui_u8g2_get_if_flags (内部函数)
  功能: 获取"if"样式的按钮标志
        if样式: 未选中=反转，选中=边框，编辑模式(mud)=反转+边框
  参数:
    ui - MUI用户界面结构体指针
  返回值: u8g2按钮标志位组合
*/
u8g2_uint_t mui_u8g2_get_if_flags(mui_t *ui)
{
  u8g2_uint_t flags = 0;
  if ( mui_IsCursorFocus(ui) )
  {
    if ( ui->is_mud )
    {
      flags |= 1;                    /* 编辑模式：边框 */
      flags |= U8G2_BTN_INV;        /* 编辑模式：反转 */
    }
    else
    {
      flags |= 1;                    /* 有焦点时仅边框 */
    }
  }
  else
  {
      flags |= U8G2_BTN_INV;        /* 无焦点时反转（默认状态） */
  }
  return flags;
}

/*
  函数: mui_u8g2_draw_button_if
  功能: 使用"if"样式绘制按钮
        if样式适用于按钮类元素（替代fi的另一种视觉风格）
  参数:
    ui        - MUI用户界面结构体指针
    width     - 按钮宽度
    padding_h - 水平内边距
    text      - 按钮文本
  返回值: 无
*/
void mui_u8g2_draw_button_if(mui_t *ui, u8g2_uint_t width, u8g2_uint_t padding_h, const char *text)
{
  mui_u8g2_draw_button_utf(ui, mui_u8g2_get_if_flags(ui), width, padding_h , MUI_U8G2_V_PADDING, text);
}


/*
  函数: mui_u8g2_handle_scroll_next_prev_events (内部函数)
  功能: 处理可滚动菜单的上下导航事件
        管理form_scroll_top（滚动偏移量），实现列表的循环滚动。
        当用户导航到可见区域边界时，自动调整滚动位置。

  参数:
    ui  - MUI用户界面结构体指针
    msg - 消息类型（MUIF_MSG_CURSOR_ENTER/EVENT_NEXT/EVENT_PREV）
  返回值:
    255 - 字段不可见，应跳过（光标进入时）
    1   - 滚动已更新，需要重绘
    0   - 无特殊处理
*/
static uint8_t mui_u8g2_handle_scroll_next_prev_events(mui_t *ui, uint8_t msg) MUI_NOINLINE;
static uint8_t mui_u8g2_handle_scroll_next_prev_events(mui_t *ui, uint8_t msg)
{
  uint8_t arg = ui->arg;  /* 当前字段在可见区域中的位置索引 */
  switch(msg)
  {
    case MUIF_MSG_CURSOR_ENTER:
      /* 如果字段位置超出了总项目数，返回255表示不可见，应跳过 */
      if ( (arg > 0) && (ui->form_scroll_top + arg >= ui->form_scroll_total) )
        return 255;
      break;
    case MUIF_MSG_EVENT_NEXT:
      /* 导航到下一个字段时，如果到达可见区域底部则滚动 */
      if ( arg+1 == ui->form_scroll_visible )
      {
        if ( ui->form_scroll_visible + ui->form_scroll_top < ui->form_scroll_total )
        {
          ui->form_scroll_top++;  /* 向下滚动一行 */
          return 1;               /* 标记需要重绘 */
        }
        else
        {
          ui->form_scroll_top = 0;  /* 循环回到顶部 */
        }
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      /* 导航到上一个字段时，如果到达可见区域顶部则向上滚动 */
      if ( arg == 0 )
      {
        if ( ui->form_scroll_top > 0 )
        {
          ui->form_scroll_top--;  /* 向上滚动一行 */
          return 1;               /* 标记需要重绘 */
        }
        else
        {
          /* 循环到底部 */
          if ( ui->form_scroll_total >  ui->form_scroll_visible  )
          {
            ui->form_scroll_top = ui->form_scroll_total - ui->form_scroll_visible;
          }
          else
          {
            ui->form_scroll_top = 0;
          }
        }
      }
      break;
  }
  return 0;
}

/*=========================================================================*/
/* simplified style function  */

/*
  函数: mui_u8g2_set_font_style_function
  功能: 设置u8g2字体的字段函数
        配合MUIF_U8G2_FONT_STYLE(n,font)宏使用，
        在绘制消息到来时设置当前字体。
  参数:
    ui  - MUI用户界面结构体指针
    msg - 消息类型
  返回值: 总是返回0（继续遍历）
*/
/*
Used for MUIF_U8G2_FONT_STYLE(n,font)
*/

uint8_t mui_u8g2_set_font_style_function(mui_t *ui, uint8_t msg)
{
  if ( msg == MUIF_MSG_DRAW )
  {
    u8g2_SetFont(mui_get_U8g2(ui), (uint8_t *)muif_get_data(ui->uif));  /* 从muif数据中获取字体指针并设置 */
  }
  return 0;
}



/*=========================================================================*/
/* field functions */

/*
  函数: mui_u8g2_draw_text
  功能: 在指定位置绘制静态文本标签（不可选择，仅显示）
        用于在菜单界面中显示说明文字、标题等。

  FDS字段格式: MUI_XYT(xy坐标, "文本内容")
    - 需要X、Y坐标
    - 需要文本参数
    - 不需要arg参数

  消息处理: 仅处理MUIF_MSG_DRAW（绘制消息）

  muif配置:
    flags: 无特殊标志（不可选择）
    data: 未使用
*/
/*
  xy: yes, arg: no, text: yes
*/

uint8_t mui_u8g2_draw_text(mui_t *ui, uint8_t msg)
{
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      u8g2_DrawStr(mui_get_U8g2(ui), mui_get_x(ui), mui_get_y(ui), ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
      break;
    case MUIF_MSG_VALUE_INCREMENT:
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
  }
  return 0;
}


/*
  函数: mui_u8g2_btn_goto_wm_fi
  功能: 最小宽度的跳转按钮（fi样式）
        按钮宽度等于文本宽度加1像素内边距。
        选中后跳转到arg指定的表单（使用自动光标位置恢复）。

  FDS字段格式: MUI_GOTO(xy坐标, 表单ID, "按钮文本")
    - 需要X、Y坐标
    - arg: 目标表单编号（必需）
    - text: 按钮标签文本

  样式:
    未选中: 文本 + 边框
    选中: 反转文本 + 边框

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE（可被光标选择）
    data: 未使用

  消息处理: DRAW（绘制）、CURSOR_SELECT/VALUE_INCREMENT/VALUE_DECREMENT（跳转）
*/
uint8_t mui_u8g2_btn_goto_wm_fi(mui_t *ui, uint8_t msg)
{
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_utf(ui, U8G2_BTN_HCENTER |mui_u8g2_get_fi_flags(ui), 0, 1, MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      //return mui_GotoForm(ui, ui->arg, 0);
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
    
  }
  return 0;
}

/*
  函数: mui_u8g2_btn_goto_wm_if
  功能: 最小宽度的跳转按钮（if样式）
        if样式: 未选中=反转，选中=边框，编辑模式=反转+边框
        选中后跳转到arg指定的表单。
  参数:
    ui  - MUI用户界面结构体指针
    msg - 消息类型
  返回值: 0（继续遍历），跳转时返回值由mui_GotoFormAutoCursorPosition决定
*/
uint8_t mui_u8g2_btn_goto_wm_if(mui_t *ui, uint8_t msg)
{
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_utf(ui, U8G2_BTN_HCENTER |mui_u8g2_get_if_flags(ui), 0, 1, MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      //return mui_GotoForm(ui, ui->arg, 0);
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
   case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
    
  }
  return 0;
}

/*
  函数: mui_u8g2_btn_goto_w2_fi
  功能: 半屏宽度的跳转按钮（fi样式）
        按钮宽度为显示器宽度的一半减去10像素。
        适合在一行中并排放置两个按钮。
  参数/返回值: 同mui_u8g2_btn_goto_wm_fi
*/
uint8_t mui_u8g2_btn_goto_w2_fi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_utf(ui, U8G2_BTN_HCENTER | mui_u8g2_get_fi_flags(ui), u8g2_GetDisplayWidth(u8g2)/2 - 10, 0, MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      //return mui_GotoForm(ui, ui->arg, 0);
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
  }
  return 0;
}

/*
  函数: mui_u8g2_btn_goto_w2_if
  功能: 半屏宽度的跳转按钮（if样式）
        按钮宽度为显示器宽度的一半减去10像素。
  参数/返回值: 同mui_u8g2_btn_goto_wm_fi
*/
uint8_t mui_u8g2_btn_goto_w2_if(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_utf(ui, U8G2_BTN_HCENTER | mui_u8g2_get_if_flags(ui), u8g2_GetDisplayWidth(u8g2)/2 - 10, 0, MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      //return mui_GotoForm(ui, ui->arg, 0);
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
  }
  return 0;
}

/*
  函数: mui_u8g2_btn_exit_wm_fi
  功能: 最小宽度的退出按钮（fi样式）
        按钮宽度等于文本宽度加1像素内边距。
        选中后退出菜单系统，并将arg值存储到muif的data指针指向的变量中（如果不是NULL）。
        arg值可以用作按钮的退出码，供应用程序判断是哪个退出按钮被按下。

  FDS字段格式: MUI_XYA(xy坐标, arg值, "按钮文本")
    - 需要X、Y坐标
    - arg: 存储到*data的退出值（可选）
    - text: 按钮标签文本

  样式:
    未选中: 文本 + 边框
    选中: 反转文本 + 边框

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: 可选地指向uint8_t变量，接收退出值

  消息处理: DRAW（绘制）、CURSOR_SELECT/VALUE_INCREMENT/VALUE_DECREMENT（退出并存储值）
*/
uint8_t mui_u8g2_btn_exit_wm_fi(mui_t *ui, uint8_t msg)
{
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_utf(ui, U8G2_BTN_HCENTER |mui_u8g2_get_fi_flags(ui), 0, 1, MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      {
        uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
        if ( value != NULL )
          *value = ui->arg;
      }
      mui_SaveForm(ui);          // store the current form and position so that the child can jump back
      mui_LeaveForm(ui);
      return 1;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
  }
  return 0;
}


/*
  函数: mui_u8g2_btn_goto_w1_pi
  功能: 全屏宽度的跳转按钮（pi样式）
        按钮宽度为显示器宽度减去左右边距。
  参数:
    ui  - MUI用户界面结构体指针
    msg - 消息类型
  返回值: 0（继续遍历），跳转时由mui_GotoFormAutoCursorPosition决定
*/
uint8_t mui_u8g2_btn_goto_w1_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_pi(ui, u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui) , ui->text);
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui) , MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      //return mui_GotoForm(ui, ui->arg, 0);
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
  }
  return 0;
}


/*
  函数: mui_u8g2_btn_goto_w1_fi
  功能: 全屏宽度的跳转按钮（fi样式）
        按钮宽度为显示器宽度减去左右边距。
  参数/返回值: 同mui_u8g2_btn_goto_w1_pi
*/
uint8_t mui_u8g2_btn_goto_w1_fi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_fi(ui, u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui)-1 , ui->text);
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui) , MUI_U8G2_V_PADDING, ui->text);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      //return mui_GotoForm(ui, ui->arg, 0);
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;    
  }
  return 0;
}

/*===============================================================================*/

/*
  函数: mui_u8g2_u8_vmm_draw_wm_pi (内部函数)
  功能: 绘制带最小/最大值约束的uint8数值按钮（pi样式）
        根据最大值自动确定显示位数（1-3位），
        对数值进行范围限制后以按钮形式绘制。
  参数:
    ui - MUI用户界面结构体指针（muif数据应为mui_u8g2_u8_min_max_t*类型）
  返回值: 无
*/
static void mui_u8g2_u8_vmm_draw_wm_pi(mui_t *ui) MUI_NOINLINE;
static void mui_u8g2_u8_vmm_draw_wm_pi(mui_t *ui)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_u8_min_max_t *vmm= (mui_u8g2_u8_min_max_t *)muif_get_data(ui->uif);
  char buf[4] = "999";
  char *s = buf;
  uint8_t *value = mui_u8g2_u8mm_get_valptr(vmm);
  uint8_t min = mui_u8g2_u8mm_get_min(vmm);
  uint8_t max = mui_u8g2_u8mm_get_max(vmm);
  uint8_t cnt = 3;
  
  if ( *value > max ) 
    *value = max;
  if ( *value <= min )
    *value = min;
  if ( max < 100 )
  {
    s++;
    cnt--;
  }
  if ( max < 10 )
  {
    s++;
    cnt--;
  }
  //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetStrWidth(u8g2, s)+1, 1, MUI_U8G2_V_PADDING, u8x8_u8toa(*value, cnt));
  mui_u8g2_draw_button_pi(ui, u8g2_GetStrWidth(u8g2, s)+1, 1, u8x8_u8toa(*value, cnt));
}


/*
  函数: mui_u8g2_u8_min_max_wm_mse_pi
  功能: 最小宽度的uint8数值输入字段（mse选择模式，pi样式）
        选择事件直接增/减值，到达最大值后循环到最小值。
        适用于旋转编码器的"按下旋转"操作方式。

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: mui_u8g2_u8_min_max_t*，包含值指针、最小值、最大值

  FDS字段格式: MUI_DATA2(xy坐标, "标签")，arg未使用
  返回值: 总是返回0
*/
uint8_t mui_u8g2_u8_min_max_wm_mse_pi(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_t *vmm= (mui_u8g2_u8_min_max_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mm_get_valptr(vmm);
  uint8_t min = mui_u8g2_u8mm_get_min(vmm);
  uint8_t max = mui_u8g2_u8mm_get_max(vmm);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_vmm_draw_wm_pi(ui);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*value)++;
      if ( *value > max ) *value = min;
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *value > min ) (*value)--; else *value = max;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_min_max_wm_mud_pi
  功能: 最小宽度的uint8数值输入字段（mud上下模式，pi样式）
        选择事件切换编辑模式（is_mud），进入编辑模式后
        使用上一个/下一个事件来增/减值。
        适用于旋转编码器的"按下释放"操作方式。
  参数/返回值: 同mui_u8g2_u8_min_max_wm_mse_pi
*/
uint8_t mui_u8g2_u8_min_max_wm_mud_pi(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_t *vmm= (mui_u8g2_u8_min_max_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mm_get_valptr(vmm);
  uint8_t min = mui_u8g2_u8mm_get_min(vmm);
  uint8_t max = mui_u8g2_u8mm_get_max(vmm);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_vmm_draw_wm_pi(ui);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
     /* toggle between normal mode and capture next/prev mode */
      ui->is_mud = !ui->is_mud;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*value)++;
        if ( *value > max )
          *value = min;
        return 1; 
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *value <= min )
          *value = max;
        else
          (*value)--;
        return 1;
      }
      break;
  }
  return 0;
}



/*
  函数: mui_u8g2_u8_vmm_draw_wm_pf (内部函数)
  功能: 绘制带最小/最大值约束的uint8数值按钮（pf样式）
        与pi版本类似，但使用pf样式标志
  参数:
    ui - MUI用户界面结构体指针
  返回值: 无
*/
static void mui_u8g2_u8_vmm_draw_wm_pf(mui_t *ui) MUI_NOINLINE;
static void mui_u8g2_u8_vmm_draw_wm_pf(mui_t *ui)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_u8_min_max_t *vmm= (mui_u8g2_u8_min_max_t *)muif_get_data(ui->uif);
  char buf[4] = "999";
  char *s = buf;
  uint8_t *value = mui_u8g2_u8mm_get_valptr(vmm);
  uint8_t min = mui_u8g2_u8mm_get_min(vmm);
  uint8_t max = mui_u8g2_u8mm_get_max(vmm);
  uint8_t cnt = 3;
  
  if ( *value > max ) 
    *value = max;
  if ( *value <= min )
    *value = min;
  if ( max < 100 )
  {
    s++;
    cnt--;
  }
  if ( max < 10 )
  {
    s++;
    cnt--;
  }
  //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetStrWidth(u8g2, s)+1, 1, MUI_U8G2_V_PADDING, u8x8_u8toa(*value, cnt));
  mui_u8g2_draw_button_pf(ui, u8g2_GetStrWidth(u8g2, s)+1, 1, u8x8_u8toa(*value, cnt));
}


/*
  函数: mui_u8g2_u8_min_max_wm_mse_pf
  功能: 最小宽度的uint8数值输入字段（mse选择模式，pf样式）
        pf样式: 未选中=普通，选中=边框，编辑=反转+边框
  参数/返回值: 同mui_u8g2_u8_min_max_wm_mse_pi
*/
uint8_t mui_u8g2_u8_min_max_wm_mse_pf(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_t *vmm= (mui_u8g2_u8_min_max_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mm_get_valptr(vmm);
  uint8_t min = mui_u8g2_u8mm_get_min(vmm);
  uint8_t max = mui_u8g2_u8mm_get_max(vmm);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_vmm_draw_wm_pf(ui);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*value)++;
      if ( *value > max ) *value = min;
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *value > min ) (*value)--; else *value = max;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_min_max_wm_mud_pf
  功能: 最小宽度的uint8数值输入字段（mud上下模式，pf样式）
  参数/返回值: 同mui_u8g2_u8_min_max_wm_mse_pi
*/
uint8_t mui_u8g2_u8_min_max_wm_mud_pf(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_t *vmm= (mui_u8g2_u8_min_max_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mm_get_valptr(vmm);
  uint8_t min = mui_u8g2_u8mm_get_min(vmm);
  uint8_t max = mui_u8g2_u8mm_get_max(vmm);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_vmm_draw_wm_pf(ui);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      /* toggle between normal mode and capture next/prev mode */
      ui->is_mud = !ui->is_mud;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*value)++;
        if ( *value > max )
          *value = min;
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *value <= min )
          *value = max;
        else
          (*value)--;
        return 1;
      }
      break;
  }
  return 0;
}


/*===============================================================================*/

/*
  函数: mui_u8g2_u8_bar_draw_wm (内部函数)
  功能: 绘制带进度条的uint8数值显示
        绘制一个水平进度条来可视化当前值在最小/最大值范围中的位置。
        支持2x/4x缩放以增加进度条精度，可选择是否在进度条旁边显示数值。

  参数:
    ui    - MUI用户界面结构体指针（muif数据应为mui_u8g2_u8_min_max_step_t*类型）
    flags - 按钮边框标志（用于控制选中时的视觉样式）
  返回值: 无
*/
static void mui_u8g2_u8_bar_draw_wm(mui_t *ui, uint8_t flags) MUI_NOINLINE;
static void mui_u8g2_u8_bar_draw_wm(mui_t *ui, uint8_t flags)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_u8_min_max_step_t *vmms= (mui_u8g2_u8_min_max_step_t *)muif_get_data(ui->uif);
  char buf[4] = "999";
  char *s = buf;
  uint8_t *value = mui_u8g2_u8mms_get_valptr(vmms);
  uint8_t min = mui_u8g2_u8mms_get_min(vmms);
  uint8_t max = mui_u8g2_u8mms_get_max(vmms);
  uint8_t scale = 0;
  //uint8_t step = mui_u8g2_u8mms_get_step(vmms);
  uint8_t mms_flags = mui_u8g2_u8mms_get_flags(vmms);
  uint8_t cnt = 3;
  uint8_t height = u8g2_GetAscent(u8g2);
  int8_t backup_descent;
  u8g2_uint_t x = mui_get_x(ui);
  u8g2_uint_t w = 0;
  
  if ( mms_flags & MUI_MMS_2X_BAR )
    scale |= 1;
  if ( mms_flags & MUI_MMS_4X_BAR )
    scale |= 2;
  
  if ( *value > max ) 
    *value = max;
  if ( *value <= min )
    *value = min;
  if ( max < 100 )
  {
    s++;
    cnt--;
  }
  if ( max < 10 )
  {
    s++;
    cnt--;
  }
  //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetStrWidth(u8g2, s)+1, 1, MUI_U8G2_V_PADDING, u8x8_u8toa(*value, cnt));
  //mui_u8g2_draw_button_pi(ui, u8g2_GetStrWidth(u8g2, s)+1, 1, u8x8_u8toa(*value, cnt));
  
  w += (max<<scale)+2;
  u8g2_DrawFrame( u8g2, x, mui_get_y(ui)-height, w, height);
  u8g2_DrawBox( u8g2, x+1, mui_get_y(ui)-height+1, (*value)<<scale, height-2);
  if ( mms_flags & MUI_MMS_SHOW_VALUE )
  {
    w += 2;
    u8g2_DrawStr(u8g2,  x+w, mui_get_y(ui), u8x8_u8toa(*value, cnt) );
    w += u8g2_GetStrWidth(u8g2, s);
    w += 1; 
  }
  backup_descent = u8g2->font_ref_descent;
  u8g2->font_ref_descent = 0; /* hmm... that's a low level hack so that DrawButtonFrame ignores the descent value of the font */
  u8g2_DrawButtonFrame(u8g2, x, mui_get_y(ui), flags, w, 1, 1);
  u8g2->font_ref_descent = backup_descent;
  
}


/*
  函数: mui_u8g2_u8_bar_wm_mse_pi
  功能: 带进度条的uint8数值输入字段（mse选择模式，pi样式）
        选择事件按步长增/减值，到达最大值后循环到最小值。
        进度条会实时反映当前值在范围中的位置。

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: mui_u8g2_u8_min_max_step_t*，包含值指针、最小值、最大值、步长和标志

  FDS字段格式: MUI_DATA2(xy坐标, "标签")
  返回值: 总是返回0
*/
uint8_t mui_u8g2_u8_bar_wm_mse_pi(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_step_t *vmms= (mui_u8g2_u8_min_max_step_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mms_get_valptr(vmms);
  uint8_t min = mui_u8g2_u8mms_get_min(vmms);
  uint8_t max = mui_u8g2_u8mms_get_max(vmms);
  uint8_t step = mui_u8g2_u8mms_get_step(vmms);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_bar_draw_wm(ui, mui_u8g2_get_pi_flags(ui));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*value)+=step;
      if ( *value > max ) *value = min;
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *value >= min+step ) (*value)-=step; else *value = max;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}


/*
  函数: mui_u8g2_u8_bar_wm_mud_pi
  功能: 带进度条的uint8数值输入字段（mud上下模式，pi样式）
        选择切换编辑模式，上/下事件按步长增/减值。
  参数/返回值: 同mui_u8g2_u8_bar_wm_mse_pi
*/
uint8_t mui_u8g2_u8_bar_wm_mud_pi(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_step_t *vmms= (mui_u8g2_u8_min_max_step_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mms_get_valptr(vmms);
  uint8_t min = mui_u8g2_u8mms_get_min(vmms);
  uint8_t max = mui_u8g2_u8mms_get_max(vmms);
  uint8_t step = mui_u8g2_u8mms_get_step(vmms);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_bar_draw_wm(ui, mui_u8g2_get_pi_flags(ui));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      /* toggle between normal mode and capture next/prev mode */
      ui->is_mud = !ui->is_mud;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*value)+=step;
        if ( *value > max )
          *value = min;
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *value <= min || *value > max)
          *value = max;
        else
          (*value)-=step;
        return 1;
      }
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_bar_wm_mse_pf
  功能: 带进度条的uint8数值输入字段（mse选择模式，pf样式）
  参数/返回值: 同mui_u8g2_u8_bar_wm_mse_pi
*/
uint8_t mui_u8g2_u8_bar_wm_mse_pf(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_step_t *vmms= (mui_u8g2_u8_min_max_step_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mms_get_valptr(vmms);
  uint8_t min = mui_u8g2_u8mms_get_min(vmms);
  uint8_t max = mui_u8g2_u8mms_get_max(vmms);
  uint8_t step = mui_u8g2_u8mms_get_step(vmms);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_bar_draw_wm(ui, mui_u8g2_get_pf_flags(ui));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*value)+=step;
      if ( *value > max ) *value = min;
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *value >= min+step ) (*value)-=step; else *value = max;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_bar_wm_mud_pf
  功能: 带进度条的uint8数值输入字段（mud上下模式，pf样式）
  参数/返回值: 同mui_u8g2_u8_bar_wm_mse_pi
*/
uint8_t mui_u8g2_u8_bar_wm_mud_pf(mui_t *ui, uint8_t msg)
{
  mui_u8g2_u8_min_max_step_t *vmms= (mui_u8g2_u8_min_max_step_t *)muif_get_data(ui->uif);
  uint8_t *value = mui_u8g2_u8mms_get_valptr(vmms);
  uint8_t min = mui_u8g2_u8mms_get_min(vmms);
  uint8_t max = mui_u8g2_u8mms_get_max(vmms);
  uint8_t step = mui_u8g2_u8mms_get_step(vmms);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_u8_bar_draw_wm(ui, mui_u8g2_get_pf_flags(ui));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      /* toggle between normal mode and capture next/prev mode */
      ui->is_mud = !ui->is_mud;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*value)+=step;
        if ( *value > max )
          *value = min;
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *value <= min || *value > max)
          *value = max;
        else
          (*value)-=step;
        return 1;
      }
      break;
  }
  return 0;
}

/*===============================================================================*/

/*
  函数: mui_is_valid_char (内部函数)
  功能: 判断字符是否为有效的可输入字符
        有效字符包括：空格、大写字母A-Z、小写字母a-z、数字0-9
  参数:
    c - 要判断的字符
  返回值: 1=有效字符，0=无效字符
*/
static uint8_t mui_is_valid_char(uint8_t c) MUI_NOINLINE;
uint8_t mui_is_valid_char(uint8_t c)
{
  if ( c == 32 )
    return 1;
  if ( c >= 'A' && c <= 'Z' )
    return 1;
  if ( c >= 'a' && c <= 'z' )
    return 1;
  if ( c >= '0' && c <= '9' )
    return 1;
  return 0;
}



/*
  函数: mui_u8g2_u8_char_wm_mud_pi
  功能: 单字符输入字段（mud上下模式，pi样式）
        可以输入空格、字母和数字。选择切换编辑模式，
        上/下事件循环遍历所有有效字符。

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: uint8_t*，存储当前字符值

  FDS字段格式: MUI_XY(xy坐标)，无arg和text
  返回值: 总是返回0
*/
uint8_t mui_u8g2_u8_char_wm_mud_pi(mui_t *ui, uint8_t msg)
{
  //ui->dflags                          MUIF_DFLAG_IS_CURSOR_FOCUS       MUIF_DFLAG_IS_TOUCH_FOCUS
  //mui_get_cflags(ui->uif)       MUIF_CFLAG_IS_CURSOR_SELECTABLE
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  char buf[6];
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      while( mui_is_valid_char(*value) == 0 )
          (*value)++;
      buf[0] = *value;
      buf[1] = '\0';
      mui_u8g2_draw_button_pi(ui, u8g2_GetMaxCharWidth(u8g2), 1, buf);
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetMaxCharWidth(u8g2), 1, MUI_U8G2_V_PADDING, buf);
      //u8g2_DrawButtonUTF8(u8g2, mui_get_x(ui), mui_get_y(ui), mui_u8g2_get_pi_flags(ui), u8g2_GetMaxCharWidth(u8g2), 1, MUI_U8G2_V_PADDING, buf);
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
     case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
     /* toggle between normal mode and capture next/prev mode */
       ui->is_mud = !ui->is_mud;
     break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        do {
          (*value)++;
        } while( mui_is_valid_char(*value) == 0 );
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        do {
          (*value)--;
        } while( mui_is_valid_char(*value) == 0 );
        return 1;
      }
      break;
  }
  return 0;
}





/*
  函数: mui_u8g2_u8_opt_line_wa_mse_pi
  功能: 单行选项选择字段（wa宽度，mse选择模式，pi样式）
        从多个选项中选择一个，第一个选项值为0。
        同时只显示当前选中的选项，选择事件循环切换选项。
        例如："apple|banana|cherry"显示为可切换的选项列表。

  FDS字段格式: MUI_XYA(xy坐标, 宽度, "选项1|选项2|选项3")
    - xy: 文本左上角坐标（必需）
    - arg: 选项显示的总宽度（可选）
    - text: 用'|'分隔的选项列表

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: uint8_t*，存储当前选中的选项索引

  消息处理: DRAW（绘制当前选项）、SELECT/INCREMENT/DECREMENT（切换选项）
  返回值: 总是返回0
*/
uint8_t mui_u8g2_u8_opt_line_wa_mse_pi(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_fds_get_nth_token(ui, *value) == 0 )
      {
        *value = 0;
        mui_fds_get_nth_token(ui, *value);
      }
      mui_u8g2_draw_button_pi(ui, ui->arg, 1, ui->text);
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), ui->arg, 1, MUI_U8G2_V_PADDING, ui->text);
      //u8g2_DrawButtonUTF8(u8g2, mui_get_x(ui), mui_get_y(ui), mui_u8g2_get_pi_flags(ui), ui->arg, 1, MUI_U8G2_V_PADDING, ui->text);
      
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*value)++;
      if ( mui_fds_get_nth_token(ui, *value) == 0 ) 
        *value = 0;      
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *value > 0 ) 
        (*value)--;
      else
        (*value) = mui_fds_get_token_cnt(ui)-1;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_opt_line_wa_mse_pf
  功能: 单行选项选择字段（wa宽度，mse选择模式，pf样式）
  参数/返回值: 同mui_u8g2_u8_opt_line_wa_mse_pi
*/
uint8_t mui_u8g2_u8_opt_line_wa_mse_pf(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_fds_get_nth_token(ui, *value) == 0 )
      {
        *value = 0;
        mui_fds_get_nth_token(ui, *value);
      }
      mui_u8g2_draw_button_pf(ui, ui->arg, 1, ui->text);
      
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*value)++;
      if ( mui_fds_get_nth_token(ui, *value) == 0 ) 
        *value = 0;      
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *value > 0 ) 
        (*value)--;
      else
        (*value) = mui_fds_get_token_cnt(ui)-1;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_opt_line_wa_mud_pi
  功能: 单行选项选择字段（wa宽度，mud上下模式，pi样式）
        选择切换编辑模式，上/下事件切换选项。
  参数/返回值: 同mui_u8g2_u8_opt_line_wa_mse_pi
*/
uint8_t mui_u8g2_u8_opt_line_wa_mud_pi(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_fds_get_nth_token(ui, *value) == 0 )
      {
        *value = 0;
        mui_fds_get_nth_token(ui, *value);
      }
      mui_u8g2_draw_button_pi(ui, ui->arg, 1, ui->text);
      
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      /* toggle between normal mode and capture next/prev mode */
       ui->is_mud = !ui->is_mud;
     break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*value)++;
        if ( mui_fds_get_nth_token(ui, *value) == 0 ) 
          *value = 0;      
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *value == 0 )
          *value = mui_fds_get_token_cnt(ui);
        (*value)--;
        return 1;
      }
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_opt_line_wa_mud_pf
  功能: 单行选项选择字段（wa宽度，mud上下模式，pf样式）
  参数/返回值: 同mui_u8g2_u8_opt_line_wa_mse_pi
*/
uint8_t mui_u8g2_u8_opt_line_wa_mud_pf(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_fds_get_nth_token(ui, *value) == 0 )
      {
        *value = 0;
        mui_fds_get_nth_token(ui, *value);
      }
      mui_u8g2_draw_button_pf(ui, ui->arg, 1, ui->text);
      
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      /* toggle between normal mode and capture next/prev mode */
       ui->is_mud = !ui->is_mud;
     break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*value)++;
        if ( mui_fds_get_nth_token(ui, *value) == 0 ) 
          *value = 0;      
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *value == 0 )
          *value = mui_fds_get_token_cnt(ui);
        (*value)--;
        return 1;
      }
      break;
  }
  return 0;
}



/*
  函数: mui_u8g2_u8_chkbox_wm_pi
  功能: 复选框字段（最小宽度，pi样式）
        值为0（未选中）或1（选中），选择事件在两个状态间切换。
        可选地在复选框后面显示说明文字。

  FDS字段格式: MUI_XYT(xy坐标, "说明文字")
    - xy: 复选框左上角坐标（必需）
    - arg: 未使用
    - text: 可选，显示在复选框后面的说明文字

  样式:
    未选中: 空心方框 + 文本
    选中: 实心方框 + 文本
    有光标焦点时整体反转显示

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: uint8_t*，存储0或1

  消息处理: DRAW（绘制）、SELECT/INCREMENT/DECREMENT（切换状态）
  返回值: 总是返回0
*/
uint8_t mui_u8g2_u8_chkbox_wm_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  u8g2_uint_t flags = 0;
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( *value > 1 ) *value = 1;
      if ( mui_IsCursorFocus(ui) )
      {
        flags |= U8G2_BTN_INV;
      }
      
      {
        u8g2_uint_t w = 0;
        u8g2_uint_t a = u8g2_GetAscent(u8g2);
        if ( *value )
          u8g2_DrawCheckbox(u8g2, mui_get_x(ui), mui_get_y(ui), a, 1);
        else
          u8g2_DrawCheckbox(u8g2, mui_get_x(ui), mui_get_y(ui), a, 0);
        
        if ( ui->text[0] != '\0' )
        {
          w =  u8g2_GetUTF8Width(u8g2, ui->text);
          //u8g2_SetFontMode(u8g2, 1);
          a += 2;       /* add gap between the checkbox and the text area */
          u8g2_DrawUTF8(u8g2, mui_get_x(ui)+a, mui_get_y(ui), ui->text);
        }
        
        u8g2_DrawButtonFrame(u8g2, mui_get_x(ui), mui_get_y(ui), flags, w+a, 1, MUI_U8G2_V_PADDING);
      }
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      (*value)++;
      if ( *value > 1 ) *value = 0;      
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u8_radio_wm_pi
  功能: 单选按钮字段（最小宽度，pi样式）
        选中时将arg值赋给*value。多个单选按钮共享同一个value变量，
        每个按钮的arg值不同，通过选中操作来切换当前选项。
        当*value等于当前按钮的arg时显示选中状态（实心方块）。

  FDS字段格式: MUI_XYA(xy坐标, 选项值, "选项文字")
    - xy: 单选按钮左上角坐标（必需）
    - arg: 选中此按钮时赋给*value的值（必需）
    - text: 可选，显示在按钮后面的选项文字

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: uint8_t*，存储当前选中的选项值

  消息处理: DRAW（绘制）、SELECT（赋值为arg）
  返回值: 总是返回0
*/
/*
  radio button style, arg is assigned as value
*/
uint8_t mui_u8g2_u8_radio_wm_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  u8g2_uint_t flags = 0;
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
       if ( mui_IsCursorFocus(ui) )
      {
        flags |= U8G2_BTN_INV;
      }
      
      {
        u8g2_uint_t w = 0;
        u8g2_uint_t a = u8g2_GetAscent(u8g2);
        if ( *value == ui->arg )
          u8g2_DrawCheckbox(u8g2, mui_get_x(ui), mui_get_y(ui), a, 1);
        else
          u8g2_DrawCheckbox(u8g2, mui_get_x(ui), mui_get_y(ui), a, 0);
        
        if ( ui->text[0] != '\0' )
        {
          w =  u8g2_GetUTF8Width(u8g2, ui->text);
          //u8g2_SetFontMode(u8g2, 1);
          a += 2;       /* add gap between the checkbox and the text area */
          u8g2_DrawUTF8(u8g2, mui_get_x(ui)+a, mui_get_y(ui), ui->text);
        }
        
        u8g2_DrawButtonFrame(u8g2, mui_get_x(ui), mui_get_y(ui), flags, w+a, 1, MUI_U8G2_V_PADDING);
      }
      break;
   case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      *value = ui->arg;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;  
}


/*
  函数: mui_u8g2_u8_opt_parent_wm_pi
  功能: 选项选择的父菜单字段（最小宽度，pi样式）
        显示当前选中的选项文本，选择后跳转到子菜单表单进行选项编辑。
        父子菜单配合使用，实现多选项的列表选择界面。
        进入子菜单前会保存当前表单状态。

  FDS字段格式: MUI_XYA(xy坐标, 子表单ID, "选项1|选项2|...")
    - xy: 字段左上角坐标（必需）
    - arg: 子菜单的表单ID（必需）
    - text: 用'|'分隔的选项列表

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: uint8_t*，存储当前选中的选项索引

  消息处理: DRAW（显示当前选项）、SELECT（保存状态并跳转到子菜单）
  返回值: 总是返回0
*/
uint8_t mui_u8g2_u8_opt_parent_wm_pi(mui_t *ui, uint8_t msg)
{
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_fds_get_nth_token(ui, *value) == 0 )
      {
        *value = 0;
        mui_fds_get_nth_token(ui, *value);
      }      
      mui_u8g2_draw_button_pi(ui, 0, 1, ui->text);
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), 0, 1, MUI_U8G2_V_PADDING, ui->text);
      
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      mui_SaveForm(ui);          // store the current form and position so that the child can jump back
      mui_GotoForm(ui, ui->arg, *value);  // assumes that the selectable values are at the beginning of the form definition
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}


/*
  函数: mui_u8g2_u8_opt_child_mse_common (内部函数)
  功能: 选项子菜单的通用事件处理函数
        处理可滚动选项列表的导航、选择和退出逻辑。
        选择某个选项后将其索引存储到*value中，然后恢复父菜单表单。

  参数:
    ui  - MUI用户界面结构体指针
    msg - 消息类型
  返回值: 0=继续遍历，255=字段不可见，1=滚动更新需重绘
*/
uint8_t mui_u8g2_u8_opt_child_mse_common(mui_t *ui, uint8_t msg)
{
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      /* done by the calling function */
      break;
    case MUIF_MSG_FORM_START:
      /* we can assume that the list starts at the top. It will be adjisted by cursor down events later */
      /* ui->form_scroll_top = 0 and all other form_scroll values are set to 0 if a new form is entered in mui_EnterForm() */
      if ( ui->form_scroll_visible <= arg )
        ui->form_scroll_visible = arg+1;
      if ( ui->form_scroll_total == 0 )
          ui->form_scroll_total = mui_GetSelectableFieldOptionCnt(ui, ui->last_form_fds);
      //printf("MUIF_MSG_FORM_START: arg=%d visible=%d top=%d total=%d\n", arg, ui->form_scroll_visible, ui->form_scroll_top, ui->form_scroll_total);
      break;
    case MUIF_MSG_FORM_END:  
      break;
    case MUIF_MSG_CURSOR_ENTER:
      return mui_u8g2_handle_scroll_next_prev_events(ui, msg);
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      if ( value != NULL )
        *value = ui->form_scroll_top + arg;
      mui_RestoreForm(ui);
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      return mui_u8g2_handle_scroll_next_prev_events(ui, msg);
    case MUIF_MSG_EVENT_PREV:
      return mui_u8g2_handle_scroll_next_prev_events(ui, msg);
  }
  return 0;
}


/*
  函数: mui_u8g2_u8_opt_radio_child_wm_pi
  功能: 单选样式的选项子菜单字段（最小宽度，pi样式）
        每个选项显示为一行，当前选中项前面有实心方块标记。
        选项文本可从父菜单的text字段获取，也可从FDS中直接指定。
        支持滚动浏览长选项列表。

  FDS字段格式: MUI_XYA(xy坐标, 选项索引偏移, "可选文本")
  返回值: 0或由mui_u8g2_u8_opt_child_mse_common决定
*/
uint8_t mui_u8g2_u8_opt_radio_child_wm_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      {
        u8g2_uint_t w = 0;
        u8g2_uint_t a = u8g2_GetAscent(u8g2) - 2;
        u8g2_uint_t x = mui_get_x(ui);   // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        u8g2_uint_t y = mui_get_y(ui);  // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        uint8_t is_focus = mui_IsCursorFocus(ui);
        if ( *value == arg + ui->form_scroll_top )
          u8g2_DrawValueMark(u8g2, x, y, a);

        if ( ui->text[0] == '\0' )
        {
          /* if the text is not provided, then try to get the text from the previous (saved) element, assuming that this contains the selection */
          /* this will overwrite all ui member functions, so we must not access any ui members (except ui->text) any more */
          mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, arg + ui->form_scroll_top);
        }
        
        if ( ui->text[0] != '\0' )
        {
          w =  u8g2_GetUTF8Width(u8g2, ui->text);
          //u8g2_SetFontMode(u8g2, 1);
          a += 2;       /* add gap between the checkbox and the text area */
          u8g2_DrawUTF8(u8g2, x+a, y, ui->text);
        }        
        if ( is_focus )
        {
          u8g2_DrawButtonFrame(u8g2, x, y, U8G2_BTN_INV, w+a, 1, MUI_U8G2_V_PADDING);
        }
      }
      break;
    default:
      return mui_u8g2_u8_opt_child_mse_common(ui, msg);
  }
  return 0;
}


/*
  函数: mui_u8g2_u8_opt_radio_child_w1_pi
  功能: 单选样式的选项子菜单字段（全屏宽度，pi样式）
        与wm版本类似，但每个选项占满整个屏幕宽度，
        选中时有反转高亮效果。
  参数/返回值: 同mui_u8g2_u8_opt_radio_child_wm_pi
*/
uint8_t mui_u8g2_u8_opt_radio_child_w1_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      {
        //u8g2_uint_t w = 0;
        u8g2_uint_t a = u8g2_GetAscent(u8g2) - 2;
        u8g2_uint_t x = mui_get_x(ui);   // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        u8g2_uint_t y = mui_get_y(ui);  // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        uint8_t is_focus = mui_IsCursorFocus(ui);
        
        if ( *value == arg + ui->form_scroll_top )
          u8g2_DrawValueMark(u8g2, x, y, a);

        if ( ui->text[0] == '\0' )
        {
          /* if the text is not provided, then try to get the text from the previous (saved) element, assuming that this contains the selection */
          /* this will overwrite all ui member functions, so we must not access any ui members (except ui->text) any more */
          mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, arg + ui->form_scroll_top);
        }
        
        if ( ui->text[0] != '\0' )
        {
          //w =  u8g2_GetUTF8Width(u8g2, ui->text);
          //u8g2_SetFontMode(u8g2, 1);
          a += 2;       /* add gap between the checkbox and the text area */
          u8g2_DrawUTF8(u8g2, x+a, y, ui->text);
        }        
        if ( is_focus )
        {
          u8g2_DrawButtonFrame(u8g2, 0, y, U8G2_BTN_INV, u8g2_GetDisplayWidth(u8g2), 0, MUI_U8G2_V_PADDING);
        }
      }
      break;
    default:
      return mui_u8g2_u8_opt_child_mse_common(ui, msg);
  }
  return 0;
}


/*
  函数: mui_u8g2_u8_opt_child_wm_pi
  功能: 选项子菜单字段（最小宽度，pi样式）
        以按钮形式显示每个选项，选中时高亮。
  参数/返回值: 同mui_u8g2_u8_opt_radio_child_wm_pi
*/
uint8_t mui_u8g2_u8_opt_child_wm_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  //uint8_t *value = (uint8_t *)muif_get_data(ui->uif);
  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      {
        //u8g2_uint_t w = 0;
        u8g2_uint_t x = mui_get_x(ui);   // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        u8g2_uint_t y = mui_get_y(ui);  // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        uint8_t flags = mui_u8g2_get_pi_flags(ui);
        //if ( mui_IsCursorFocus(ui) )
        //{
        //  flags = U8G2_BTN_INV;
        //}

        if ( ui->text[0] == '\0' )
        {
          /* if the text is not provided, then try to get the text from the previous (saved) element, assuming that this contains the selection */
          /* this will overwrite all ui member functions, so we must not access any ui members (except ui->text) any more */
          mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, arg + ui->form_scroll_top);
        }
        if ( ui->text[0] != '\0' )
        {
          u8g2_DrawButtonUTF8(u8g2, x, y, flags, 0, 1, MUI_U8G2_V_PADDING, ui->text);
        }        
      }
      break;
    default:
      return mui_u8g2_u8_opt_child_mse_common(ui, msg);
  }
  return 0;
}

/*
  函数: mui_u8g2_goto_data
  功能: 不可见的数据提供字段
        此字段不显示任何内容，也不可被选择。
        它的作用是在表单开始时将自己的FDS位置保存到ui->last_form_fds中，
        供后续的子菜单字段（如opt_child系列）访问选项数据。
        通常与mui_u8g2_goto_form_w1_pi配合使用。

  MUIF: MUIF_RO()（只读，不可选择）
  FDS: MUI_DATA()（仅包含数据，无可视元素）

  消息处理: 仅处理MUIF_MSG_FORM_START（保存FDS位置）
  返回值: 总是返回0
*/
/*
  an invisible field (which will not show anything). It should also not be selectable
  it just provides the menu entries, see "mui_u8g2_u8_opt_child_mse_common" and friends
  as a consequence it does not have width, input mode and style

  MUIF: MUIF_RO()
  FDS: MUI_DATA()

  mui_u8g2_goto_parent --> mui_u8g2_goto_data

  Used together with mui_u8g2_goto_form_w1_pi

*/
uint8_t mui_u8g2_goto_data(mui_t *ui, uint8_t msg)
{
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      break;
    case MUIF_MSG_FORM_START:
      // store the field (and the corresponding elements) in the last_form_fds variable.
      // last_form_fds is later used to access the elements (see mui_u8g2_u8_opt_child_mse_common and friends)
      ui->last_form_fds = ui->fds;
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}


/*
  函数: mui_u8g2_goto_form_w1_pi
  功能: 全屏宽度的子菜单跳转字段（pi样式）
        显示从父菜单数据中获取的选项文本，选择后跳转到选项文本首字符指定的表单。
        支持滚动浏览长列表，并保存光标位置以便返回时恢复。
        通常与mui_u8g2_goto_data配合使用。

  FDS字段格式: MUI_XYA(xy坐标, 列表项偏移)
  返回值: 0或由子函数决定
*/
/*
mui_u8g2_goto_child_w1_mse_pi --> mui_u8g2_goto_form_w1_pi
*/
uint8_t mui_u8g2_goto_form_w1_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, arg + ui->form_scroll_top) )
        mui_u8g2_draw_button_pi(ui, u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui), ui->text+1);
      break;
    case MUIF_MSG_CURSOR_SELECT:
      if ( mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, ui->arg + ui->form_scroll_top) )
      {
        mui_SaveCursorPosition(ui, ui->arg + ui->form_scroll_top);     // store the current cursor position, so that the user can jump back to the corresponding cursor position
        return mui_GotoFormAutoCursorPosition(ui, (uint8_t)ui->text[0]);
      }
      break;
    default:
      return mui_u8g2_u8_opt_child_mse_common(ui, msg);
  }
  return 0;
}

/*
  函数: mui_u8g2_goto_form_w1_pf
  功能: 全屏宽度的子菜单跳转字段（pf样式）
        与w1_pi版本功能相同，但使用pf样式。
  参数/返回值: 同mui_u8g2_goto_form_w1_pi
*/
uint8_t mui_u8g2_goto_form_w1_pf(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      if ( mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, arg + ui->form_scroll_top) )
        mui_u8g2_draw_button_pf(ui, u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui)-1, ui->text+1);
      break;
    case MUIF_MSG_CURSOR_SELECT:
      if ( mui_GetSelectableFieldTextOption(ui, ui->last_form_fds, ui->arg + ui->form_scroll_top) )
      {
        mui_SaveCursorPosition(ui, ui->arg + ui->form_scroll_top);     // store the current cursor position, so that the user can jump back to the corresponding cursor position
        return mui_GotoFormAutoCursorPosition(ui, (uint8_t)ui->text[0]);
     }
      break;
    default:
      return mui_u8g2_u8_opt_child_mse_common(ui, msg);
  }
  return 0;
}


/*
  函数: mui_u8g2_u16_list_line_wa_mse_pi
  功能: 16位列表的单行选择字段（wa宽度，mse选择模式，pi样式）
        使用回调函数获取列表元素和数量，支持16位索引（最多65536项）。
        选择事件循环切换列表项。

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: mui_u8g2_list_t*，包含选择指针、数据指针、元素回调、计数回调

  FDS字段格式: MUI_XYA(xy坐标, 显示宽度)
  返回值: 总是返回0
*/
/*
  data: mui_u8g2_list_t *
*/
uint8_t mui_u8g2_u16_list_line_wa_mse_pi(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_list_t *list = (mui_u8g2_list_t *)muif_get_data(ui->uif);
  uint16_t *selection =  mui_u8g2_list_get_selection_ptr(list);
  void *data = mui_u8g2_list_get_data_ptr(list);
  mui_u8g2_get_list_element_cb element_cb =  mui_u8g2_list_get_element_cb(list);
  mui_u8g2_get_list_count_cb count_cb = mui_u8g2_list_get_count_cb(list);
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_pi(ui, ui->arg, 1, element_cb(data, *selection));
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), ui->arg, 1, MUI_U8G2_V_PADDING, element_cb(data, *selection));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
      (*selection)++;
      if ( *selection >= count_cb(data) ) 
        *selection = 0;
      break;
    case MUIF_MSG_VALUE_DECREMENT:
      if ( *selection > 0 )
        (*selection)--;
      else
        (*selection) = count_cb(data)-1;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u16_list_line_wa_mud_pi
  功能: 16位列表的单行选择字段（wa宽度，mud上下模式，pi样式）
        选择切换编辑模式，上/下事件切换列表项。
  参数/返回值: 同mui_u8g2_u16_list_line_wa_mse_pi
*/
uint8_t mui_u8g2_u16_list_line_wa_mud_pi(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_list_t *list = (mui_u8g2_list_t *)muif_get_data(ui->uif);
  uint16_t *selection =  mui_u8g2_list_get_selection_ptr(list);
  void *data = mui_u8g2_list_get_data_ptr(list);
  mui_u8g2_get_list_element_cb element_cb =  mui_u8g2_list_get_element_cb(list);
  mui_u8g2_get_list_count_cb count_cb = mui_u8g2_list_get_count_cb(list);
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_pi(ui, ui->arg, 1, element_cb(data, *selection));
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), ui->arg, 1, MUI_U8G2_V_PADDING, element_cb(data, *selection));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      /* toggle between normal mode and capture next/prev mode */
       ui->is_mud = !ui->is_mud;
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      if ( ui->is_mud )
      {
        (*selection)++;
        if ( *selection >= count_cb(data)  ) 
          *selection = 0;      
        return 1;
      }
      break;
    case MUIF_MSG_EVENT_PREV:
      if ( ui->is_mud )
      {
        if ( *selection == 0 )
          *selection = count_cb(data);
        (*selection)--;
        return 1;
      }
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u16_list_parent_wm_pi
  功能: 16位列表的父菜单字段（最小宽度，pi样式）
        显示当前选中的列表项，选择后跳转到子菜单表单进行列表浏览。
        进入子菜单前保存当前表单状态。

  muif配置:
    flags: MUIF_CFLAG_IS_CURSOR_SELECTABLE
    data: mui_u8g2_list_t*

  FDS字段格式: MUI_XYA(xy坐标, 子表单ID)
  返回值: 总是返回0
*/
/*
  MUIF: MUIF_U8G2_U16_LIST
  FDS: MUI_XYA, arg=form id
  data: mui_u8g2_list_t *
*/
uint8_t mui_u8g2_u16_list_parent_wm_pi(mui_t *ui, uint8_t msg)
{
  //u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_list_t *list = (mui_u8g2_list_t *)muif_get_data(ui->uif);
  uint16_t *selection =  mui_u8g2_list_get_selection_ptr(list);
  void *data = mui_u8g2_list_get_data_ptr(list);
  mui_u8g2_get_list_element_cb element_cb =  mui_u8g2_list_get_element_cb(list);
  //mui_u8g2_get_list_count_cb count_cb = mui_u8g2_list_get_count_cb(list);
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_pi(ui, 0, 1, element_cb(data, *selection));
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), ui->arg, 1, MUI_U8G2_V_PADDING, element_cb(data, *selection));
      break;
    case MUIF_MSG_FORM_START:
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      mui_SaveForm(ui);          // store the current form and position so that the child can jump back
      mui_GotoForm(ui, ui->arg, *selection);  // assumes that the selectable values are at the beginning of the form definition
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
  }
  return 0;
}

/*
  函数: mui_u8g2_u16_list_child_mse_common (内部函数)
  功能: 16位列表子菜单的通用事件处理函数
        处理可滚动列表的导航、选择和退出逻辑。
        选择后将选中项索引存储到*selection中，然后恢复父菜单。
  参数/返回值: 同mui_u8g2_u8_opt_child_mse_common
*/
static uint8_t mui_u8g2_u16_list_child_mse_common(mui_t *ui, uint8_t msg)
{
  mui_u8g2_list_t *list = (mui_u8g2_list_t *)muif_get_data(ui->uif);
  uint16_t *selection =  mui_u8g2_list_get_selection_ptr(list);
  void *data = mui_u8g2_list_get_data_ptr(list);
  //mui_u8g2_get_list_element_cb element_cb =  mui_u8g2_list_get_element_cb(list);
  mui_u8g2_get_list_count_cb count_cb = mui_u8g2_list_get_count_cb(list);

  uint8_t arg = ui->arg;        // remember the arg value, because it might be overwritten  
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      /* done by the calling function */
      break;
    case MUIF_MSG_FORM_START:
      /* we can assume that the list starts at the top. It will be adjisted by cursor down events later */
      ui->form_scroll_top = 0;
      if ( ui->form_scroll_visible <= arg )
        ui->form_scroll_visible = arg+1;
      if ( ui->form_scroll_total == 0 )
          ui->form_scroll_total = count_cb(data);
      break;
    case MUIF_MSG_FORM_END:
      break;
    case MUIF_MSG_CURSOR_ENTER:
      return mui_u8g2_handle_scroll_next_prev_events(ui, msg);
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      if ( selection != NULL )
        *selection = ui->form_scroll_top + arg;
      mui_RestoreForm(ui);
      break;
    case MUIF_MSG_CURSOR_LEAVE:
      break;
    case MUIF_MSG_TOUCH_DOWN:
      break;
    case MUIF_MSG_TOUCH_UP:
      break;
    case MUIF_MSG_EVENT_NEXT:
      return mui_u8g2_handle_scroll_next_prev_events(ui, msg);
    case MUIF_MSG_EVENT_PREV:
      return mui_u8g2_handle_scroll_next_prev_events(ui, msg);
  }
  return 0;
}

/*
  函数: mui_u8g2_u16_list_child_w1_pi
  功能: 16位列表的子菜单显示字段（全屏宽度，pi样式）
        每个列表项占满整个屏幕宽度，当前选中项有实心方块标记。
        支持滚动浏览长列表。
  参数/返回值: 由mui_u8g2_u16_list_child_mse_common处理
*/
uint8_t mui_u8g2_u16_list_child_w1_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_list_t *list = (mui_u8g2_list_t *)muif_get_data(ui->uif);
  uint16_t *selection =  mui_u8g2_list_get_selection_ptr(list);
  void *data = mui_u8g2_list_get_data_ptr(list);
  mui_u8g2_get_list_element_cb element_cb =  mui_u8g2_list_get_element_cb(list);
  mui_u8g2_get_list_count_cb count_cb = mui_u8g2_list_get_count_cb(list);
  uint16_t pos = ui->arg;        // remember the arg value, because it might be overwritten  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      {
        //u8g2_uint_t w = 0;
        u8g2_uint_t a = u8g2_GetAscent(u8g2) - 2;
        u8g2_uint_t x = mui_get_x(ui);   // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        u8g2_uint_t y = mui_get_y(ui);  // if mui_GetSelectableFieldTextOption is called, then field vars are overwritten, so get the value
        uint8_t is_focus = mui_IsCursorFocus(ui);

        pos += ui->form_scroll_top;
        
        if ( *selection == pos )
          u8g2_DrawValueMark(u8g2, x, y, a);

        //u8g2_SetFontMode(u8g2, 1);
        a += 2;       /* add gap between the checkbox and the text area */
        if ( pos < count_cb(data) )
          u8g2_DrawUTF8(u8g2, x+a, y, element_cb(data, pos));
        if ( is_focus )
        {
          u8g2_DrawButtonFrame(u8g2, 0, y, U8G2_BTN_INV, u8g2_GetDisplayWidth(u8g2), 0, MUI_U8G2_V_PADDING);
        }
      }
      break;
    default:
      return mui_u8g2_u16_list_child_mse_common(ui, msg);
  }
  return 0;
}

/*
  函数: mui_u8g2_u16_list_goto_w1_pi
  功能: 16位列表的子菜单跳转字段（全屏宽度，pi样式）
        显示列表项文本，选择后跳转到文本首字符指定的表单。
        保存光标位置以便返回时恢复。
  参数/返回值: 由mui_u8g2_u16_list_child_mse_common处理
*/
uint8_t mui_u8g2_u16_list_goto_w1_pi(mui_t *ui, uint8_t msg)
{
  u8g2_t *u8g2 = mui_get_U8g2(ui);
  mui_u8g2_list_t *list = (mui_u8g2_list_t *)muif_get_data(ui->uif);
  uint16_t *selection =  mui_u8g2_list_get_selection_ptr(list);
  void *data = mui_u8g2_list_get_data_ptr(list);
  mui_u8g2_get_list_element_cb element_cb =  mui_u8g2_list_get_element_cb(list);
  //mui_u8g2_get_list_count_cb count_cb = mui_u8g2_list_get_count_cb(list);

  uint16_t pos = ui->arg;        // remember the arg value, because it might be overwritten  
  pos += ui->form_scroll_top;
  
  switch(msg)
  {
    case MUIF_MSG_DRAW:
      mui_u8g2_draw_button_pi(ui, u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui), element_cb(data, pos)+1);
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_pi_flags(ui), u8g2_GetDisplayWidth(u8g2)-mui_get_x(ui)*2, mui_get_x(ui), MUI_U8G2_V_PADDING, element_cb(data, pos)+1);
      break;
    case MUIF_MSG_CURSOR_SELECT:
    case MUIF_MSG_VALUE_INCREMENT:
    case MUIF_MSG_VALUE_DECREMENT:
      if ( selection != NULL )
        *selection = pos;
      mui_SaveCursorPosition(ui, pos >= 255 ? 0 : pos);     // store the current cursor position, so that the user can jump back to the corresponding cursor position
      mui_GotoFormAutoCursorPosition(ui, (uint8_t)element_cb(data, pos)[0]); 
      break;
    default:
      return mui_u8g2_u16_list_child_mse_common(ui, msg);
  }
  return 0;
}
