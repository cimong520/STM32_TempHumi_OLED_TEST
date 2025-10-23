#ifndef __OLED_H
#define __OLED_H

#include "stdint.h"
#include "OLED_Data.h"
#include "../Start/stm32f10x.h"
#include "./Initialization_Data.h"

// I2C接口定义
#define OLED_I2C_ADDR         0x78    // OLED模块I2C
#define OLED_CMD              0x00    // 写命令
#define OLED_DATA             0x40    // 写数据


// OLED屏幕尺寸
#define OLED_WIDTH            128     // 屏幕宽度
#define OLED_HEIGHT           64      // 屏幕高度
#define OLED_PAGE_COUNT       8       // 页数
#define OLED_COLUMN_COUNT     128     // 列数
#define OLED_CHN_CHAR_WIDTH   3       // 汉字字符宽度(字节)

// 颜色定义
#define OLED_COLOR_BLACK      0x00    // 黑色
#define OLED_COLOR_WHITE      0x01    // 白色
#define OLED_8X16             16      // 8x16字体
#define OLED_6X8              8       // 6x8字体

// 函数声明
// I2C通信函数
void OLED_I2C_Init(void);
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_WaitAck(void);
void OLED_I2C_SendByte(uint8_t Byte);

// OLED控制函数
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_DrawPoint(int16_t x, int16_t y, uint8_t Color);
void OLED_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t Color);
void OLED_DrawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t Color);
void OLED_DrawFillRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t Color);
void OLED_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t Color);
void OLED_DrawFillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t Color);
void OLED_ShowChar(int16_t x, int16_t y, char c, uint8_t FontSize);
void OLED_ShowString(int16_t x, int16_t y, const char *str, uint8_t FontSize);
void OLED_ShowChinese(int16_t x, int16_t y, const char *ch, uint8_t FontSize);
void OLED_ShowImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *Image);
void OLED_Printf(int16_t x, int16_t y, uint8_t FontSize, const char *format, ...);
void OLED_UpdateArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

// 折线图函数
void OLED_DrawLineChart(int16_t x0, int16_t y0, int16_t width, int16_t height, 
                        const int16_t* xData, const int16_t* yData, uint8_t pointCount, 
                        uint8_t color, uint8_t drawAxis);

// 时间横轴折线图函数
// 数据类型枚举
typedef enum {
    DATA_TYPE_INT16,
    DATA_TYPE_FLOAT
} DataType;

// 时间横轴折线图函数 - 支持多种数据类型
void OLED_DrawTimeLineChart(int16_t x0, int16_t y0, int16_t width, int16_t height, 
                            const void* yData, DataType dataType, uint8_t pointCount, uint16_t timeInterval, 
                            uint8_t color, uint8_t drawAxis, uint8_t showLatest);

#endif

