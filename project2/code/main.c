/**
 * Pressure Injection System - Main Application
 *
 * README-compliant medical pressure injection device firmware.
 * Hardware: STM32F103C8, OLED SSD1306 (I2C PB12/PB13),
 *           ULN2003A stepper motor (PA10-PA13),
 *           Pressure sensor ADC PA0, Battery ADC PA1,
 *           Keys: KEY1=PB0, KEY2=PA5, KEY3=PA6,
 *           EEPROM 24C02C (I2C PA8/PA9),
 *           Buzzer PB3
 *
 * State machine: IDLE -> SETUP -> PURGE -> INJECT -> COMPRESS -> RETRACT -> IDLE
 * ERROR state as safety fallback from any state.
 */

#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "key.h"
#include "led.h"
#include "eeprom.h"
#include "stdio.h"
#include "string.h"


/*===========================================================================
 * CONSTANTS
 *===========================================================================*/

/* ADC channels */
#define ADC_CH_PRESSURE     ADC_Channel_0   /* PA0 - pressure sensor */
#define ADC_CH_BATTERY      ADC_Channel_1   /* PA1 - battery voltage */

/* ADC scaling (12-bit, Vref=3.3V) */
#define ADC_VREF            3.3f
#define ADC_MAX             4095.0f
#define ADC_TO_VOLT(adc)    ((float)(adc) * ADC_VREF / ADC_MAX)

/* Pressure sensor: 0-3.3V maps to full-scale range (configurable, stored in EEPROM) */
/* Battery: 0-3.3V maps to 0-100%, assuming voltage divider */
#define BATTERY_FULLSCALE   100.0f

/* Motor calibration */
#define MOTOR_STEPS_PER_ML  5000    /* steps per mL (tune to match your syringe) */
#define MOTOR_MAX_STEPS     3500    /* 0.7 mL (5000*0.7) */
#define MOTOR_SPEED_MIN     300     /* Hz - slow speed */
#define MOTOR_SPEED_MAX     1200    /* Hz - fast speed */
#define MOTOR_RAMP_STEPS    500     /* steps for soft start/stop ramp */

/* Timing (ms) */
#define DISPLAY_INTERVAL    100     /* 10 Hz refresh */
#define KEY_SCAN_INTERVAL   16      /* 16 ms key scan period */
#define PRESSURE_SAMPLE_S   5       /* 5 seconds per pressure history point */

/* Compression */
#define COMPRESS_TIME_SEC   600     /* 10 minutes */
#define MAX_PRESSURE_POINTS 120     /* 600s / 5s = 120 points */

/* Safety limits */
#define MAX_VOL_ML          7.0f    /* max volume */
#define BATTERY_LOW_PCT     10      /* low battery threshold % */

/*===========================================================================
 * TYPE DEFINITIONS
 *===========================================================================*/

typedef enum {
    STATE_IDLE = 0,
    STATE_SETUP,
    STATE_PURGE,
    STATE_INJECT,
    STATE_COMPRESS,
    STATE_RETRACT,
    STATE_ERROR
} State_t;

typedef enum {
    PAGE_STANDBY = 0,
    PAGE_MONITOR,
    PAGE_RESULT
} Page_t;

typedef struct {
    float pressure;               /* current pressure (kPa) */
    float battery;                /* battery level (%) */
    float volume_live;            /* live volume during injection (mL) */
    float volume_final;           /* locked final volume after injection stops */
    float max_pressure;           /* max pressure during injection */
    float pressure_sum;           /* for averaging */
    uint16_t pressure_count;      /* for averaging */
    uint32_t compress_start;      /* compression start tick (seconds) */
    uint32_t compress_remain;     /* remaining seconds */
    uint16_t pressure_history[MAX_PRESSURE_POINTS];
    uint8_t  history_idx;
    uint32_t last_history_tick;   /* seconds */
} RuntimeData_t;

/*===========================================================================
 * GLOBAL VARIABLES
 *===========================================================================*/

static State_t       g_state = STATE_IDLE;
static Page_t        g_page  = PAGE_STANDBY;
static RuntimeData_t g_rt;

/* System tick (ms) from TIM3 */
static volatile uint32_t g_sys_tick = 0;

