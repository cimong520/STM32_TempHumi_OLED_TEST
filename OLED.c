#include "OLED.h"
#include "stdint.h"
#include "stdbool.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
// 显存数组
static uint8_t OLED_GRAM[OLED_PAGE_COUNT][OLED_COLUMN_COUNT] = {0};

// 根据选择的I2C类型包含不同的实现
#if OLED_I2C_TYPE == 1
// OLED地址和超时定义
#define OLED_ADDRESS 0x78
#define I2C_TIMEOUT 10000

// 函数声明
void I2C_Start(void);
void I2C_Stop(void);
uint8_t I2C_WaitEvent(uint32_t event);
void I2C_SendAddress(uint8_t address, uint8_t direction);
void I2C_SendByte(uint8_t data);
void I2C_SendBytes(const uint8_t* data, uint16_t length);

// I2C起始信号
void I2C_Start(void)
{
    I2C_GenerateSTART(OLED_IIC, ENABLE);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) {
        // 错误处理
    }
}

// I2C停止信号
void I2C_Stop(void)
{
    I2C_GenerateSTOP(OLED_IIC, ENABLE);
}

// 等待I2C事件发生,带超时功能
uint8_t I2C_WaitEvent(uint32_t event)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!I2C_CheckEvent(OLED_IIC, event)) {
        if (--timeout == 0) {
            return 0; // 超时返回0
        }
    }
    return 1; // 成功返回1
}

// 发送I2C地址
void I2C_SendAddress(uint8_t address, uint8_t direction)
{
    I2C_Send7bitAddress(OLED_IIC, address, direction);
    if (direction == I2C_Direction_Transmitter) {
        if (!I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
            // 错误处理
            I2C_InitTypeDef I2C_InitStructure;
            RCC_APB1PeriphClockCmd(OLED_IIC_Clock, ENABLE);
            I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
            I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
            I2C_InitStructure.I2C_OwnAddress1 = 0x00;
            I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
            I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
            I2C_InitStructure.I2C_ClockSpeed = 800000;
            I2C_Init(OLED_IIC, &I2C_InitStructure);
            I2C_Cmd(OLED_IIC, ENABLE);
        }
    } else {
        // 接收模式处理
    }
}

// 发送单个字节
void I2C_SendByte(uint8_t data)
{
    I2C_SendData(OLED_IIC, data);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        // 错误处理
    }
}

// 批量发送字节
void I2C_SendBytes(const uint8_t* data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        I2C_SendData(OLED_IIC, data[i]);
        if (i == length - 1) {
            if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
                break;
            }
        } else {
            if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
                break;
            }
        }
    }
}

// 写入I2C命令
void Write_IIC_Command(unsigned char I2C_Command)
{
    uint8_t command[] = {0x00, I2C_Command};
    uint32_t timeout = I2C_TIMEOUT;
    while (I2C_GetFlagStatus(OLED_IIC, I2C_FLAG_BUSY)) {
        if (--timeout == 0) return;
    }
    I2C_Start();
    I2C_SendAddress(OLED_ADDRESS, I2C_Direction_Transmitter);
    I2C_SendBytes(command, 2);
    I2C_Stop();
}

// 写入I2C数据
void Write_IIC_Data(const uint8_t *Data, uint8_t Len)
{
    uint8_t buffer[256];
    buffer[0] = 0x40;
    for (int i = 0; i < Len; i++) {
        buffer[i + 1] = Data[i];
    }
    uint32_t timeout = I2C_TIMEOUT;
    while (I2C_GetFlagStatus(OLED_IIC, I2C_FLAG_BUSY)) {
        if (--timeout == 0) return;
    }
    I2C_Start();
    I2C_SendAddress(OLED_ADDRESS, I2C_Direction_Transmitter);
    I2C_SendBytes(buffer, Len + 1);
    I2C_Stop();
}

// 设置OLED显示位置
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    Write_IIC_Command(0xb0 | y);
    Write_IIC_Command(((x & 0xf0) >> 4) | 0x10);
    Write_IIC_Command(0x00 | (x & 0x0f));
}

#else
/* 软件I2C实现 */

// 软件I2C函数
void OLED_W_SCL(uint8_t BitValue);
void OLED_W_SDA(uint8_t BitValue);
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_SendByte(uint8_t Byte);

// OLED写SCL高低电平
void OLED_W_SCL(uint8_t BitValue)
{
    GPIO_WriteBit(OLED_GPIO, OLED_SCL_PIN, (BitAction)BitValue);
}

// OLED写SDA高低电平
void OLED_W_SDA(uint8_t BitValue)
{
    GPIO_WriteBit(OLED_GPIO, OLED_SDA_PIN, (BitAction)BitValue);
}

// I2C起始
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

// I2C终止
void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

// I2C发送一个字节
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        OLED_W_SDA(!!(Byte & (0x80 >> i)));
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }
    OLED_W_SCL(1);
    OLED_W_SCL(0);
}

// 写入I2C命令
void Write_IIC_Command(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);    // OLED地址
    OLED_I2C_SendByte(0x00);    // 命令标识
    OLED_I2C_SendByte(Command); // 命令值
    OLED_I2C_Stop();
}

// 写入I2C数据
void Write_IIC_Data(const uint8_t *Data, uint8_t Len)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);    // OLED地址
    OLED_I2C_SendByte(0x40);    // 数据标识
    
    for (uint8_t i = 0; i < Len; i++) {
        OLED_I2C_SendByte(Data[i]); // 发送数据
    }
    
    OLED_I2C_Stop();
}

// 设置OLED显示位置
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    Write_IIC_Command(0xB0 | y);          // 设置页地址
    Write_IIC_Command(0x10 | (x >> 4));   // 设置列地址高4位
    Write_IIC_Command(0x00 | (x & 0x0F)); // 设置列地址低4位
}

#endif

// 写入数据或命令到OLED
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if (cmd == OLED_CMD) {
        Write_IIC_Command(dat);
    } else {
        uint8_t data[1] = {dat};
        Write_IIC_Data(data, 1);
    }
}

#if OLED_USE_DMA

// 定义DMA相关资源(根据实际硬件修改)
#define OLED_DMA                 DMA1
#define OLED_DMA_CHANNEL         DMA1_Channel6
#define OLED_DMA_CLOCK           RCC_AHBPeriph_DMA1
#define OLED_DMA_TC_FLAG         DMA1_FLAG_TC6
#define OLED_DMA_IRQn            DMA1_Channel6_IRQn
#define OLED_DMA_IRQHandler      DMA1_Channel6_IRQHandler

// DMA传输状态
volatile uint8_t DMA_TransferComplete = 0;

// DMA中断处理函数
void OLED_DMA_IRQHandler(void)
{
    if (DMA_GetITStatus(OLED_DMA_TC_FLAG))
    {
        DMA_ClearITPendingBit(OLED_DMA_TC_FLAG);
        DMA_TransferComplete = 1;
    }
}

// 初始化DMA
void OLED_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // 启用DMA时钟
    RCC_AHBPeriphClockCmd(OLED_DMA_CLOCK, ENABLE);
    
    // 配置DMA
    DMA_DeInit(OLED_DMA_CHANNEL);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(OLED_IIC->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = 0; // 将在传输时设置
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 128;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(OLED_DMA_CHANNEL, &DMA_InitStructure);
    
    // 配置DMA中断
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = OLED_DMA_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMA_ITConfig(OLED_DMA_CHANNEL, DMA_IT_TC, ENABLE);
}
#endif

