/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 四驱智能小车 —— 主程序
  *
  * 小车有5种模式，用手机蓝牙APP发指令切换：
  *   0 = 停止（待机，电机不动）
  *   1 = 蓝牙遥控（F前 B后 L左 R右 S停，+/-调速度）
  *   2 = 循迹     （沿黑线自动跑）
  *   3 = 避障     （自动躲避前方障碍物）
  *   4 = 传感器监视（打印所有传感器数值，调试用，车不动）
  *
  * 辅助指令（任何模式都有效）：
  *   D = 测前方距离   V = 查速度和里程   T = 打印一次传感器   H = 帮助
  ******************************************************************************
  */
#include "stm32f10x.h"
#include "config.h"
#include "motor.h"
#include "encoder.h"
#include "track.h"
#include "iravoid.h"
#include "ultrasonic.h"
#include "bluetooth.h"

/* ---------------- 模式定义 ---------------- */
#define MODE_STOP    0
#define MODE_RC      1
#define MODE_LINE    2
#define MODE_AVOID   3
#define MODE_SENSOR  4

/* ---------------- 全局变量 ---------------- */
volatile uint32_t g_ms = 0;          /* 毫秒计数器，SysTick中断里+1 */
volatile uint16_t g_frontCm = 999;   /* 超声波前方距离 */

static uint8_t s_mode = MODE_STOP;   /* 当前模式 */
static uint8_t s_gear = 2;           /* 速度档位 1慢 2中 3快 */

static const uint16_t s_gearPwm[4] = {0, SPEED_LOW, SPEED_MID, SPEED_HIGH};

/* ---------------- 板载LED ---------------- */
static void LED_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin   = GPIO_Pin_13;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);   /* 板载灯低电平亮，先灭掉 */
}

/* ---------------- 蓝牙指令处理 ---------------- */
/* 打印一次所有传感器的状态（T指令 / 传感器监视模式用） */
static void Sensor_Print(void)
{
    uint8_t l, m, r;

    Track_Read(&l, &m, &r);
    printf("XJ L%d M%d R%d | ", l, m, r);          /* 循迹: 1=黑线 */
    Ir_Read(&l, &m, &r);
    printf("ZA L%d M%d R%d | ", l, m, r);          /* 避障: 1=有障碍 */
    printf("US %d cm | ", g_frontCm);
    printf("ENC %d %d %d %d\r\n",
           (int)Encoder_GetTotal(0), (int)Encoder_GetTotal(1),
           (int)Encoder_GetTotal(2), (int)Encoder_GetTotal(3));
}

static void Cmd_Process(void)
{
    char c;
    int16_t sp = (int16_t)s_gearPwm[s_gear];
    int16_t half = (int16_t)(sp * 3 / 5);   /* 转向时慢的一侧给60%速度 */

    while (BT_GetChar(&c)) {
        switch (c) {

        /* ---- 模式切换 ---- */
        case '0':
            s_mode = MODE_STOP;
            Motor_Stop();
            printf("MODE: STOP\r\n");
            break;
        case '1':
            s_mode = MODE_RC;
            Motor_Stop();
            printf("MODE: RC (F/B/L/R/S)\r\n");
            break;
        case '2':
            s_mode = MODE_LINE;
            printf("MODE: LINE\r\n");
            break;
        case '3':
            s_mode = MODE_AVOID;
            printf("MODE: AVOID\r\n");
            break;
        case '4':
            s_mode = MODE_SENSOR;
            Motor_Stop();
            printf("MODE: SENSOR (300ms)\r\n");
            break;

        /* ---- 速度档位 ---- */
        case '+':
            if (s_gear < 3) s_gear++;
            printf("GEAR %d\r\n", s_gear);
            break;
        case '-':
            if (s_gear > 1) s_gear--;
            printf("GEAR %d\r\n", s_gear);
            break;

        /* ---- 查询 ---- */
        case 'D':
            g_frontCm = Ultra_GetDistanceCm();   /* 当场测一次再报 */
            printf("DIST %d cm\r\n", g_frontCm);
            break;
        case 'V':
            printf("SPD L%d R%d pps (%d %d cm/s) | DST %d mm\r\n",
                   (int)Encoder_GetSpeedPps(0), (int)Encoder_GetSpeedPps(1),
                   (int)Encoder_GetSpeedPps(0) * WHEEL_MM / ENCODER_PPR / 10,
                   (int)Encoder_GetSpeedPps(1) * WHEEL_MM / ENCODER_PPR / 10,
                   (int)Encoder_GetDistanceMm());
            break;
        case 'T':
            Sensor_Print();
            break;
        case 'H':
            BT_PrintHelp();
            break;

        /* ---- 遥控模式的方向键 ---- */
        case 'F':
        case 'B':
        case 'L':
        case 'R':
        case 'S':
            if (s_mode == MODE_RC) {
                switch (c) {
                case 'F': Motor_SetSpeed(sp, sp);     break;
                case 'B': Motor_SetSpeed(-sp, -sp);   break;
                case 'L': Motor_SetSpeed(-half, sp);  break;
                case 'R': Motor_SetSpeed(sp, -half);  break;
                case 'S': Motor_Stop();               break;
                }
            } else if (c == 'S') {
                Motor_Stop();            /* 任何模式下S都能急停，保险 */
            }
            break;

        default:
            /* 忽略换行回车等无用字符 */
            break;
        }
    }
}