/* Motor state */
static volatile uint8_t  g_motor_step_idx  = 0;
static volatile uint16_t g_motor_step_cnt  = 0;
static volatile uint16_t g_motor_step_total = 0;
static volatile uint8_t  g_motor_dir       = 0;  /* 0=forward, 1=reverse */
static volatile uint8_t  g_motor_running   = 0;
static volatile uint8_t  g_motor_ramping   = 0;
static volatile uint16_t g_motor_period    = 0;  /* TIM2 period for step rate */

/* Safety flags */
static volatile uint8_t g_stop_flag   = 0;
static volatile uint8_t g_error_code  = 0;

/* ADC zero calibration */
static uint16_t g_adc_pressure_zero = 0;

/* Display state cache */
static uint8_t g_last_page = 0xFF;
static uint8_t g_blink_tick = 0;
static uint8_t g_blink_state = 0;

/*===========================================================================
 * STEPPER MOTOR STEP SEQUENCE (half-step, 8 steps)
 * PA10=IN1, PA11=IN2, PA12=IN3, PA13=IN4
 *===========================================================================*/

static const uint16_t g_step_seq[8] = {
    (GPIO_Pin_10 | GPIO_Pin_11),                        /* AB   */
    (GPIO_Pin_11),                                      /*  B   */
    (GPIO_Pin_11 | GPIO_Pin_12),                        /*  BC  */
    (GPIO_Pin_12),                                      /*   C  */
    (GPIO_Pin_12 | GPIO_Pin_13),                        /*   CD */
    (GPIO_Pin_13),                                      /*    D */
    (GPIO_Pin_10 | GPIO_Pin_13),                        /* A  D */
    (GPIO_Pin_10),                                      /* A    */
};

static const uint16_t g_step_seq_rev[8] = {
    (GPIO_Pin_10),                                      /* A    */
    (GPIO_Pin_10 | GPIO_Pin_13),                        /* A  D */
    (GPIO_Pin_13),                                      /*    D */
    (GPIO_Pin_12 | GPIO_Pin_13),                        /*   CD */
    (GPIO_Pin_12),                                      /*   C  */
    (GPIO_Pin_11 | GPIO_Pin_12),                        /*  BC  */
    (GPIO_Pin_11),                                      /*  B   */
    (GPIO_Pin_10 | GPIO_Pin_11),                        /* AB   */
};

/*===========================================================================
 * FORWARD DECLARATIONS
 *===========================================================================*/

static void State_Idle(void);
static void State_Setup(void);
static void State_Purge(void);
static void State_Inject(void);
static void State_Compress(void);
static void State_Retract(void);
static void State_Error(void);
static void Motor_Start(uint8_t dir, uint16_t steps, uint16_t speed_hz);
static void Motor_Stop(void);
static void Motor_EmergencyStop(void);
static void Sensor_Read(void);
static void Alarm_Check(void);
static void OLED_ShowStandby(void);
static void OLED_ShowMonitor(void);
static void OLED_ShowResult(void);
static void OLED_Update(void);
static void Key_Process(void);
static void EnterError(uint8_t code);

/*===========================================================================
 * INITIALIZATION
 *===========================================================================*/

static void GPIO_Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA10-PA13: motor phase outputs, push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* All phases off */
    GPIO_ResetBits(GPIOA, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13);
}

static void ADC_Poll_Init(void)
{
    ADC_InitTypeDef  ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* PA0, PA1 = analog input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

static uint16_t ADC_ReadChannel(u8 ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

static uint16_t ADC_ReadAvg(u8 ch, u8 times)
{
    uint32_t sum = 0;
    u8 i;
    for (i = 0; i < times; i++)
    {
        sum += ADC_ReadChannel(ch);
    }
    return (uint16_t)(sum / times);
}

static void TIM2_Motor_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* Default period: 1ms step (motor stopped initially) */
    TIM_TimeBaseStructure.TIM_Period        = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler     = 72 - 1;   /* 1 MHz (72MHz/72) */
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel    = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* TIM2 OFF initially - started when motor runs */
    TIM_Cmd(TIM2, DISABLE);
}

static void TIM3_KeyScan_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 16ms period: 72MHz / 72 = 1MHz, 16000-1 = 16ms */
    TIM_TimeBaseStructure.TIM_Period        = 16000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler     = 72 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel    = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

/*===========================================================================
 * MOTOR CONTROL
 *===========================================================================*/

static void Motor_SetPhase(uint16_t phase_mask)
{
    uint16_t odr;

    odr  = GPIOA->ODR & ~(GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13);
    odr |= phase_mask;
    GPIOA->ODR = odr;
}

static void Motor_Start(uint8_t dir, uint16_t steps, uint16_t speed_hz)
{
    uint32_t period;

    if (steps == 0) return;

    period = 1000000UL / speed_hz;  /* period in us (timer runs at 1MHz) */

    if (period < 10)  period = 10;
    if (period > 65535) period = 65535;

    g_motor_step_idx   = 0;
    g_motor_step_cnt   = 0;
    g_motor_step_total = steps;
    g_motor_dir        = dir;
    g_motor_running    = 1;
    g_motor_ramping    = 1;
    g_motor_period     = (uint16_t)period;

    /* Set initial phase */
    if (dir == 0)
        Motor_SetPhase(g_step_seq[0]);
    else
        Motor_SetPhase(g_step_seq_rev[0]);

    /* Configure TIM2 for the step rate */
    TIM_SetAutoreload(TIM2, period - 1);
    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);
}