// 初始化SSD1306控制器(实际为I2C初始化)
void OLED_Init(void)
{
    #if OLED_I2C_TYPE == 1
    /* 硬件I2C初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    RCC_APB1PeriphClockCmd(OLED_IIC_Clock, ENABLE);
    RCC_APB2PeriphClockCmd(OLED_GPIO_Clock, ENABLE);

    GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN | OLED_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_GPIO, &GPIO_InitStructure);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 800000;
    I2C_Init(OLED_IIC, &I2C_InitStructure);
    I2C_Cmd(OLED_IIC, ENABLE);
#else
    /* 软件I2C初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 初始化延时
    for (uint32_t i = 0; i < 1000; i++) {
        for (uint32_t j = 0; j < 1000; j++);
    }
    
    RCC_APB2PeriphClockCmd(OLED_GPIO_Clock, ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN | OLED_SDA_PIN;
    GPIO_Init(OLED_GPIO, &GPIO_InitStructure);
    
    OLED_W_SCL(1);
    OLED_W_SDA(1);
#endif

    OLED_WR_Byte(0xAE, OLED_CMD); // 关闭显示屏
    OLED_WR_Byte(0x40, OLED_CMD); // 设置起始行地址
    OLED_WR_Byte(0xB0, OLED_CMD); // 设置页面起始地址为页面寻址模式,0-7
    OLED_WR_Byte(0xC8, OLED_CMD); // 上下反置关(行重映射),C8,从COM[N-1]扫描到COM0;C0,设置 从COM0扫描到COM[N-1],N为复用率
    OLED_WR_Byte(0x81, OLED_CMD); // 设置对比度
    OLED_WR_Byte(0xff, OLED_CMD); // 选择0xff对比度,选择范围0x00-0xff
    OLED_WR_Byte(0xa1, OLED_CMD); // 左右反置关(段重映射),A0H 设置GDDRAM的COL0映射到驱动器输出SEG0,A1H 设置COL127映射到SEG0
    OLED_WR_Byte(0xa6, OLED_CMD); // 正常显示(1亮0灭)
    OLED_WR_Byte(0xa8, OLED_CMD); // 设置多路传输比率,显示行数
    OLED_WR_Byte(0x3f, OLED_CMD); //  MUX=31	 (显示31行)
    OLED_WR_Byte(0xd3, OLED_CMD); // 设置垂直显示偏移(向上)
    OLED_WR_Byte(0x00, OLED_CMD); // 偏移0行
    OLED_WR_Byte(0xd5, OLED_CMD); // 设置DCLK分频和OSC频率
    OLED_WR_Byte(0xf0, OLED_CMD); // 频率最高
    OLED_WR_Byte(0xd9, OLED_CMD); // 设置预充电的持续时间
    OLED_WR_Byte(0x22, OLED_CMD);
    OLED_WR_Byte(0xda, OLED_CMD); // 设置COM引脚配置
    OLED_WR_Byte(0x12, OLED_CMD); // 交替COM,左右不反置
    OLED_WR_Byte(0xdb, OLED_CMD); // 调整Vcomh调节器的输出
    OLED_WR_Byte(0x49, OLED_CMD);
    OLED_WR_Byte(0x8d, OLED_CMD); // 启用电荷泵
    OLED_WR_Byte(0x14, OLED_CMD); // 启用电荷泵
    OLED_WR_Byte(0xaf, OLED_CMD); // 开OLED显示
	
	#if OLED_I2C_TYPE == 1 && OLED_USE_DMA
		OLED_DMA_Init();  // 仅在启用DMA时初始化
	#endif
}

// 清空OLED显示
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++) // 遍历8页
    {
        for (i = 0; i < 128; i++) // 遍历128列
        {
            OLED_GRAM[j][i] = 0x00; // 将显存数组数据全部清零
        }
    }
}

// 将OLED显存数组更新到OLED屏幕
void OLED_Update(void)
{
#if OLED_USE_DMA && OLED_I2C_TYPE == 1
    // 使用DMA更新
    for (uint8_t j = 0; j < 8; j++)
    {
        OLED_Set_Pos(0, j);
        
        // 等待总线空闲
        uint32_t timeout = I2C_TIMEOUT;
        while (I2C_GetFlagStatus(OLED_IIC, I2C_FLAG_BUSY)) {
            if (--timeout == 0) return;
        }
        
        // 启动I2C传输
        I2C_GenerateSTART(OLED_IIC, ENABLE);
        timeout = I2C_TIMEOUT;
        while (!I2C_CheckEvent(OLED_IIC, I2C_EVENT_MASTER_MODE_SELECT)) {
            if (--timeout == 0) return;
        }
        
        // 发送地址
        I2C_Send7bitAddress(OLED_IIC, OLED_ADDRESS, I2C_Direction_Transmitter);
        timeout = I2C_TIMEOUT;
        while (!I2C_CheckEvent(OLED_IIC, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
            if (--timeout == 0) return;
        }
        
        // 发送控制字节(0x40表示数据)
        I2C_SendData(OLED_IIC, 0x40);
        timeout = I2C_TIMEOUT;
        while (!I2C_CheckEvent(OLED_IIC, I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
            if (--timeout == 0) return;
        }
        
        // 配置并启动DMA传输
        DMA_TransferComplete = 0;
        DMA_Cmd(OLED_DMA_CHANNEL, DISABLE);
        
        OLED_DMA_CHANNEL->CNDTR = 128;
        OLED_DMA_CHANNEL->CMAR = (uint32_t)OLED_GRAM[j];
        
        DMA_Cmd(OLED_DMA_CHANNEL, ENABLE);
        I2C_DMACmd(OLED_IIC, ENABLE);
        
        // 等待DMA传输完成
        timeout = I2C_TIMEOUT * 10;
        while (!DMA_TransferComplete) {
            if (--timeout == 0) {
                DMA_Cmd(OLED_DMA_CHANNEL, DISABLE);
                break;
            }
        }
        
        // 等待I2C传输完成
        timeout = I2C_TIMEOUT;
        while (!I2C_CheckEvent(OLED_IIC, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if (--timeout == 0) break;
        }
        
        // 停止I2C传输
        I2C_GenerateSTOP(OLED_IIC, ENABLE);
    }
#elif OLED_I2C_TYPE == 1
    // 使用普通更新(原OLED_Update函数内容)
    uint8_t j;
    for (j = 0; j < 8; j++)
    {
        OLED_Set_Pos(0, j);
        
        uint32_t timeout = I2C_TIMEOUT;
        while (I2C_GetFlagStatus(OLED_IIC, I2C_FLAG_BUSY)) {
            if (--timeout == 0) return;
        }
        
        I2C_Start();
        I2C_SendAddress(OLED_ADDRESS, I2C_Direction_Transmitter);
        
        I2C_SendData(OLED_IIC, 0x40);
        if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            I2C_Stop();
            return;
        }
        
        for (int i = 0; i < 128; i++) {
            I2C_SendData(OLED_IIC, OLED_GRAM[j][i]);
            
            if (i == 127) {
                if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
                    break;
                }
            } else {
                if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
                    break;
                }
            }
        }
        
        I2C_Stop();
    }
#else
	for(uint8_t page = 0; page < OLED_PAGE_COUNT; page++) {
        OLED_Set_Pos(0, page);
        // 使用软件I2C的Write_IIC_Data函数
        Write_IIC_Data(OLED_GRAM[page], 128);
    }
#endif
}

/**
  * 函    数:更新指定区域显存到屏幕
  * 参    数:x1 起始列坐标
  * 参    数:y1 起始行坐标
  * 参    数:x2 结束列坐标
  * 参    数:y2 结束行坐标
  * 返 回 值:无
  * 说    明:调用此函数后,指定区域的内容将被更新到屏幕上
  */
void OLED_UpdateArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    if (x1 >= 128 || y1 >= 64 || x2 < 0 || y2 < 0 || x1 > x2 || y1 > y2)
        return;

    uint8_t start_page = y1 / 8;
    uint8_t end_page = y2 / 8;

    for (uint8_t page = start_page; page <= end_page; page++)
    {
        OLED_Set_Pos(x1, page);
        Write_IIC_Data(&OLED_GRAM[page][x1], x2 - x1 + 1);
    }
}

