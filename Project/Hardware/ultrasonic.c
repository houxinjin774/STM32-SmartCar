/**
  ******************************************************************************
  * @file    ultrasonic.c
  * @brief   HC-SR04 超声波测距
  *
  * 原理（小白版）：
  *   1. 单片机给 Trig 脚一个 10微秒以上的高电平 -> 模块发射8个超声波脉冲
  *   2. 声波碰到物体反射回来，模块把 Echo 脚拉高，高电平持续的时间
  *      = 声波来回飞的时间
  *   3. 距离(厘米) = 时间(微秒) / 58   （声速约340米/秒，除以2是因为来回）
  *
  * 实现：Echo接在PB9（双边沿外部中断）。上升沿时把"微秒表"(TIM2)清零，
  *       下降沿时读出计数值，就是高电平持续了多少微秒。
  ******************************************************************************
  */
#include "ultrasonic.h"
#include "config.h"
#include "stm32f10x.h"

static volatile uint32_t s_echoUs   = 0;   /* Echo高电平宽度（微秒） */
static volatile uint8_t  s_echoDone = 0;   /* 1=这次测量完成了 */

/* 粗略的微秒延时（触发脉冲用，不需要精确） */
static void DelayUs(uint16_t us)
{
    volatile uint16_t n;
    while (us--) {
        n = 8;                       /* 72MHz下大约1微秒，不精确没关系 */
        while (n--) {
            __NOP();
        }
    }
}

void Ultra_Init(void)
{
    GPIO_InitTypeDef      gpio;
    EXTI_InitTypeDef      exti;
    NVIC_InitTypeDef      nvic;
    TIM_TimeBaseInitTypeDef tim;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* PA0 -> Trig：推挽输出，默认低 */
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin   = U_TRIG_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(U_TRIG_PORT, &gpio);
    GPIO_ResetBits(U_TRIG_PORT, U_TRIG_PIN);

    /* PB9 <- Echo：下拉输入 + 双边沿外部中断 */
    gpio.GPIO_Pin  = U_ECHO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(U_ECHO_PORT, &gpio);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9);

    EXTI_StructInit(&exti);
    exti.EXTI_Line    = EXTI_Line9;
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;   /* 上升沿和下降沿都要 */
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    nvic.NVIC_IRQChannel                   = EXTI9_5_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1; /* 超声波优先级最高（时间敏感） */
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    /* TIM2当"微秒表"用：72MHz/72 = 1MHz，即1微秒计1个数，不停地数 */
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Prescaler     = 72 - 1;
    tim.TIM_Period        = 0xFFFF;
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
}

/* 外部中断服务程序会调用这个函数（在stm32f10x_it.c里） */
void Ultra_EchoISR(void)
{
    if (GPIO_ReadInputDataBit(U_ECHO_PORT, U_ECHO_PIN) != 0) {
        /* 上升沿：Echo变高了，声波刚出发 -> 微秒表清零开始计时 */
        TIM_SetCounter(TIM2, 0);
    } else {
        /* 下降沿：Echo变低了，声波回来了 -> 读出经过了多少微秒 */
        s_echoUs   = TIM_GetCounter(TIM2);
        s_echoDone = 1;
    }
}

/* 测一次距离，单位厘米。999 = 没测到（超时或没有障碍物） */
uint16_t Ultra_GetDistanceCm(void)
{
    uint32_t t0 = g_ms;

    /* 1. 发一个约15微秒的触发脉冲 */
    GPIO_SetBits(U_TRIG_PORT, U_TRIG_PIN);
    DelayUs(15);
    GPIO_ResetBits(U_TRIG_PORT, U_TRIG_PIN);

    /* 2. 等Echo结束（最多等30毫秒，正常最远4米也就23毫秒） */
    s_echoDone = 0;
    while (!s_echoDone) {
        if ((g_ms - t0) > 30) {
            return 999;               /* 超时：前方没有反射物或模块没接好 */
        }
    }

    /* 3. 微秒数 / 58 = 厘米数 */
    return (uint16_t)(s_echoUs / 58);
}