static void Motor_Stop(void)
{
    g_motor_running = 0;
    TIM_Cmd(TIM2, DISABLE);
    /* All phases off (low power, no holding torque) */
    GPIO_ResetBits(GPIOA, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13);
}

static void Motor_EmergencyStop(void)
{
    g_motor_running = 0;
    TIM_Cmd(TIM2, DISABLE);
    GPIO_ResetBits(GPIOA, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13);
    g_stop_flag = 1;
}

static void Motor_SetSpeed(uint16_t speed_hz)
{
    uint32_t period = 1000000UL / speed_hz;
    if (period < 10)  period = 10;
    if (period > 65535) period = 65535;
    g_motor_period = (uint16_t)period;
    TIM_SetAutoreload(TIM2, period - 1);
}

/*===========================================================================
 * SENSOR READING
 *===========================================================================*/

static void Sensor_Read(void)
{
    float avg_volt;
    uint16_t adc_val;
    int32_t adc_corrected;

    /* 8-point average for pressure (PA0), with zero calibration */
    adc_val = ADC_ReadAvg(ADC_CH_PRESSURE, 8);
    adc_corrected = (int32_t)adc_val - (int32_t)g_adc_pressure_zero;
    if (adc_corrected < 0) adc_corrected = 0;
    g_rt.pressure = (float)adc_corrected / ADC_MAX
                    * (float)g_ee_pressure_range / 10.0f;

    /* 8-point average for battery (PA1) */
    adc_val = ADC_ReadAvg(ADC_CH_BATTERY, 8);
    avg_volt = ADC_TO_VOLT(adc_val);
    g_rt.battery = avg_volt / ADC_VREF * BATTERY_FULLSCALE;
    if (g_rt.battery > 100.0f) g_rt.battery = 100.0f;
    if (g_rt.battery < 0.0f) g_rt.battery = 0.0f;

    /* Track max pressure */
    if (g_rt.pressure > g_rt.max_pressure)
        g_rt.max_pressure = g_rt.pressure;

    /* Accumulate for average */
    g_rt.pressure_sum += g_rt.pressure;
    g_rt.pressure_count++;
}

/*===========================================================================
 * ALARM CHECK
 *===========================================================================*/

static void Alarm_Check(void)
{
    uint8_t fault = 0;
    uint8_t code  = 0;
    float overpressure_limit;
    float range_kpa;

    /* Only check alarms when not already in error (prevents re-entry) */
    if (g_state == STATE_ERROR) return;

    range_kpa = (float)g_ee_pressure_range / 10.0f;
    overpressure_limit = range_kpa * 0.98f;  /* 98% of full-scale */

    /* Over-pressure: pressure exceeds 98% of sensor range */
    if (g_rt.pressure > overpressure_limit && range_kpa > 0.1f)
    {
        fault = 1;
        code  = 1;
    }

    /* Low battery: only when battery ADC is actually connected (voltage > 0.5V) */
    if (g_rt.battery < BATTERY_LOW_PCT && g_rt.battery > 0.0f
        && ADC_ReadAvg(ADC_CH_BATTERY, 4) > 620)  /* 0.5V min = 620 counts */
    {
        fault = 1;
        code  = 2;
    }

    if (fault)
    {
        EnterError(code);
    }
}