/**
  * 函    数:次方函数
  * 参    数:X 底数
  * 参    数:Y 指数
  * 返 回 值:等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//结果默认为1
	while (Y --)			//累乘Y次
	{
		Result *= X;		//每次把X累乘到结果上
	}
	return Result;
}

/**
  * 函    数:OLED绘制折线图
  * 参    数:x0 指定折线图左上角的横坐标,范围:0~127
  * 参    数:y0 指定折线图左上角的纵坐标,范围:0~63
  * 参    数:width 指定折线图的宽度,范围:0~128
  * 参    数:height 指定折线图的高度,范围:0~64
  * 参    数:xData 指向X轴数据数组的指针
  * 参    数:yData 指向Y轴数据数组的指针
  * 参    数:pointCount 数据点的数量
  * 参    数:color 折线颜色,OLED_COLOR_BLACK或OLED_COLOR_WHITE
  * 参    数:drawAxis 是否绘制坐标轴,1:绘制,0:不绘制
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawLineChart(int16_t x0, int16_t y0, int16_t width, int16_t height, 
                        const int16_t* xData, const int16_t* yData, uint8_t pointCount, 
                        uint8_t color, uint8_t drawAxis)
{
    if (pointCount < 2) return;  // 至少需要2个点才能绘制折线

    // 1. 绘制坐标轴
    if (drawAxis)
    {
        // X轴
        OLED_DrawLine(x0, y0 + height, x0 + width, y0 + height, color);
        // Y轴
        OLED_DrawLine(x0, y0, x0, y0 + height, color);

        // 绘制箭头
        OLED_DrawLine(x0 + width, y0 + height, x0 + width - 5, y0 + height - 3, color);
        OLED_DrawLine(x0 + width, y0 + height, x0 + width - 5, y0 + height + 3, color);
        OLED_DrawLine(x0, y0, x0 - 3, y0 + 5, color);
        OLED_DrawLine(x0, y0, x0 + 3, y0 + 5, color);
    }

    // 2. 找到X和Y数据的最小值和最大值，用于归一化
    int16_t minX = xData[0], maxX = xData[0];
    int16_t minY = yData[0], maxY = yData[0];
    for (uint8_t i = 1; i < pointCount; i++)
    {
        if (xData[i] < minX) minX = xData[i];
        if (xData[i] > maxX) maxX = xData[i];
        if (yData[i] < minY) minY = yData[i];
        if (yData[i] > maxY) maxY = yData[i];
    }

    // 避免除零错误
    if (maxX == minX) maxX = minX + 1;
    if (maxY == minY) maxY = minY + 1;

    // 3. 计算并绘制均值线
    if (drawAxis)
    {
        // 计算最近数据的均值
        int32_t sum = 0;
        for (uint8_t i = 0; i < pointCount; i++)
        {
            sum += yData[i];
        }
        int16_t meanValue = sum / pointCount;
        
        // 计算均值对应的Y坐标
        int16_t meanY = y0 + height - ((meanValue - minY) * height) / (maxY - minY);
        // 绘制均值线
        OLED_DrawLine(x0, meanY, x0 + width, meanY, color);
        // 绘制均值标签
        char meanLabel[20];
        sprintf(meanLabel, "均值: %d", meanValue);
        // 在图表中间位置显示均值
        OLED_Printf(x0 + width/2 - 30, y0 - 10, 8, "%s", meanLabel);
    }

    // 4. 绘制折线
    int16_t prevX, prevY;
    for (uint8_t i = 0; i < pointCount; i++)
    {
        // 归一化X和Y坐标
        int16_t x = x0 + ((xData[i] - minX) * width) / (maxX - minX);
        int16_t y = y0 + height - ((yData[i] - minY) * height) / (maxY - minY);

        // 确保坐标在屏幕范围内
        if (x < x0) x = x0;
        if (x > x0 + width) x = x0 + width;
        if (x > 127) x = 127;  // 额外的屏幕边界检查
        if (y < y0) y = y0;
        if (y > y0 + height) y = y0 + height;

        // 绘制点
        OLED_DrawPoint(x, y, color);

        // 绘制连接线
        if (i > 0)
        {
            OLED_DrawLine(prevX, prevY, x, y, color);
        }

        prevX = x;
        prevY = y;
    }
}

/**
  * 函    数:将OLED显存数组全部取反
  * 参    数:无
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_Reverse(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j ++)				//遍历8页
	{
		for (i = 0; i < 128; i ++)			//遍历128列
		{
			OLED_GRAM[j][i] ^= 0xFF;	//将显存数组数据全部取反
		}
	}
}

/**
  * 函    数:OLED绘制时间横轴折线图
  * 参    数:x0 指定折线图左上角的横坐标,范围:0~127
  * 参    数:y0 指定折线图左上角的纵坐标,范围:0~63
  * 参    数:width 指定折线图的宽度,范围:0~128
  * 参    数:height 指定折线图的高度,范围:0~64
  * 参    数:yData 指向Y轴数据数组的指针(支持int16_t或float类型)
  * 参    数:dataType 数据类型(DATA_TYPE_INT16或DATA_TYPE_FLOAT)
  * 参    数:pointCount 数据点的数量
  * 参    数:timeInterval 数据点之间的时间间隔(单位:任意)
  * 参    数:color 折线颜色,OLED_COLOR_BLACK或OLED_COLOR_WHITE
  * 参    数:drawAxis 是否绘制坐标轴,1:绘制,0:不绘制
  * 参    数:showLatest 是否只显示最近20个数据点,1:是,0:否
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawTimeLineChart(int16_t x0, int16_t y0, int16_t width, int16_t height, 
                            const void* yData, DataType dataType, uint8_t pointCount, uint16_t timeInterval, 
                            uint8_t color, uint8_t drawAxis, uint8_t showLatest)
{
    // 确定要显示的数据点数量和起始索引
    uint8_t displayCount = pointCount;
    uint8_t startIndex = 0;
    
    if (showLatest && pointCount > 20)
    {
        displayCount = 20;
        startIndex = pointCount - 20;
    }
    
    if (displayCount < 2) return;  // 至少需要2个点才能绘制折线

    // 1. 找到Y数据的最小值和最大值，用于归一化
    float minYf, maxYf;
    int16_t minYi, maxYi;

    if (dataType == DATA_TYPE_INT16)
    {
        const int16_t* yDataInt = (const int16_t*)yData;
        minYi = yDataInt[startIndex];
        maxYi = yDataInt[startIndex];
        for (uint8_t i = startIndex + 1; i < startIndex + displayCount; i++)
        {
            if (yDataInt[i] < minYi) minYi = yDataInt[i];
            if (yDataInt[i] > maxYi) maxYi = yDataInt[i];
        }
        minYf = (float)minYi;
        maxYf = (float)maxYi;
    }
    else // DATA_TYPE_FLOAT
    {
        const float* yDataFloat = (const float*)yData;
        minYf = yDataFloat[startIndex];
        maxYf = yDataFloat[startIndex];
        for (uint8_t i = startIndex + 1; i < startIndex + displayCount; i++)
        {
            if (yDataFloat[i] < minYf) minYf = yDataFloat[i];
            if (yDataFloat[i] > maxYf) maxYf = yDataFloat[i];
        }
        minYi = (int16_t)minYf;
        maxYi = (int16_t)maxYf;
    }

    // 避免除零错误
    if (maxYf == minYf) maxYf = minYf + 1.0f;

    // 调整Y轴范围，使其比实际数据范围稍大
    float rangeFloat = maxYf - minYf;
    minYf -= rangeFloat * 0.1f;  // 减去10%的范围
    maxYf += rangeFloat * 0.1f;  // 加上10%的范围

    // 2. 绘制坐标轴
    if (drawAxis)
    {
        // X轴
        OLED_DrawLine(x0, y0 + height, x0 + width, y0 + height, color);
        // Y轴
        OLED_DrawLine(x0, y0, x0, y0 + height, color);

        // 绘制箭头
        OLED_DrawLine(x0 + width, y0 + height, x0 + width - 5, y0 + height - 3, color);
        OLED_DrawLine(x0 + width, y0 + height, x0 + width - 5, y0 + height + 3, color);
        OLED_DrawLine(x0, y0, x0 - 3, y0 + 5, color);
        OLED_DrawLine(x0, y0, x0 + 3, y0 + 5, color);

        // 绘制X轴刻度和时间标签
        uint8_t labelIntervalX = width / 5;  // X轴刻度间隔
        for (uint8_t i = 1; i <= 5; i++)
        {
            int16_t xPos = x0 + i * labelIntervalX;
            // 绘制刻度线
            OLED_DrawLine(xPos, y0 + height, xPos, y0 + height + 3, color);
            // 绘制时间标签
            uint16_t timeValue;
            if (showLatest && pointCount > 20)
            {
                // 如果只显示最近20个点，时间从(startIndex)开始
                timeValue = (startIndex + i * labelIntervalX * displayCount / width) * timeInterval;
            }
            else
            {
                // 否则时间从0开始
                timeValue = i * labelIntervalX * pointCount / width * timeInterval;
            }
            OLED_Printf(xPos - 15, y0 + height + 5, 8, "%d", timeValue);
        }

        // 绘制Y轴刻度和数值标签
        uint8_t labelIntervalY = height / 5;  // Y轴刻度间隔
        int16_t range = maxYi - minYi;
        // 防止除零错误
        if (range == 0) range = 1;
        for (uint8_t i = 1; i <= 5; i++)
        {
            int16_t yPos = y0 + height - i * labelIntervalY;
            // 绘制刻度线
            OLED_DrawLine(x0 - 3, yPos, x0, yPos, color);
            // 计算对应的Y值
            int16_t yValue = minYi + (i * labelIntervalY * range) / height;
            // 绘制数值标签
            char yLabel[10];
            sprintf(yLabel, "%d", yValue);
            OLED_Printf(x0 - 30, yPos - 4, 8, "%s", yLabel);
        }
    }
    
    // 计算最近数据的均值
    float sum = 0.0f;
    uint8_t meanCount = displayCount;
    for (uint8_t i = 0; i < meanCount; i++)
    {
        uint8_t dataIndex = startIndex + i;
        if (dataType == DATA_TYPE_INT16)
        {
            const int16_t* yDataInt = (const int16_t*)yData;
            sum += (float)yDataInt[dataIndex];
        }
        else // DATA_TYPE_FLOAT
        {
            const float* yDataFloat = (const float*)yData;
            sum += yDataFloat[dataIndex];
        }
    }
    float meanValue = sum / meanCount;

    // 绘制均值线
    int16_t meanY = y0 + height - (int16_t)((meanValue - minYf) * height / (maxYf - minYf));
    OLED_DrawLine(x0, meanY, x0 + width, meanY, color);

    // 3. 绘制折线
    if (displayCount > 0)
    {
        // 处理第一个点
        uint8_t firstDataIndex = startIndex;
        float firstYValue;
        if (dataType == DATA_TYPE_INT16)
        {
            const int16_t* yDataInt = (const int16_t*)yData;
            firstYValue = (float)yDataInt[firstDataIndex];
        }
        else // DATA_TYPE_FLOAT
        {
            const float* yDataFloat = (const float*)yData;
            firstYValue = yDataFloat[firstDataIndex];
        }
        int16_t prevX = x0;
        int16_t prevY = y0 + height - ((firstYValue - minYf) * height) / (maxYf - minYf);

        // 处理剩余点并绘制折线
        for (uint8_t i = 1; i < displayCount; i++)
        {
            // 计算X坐标(基于时间序列)，越靠右数据越新
            int16_t x = x0 + (i * width) / (displayCount - 1);
            // 确保X坐标不超出屏幕边界
            if (x > 127) x = 127;
            // 当前数据点在原数组中的索引
            uint8_t dataIndex = startIndex + i;
            // 归一化Y坐标
            float yValue;
            if (dataType == DATA_TYPE_INT16)
            {
                const int16_t* yDataInt = (const int16_t*)yData;
                yValue = (float)yDataInt[dataIndex];
            }
            else // DATA_TYPE_FLOAT
            {
                const float* yDataFloat = (const float*)yData;
                yValue = yDataFloat[dataIndex];
            }
            
            int16_t y = y0 + height - ((yValue - minYf) * height) / (maxYf - minYf);

            // 确保坐标在屏幕范围内
            if (x < x0) x = x0;
            if (x > x0 + width) x = x0 + width;
            if (y < y0) y = y0;
            if (y > y0 + height) y = y0 + height;

            // 绘制线段
            OLED_DrawLine(prevX, prevY, x, y, color);

            // 更新前一个点的坐标
            prevX = x;
            prevY = y;
        }
    }
}



/**
  * 函    数:将OLED显存数组部分取反
  * 参    数:X 指定区域左上角的横坐标,范围:0~127
  * 参    数:Y 指定区域左上角的纵坐标,范围:0~63
  * 参    数:Width 指定区域的宽度,范围:0~128
  * 参    数:Height 指定区域的高度,范围:0~64
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	uint8_t i, j;
	
	/*参数检查,保证指定区域不会超出屏幕范围*/
	if (X > 127) {return;}
	if (Y > 63) {return;}
	if (X + Width > 128) {Width = 128 - X;}
	if (Y + Height > 64) {Height = 64 - Y;}
	
	for (j = Y; j < Y + Height; j ++)		//遍历指定页
	{
		for (i = X; i < X + Width; i ++)	//遍历指定列
		{
			OLED_GRAM[j / 8][i] ^= 0x01 << (j % 8);	//将显存数组指定数据取反
		}
	}
}

