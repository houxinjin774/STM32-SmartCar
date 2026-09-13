/**
  ******************************************************************************
  * @file    encoder.c
  * @brief   编码器测速模块
  *
  * 原理（小白版）：
  *   车轮上装了一个带孔的码盘，码盘旁边有个光电传感器。
  *   轮子转动时，码盘的孔不断挡住/放开光线，传感器就输出一串脉冲。
  *   数脉冲的个数 = 轮子转了多少圈 = 走了多少距离；
  *   数每秒多少个脉冲 = 车速。
  *
  * 实现：每个编码器接一个外部中断引脚，脉冲的上升沿触发中断，计数+1。
  ******************************************************************************
  */
#include "encoder.h"
#include "config.h"
#include "motor.h"

/* 4个编码器的累计脉冲数（中断里+1，所以必须加volatile） */
volatile uint32_t g_encTotal[4] = {0};

/* 上次读取时的值，用来算"这100ms里多了多少个脉冲" */
static uint32_t s_last[4] = {0};
static int32_t  s_pps[4]  = {0};    /* 每个编码器的速度：脉冲/秒 */

/* 左/右侧合成的速度（脉冲/秒），供主程序显示 */
static int32_t s_speedL = 0;
static int32_t s_speedR = 0;
/* 总里程（毫米） */
static int32_t s_distMm = 0;

/* 4个编码器对应PB12~PB15 */
static const uint16_t s_encPin[4] = {GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15};

void Encoder_Init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;
    uint8_t i;

    /* 外部中断要用到AFIO这个"开关矩阵"，时钟必须开 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* PB12~PB15 配成上拉输入 */
    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    for (i = 0; i < 4; i++) {
        gpio.GPIO_Pin = s_encPin[i];
        GPIO_Init(GPIOB, &gpio);
        /* 把PB12~15接到外部中断线12~15上 */
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, (uint8_t)(GPIO_PinSource12 + i));
    }

    /* 配置4条中断线：上升沿触发（脉冲从0变1的瞬间） */
    EXTI_StructInit(&exti);
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising;
    exti.EXTI_LineCmd = ENABLE;
    for (i = 0; i < 4; i++) {
        exti.EXTI_Line = EXTI_Line12 + i;
        EXTI_Init(&exti);
    }

    /* 中断优先级：比超声波低，比串口高 */
    nvic.NVIC_IRQChannel                   = EXTI15_10_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

/* 每100ms被主程序调用一次 */
void Encoder_Task(void)
{
    uint8_t i;
    int32_t delta[4];

    for (i = 0; i < 4; i++) {
        uint32_t now = g_encTotal[i];           /* 32位读取是原子的，不怕中断打断 */
        delta[i]  = (int32_t)(now - s_last[i]);
        s_last[i] = now;
        s_pps[i]  = delta[i] * 10;              /* 100ms里的脉冲数 x10 = 每秒脉冲数 */
    }

    /* 左侧速度 = (左前+左后)/2，右侧同理 */
    s_speedL = (s_pps[0] + s_pps[2]) / 2;
    s_speedR = (s_pps[1] + s_pps[3]) / 2;

    /* 里程累加：用"当前给电机的方向"当符号（编码器本身分不清正反） */
    {
        int32_t vL = s_speedL * Motor_GetDir(0);
        int32_t vR = s_speedR * Motor_GetDir(1);
        /* 速度(脉冲/秒) x 每脉冲前进的毫米数 x 0.1秒 = 这100ms走的毫米数 */
        s_distMm += (int32_t)(((vL + vR) / 2) * WHEEL_MM / ENCODER_PPR / 10);
    }
}

uint32_t Encoder_GetTotal(uint8_t idx)
{
    if (idx > 3) return 0;
    return g_encTotal[idx];
}

int32_t Encoder_GetSpeedPps(uint8_t side)
{
    return (side == 0) ? s_speedL : s_speedR;
}

int32_t Encoder_GetDistanceMm(void)
{
    return s_distMm;
}