/*===========================================================================
 * ERROR HANDLING
 *===========================================================================*/

static void EnterError(uint8_t code)
{
    g_error_code = code;
    g_state = STATE_ERROR;
    Motor_EmergencyStop();
    Alarm_On();
    /* Save error log to EEPROM at address 0xF0 */
    EEPROM_WriteByte(0xF0, code);
}

/*===========================================================================
 * OLED DISPLAY FUNCTIONS
 *===========================================================================*/

static void OLED_ShowStandby(void)
{
    char buf[20];

    if (g_state == STATE_SETUP)
    {
        /* ─── SETUP MODE ─── */
        OLED_ShowString(0, 0, "BAT:", 16);
        sprintf(buf, "%3.0f%%", (double)g_rt.battery);
        OLED_ShowString(4 * 8, 0, (u8 *)buf, 16);
        OLED_ShowString(10 * 8, 0, "SETUP", 16);

        /* Target pressure (editable) */
        sprintf(buf, "Pset: %3.1f kPa", (double)g_ee_p_set / 10.0f);
        OLED_ShowString(0, 2, (u8 *)buf, 16);

        OLED_ShowString(0, 4, "KEY2/+  KEY3/-", 16);
        OLED_ShowString(0, 6, "KEY1:Start", 16);
    }
    else
    {
        /* ─── IDLE MODE ─── */
        OLED_ShowString(0, 0, "BAT:", 16);
        sprintf(buf, "%3.0f%%", (double)g_rt.battery);
        OLED_ShowString(4 * 8, 0, (u8 *)buf, 16);
        OLED_ShowString(10 * 8, 0, "IDLE", 16);

        sprintf(buf, "v%d.%d %s",
                g_ee_version_major, g_ee_version_minor,
                g_ee_language ? "EN" : "CN");
        OLED_ShowString(0, 2, (u8 *)buf, 16);

        sprintf(buf, "Pset: %3.1f kPa", (double)g_ee_p_set / 10.0f);
        OLED_ShowString(0, 4, (u8 *)buf, 16);

        OLED_ShowString(0, 6, "KEY1:Start", 16);
    }
}

static void OLED_ShowMonitor(void)
{
    char buf[20];

    /* Line 0-1: Pressure (large) */
    OLED_ShowString(0, 0, "P:", 16);
    sprintf(buf, "%5.1f kPa", (double)g_rt.pressure);
    OLED_ShowString(2 * 8, 0, (u8 *)buf, 16);

    /* Line 2-3: Volume (live during injection, final after stop) */
    OLED_ShowString(0, 2, "Vol:", 16);
    if (g_state == STATE_INJECT)
        sprintf(buf, "%4.2f mL", (double)g_rt.volume_live);
    else
        sprintf(buf, "%4.2f mL", (double)g_rt.volume_final);
    OLED_ShowString(4 * 8, 2, (u8 *)buf, 16);

    /* Line 4-5: Remaining time */
    OLED_ShowString(0, 4, "T:", 16);
    if (g_state == STATE_COMPRESS)
    {
        uint32_t rem = g_rt.compress_remain;
        sprintf(buf, "%02lu:%02lu", rem / 60, rem % 60);
    }
    else
    {
        sprintf(buf, "--:--");
    }
    OLED_ShowString(2 * 8, 4, (u8 *)buf, 16);

    /* State indicator */
    switch (g_state)
    {
    case STATE_PURGE:     OLED_ShowString(9 * 8, 4, "PURGE", 16); break;
    case STATE_INJECT:    OLED_ShowString(9 * 8, 4, "INJECT", 16); break;
    case STATE_COMPRESS:  OLED_ShowString(9 * 8, 4, "HOLD", 16); break;
    case STATE_RETRACT:   OLED_ShowString(9 * 8, 4, "RETRACT", 16); break;
    default: break;
    }

    /* Line 6-7: Status bar */
    if (g_stop_flag)
    {
        if (g_blink_state)
            OLED_ShowString(0, 6, "STOP", 16);
        else
            OLED_ShowString(0, 6, "    ", 16);
    }
    else
    {
        OLED_ShowString(0, 6, "RUN", 16);
    }
}

