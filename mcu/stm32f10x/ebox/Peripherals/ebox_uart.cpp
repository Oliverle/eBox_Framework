/**
  ******************************************************************************
  * @file    uart.cpp
  * @author  shentq
  * @version V1.2
  * @date    2016/08/14
  * @brief   
  ******************************************************************************
  * @attention
  *
  * No part of this software may be used for any commercial activities by any form 
  * or means, without the prior written consent of shentq. This specification is 
  * preliminary and is subject to change at any time without notice. shentq assumes
  * no responsibility for any errors contained herein.
  * <h2><center>&copy; Copyright 2015 shentq. All Rights Reserved.</center></h2>
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "ebox_uart.h"
#include "ebox_mem.h"


uint8_t busy[5];

static uint32_t serial_irq_ids[UART_NUM] = {0, 0, 0,0,0};

static uart_irq_handler irq_handler;


/**
 *@name     Uart::Uart(USART_TypeDef *USARTx,Gpio *tx_pin,Gpio *rx_pin)
 *@brief    串口的构造函数
 *@param    USARTx:  USART1,2,3和UART4,5
 *          tx_pin:  外设所对应的tx引脚
 *          rx_pin:  外设所对应的rx引脚
 *@retval   None
*/
Uart::Uart(USART_TypeDef *USARTx, Gpio *tx_pin, Gpio *rx_pin)
{
    _USARTx = USARTx;
    tx_pin->mode(AF_PP);
    rx_pin->mode(INPUT);
}

/**
 *@name     void Uart::begin(uint32_t baud_rate)
 *@brief    串口初始化函数，除了波特率可控外，其他参数默认：8bit数据，1个停止位，无校验，无硬件控制流
 *@param    baud_rate:  波特率，例如9600，115200，38400等等
 *@retval   None
*/
void Uart::begin(uint32_t baud_rate)
{
    uint8_t index;
		USART_InitTypeDef USART_InitStructure;

    if(USE_DMA == 1)
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA时钟

    switch((uint32_t)_USARTx)
    {
    case (uint32_t)USART1_BASE:
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        _DMA1_Channelx = DMA1_Channel4;
				index = NUM_UART1;
        break;

    case (uint32_t)USART2_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        _DMA1_Channelx = DMA1_Channel7;
				index = NUM_UART2;
        break;

    case (uint32_t)USART3_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        _DMA1_Channelx = DMA1_Channel2;
				index = NUM_UART3;
        break;

#if defined (STM32F10X_HD)
    case (uint32_t)UART4_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
				index = NUM_UART4;
        break;

    case (uint32_t)UART5_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
				index = NUM_UART5;
        break;
#endif
    }

    serial_irq_handler(index, Uart::_irq_handler, (uint32_t)this);
		
    USART_InitStructure.USART_BaudRate = baud_rate;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(_USARTx, &USART_InitStructure);

    if((_USARTx == USART1 | _USARTx == USART2 | _USARTx == USART3) && (USE_DMA == 1) )
        USART_DMACmd(_USARTx, USART_DMAReq_Tx, ENABLE);
    USART_Cmd(_USARTx, ENABLE);
    interrupt(ENABLE);
}
/**
 *@name     void    begin(uint32_t baud_rate,uint8_t data_bit,uint8_t parity,float stop_bit);
 *@brief    串口初始化函数，并带有更多配置参数
 *@param    baud_rate:  波特率，例如9600，115200，38400等等
 *          data_bit:   数据位数，只能输入8或者9
 *          parity:     检验位；0：无校验位，1奇校验，2偶校验
 *          stop_bit:   停止位；0.5,1,1.5,2四个可选参数
 *@retval   None
*/
void Uart::begin(uint32_t baud_rate, uint8_t data_bit, uint8_t parity, float stop_bit)
{
    USART_InitTypeDef USART_InitStructure;
    if (USE_DMA == 1)
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA时钟

    switch((uint32_t)_USARTx)
    {
    case (uint32_t)USART1_BASE:
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        _DMA1_Channelx = DMA1_Channel4;
        break;

    case (uint32_t)USART2_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        _DMA1_Channelx = DMA1_Channel7;
        break;

    case (uint32_t)USART3_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        _DMA1_Channelx = DMA1_Channel2;
        break;

#if defined (STM32F10X_HD)
    case (uint32_t)UART4_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
        break;

    case (uint32_t)UART5_BASE:
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
        break;
#endif
    }

    USART_InitStructure.USART_BaudRate = baud_rate;
    switch(data_bit)
    {
    case 8:
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        break;
    case 9:
        USART_InitStructure.USART_WordLength = USART_WordLength_9b;
        break;
    default :
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        break;
    }
    switch(parity)
    {
    case 0:
        USART_InitStructure.USART_Parity = USART_Parity_No;
        break;
    case 1:
        USART_InitStructure.USART_Parity = USART_Parity_Even;
        break;
    case 2:
        USART_InitStructure.USART_Parity = USART_Parity_Odd;
        break;
    default :
        USART_InitStructure.USART_Parity = USART_Parity_No;
        break;
    }
    if(parity == 0.5)
        USART_InitStructure.USART_StopBits = USART_StopBits_0_5;
    else if(parity == 1)
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
    else if(parity == 1.5)
        USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
    else if(parity == 2)
        USART_InitStructure.USART_StopBits = USART_StopBits_2;

    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(_USARTx, &USART_InitStructure);

    if((_USARTx == USART1 | _USARTx == USART2 | _USARTx == USART3) && (USE_DMA == 1) )
        USART_DMACmd(_USARTx, USART_DMAReq_Tx, ENABLE);
    USART_Cmd(_USARTx, ENABLE);
}