/**
  * 函    数:将OLED显存数组部分清零
  * 参    数:X 指定区域左上角的横坐标,范围:0~127
  * 参    数:Y 指定区域左上角的纵坐标,范围:0~63
  * 参    数:Width 指定区域的宽度,范围:0~128
  * 参    数:Height 指定区域的高度,范围:0~64
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	uint8_t i, j;
	
	/*参数检查,保证指定区域不会超出屏幕范围*/
	if (X > 127) {return;}
	if (Y > 63) {return;}
	if (X + Width > 128) {Width = 128 - X;}
	if (Y + Height > 64) {Height = 64 - Y;}
	
	for (j = Y; j < Y + Height; j ++)		//遍历指定页
	{
		for (i = X; i < X + Width; i ++)	//遍历指定列
		{
			OLED_GRAM[j / 8][i] &= ~(0x01 << (j % 8));	//将显存数组指定数据清零
		}
	}
}

/**
  * 函    数:OLED显示图像
  * 参    数:X 指定图像左上角的横坐标,范围:无限制
  * 参    数:Y 指定图像左上角的纵坐标,范围:无限制
  * 参    数:Width 指定图像的宽度,范围:0~128
  * 参    数:Height 指定图像的高度,范围:0~64
  * 参    数:Image 指定要显示的图像
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    // 计算实际显示区域(处理负坐标和超出边界的情况)
    int16_t displayX = (X < 0) ? 0 : X;
    int16_t displayY = (Y < 0) ? 0 : Y;
    
    // 计算可显示的宽度和高度(防止负数)
    int16_t displayWidth = (X < 0) ? (Width + X) : Width;
    int16_t displayHeight = (Y < 0) ? (Height + Y) : Height;
    
    // 调整负值情况
    if (displayWidth < 0) displayWidth = 0;
    if (displayHeight < 0) displayHeight = 0;
    
    // 限制在屏幕范围内
    if (displayX + displayWidth > 128) displayWidth = 128 - displayX;
    if (displayY + displayHeight > 64) displayHeight = 64 - displayY;
    
    // 检查是否完全在屏幕外
    if (displayWidth <= 0 || displayHeight <= 0 || displayX >= 128 || displayY >= 64) {
        return;
    }
    
    // 计算源图像起始偏移
    int16_t srcXOffset = (X < 0) ? -X : 0;
    int16_t srcYOffset = (Y < 0) ? -Y : 0;
    
    // 计算目标起始页和位偏移
    uint8_t destStartPage = displayY / 8;
    uint8_t destStartBit = displayY % 8;
    
    // 计算需要处理的页数
    uint8_t pageCount = (displayHeight + destStartBit + 7) / 8;
    
    // 逐页处理
    for (uint8_t page = 0; page < pageCount; page++) {
        uint8_t destPage = destStartPage + page;
        if (destPage >= 8) break;  // 最多8页(64像素)
        
        // 计算当前页在源图像中的起始行
        int16_t srcStartRow = srcYOffset + (page * 8) - destStartBit;
        if (srcStartRow < 0) {
            // 处理负行偏移(当Y<0且未完全进入屏幕时)
            srcStartRow = 0;
        }
        
        // 计算源图像起始页和位偏移
        uint8_t srcPage = srcStartRow / 8;
        uint8_t srcBitOffset = srcStartRow % 8;
        
        // 当前目标页的位偏移(只有第一页有初始偏移)
        uint8_t currentDestBitOffset = (page == 0) ? destStartBit : 0;
        
        // 遍历每列
        for (int16_t col = 0; col < displayWidth; col++) {
            // 源图像索引
            uint16_t srcIndex = (srcPage * Width) + srcXOffset + col;
            
            // 检查索引是否有效
            if (srcIndex >= (uint16_t)(Width * Height)) continue;
            
            uint8_t srcData = Image[srcIndex];
            uint8_t nextData = 0;
            
            // 获取下一行数据(如果需要)
            if (srcBitOffset != 0 && (srcPage + 1) * 8 < Height) {
                uint16_t nextIndex = srcIndex + Width;
                if (nextIndex < (uint16_t)(Width * Height)) {
                    nextData = Image[nextIndex];
                }
            }
            
            // 组合源数据
            uint8_t combinedData;
            if (srcBitOffset == 0) {
                combinedData = srcData;
            } else {
                combinedData = (srcData >> srcBitOffset) | (nextData << (8 - srcBitOffset));
            }
            
            // 应用目标位偏移
            if (currentDestBitOffset == 0) {
                // 无偏移,直接写入
                OLED_GRAM[destPage][displayX + col] |= combinedData;
            } else {
                // 写入当前页
                OLED_GRAM[destPage][displayX + col] |= combinedData << currentDestBitOffset;
                
                // 写入下一页(如果需要)
                if (destPage < 7) {
                    OLED_GRAM[destPage + 1][displayX + col] |= combinedData >> (8 - currentDestBitOffset);
                }
            }
        }
    }
}
    

/**
  * 函    数:OLED使用printf函数打印格式化字符串
  * 参    数:X 指定格式化字符串左上角的横坐标,范围:0~127
  * 参    数:Y 指定格式化字符串左上角的纵坐标,范围:0~63
  * 参    数:FontSize 指定字体大小
  *           范围:OLED_8X16		宽8像素,高16像素
  *                 OLED_6X8		宽6像素,高8像素
  * 参    数:format 指定要显示的格式化字符串,范围:ASCII码可见字符组成的字符串
  * 参    数:... 格式化字符串参数列表
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, const char *format, ...)
{
    char String[256];  // 扩大字符数组的大小,以处理更长的格式化字符串
    va_list arg;
    va_start(arg, format);
    vsnprintf(String, sizeof(String), format, arg);  // 使用 vsnprintf 避免缓冲区溢出
    va_end(arg);

    int16_t currentX = X;
    int16_t currentY = Y;
    uint8_t lineHeight = (FontSize == 16) ? 16 : 8;

    uint8_t i = 0;
    while (String[i] != '\0') {
        if ((String[i] & 0x80) == 0) {  // ASCII字符
			if (String[i] == '\n') {
				// 处理换行符
				currentY += lineHeight;
				currentX = X;
				i++;
				continue;
			} else if (String[i] == '\r') {
				// 处理回车符
				currentX = X;
				i++;
				continue;
			}
			if (lineHeight == 16) {
					OLED_ShowImage(currentX, currentY, 8, 16, OLED_F8x16[String[i] - ' ']);
				currentX += 8;
			} else if (lineHeight == 8) {
					OLED_ShowImage(currentX, currentY, 6, 8, OLED_F6x8[String[i] - ' ']);
				currentX += 6;
			}
			
			i++;
				} else {  // 可能是汉字(多字节编码)
					char singleChinese[OLED_CHN_CHAR_WIDTH + 1] = {0};
					for (uint8_t j = 0; j < OLED_CHN_CHAR_WIDTH; j++) {
						singleChinese[j] = String[i + j];
					}
					uint8_t pIndex;
            // 遍历整个汉字字模库,寻找匹配的汉字
					for (pIndex = 0; strcmp(OLED_CF16x16[pIndex].Name, "") != 0; pIndex++) {
						if (strcmp(OLED_CF16x16[pIndex].Name, singleChinese) == 0) {
							break;  // 跳出循环,此时pIndex的值为指定汉字的索引
							}
					}
					if (currentX >= 0 && currentY >= 0) {
                // 将汉字字模库OLED_CF16x16的指定数据以16*16的图像格式显示
						OLED_ShowImage(currentX, currentY, 16, 16, OLED_CF16x16[pIndex].Data);
					}
					currentX += 16;
					i += OLED_CHN_CHAR_WIDTH;
				}
		}
}


/**
  * 函    数:OLED在指定位置画一个点
  * 参    数:X 指定点的横坐标,范围:0~127
  * 参    数:Y 指定点的纵坐标,范围:0~63
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawPoint(int16_t X, int16_t Y, uint8_t Color)

{
	/*参数检查,保证指定位置不会超出屏幕范围*/
	if (X > 127) {return;}
	if (Y > 63) {return;}
	
	/*将显存数组指定位置的一个Bit数据置1*/
	OLED_GRAM[Y / 8][X] |= 0x01 << (Y % 8);
}