static void OLED_ShowResult(void)
{
    char buf[20];
    float avg_pressure;

    avg_pressure = (g_rt.pressure_count > 0)
        ? g_rt.pressure_sum / g_rt.pressure_count : 0.0f;

    /* Line 0: Title */
    OLED_ShowString(0, 0, "-- Results --", 16);

    /* Line 2: Max pressure */
    sprintf(buf, "Pmax:%5.1f kPa", (double)g_rt.max_pressure);
    OLED_ShowString(0, 2, (u8 *)buf, 16);

    /* Line 4: Average pressure */
    sprintf(buf, "Pavg:%5.1f kPa", (double)avg_pressure);
    OLED_ShowString(0, 4, (u8 *)buf, 16);

    /* Line 6: Total volume + prompt */
    sprintf(buf, "Vol:%4.2f mL", (double)g_rt.volume_final);
    OLED_ShowString(0, 6, (u8 *)buf, 16);
}

static void OLED_Update(void)
{
    if (g_page != g_last_page)
    {
        OLED_Clear();
        g_last_page = g_page;
    }

    switch (g_page)
    {
    case PAGE_STANDBY: OLED_ShowStandby(); break;
    case PAGE_MONITOR: OLED_ShowMonitor(); break;
    case PAGE_RESULT:  OLED_ShowResult();  break;
    default: g_page = PAGE_STANDBY; break;
    }
}

/*===========================================================================
 * KEY PROCESSING
 *===========================================================================*/

static void Key_Process(void)
{
    u8 key = KEY_Scan(0);
    if (key == 0) return;

    switch (g_state)
    {
    case STATE_IDLE:
        if (key == KEY1_PRES)
        {
            /* Go to setup */
            g_state = STATE_SETUP;
            g_page  = PAGE_STANDBY;
            OLED_Update();
        }
        break;

    case STATE_SETUP:
        if (key == KEY1_PRES)
        {
            /* Confirm and start */
            Buzzer_Beep(1, 50);
            g_state = STATE_PURGE;
            g_page  = PAGE_MONITOR;
            Motor_Start(0, MOTOR_STEPS_PER_ML / 10, MOTOR_SPEED_MIN);
            OLED_Update();
        }
        else if (key == KEY2_PRES)
        {
            /* Increase p_set */
            g_ee_p_set += 5;
            if (g_ee_p_set > 5000) g_ee_p_set = 5000;
            EEPROM_SaveConfig();
            OLED_Update();
        }
        else if (key == KEY3_PRES)
        {
            /* Decrease p_set */
            if (g_ee_p_set >= 10) g_ee_p_set -= 5;
            if (g_ee_p_set < 10) g_ee_p_set = 10;
            EEPROM_SaveConfig();
            OLED_Update();
        }
        break;

    case STATE_INJECT:
        /* In INJECT state, KEY1 can trigger emergency stop */
        if (key == KEY1_PRES)
        {
            g_stop_flag = 1;
            Motor_Stop();
        }
        break;

    case STATE_ERROR:
        if (key == KEY1_PRES)
        {
            /* Clear error and return to idle */
            Alarm_Off();
            g_error_code = 0;
            g_state = STATE_IDLE;
            g_page  = PAGE_STANDBY;
            OLED_Update();
        }
        break;

    case STATE_COMPRESS:
        /* Manual abort compression */
        if (key == KEY1_PRES)
        {
            g_state = STATE_RETRACT;
            /* Start motor reverse to retract */
            Motor_Start(1, MOTOR_MAX_STEPS, MOTOR_SPEED_MAX);
            OLED_Update();
        }
        break;

    case STATE_RETRACT:
        if (key == KEY1_PRES)
        {
            if (g_motor_running)
            {
                /* Skip retraction → stop motor, jump to results */
                Motor_Stop();
            }
            else
            {
                /* At results page → return to idle */
                g_state = STATE_IDLE;
                g_page  = PAGE_STANDBY;
                OLED_Update();
            }
        }
        break;

    default:
        break;
    }
}

/*===========================================================================
 * STATE MACHINE HANDLERS
 *===========================================================================*/

static void State_Idle(void)
{
    g_page = PAGE_STANDBY;
    /* Wait for key in Key_Process to transition to SETUP */
}

static void State_Setup(void)
{
    g_page = PAGE_STANDBY;
    /* KEY1 cycles params, KEY2/KEY3 adjust, KEY1 on last param confirms */
}