/**
 *@name     void Uart::interrupt(FunctionalState enable)
 *@brief    串口中断控制函数
 *@param    enable:  ENABLE使能串口的发送完成和接收中断；DISABLE：关闭这两个中断
 *@retval   None
*/
void Uart::interrupt(FunctionalState enable)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    USART_ITConfig(_USARTx, USART_IT_RXNE, enable);
    USART_ITConfig(_USARTx, USART_IT_TC, enable);
    USART_ClearITPendingBit(_USARTx, USART_IT_TC);

    switch((uint32_t)_USARTx)
    {
    case (uint32_t)USART1_BASE:
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        break;

    case (uint32_t)USART2_BASE:
        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        break;

    case (uint32_t)USART3_BASE:
        NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        break;
#if defined (STM32F10X_HD)
    case (uint32_t)UART4_BASE:
        NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        break;
    case (uint32_t)UART5_BASE:
        NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        break;
#endif
    }
    NVIC_Init(&NVIC_InitStructure);
}


/**
 *@name     uint16_t Uart::receive()
 *@brief    串口接收数据，此函数只能在用户接收中断中调用
 *@param    NONE
 *@retval   串口所接收到的数据
*/
uint16_t Uart::receive()
{
    return (uint16_t)(_USARTx->DR & (uint16_t)0x01FF);
}

/**
 *@name     uint16_t Uart::dma_send_string(const char *str,uint16_t length)
 *@brief    串口DMA方式发送字符串，缓冲区数据
 *@param    str：       要发送的字符串，数据缓冲区
            length：    缓冲区的长度
 *@retval   发送数据的长度
*/
uint16_t Uart::dma_send_string(const char *str, uint16_t length)
{
    DMA_DeInit(_DMA1_Channelx);   //将DMA的通道1寄存器重设为缺省值
    _DMA1_Channelx->CPAR = (uint32_t)&_USARTx->DR; //外设地址
    _DMA1_Channelx->CMAR = (uint32_t) str; //mem地址
    _DMA1_Channelx->CNDTR = length ; //传输长度
    _DMA1_Channelx->CCR = (0 << 14) | // 非存储器到存储器模式
                          (2 << 12) | // 通道优先级高
                          (0 << 11) | // 存储器数据宽度8bit
                          (0 << 10) | // 存储器数据宽度8bit
                          (0 <<  9) | // 外设数据宽度8bit
                          (0 <<  8) | // 外设数据宽度8bit
                          (1 <<  7) | // 存储器地址增量模式
                          (0 <<  6) | // 外设地址增量模式(不增)
                          (0 <<  5) | // 非循环模式
                          (1 <<  4) | // 从存储器读
                          (1 <<  3) | // 是否允许传输错误中断
                          (0 <<  2) | // 是否允许半传输中断
                          (0 <<  1) | // 是否允许传输完成中断
                          (1);        // 通道开启
    return length;
}