/**
  * 函    数:判断指定点是否在指定多边形内部
  * 参    数:nvert 多边形的顶点数
  * 参    数:vertx verty 包含多边形顶点的x和y坐标的数组
  * 参    数:testx testy 测试点的X和y坐标
  * 返 回 值:指定点是否在指定多边形内部,1:在内部,0:不在内部
  */
uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
	int16_t i, j, c = 0;
	
	/*此算法由W. Randolph Franklin提出*/
	/*参考链接:https://wrfranklin.org/Research/Short_Notes/pnpoly.html*/
	for (i = 0, j = nvert - 1; i < nvert; j = i++)
	{
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
		{
			c = !c;
		}
	}
	return c;
}

/**
  * 函    数:判断指定点是否在指定角度内部
  * 参    数:X Y 指定点的坐标
  * 参    数:StartAngle EndAngle 起始角度和终止角度,范围:-180~180
  *           水平向右为0度,水平向左为180度或-180度,下方为正数,上方为负数,顺时针旋转
  * 返 回 值:指定点是否在指定角度内部,1:在内部,0:不在内部
  */
uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
	int16_t PointAngle;
	PointAngle = atan2(Y, X) / 3.14 * 180;	//计算指定点的弧度,并转换为角度表示
	if (StartAngle < EndAngle)	//起始角度小于终止角度的情况
	{
		/*如果指定角度在起始终止角度之间,则判定指定点在指定角度*/
		if (PointAngle >= StartAngle && PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	else			//起始角度大于于终止角度的情况
	{
		/*如果指定角度大于起始角度或者小于终止角度,则判定指定点在指定角度*/
		if (PointAngle >= StartAngle || PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	return 0;		//不满足以上条件,则判断判定指定点不在指定角度
}

/**
  * 函    数:OLED画线
  * 参    数:X0 指定一个端点的横坐标,范围:0~127
  * 参    数:Y0 指定一个端点的纵坐标,范围:0~63
  * 参    数:X1 指定另一个端点的横坐标,范围:0~127
  * 参    数:Y1 指定另一个端点的纵坐标,范围:0~63
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t Color)
{
	int16_t x, y, dx, dy, d, incrE, incrNE, temp;
	int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
	uint8_t yflag = 0, xyflag = 0;
	
	if (y0 == y1)		//横线单独处理
	{
		/*0号点X坐标大于1号点X坐标,则交换两点X坐标*/
		if (x0 > x1) {temp = x0; x0 = x1; x1 = temp;}
		
		/*遍历X坐标*/
		for (x = x0; x <= x1; x ++)
		{
            OLED_DrawPoint(x, y0, 1);	//依次画点
		}
	}
	else if (x0 == x1)	//竖线单独处理
	{
		/*0号点Y坐标大于1号点Y坐标,则交换两点Y坐标*/
		if (y0 > y1) {temp = y0; y0 = y1; y1 = temp;}
		
		/*遍历Y坐标*/
		for (y = y0; y <= y1; y ++)
		{
			OLED_DrawPoint(x0, y, 1);	//依次画点
		}
	}
	else				//斜线
	{
		/*使用Bresenham算法画直线,可以避免耗时的浮点运算,效率更高*/
		/*参考文档:https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
		/*参考教程:https://www.bilibili.com/video/BV1364y1d7Lo*/
		
		if (x0 > x1)	//0号点X坐标大于1号点X坐标
		{
			/*交换两点坐标*/
			/*交换后不影响画线,但是画线方向由第一、二、三、四象限变为第一、四象限*/
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}
		
		if (y0 > y1)	//0号点Y坐标大于1号点Y坐标
		{
			/*将Y坐标取负*/
			/*取负后影响画线,但是画线方向由第一、四象限变为第一象限*/
			y0 = -y0;
			y1 = -y1;
			
			/*置标志位yflag,记住当前变换,在后续实际画线时,再将坐标换回来*/
			yflag = 1;
		}
		
		if (y1 - y0 > x1 - x0)	//画线斜率大于1
		{
			/*将X坐标与Y坐标互换*/
			/*互换后影响画线,但是画线方向由第一象限0~90度范围变为第一象限0~45度范围*/
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;
			
			/*置标志位xyflag,记住当前变换,在后续实际画线时,再将坐标换回来*/
			xyflag = 1;
		}
		
		/*以下为Bresenham算法画直线*/
		/*算法要求,画线方向必须为第一象限0~45度范围*/
		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);
		d = 2 * dy - dx;
		x = x0;
		y = y0;
		
		/*画起始点,同时判断标志位,将坐标换回来*/
		if (yflag && xyflag){OLED_DrawPoint(y, -x, 1);}
		else if (yflag)		{OLED_DrawPoint(x, -y, 1);}
		else if (xyflag)	{OLED_DrawPoint(y, x, 1);}
		else				{OLED_DrawPoint(x, y, 1);}
		
		while (x < x1)		//遍历X轴的每个点
		{
			x ++;
			if (d < 0)		//下一个点在当前点东方
			{
				d += incrE;
			}
			else			//下一个点在当前点东北方
			{
				y ++;
				d += incrNE;
			}
			
			/*画每一个点,同时判断标志位,将坐标换回来*/
			if (yflag && xyflag){OLED_DrawPoint(y, -x, 1);}
			else if (yflag)		{OLED_DrawPoint(x, -y, 1);}
			else if (xyflag)	{OLED_DrawPoint(y, x, 1);}
			else				{OLED_DrawPoint(x, y, 1);}
		}	
	}
}