static void State_Purge(void)
{
    g_page = PAGE_MONITOR;

    /* Purge: run motor forward a small amount, then auto-transition */
    if (!g_motor_running)
    {
        /* Purge complete, move to INJECT */
        g_state = STATE_INJECT;
        /* Reset pressure statistics for this injection run */
        g_rt.max_pressure   = 0.0f;
        g_rt.pressure_sum   = 0.0f;
        g_rt.pressure_count = 0;
        /* Start full injection */
        Motor_Start(0, MOTOR_MAX_STEPS, MOTOR_SPEED_MIN);
    }
}

static void State_Inject(void)
{
    float p_set_kpa;
    float max_vol_ml;

    g_page    = PAGE_MONITOR;
    p_set_kpa = (float)g_ee_p_set / 10.0f;
    max_vol_ml = (float)g_ee_max_vol / 10.0f;

    /* Live volume from step count (for real-time display during injection) */
    if (g_motor_step_total > 0)
    {
        g_rt.volume_live = (float)g_motor_step_cnt / (float)MOTOR_STEPS_PER_ML;
    }

    /* Stop conditions */
    if (g_rt.pressure >= p_set_kpa ||
        g_rt.volume_live >= max_vol_ml ||
        !g_motor_running ||
        g_stop_flag)
    {
        /* Lock final volume — this value is preserved forever after injection ends */
        g_rt.volume_final = g_rt.volume_live;
        g_stop_flag = 1;
        Motor_Stop();

        /* Transition to compression */
        g_state = STATE_COMPRESS;
        g_rt.compress_start  = g_sys_tick / 1000;  /* seconds */
        g_rt.compress_remain = COMPRESS_TIME_SEC;
        g_rt.history_idx     = 0;
        g_rt.last_history_tick = g_rt.compress_start;

        Buzzer_Beep(2, 100);
        OLED_Update();
    }

    /* Soft start ramp */
    if (g_motor_ramping && g_motor_running)
    {
        if (g_motor_step_cnt < MOTOR_RAMP_STEPS)
        {
            /* Ramp up: linear from MIN to MAX */
            uint16_t speed = MOTOR_SPEED_MIN +
                (uint16_t)((uint32_t)(MOTOR_SPEED_MAX - MOTOR_SPEED_MIN)
                           * g_motor_step_cnt / MOTOR_RAMP_STEPS);
            Motor_SetSpeed(speed);
        }
        else
        {
            g_motor_ramping = 0;
            Motor_SetSpeed(MOTOR_SPEED_MAX);
        }
    }
}

static void State_Compress(void)
{
    uint32_t now_sec;

    g_page  = PAGE_MONITOR;
    now_sec = g_sys_tick / 1000;

    /* Update remaining time */
    if (now_sec >= g_rt.compress_start)
    {
        uint32_t elapsed = now_sec - g_rt.compress_start;
        if (elapsed >= COMPRESS_TIME_SEC)
        {
            g_rt.compress_remain = 0;
        }
        else
        {
            g_rt.compress_remain = COMPRESS_TIME_SEC - elapsed;
        }
    }

    /* Sample pressure every 5 seconds for history */
    if (now_sec - g_rt.last_history_tick >= PRESSURE_SAMPLE_S)
    {
        g_rt.last_history_tick = now_sec;
        if (g_rt.history_idx < MAX_PRESSURE_POINTS)
        {
            uint16_t p_x10 = (uint16_t)(g_rt.pressure * 10.0f);
            g_rt.pressure_history[g_rt.history_idx] = p_x10;
            /* Save to EEPROM */
            EEPROM_SavePressurePoint(g_rt.history_idx, p_x10);
            g_rt.history_idx++;
        }
    }

    /* Compression complete? */
    if (g_rt.compress_remain == 0)
    {
        g_state = STATE_RETRACT;
        /* Reverse motor to retract piston */
        Motor_Start(1, MOTOR_MAX_STEPS, MOTOR_SPEED_MAX);
        Buzzer_Beep(3, 200);
        OLED_Update();
    }
}

static void State_Retract(void)
{
    static uint8_t beep_done = 0;

    if (g_motor_running)
    {
        g_page = PAGE_MONITOR;
        beep_done = 0;
    }
    else
    {
        /* Retraction complete, show results */
        g_page = PAGE_RESULT;
        if (!beep_done)
        {
            Buzzer_Beep(1, 500);
            OLED_Update();
            beep_done = 1;
        }
        /* Stay in STATE_RETRACT; KEY1 in Key_Process resets to IDLE */
    }
}