/**
 *@name     int Uart::put_char(uint16_t ch)
 *@brief    串口方式发送一个字节
 *@param    ch：    要发送的字符
 *@retval   已发送的数据
*/
int Uart::put_char(uint16_t ch)
{
    while(USART_GetFlagStatus(_USARTx, USART_FLAG_TXE) == RESET);//单字节等待，等待寄存器空
    USART_SendData(_USARTx, ch);
    return ch;
}

/**
 *@name     void Uart::put_string(const char *str,uint16_t length)
 *@brief    串口方式发送字符串，缓冲区数据
 *@param    str：       要发送的字符串，数据缓冲区
            length：    缓冲区的长度
 *@retval   NONE
*/
void Uart::put_string(const char *str, uint16_t length)
{
    if((_USARTx == USART1 | _USARTx == USART2 | _USARTx == USART3 ) && (USE_DMA == 1))
    {
        dma_send_string(str, length);
    }
    else
    {
        while(length--)
        {
            while(USART_GetFlagStatus(_USARTx, USART_FLAG_TXE) == RESET);//单字节等待，等待寄存器空
            USART_SendData(_USARTx, *str++);
        }
    }
}


/////////////////////////////////////////////////
/**
 *@name     int Uart::put_char(uint16_t ch)
 *@brief    串口方式发送一个字节
 *@param    str：       要发送的字符串，数据缓冲区
            length：    缓冲区的长度
 *@retval   已发送的数据
*/
void Uart::printf_length(const char *str, uint16_t length)
{
    wait_busy();
    ebox_free(uart_buf);
    set_busy();
    uart_buf = (char *)ebox_malloc(length);
    if(uart_buf == NULL)
    {
        return ;
    }
    for(int i = 0; i < length; i++)
        uart_buf[i] = *str++;
    put_string(uart_buf, length);
}

/**
 *@name     int Uart::put_char(uint16_t ch)
 *@brief    串口方式发送一个字节
 *@param    str：       要发送的字符串，数据缓冲区
            length：    缓冲区的长度
 *@retval   已发送的数据
*/

void Uart::printf(const char *fmt, ...)
{
    size_t size1=128,size2=128;

    wait_busy();
    ebox_free(uart_buf);
    set_busy();
    va_list va_params;
    va_start(va_params, fmt);
    
    do{
        uart_buf = (char *)ebox_malloc(size2);
        size1 = _vsnprintf(uart_buf, size2,fmt, va_params);
        if(size1 == -1)
            size2+=128;
        ebox_free(uart_buf);
    }while(size1 == -1);

    uart_buf = (char *)ebox_malloc(size1);
    vsprintf(uart_buf, fmt, va_params); //存在内存溢出的风险
    va_end(va_params);
    put_string(uart_buf, size1);

}


/**
 *@name     int Uart::put_char(uint16_t ch)
 *@brief    串口方式发送一个字节
 *@param    str：       要发送的字符串，数据缓冲区
            length：    缓冲区的长度
 *@retval   已发送的数据
*/
void Uart::wait_busy()
{
    switch((uint32_t)_USARTx)
    {
    case (uint32_t)USART1_BASE:
        while(busy[0] == 1);
        break;
    case (uint32_t)USART2_BASE:
        while(busy[1] == 1);
        break;
    case (uint32_t)USART3_BASE:
        while(busy[2] == 1);
        break;
    case (uint32_t)UART4_BASE:
        while(busy[3] == 1);
        break;
    case (uint32_t)UART5_BASE:
        while(busy[4] == 1);
        break;
    }
}

/**
 *@name     int Uart::put_char(uint16_t ch)
 *@brief    串口方式发送一个字节
 *@param    str：       要发送的字符串，数据缓冲区
            length：    缓冲区的长度
 *@retval   已发送的数据
*/
void Uart::set_busy()
{
    switch((uint32_t)_USARTx)
    {
    case (uint32_t)USART1_BASE:
        busy[0] = 1;
        break;
    case (uint32_t)USART2_BASE:
        busy[1] = 1;
        break;
    case (uint32_t)USART3_BASE:
        busy[2] = 1;
        break;
    case (uint32_t)UART4_BASE:
        busy[3] = 1;
        break;
    case (uint32_t)UART5_BASE:
        busy[4] = 1;
        break;
    }
}
//void Uart::clear_buf()
//{
//    switch((uint32_t)_USARTx)
//    {
//    case (uint32_t)USART1_BASE:
//        ebox_free(uart_buf);
//        break;
//    case (uint32_t)USART2_BASE:
//        busy[1] = 0;
//        break;
//    case (uint32_t)USART3_BASE:
//        busy[2] = 0;
//        break;
//    case (uint32_t)UART4_BASE:
//        busy[3] = 0;
//        break;
//    case (uint32_t)UART5_BASE:
//        busy[4] = 0;
//        break;
//    }
//}