/**
  * 函    数:OLED矩形(带坐标循环)
  * 参    数:X 指定矩形左上角的横坐标,范围:自动循环处理
  * 参    数:Y 指定矩形左上角的纵坐标,范围:自动循环处理
  * 参    数:Width 指定矩形的宽度,范围:0~128
  * 参    数:Height 指定矩形的高度,范围:0~64
  * 参    数:IsFilled 指定矩形是否填充
  *           范围:OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * 返 回 值:无
  * 说    明:1. 当X或Y超出屏幕范围时,自动循环到另一侧
  *           2. 调用此函数后,需调用OLED_Update更新屏幕显示
  */
void OLED_DrawRectangle(int16_t X, int16_t Y, int16_t Width, int16_t Height, uint8_t IsFilled)

{
    // 处理X坐标循环(超出右边界时回到左侧)
    if (X >= 128) X %= 128;
    if (X < 0) X = 128 + (X % 128);
    
    // 处理Y坐标循环(超出下边界时回到顶部)
    if (Y >= 64) Y %= 64;
    if (Y < 0) Y = 64 + (Y % 64);
    
    uint8_t i, j;
    if (!IsFilled)        // 指定矩形不填充
    {
        /*遍历上下X坐标,画矩形上下两条线*/
        for (i = X; i < X + Width; i++)
        {
            OLED_DrawPoint(i % 128, Y, 1);
            OLED_DrawPoint(i % 128, (Y + Height - 1) % 64, 1);
        }
        /*遍历左右Y坐标,画矩形左右两条线*/
        for (i = Y; i < Y + Height; i++)
        {
            OLED_DrawPoint(X, i % 64, 1);
            OLED_DrawPoint((X + Width - 1) % 128, i % 64, 1);
        }
    }
    else                  // 指定矩形填充
    {
        /*遍历X坐标*/
        for (i = X; i < X + Width; i++)
        {
            /*遍历Y坐标*/
            for (j = Y; j < Y + Height; j++)
            {
                /*在指定区域画点,填充满矩形*/
                OLED_DrawPoint(i % 128, j % 64, 1);

            }
        }
    }
}

/**
  * 函    数:OLED反色矩形(带坐标循环)
  * 参    数:X 指定矩形左上角的横坐标,范围:自动循环处理
  * 参    数:Y 指定矩形左上角的纵坐标,范围:自动循环处理
  * 参    数:Width 指定矩形的宽度,范围:0~128
  * 参    数:Height 指定矩形的高度,范围:0~64
  * 参    数:IsFilled 指定反色范围(空心/实心)
  *           范围:OLED_UNFILLED		仅反色矩形边框
  *                 OLED_FILLED			反色整个矩形区域
  * 返 回 值:无
  * 说    明:1. 当X或Y超出屏幕范围时,自动循环到另一侧
  *           2. 调用后需调用OLED_Update更新屏幕显示
  *           3. 连续调用两次可恢复原显示(反色两次等价于无操作)
  */
