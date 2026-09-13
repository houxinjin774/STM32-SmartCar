/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * @brief   中断服务函数 —— "事件一发生就自动跳进来的函数"
  *
  * 本工程用到的中断：
  *   SysTick    : 每1ms一次，给系统提供时间基准(g_ms++)
  *   EXTI9_5    : PB9超声波Echo的上升/下降沿 -> 测回波时间
  *   EXTI15_10  : PB12~15四个编码器脉冲 -> 计数
  *   USART1     : 蓝牙收到1个字节 -> 放进缓冲区
  ******************************************************************************
  */
#include "stm32f10x_it.h"
#include "config.h"
#include "encoder.h"
#include "ultrasonic.h"
#include "bluetooth.h"

/* 以下系统异常处理函数保持空实现（用不到） */
void NMI_Handler(void)        {}
void HardFault_Handler(void)  { while (1) {} }   /* 程序跑飞会卡在这 */
void MemManage_Handler(void)  { while (1) {} }
void BusFault_Handler(void)   { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void SVC_Handler(void)        {}
void DebugMon_Handler(void)   {}
void PendSV_Handler(void)     {}

/**
  * @brief  SysTick中断：每1毫秒自动进来一次
  */
void SysTick_Handler(void)
{
    g_ms++;   /* 系统时间+1毫秒，全程序都靠它知道"现在几点了" */
}

/**
  * @brief  外部中断9~5：这里只用了第9线（PB9 = HC-SR04 Echo）
  */
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line9) != RESET) {
        Ultra_EchoISR();                        /* 测量回波宽度 */
        EXTI_ClearITPendingBit(EXTI_Line9);     /* 清除中断标志，必须清！ */
    }
}

/**
  * @brief  外部中断15~10：PB12~15 = 4个编码器的脉冲计数
  */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line12) != RESET) {   /* 左前轮 */
        g_encTotal[0]++;
        EXTI_ClearITPendingBit(EXTI_Line12);
    }
    if (EXTI_GetITStatus(EXTI_Line13) != RESET) {   /* 右前轮 */
        g_encTotal[1]++;
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
    if (EXTI_GetITStatus(EXTI_Line14) != RESET) {   /* 左后轮 */
        g_encTotal[2]++;
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
    if (EXTI_GetITStatus(EXTI_Line15) != RESET) {   /* 右后轮 */
        g_encTotal[3]++;
        EXTI_ClearITPendingBit(EXTI_Line15);
    }
}

/**
  * @brief  串口1中断：蓝牙发来1个字节
  */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        BT_RxPush((uint8_t)USART_ReceiveData(USART1));   /* 存进缓冲区 */
    }
}