static void State_Error(void)
{
    static uint8_t last_code = 0xFF;
    char buf[20];

    /* Only redraw when error code changes (prevents flicker) */
    if (g_error_code != last_code)
    {
        last_code = g_error_code;
        OLED_Clear();
    }

    OLED_ShowString(0, 0, "!! ERROR !!", 16);
    sprintf(buf, "Code: %d", g_error_code);
    OLED_ShowString(0, 2, (u8 *)buf, 16);
    OLED_ShowString(0, 4, "Motor Locked", 16);
    OLED_ShowString(0, 6, "KEY1:Reset", 16);
}

/*===========================================================================
 * INTERRUPT HANDLERS
 *===========================================================================*/

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        if (!g_motor_running) return;

        /* Advance one step */
        g_motor_step_cnt++;

        if (g_motor_step_cnt >= g_motor_step_total)
        {
            /* Motion complete */
            g_motor_running = 0;
            TIM_Cmd(TIM2, DISABLE);
            GPIO_ResetBits(GPIOA,
                GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13);
            return;
        }

        /* Advance phase */
        g_motor_step_idx++;
        if (g_motor_step_idx >= 8)
            g_motor_step_idx = 0;

        if (g_motor_dir == 0)
            Motor_SetPhase(g_step_seq[g_motor_step_idx]);
        else
            Motor_SetPhase(g_step_seq_rev[g_motor_step_idx]);
    }
}

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        /* 16ms tick for system time */
        g_sys_tick += 16;
    }
}

/*===========================================================================
 * MAIN FUNCTION
 *===========================================================================*/

int main(void)
{
    uint32_t last_display_tick = 0;

    /* System init */
    delay_init();

    /* OLED init */
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "Injector Sys", 16);
    OLED_ShowString(0, 2, "Init...", 16);

    /* Peripheral init */
    GPIO_Motor_Init();
    ADC_Poll_Init();
    Key_Init();
    Alarm_Init();
    EEPROM_Init();
    TIM2_Motor_Init();
    TIM3_KeyScan_Init();

    /* Load configuration from EEPROM */
    EEPROM_LoadConfig();

    /* Zero calibration: sample pressure sensor baseline at startup */
    g_adc_pressure_zero = ADC_ReadAvg(ADC_CH_PRESSURE, 16);
    /* Clamp to a reasonable max offset (prevents bad calibration if sensor is loaded at boot) */
    if (g_adc_pressure_zero > 500) g_adc_pressure_zero = 0;

    /* Initialize runtime data */
    memset(&g_rt, 0, sizeof(g_rt));

    delay_ms(1000);

    OLED_Clear();
    OLED_ShowString(0, 0, "System Ready", 16);
    delay_ms(500);

    /* Enter idle state */
    g_state = STATE_IDLE;
    g_page  = PAGE_STANDBY;
    OLED_Update();

    /*===========================================================================
     * MAIN LOOP
     *===========================================================================*/

    while (1)
    {
        /* --- 10 Hz display update --- */
        if (g_sys_tick - last_display_tick >= DISPLAY_INTERVAL)
        {
            last_display_tick = g_sys_tick;

            /* Read sensors (8-point DMA average) */
            Sensor_Read();

            /* Safety check */
            Alarm_Check();

            /* Update display (skip normal OLED_Update in ERROR state) */
            if (g_state != STATE_ERROR)
                OLED_Update();

            /* Blink control */
            g_blink_tick++;
            if (g_blink_tick >= 5)
            {
                g_blink_tick = 0;
                g_blink_state = !g_blink_state;
            }
        }

        /* --- Key scan (polling, debounced internally) --- */
        Key_Process();

        /* --- State machine --- */
        switch (g_state)
        {
        case STATE_IDLE:      State_Idle();      break;
        case STATE_SETUP:     State_Setup();     break;
        case STATE_PURGE:     State_Purge();     break;
        case STATE_INJECT:    State_Inject();    break;
        case STATE_COMPRESS:  State_Compress();  break;
        case STATE_RETRACT:   State_Retract();   break;
        case STATE_ERROR:     State_Error();     break;
        default:              g_state = STATE_IDLE; break;
        }
    }
}