void OLED_ReverseRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    // 处理X/Y坐标循环(超出边界时自动循环)
    if (X >= 128) X %= 128;
    if (X < 0) X = 128 + (X % 128);
    if (Y >= 64) Y %= 64;
    if (Y < 0) Y = 64 + (Y % 64);
    
    uint8_t i, j;
    // 计算右下角坐标(用于边界判断)
    uint8_t x2 = (X + Width - 1) % 128;  // 右边框X坐标
    uint8_t y2 = (Y + Height - 1) % 64;   // 下边框Y坐标
    
    if (!IsFilled)  // 反色空心矩形(仅边框)
    {
        /* 1. 反色上下两条横线(完整范围,包含四个角点) */
        // 上横线(Y坐标=Y)
        for (i = X; i < X + Width; i++)
        {
            uint8_t curr_x = i % 128;
            OLED_GRAM[Y / 8][curr_x] ^= 0x01 << (Y % 8);
        }
        // 下横线(Y坐标=Y+Height-1)
        for (i = X; i < X + Width; i++)
        {
            uint8_t curr_x = i % 128;
            OLED_GRAM[y2 / 8][curr_x] ^= 0x01 << (y2 % 8);
        }
        
        /* 2. 反色左右两条竖线(缩小Y范围,避开四个角点) */
        // 竖线Y范围:从Y+1到Y+Height-2(不包含上下端点)
        uint8_t start_y = (Y + 1) % 64;
        uint8_t end_y = (Y + Height - 2) % 64;
        
        // 左边框(X坐标=X)
        // 处理Y循环:如果start_y <= end_y,直接循环；否则分两段(跨边界时)
        if (start_y <= end_y)
        {
            for (j = start_y; j <= end_y; j++)
            {
                OLED_GRAM[j / 8][X] ^= 0x01 << (j % 8);
            }
        }
        else
        {
            // 跨边界时:先从start_y到63,再从0到end_y
            for (j = start_y; j < 64; j++)
            {
                OLED_GRAM[j / 8][X] ^= 0x01 << (j % 8);
            }
            for (j = 0; j <= end_y; j++)
            {
                OLED_GRAM[j / 8][X] ^= 0x01 << (j % 8);
            }
        }
        
        // 右边框(X坐标=x2)
        if (start_y <= end_y)
        {
            for (j = start_y; j <= end_y; j++)
            {
                OLED_GRAM[j / 8][x2] ^= 0x01 << (j % 8);
            }
        }
        else
        {
            for (j = start_y; j < 64; j++)
            {
                OLED_GRAM[j / 8][x2] ^= 0x01 << (j % 8);
            }
            for (j = 0; j <= end_y; j++)
            {
                OLED_GRAM[j / 8][x2] ^= 0x01 << (j % 8);
            }
        }
    }
    else  // 反色实心矩形(整个区域,无需处理角点)
    {
        for (j = Y; j < Y + Height; j++)
        {
            for (i = X; i < X + Width; i++)
            {
                OLED_GRAM[(j % 64) / 8][i % 128] ^= 0x01 << (j % 8);
            }
        }
    }
}

/**
  * 函    数:OLED三角形
  * 参    数:X0 指定第一个端点的横坐标,范围:0~127
  * 参    数:Y0 指定第一个端点的纵坐标,范围:0~63
  * 参    数:X1 指定第二个端点的横坐标,范围:0~127
  * 参    数:Y1 指定第二个端点的纵坐标,范围:0~63
  * 参    数:X2 指定第三个端点的横坐标,范围:0~127
  * 参    数:Y2 指定第三个端点的纵坐标,范围:0~63
  * 参    数:IsFilled 指定三角形是否填充
  *           范围:OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
	uint8_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
	uint8_t i, j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};
	
	if (!IsFilled)			//指定三角形不填充
	{
		/*调用画线函数,将三个点用直线连接*/
		OLED_DrawLine(X0, Y0, X1, Y1, 1);
		OLED_DrawLine(X0, Y0, X2, Y2, 1);
		OLED_DrawLine(X1, Y1, X2, Y2, 1);
	}
	else					//指定三角形填充
	{
		/*找到三个点最小的X、Y坐标*/
		if (X1 < minx) {minx = X1;}
		if (X2 < minx) {minx = X2;}
		if (Y1 < miny) {miny = Y1;}
		if (Y2 < miny) {miny = Y2;}
		
		/*找到三个点最大的X、Y坐标*/
		if (X1 > maxx) {maxx = X1;}
		if (X2 > maxx) {maxx = X2;}
		if (Y1 > maxy) {maxy = Y1;}
		if (Y2 > maxy) {maxy = Y2;}
		
		/*最小最大坐标之间的矩形为可能需要填充的区域*/
		/*遍历此区域中所有的点*/
		/*遍历X坐标*/		
		for (i = minx; i <= maxx; i ++)
		{
			/*遍历Y坐标*/	
			for (j = miny; j <= maxy; j ++)
			{
				/*调用OLED_pnpoly,判断指定点是否在指定三角形之中*/
				/*如果在,则画点,如果不在,则不做处理*/
				if (OLED_pnpoly(3, vx, vy, i, j)) {OLED_DrawPoint(i, j, 1);}

			}
		}
	}
}

/**
  * 函    数:OLED画圆
  * 参    数:X 指定圆的圆心横坐标,范围:0~127
  * 参    数:Y 指定圆的圆心纵坐标,范围:0~63
  * 参    数:Radius 指定圆的半径,范围:0~255
  * 参    数:IsFilled 指定圆是否填充
  *           范围:OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawCircle(int16_t X, int16_t Y, int16_t Radius, uint8_t IsFilled)

{
	int16_t x, y, d, j;
	
	/*使用Bresenham算法画圆,可以避免耗时的浮点运算,效率更高*/
	/*参考文档:https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
	/*参考教程:https://www.bilibili.com/video/BV1VM4y1u7wJ*/
	
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	/*画每个八分之一圆弧的起始点*/
	OLED_DrawPoint(X + x, Y + y, 1);
	OLED_DrawPoint(X - x, Y - y, 1);
	OLED_DrawPoint(X + y, Y + x, 1);
	OLED_DrawPoint(X - y, Y - x, 1);
	
	if (IsFilled)		//指定圆填充
	{
		/*遍历起始点Y坐标*/
		for (j = -y; j < y; j ++)
		{
			/*在指定区域画点,填充部分圆*/
			OLED_DrawPoint(X, Y + j, 1);

		}
	}
	
	while (x < y)		//遍历X轴的每个点
	{
		x ++;
		if (d < 0)		//下一个点在当前点东方
		{
			d += 2 * x + 1;
		}
		else			//下一个点在当前点东南方
		{
			y --;
			d += 2 * (x - y) + 1;
		}
		
		/*画每个八分之一圆弧的点*/
		OLED_DrawPoint(X + x, Y + y, 1);
		OLED_DrawPoint(X + y, Y + x, 1);
		OLED_DrawPoint(X - x, Y - y, 1);
		OLED_DrawPoint(X - y, Y - x, 1);
		OLED_DrawPoint(X + x, Y - y, 1);
		OLED_DrawPoint(X + y, Y - x, 1);
		OLED_DrawPoint(X - x, Y + y, 1);
		OLED_DrawPoint(X - y, Y + x, 1);
		
		if (IsFilled)	//指定圆填充
		{
			/*遍历中间部分*/
			for (j = -y; j < y; j ++)
			{
				/*在指定区域画点,填充部分圆*/
				OLED_DrawPoint(X + x, Y + j, 1);
				OLED_DrawPoint(X - x, Y + j, 1);
			}
			
			/*遍历两侧部分*/
			for (j = -x; j < x; j ++)
			{
				/*在指定区域画点,填充部分圆*/
				OLED_DrawPoint(X - y, Y + j, 1);
				OLED_DrawPoint(X + y, Y + j, 1);
			}
		}
	}
}