void Uart::attach(void (*fptr)(void), IrqType type) {
    if (fptr) {
        _irq[type].attach(fptr);
    }
}

void Uart::_irq_handler(uint32_t id, IrqType irq_type) {
    Uart *handler = (Uart*)id;
    handler->_irq[irq_type].call();
}


extern "C" {

    void USART1_IRQHandler(void)
    {
        if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
        {
						irq_handler(serial_irq_ids[NUM_UART1],RxIrq);
            USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        }
        if(USART_GetITStatus(USART1, USART_IT_TC) == SET)
        {
            busy[0] = 0;   

						irq_handler(serial_irq_ids[NUM_UART1],TcIrq);
            USART_ClearITPendingBit(USART1, USART_IT_TC);
        }
    }
    void USART2_IRQHandler(void)
    {
        if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
        {
						irq_handler(serial_irq_ids[NUM_UART2],RxIrq);
            USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        }
        if(USART_GetITStatus(USART2, USART_IT_TC) == SET)
        {
            busy[1] = 0;
						irq_handler(serial_irq_ids[NUM_UART2],TcIrq);
            USART_ClearITPendingBit(USART2, USART_IT_TC);
        }
    }
    void USART3_IRQHandler(void)
    {
        if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
        {
						irq_handler(serial_irq_ids[NUM_UART3],RxIrq);
            USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        }
        if(USART_GetITStatus(USART3, USART_IT_TC) == SET)
        {
            busy[2] = 0;
						irq_handler(serial_irq_ids[NUM_UART3],TcIrq);
            USART_ClearITPendingBit(USART3, USART_IT_TC);
        }
    }
    void UART4_IRQHandler(void)
    {
        if(USART_GetITStatus(UART4, USART_IT_RXNE) == SET)
        {
						irq_handler(serial_irq_ids[NUM_UART4],RxIrq);
            USART_ClearITPendingBit(UART4, USART_IT_RXNE);
        }
        if(USART_GetITStatus(UART4, USART_IT_TC) == SET)
        {
            busy[3] = 0;
						irq_handler(serial_irq_ids[NUM_UART4],TcIrq);
            USART_ClearITPendingBit(UART4, USART_IT_TC);
        }
    }
    void UART5_IRQHandler(void)
    {
        if(USART_GetITStatus(UART5, USART_IT_RXNE) == SET)
        {
						irq_handler(serial_irq_ids[NUM_UART5],RxIrq);
            USART_ClearITPendingBit(UART5, USART_IT_RXNE);
        }
        if(USART_GetITStatus(UART5, USART_IT_TC) == SET)
        {
            busy[4] = 0;
						irq_handler(serial_irq_ids[NUM_UART5],TcIrq);
            USART_ClearITPendingBit(UART5, USART_IT_TC);
        }
    }

    //void DMA1_Channel4_IRQHandler(void)
    //	{

    //		DMA_Cmd(DMA1_Channel4,DISABLE);
    //		DMA_ClearFlag(DMA1_FLAG_TC4);

    //	}
    //	void DMA1_Channel7_IRQHandler(void)
    //	{
    //		DMA_Cmd(DMA1_Channel7,DISABLE);
    //		DMA_ClearFlag(DMA1_FLAG_TC7);
    //	}
    //	void DMA1_Channel2_IRQHandler(void)
    //	{
    //		DMA_Cmd(DMA1_Channel2,DISABLE);
    //		DMA_ClearFlag(DMA1_FLAG_TC2);
    //	}
		
void serial_irq_handler(uint8_t index, uart_irq_handler handler, uint32_t id)
{
    irq_handler = handler;
    serial_irq_ids[index] = id;
}
}