/* ---------------- 主函数 ---------------- */
int main(void)
{
    uint32_t t10 = 0, t100 = 0, t150 = 0, t300 = 0, tLed = 0;

    /* 中断优先级分组（固定用第2组，一劳永逸） */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 1ms心跳：每毫秒触发一次SysTick中断，g_ms加1 */
    SysTick_Config(SystemCoreClock / 1000);

    LED_Init();
    Bluetooth_Init(BT_BAUDRATE);
    Motor_Init();
    Encoder_Init();
    Track_Init();
    IrAvoid_Init();
    Ultra_Init();

    printf("\r\nSmartCar Ready!\r\n");
    BT_PrintHelp();

    while (1) {
        /* 每10ms：处理蓝牙指令 + 执行当前模式的任务 */
        if ((g_ms - t10) >= 10) {
            t10 = g_ms;
            Cmd_Process();

            /* 蓝牙失联保护（config.h里BT_FAILSAFE_MS>0时才生效）：
             * 遥控时太久没收到指令且车在动 -> 自动停车，防止车跑丢 */
            if ((s_mode == MODE_RC) && (BT_FAILSAFE_MS > 0)) {
                if (((g_ms - BT_GetLastRxMs()) > BT_FAILSAFE_MS) &&
                    ((Motor_GetDir(0) != 0) || (Motor_GetDir(1) != 0))) {
                    Motor_Stop();
                    printf("BT TIMEOUT, STOP\r\n");
                }
            }

            if (s_mode == MODE_LINE) {
                LineFollow_Task();
            } else if (s_mode == MODE_AVOID) {
                Avoid_Task();
            }
        }

        /* 避障模式下每150ms测一次超声波距离 */
        if ((s_mode == MODE_AVOID) && ((g_ms - t150) >= 150)) {
            t150 = g_ms;
            g_frontCm = Ultra_GetDistanceCm();
        }

        /* 每100ms：刷新编码器速度 */
        if ((g_ms - t100) >= 100) {
            t100 = g_ms;
            Encoder_Task();
        }

        /* 传感器监视模式：每300ms测距+打印一次 */
        if ((s_mode == MODE_SENSOR) && ((g_ms - t300) >= 300)) {
            t300 = g_ms;
            g_frontCm = Ultra_GetDistanceCm();
            Sensor_Print();
        }

        /* 板载LED每500ms翻转一次 = 心跳，说明程序没死机 */
        if ((g_ms - tLed) >= 500) {
            tLed = g_ms;
            GPIOC->ODR ^= GPIO_Pin_13;
        }
    }
}