/**
  * 函    数:OLED画椭圆
  * 参    数:X 指定椭圆的圆心横坐标,范围:0~127
  * 参    数:Y 指定椭圆的圆心纵坐标,范围:0~63
  * 参    数:A 指定椭圆的横向半轴长度,范围:0~255
  * 参    数:B 指定椭圆的纵向半轴长度,范围:0~255
  * 参    数:IsFilled 指定椭圆是否填充
  *           范围:OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
	int16_t x, y, j;
	int16_t a = A, b = B;
	float d1, d2;
	
	/*使用Bresenham算法画椭圆,可以避免部分耗时的浮点运算,效率更高*/
	/*参考链接:https://blog.csdn.net/myf_666/article/details/128167392*/
	
	x = 0;
	y = b;
	d1 = b * b + a * a * (-b + 0.5);
	
	if (IsFilled)	//指定椭圆填充
	{
		/*遍历起始点Y坐标*/
		for (j = -y; j < y; j ++)
		{
			/*在指定区域画点,填充部分椭圆*/
			OLED_DrawPoint(X, Y + j, 1);
			OLED_DrawPoint(X, Y + j, 1);
		}
	}
	
	/*画椭圆弧的起始点*/
	OLED_DrawPoint(X + x, Y + y, 1);
	OLED_DrawPoint(X - x, Y - y, 1);
	OLED_DrawPoint(X - x, Y + y, 1);
	OLED_DrawPoint(X + x, Y - y, 1);
	
	/*画椭圆中间部分*/
	while (b * b * (x + 1) < a * a * (y - 0.5))
	{
		if (d1 <= 0)		//下一个点在当前点东方
		{
			d1 += b * b * (2 * x + 3);
		}
		else				//下一个点在当前点东南方
		{
			d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
			y --;
		}
		x ++;
		
		if (IsFilled)	//指定椭圆填充
		{
			/*遍历中间部分*/
			for (j = -y; j < y; j ++)
			{
				/*在指定区域画点,填充部分椭圆*/
				OLED_DrawPoint(X + x, Y + j, 1);
				OLED_DrawPoint(X - x, Y + j, 1);
			}
		}
		
		/*画椭圆中间部分圆弧*/
		OLED_DrawPoint(X + x, Y + y, 1);
		OLED_DrawPoint(X - x, Y - y, 1);
		OLED_DrawPoint(X - x, Y + y, 1);
		OLED_DrawPoint(X + x, Y - y, 1);
	}
	
	/*画椭圆两侧部分*/
	d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;
	
	while (y > 0)
	{
		if (d2 <= 0)		//下一个点在当前点东方
		{
			d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
			x ++;
			
		}
		else				//下一个点在当前点东南方
		{
			d2 += a * a * (-2 * y + 3);
		}
		y --;
		
		if (IsFilled)	//指定椭圆填充
		{
			/*遍历两侧部分*/
			for (j = -y; j < y; j ++)
			{
				/*在指定区域画点,填充部分椭圆*/
				OLED_DrawPoint(X + x, Y + j, 1);
				OLED_DrawPoint(X - x, Y + j, 1);
			}
		}
		
		/*画椭圆两侧部分圆弧*/
		OLED_DrawPoint(X + x, Y + y, 1);
		OLED_DrawPoint(X - x, Y - y, 1);
		OLED_DrawPoint(X - x, Y + y, 1);
		OLED_DrawPoint(X + x, Y - y, 1);
	}
}

/**
  * 函    数:OLED画圆弧
  * 参    数:X 指定圆弧的圆心横坐标,范围:0~127
  * 参    数:Y 指定圆弧的圆心纵坐标,范围:0~63
  * 参    数:Radius 指定圆弧的半径,范围:0~255
  * 参    数:StartAngle 指定圆弧的起始角度,范围:-180~180
  *           水平向右为0度,水平向左为180度或-180度,下方为正数,上方为负数,顺时针旋转
  * 参    数:EndAngle 指定圆弧的终止角度,范围:-180~180
  *           水平向右为0度,水平向左为180度或-180度,下方为正数,上方为负数,顺时针旋转
  * 参    数:IsFilled 指定圆弧是否填充,填充后为扇形
  *           范围:OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
	int16_t x, y, d, j;
	
	/*此函数借用Bresenham算法画圆的方法*/
	
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	/*在画圆的每个点时,判断指定点是否在指定角度内,在,则画点,不在,则不做处理*/
	if (OLED_IsInAngle(x, y, StartAngle, EndAngle))	{OLED_DrawPoint(X + x, Y + y, 1);}
	if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y, 1);}
	if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x, 1);}
	if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x, 1);}

	
	if (IsFilled)	//指定圆弧填充
	{
		/*遍历起始点Y坐标*/
		for (j = -y; j < y; j ++)
		{
			/*在填充圆的每个点时,判断指定点是否在指定角度内,在,则画点,不在,则不做处理*/
			if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) {OLED_DrawPoint(X, Y + j, 1);}

		}
	}
	
	while (x < y)		//遍历X轴的每个点
	{
		x ++;
		if (d < 0)		//下一个点在当前点东方
		{
			d += 2 * x + 1;
		}
		else			//下一个点在当前点东南方
		{
			y --;
			d += 2 * (x - y) + 1;
		}
		
		/*在画圆的每个点时,判断指定点是否在指定角度内,在,则画点,不在,则不做处理*/
		if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + y, 1);}
		if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x, 1);}
		if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y, 1);}
		if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x, 1);}
		if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y  + y, 1);}
		if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y - x, 1);}
		if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + y, 1);}
		if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + x, 1);}
		
		if (IsFilled)	//指定圆弧填充
		{
			/*遍历中间部分*/
			for (j = -y; j < y; j ++)
			{
				/*在填充圆的每个点时,判断指定点是否在指定角度内,在,则画点,不在,则不做处理*/
				if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + j, 1);}
				if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + j, 1);}
			}
			
			/*遍历两侧部分*/
			for (j = -x; j < x; j ++)
			{
				/*在填充圆的每个点时,判断指定点是否在指定角度内,在,则画点,不在,则不做处理*/
				if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + j, 1);}
				if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + j, 1);}
			}
		}
	}
}

/**
  * 函    数:OLED显示字符
  * 参    数:x 指定字符左上角的横坐标,范围:0~127
  * 参    数:y 指定字符左上角的纵坐标,范围:0~63
  * 参    数:c 指定要显示的字符,范围:ASCII码可见字符
  * 参    数:FontSize 指定字体大小
  *           范围:OLED_8X16		宽8像素,高16像素
  *                 OLED_6X8		宽6像素,高8像素
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_ShowChar(int16_t x, int16_t y, char c, uint8_t FontSize)
{
	if (FontSize == OLED_8X16)		//字体为宽8像素,高16像素
	{
		/*将ASCII字模库OLED_F8x16的指定数据以8*16的图像格式显示*/
		OLED_ShowImage(x, y, 8, 16, OLED_F8x16[c - ' ']);
	}
	else if (FontSize == OLED_6X8)	//字体为宽6像素,高8像素
	{
		/*将ASCII字模库OLED_F6x8的指定数据以6*8的图像格式显示*/
		OLED_ShowImage(x, y, 6, 8, OLED_F6x8[c - ' ']);
	}
}

/**
  * 函    数:OLED显示字符串
  * 参    数:x 指定字符串左上角的横坐标,范围:0~127
  * 参    数:y 指定字符串左上角的纵坐标,范围:0~63
  * 参    数:str 指定要显示的字符串,范围:ASCII码可见字符组成的字符串
  * 参    数:FontSize 指定字体大小
  *           范围:OLED_8X16		宽8像素,高16像素
  *                 OLED_6X8		宽6像素,高8像素
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_ShowString(int16_t x, int16_t y, const char *str, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; str[i] != '\0'; i++)		//遍历字符串的每个字符
	{
		/*调用OLED_ShowChar函数,依次显示每个字符*/
		OLED_ShowChar(x + i * (FontSize == OLED_8X16 ? 8 : 6), y, str[i], FontSize);
	}
}

/**
  * 函    数:OLED显示汉字
  * 参    数:x 指定汉字左上角的横坐标,范围:0~127
  * 参    数:y 指定汉字左上角的纵坐标,范围:0~63
  * 参    数:ch 指定要显示的汉字,范围:汉字字模库OLED_CF16x16中的汉字
  * 参    数:FontSize 指定字体大小,目前只支持16x16
  * 返 回 值:无
  * 说    明:调用此函数后,要想真正地呈现在屏幕上,还需调用更新函数
  */
void OLED_ShowChinese(int16_t x, int16_t y, const char *ch, uint8_t FontSize)
{
	const uint8_t *pData = OLED_FindChinese(ch);
	if (pData != 0)
	{
		OLED_ShowImage(x, y, 16, 16, pData);
	}
	else
	{
		// 如果找不到汉字，显示一个方框作为占位符
		OLED_DrawRectangle(x, y, x + 15, y + 15, OLED_COLOR_WHITE);
	}
}
