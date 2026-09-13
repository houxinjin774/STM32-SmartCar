/**
  ******************************************************************************
  * @file    config.h
  * @brief   智能小车配置总表 —— 所有"接线"和"参数"都定义在这一个文件里
  *
  * ★ 接线如果和说明书不一样，只需要改这个文件，不用到处找代码 ★
  * ★ 改完参数记得重新编译下载 ★
  ******************************************************************************
  */
#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

/* ======================= 一、PWM 与速度参数 ======================= */

/* PWM周期：系统时钟72MHz / 7200 = 10kHz（电机调速信号的频率） */
#define PWM_PERIOD          7200

/* 占空比满量程 = PWM_PERIOD，速度范围 -PWM_MAX ~ +PWM_MAX */
#define PWM_MAX             6800

/* 电机能转起来的最小占空比（太小电机只会嗡嗡响转不动） */
#define MIN_RUN_PWM         900

/* 遥控/避障用的3个速度档位：1=慢 2=中 3=快 */
#define SPEED_LOW           3200
#define SPEED_MID           4500
#define SPEED_HIGH          5800

/* 循迹模式的基础速度和转弯力度（数值越大越快） */
#define TRACK_BASE          3800    /* 直行速度 */
#define TRACK_DIFF          2600    /* 小转弯时内外轮速度差 */

/* 避障模式的固定速度 */
#define AVOID_PWM           4500    /* 前进/后退速度 */
#define AVOID_TURN_PWM      3200    /* 原地转向速度 */

/* ======================= 二、TB6612 电机驱动 ======================= */
/* A通道 = 左侧电机（左前+左后并联），B通道 = 右侧电机（右前+右后并联） */

#define M_AIN1_PORT     GPIOB
#define M_AIN1_PIN      GPIO_Pin_0      /* PB0 -> TB6612 AIN1 */
#define M_AIN2_PORT     GPIOB
#define M_AIN2_PIN      GPIO_Pin_1      /* PB1 -> TB6612 AIN2 */
#define M_PWMA_PORT     GPIOA
#define M_PWMA_PIN      GPIO_Pin_6      /* PA6 -> TB6612 PWMA (TIM3通道1) */

#define M_BIN1_PORT     GPIOA
#define M_BIN1_PIN      GPIO_Pin_4      /* PA4 -> TB6612 BIN1 */
#define M_BIN2_PORT     GPIOA
#define M_BIN2_PIN      GPIO_Pin_5      /* PA5 -> TB6612 BIN2 */
#define M_PWMB_PORT     GPIOA
#define M_PWMB_PIN      GPIO_Pin_7      /* PA7 -> TB6612 PWMB (TIM3通道2) */

#define M_STBY_PORT     GPIOB
#define M_STBY_PIN      GPIO_Pin_5      /* PB5 -> TB6612 STBY（高电平=允许工作） */

/* ======================= 三、TCRT5000 循迹模块 x3 ======================= */
/* 装在车头底部，从左到右一字排开 */

#define TRACK_L_PORT    GPIOB
#define TRACK_L_PIN     GPIO_Pin_6      /* PB6 -> 循迹左 DO */
#define TRACK_M_PORT    GPIOB
#define TRACK_M_PIN     GPIO_Pin_7      /* PB7 -> 循迹中 DO */
#define TRACK_R_PORT    GPIOB
#define TRACK_R_PIN     GPIO_Pin_8      /* PB8 -> 循迹右 DO */

/* ★重要★：检测到黑线时模块输出什么电平？
 * 大部分TCRT5000循迹模块：检测到黑线输出高电平，这里填 1
 * 如果调试发现小车一开机就乱跑/报的黑线反了，把 1 改成 0 再试 */
#define LINE_LEVEL      1

/* ======================= 四、红外避障模块 x3 ======================= */
/* 装在车头，探头水平朝前：左、中、右 */
/* 注意：这些引脚是"5V耐压"引脚，模块输出的5V信号可以直接接 */

#define OBS_L_PORT      GPIOA
#define OBS_L_PIN       GPIO_Pin_8      /* PA8  -> 避障左 OUT */
#define OBS_M_PORT      GPIOA
#define OBS_M_PIN       GPIO_Pin_11     /* PA11 -> 避障中 OUT */
#define OBS_R_PORT      GPIOA
#define OBS_R_PIN       GPIO_Pin_12     /* PA12 -> 避障右 OUT */

/* ★重要★：检测到障碍物时模块输出什么电平？
 * 大部分红外避障模块（包括E18-D80NK）：检测到障碍输出低电平，这里填 0
 * 如果调试发现"没障碍也说有障碍"，把 0 改成 1 再试 */
#define OBST_LEVEL      0

/* ======================= 五、编码器测速模块 x4 ======================= */

#define ENC_FL_PORT     GPIOB
#define ENC_FL_PIN      GPIO_Pin_12     /* PB12 -> 左前轮编码器 DO */
#define ENC_FR_PORT     GPIOB
#define ENC_FR_PIN      GPIO_Pin_13     /* PB13 -> 右前轮编码器 DO */
#define ENC_RL_PORT     GPIOB
#define ENC_RL_PIN      GPIO_Pin_14     /* PB14 -> 左后轮编码器 DO */
#define ENC_RR_PORT     GPIOB
#define ENC_RR_PIN      GPIO_Pin_15     /* PB15 -> 右后轮编码器 DO */

/* 码盘一圈有多少个孔/格（常见11孔或20孔，数一下你的码盘，改成一样的） */
#define ENCODER_PPR     20

/* 车轮直径，单位毫米（常见的TT小车黄色车轮直径65mm） */
#define WHEEL_MM        65

/* ======================= 六、HC-SR04 超声波 ======================= */

#define U_TRIG_PORT     GPIOA
#define U_TRIG_PIN      GPIO_Pin_0      /* PA0 -> HC-SR04 Trig */
#define U_ECHO_PORT     GPIOB
#define U_ECHO_PIN      GPIO_Pin_9      /* PB9 -> HC-SR04 Echo（PB9可承受5V，可直连） */

/* 避障安全距离：前方小于这个厘米数就转向 */
#define AVOID_DIST_CM   25

/* ======================= 七、HC-05 蓝牙 ======================= */
/* 使用 USART1：PA9=单片机发(TX) -> HC-05 RXD；PA10=单片机收(RX) <- HC-05 TXD */
#define BT_BAUDRATE     9600            /* HC-05出厂默认9600，一般不用改 */

/* 蓝牙失联保护（只对遥控模式生效）：
 * 遥控时如果超过这么多毫秒没收到任何蓝牙数据，且车正在动，就自动停车。
 * 0 = 关闭（默认，发一次F车就一直走，再发S停）
 * 想开启防跑丢就改成 1500 （1.5秒收不到指令自动停） */
#define BT_FAILSAFE_MS  0

/* ======================= 八、全局变量声明 ======================= */
/* g_ms: 系统运行了多少毫秒（在SysTick中断里每毫秒+1） */
extern volatile uint32_t g_ms;
/* g_frontCm: 超声波测到的前方距离（厘米），999表示测不到 */
extern volatile uint16_t g_frontCm;

#endif /* __CONFIG_H */
