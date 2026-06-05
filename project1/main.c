#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include "adc.h"
#include "key.h"
#include "led.h"
#include "stdio.h"

/* Threshold defaults */
#define VIB_THRESHOLD_DEFAULT   4.5f
#define TEMP_THRESHOLD_DEFAULT  85.0f
#define CUR_THRESHOLD_DEFAULT   10.0f

/* Timing */
#define UPLOAD_INTERVAL_DEFAULT 5
#define SAMPLE_BLOCK_SIZE       30

/* ADC channels: PA0=Vib, PA1=Temp, PA2/PA3/PA4=3-phase current */
#define ADC_CH_VIB      ADC_Channel_0
#define ADC_CH_TEMP     ADC_Channel_1
#define ADC_CH_CUR_A    ADC_Channel_2
#define ADC_CH_CUR_B    ADC_Channel_3
#define ADC_CH_CUR_C    ADC_Channel_4

/* ADC scaling (12-bit ADC, Vref=3.3V) */
#define ADC_VREF        3.3f
#define ADC_MAX         4095.0f
#define ADC_TO_VOLT(adc)  ((float)(adc) * ADC_VREF / ADC_MAX)

/* Sensor full-scale values (0~3.3V maps linearly) */
#define VIB_FULLSCALE   20.0f
#define TEMP_FULLSCALE  150.0f
#define CUR_FULLSCALE   10.0f

/* Display pages */
#define PAGE_MAIN       0
#define PAGE_SET_VIB    1
#define PAGE_SET_TEMP   2
#define PAGE_SET_CUR    3
#define PAGE_SET_WIFI   4
#define PAGE_MAX        5

/* Data structures */
typedef struct {
    float vib_rms;
    float temperature;
    float cur_a;
    float cur_b;
    float cur_c;
} SensorData;

typedef struct {
    float vib_threshold;
    float temp_threshold;
    float cur_threshold;
    uint8_t upload_interval;
} SysConfig;

SensorData g_sensor;
SysConfig g_config;
uint8_t g_display_page = PAGE_MAIN;
uint8_t g_alarm_flag = 0;

static uint32_t g_vib_sum = 0;
static uint32_t g_temp_sum = 0;
static uint32_t g_cur_a_sum = 0, g_cur_b_sum = 0, g_cur_c_sum = 0;
static uint16_t g_sample_count = 0;
static uint32_t g_sys_tick_ms = 0;
static uint8_t g_blink_tick = 0;
static uint8_t g_blink_state = 0;
static uint8_t g_last_display_page = 0xFF;

static void Alarm_Check(void)
{
    if (g_sensor.vib_rms > g_config.vib_threshold ||
        g_sensor.temperature > g_config.temp_threshold ||
        g_sensor.cur_a > g_config.cur_threshold ||
        g_sensor.cur_b > g_config.cur_threshold ||
        g_sensor.cur_c > g_config.cur_threshold)
    {
        g_alarm_flag = 1;
        Alarm_On();
    }
    else
    {
        g_alarm_flag = 0;
        Alarm_Off();
    }
}

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        g_sys_tick_ms++;
    }
}

static void TIM3_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_Prescaler = 7;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

static void Sensor_SampleBlock(void)
{
    uint16_t raw;
    int i;

    g_vib_sum = 0;
    g_temp_sum = 0;
    g_cur_a_sum = 0; g_cur_b_sum = 0; g_cur_c_sum = 0;
    g_sample_count = 0;

    for (i = 0; i < SAMPLE_BLOCK_SIZE; i++)
    {
        raw = Get_Adc(ADC_CH_VIB);
        g_vib_sum += raw;

        raw = Get_Adc(ADC_CH_TEMP);
        g_temp_sum += raw;

        raw = Get_Adc(ADC_CH_CUR_A);
        g_cur_a_sum += raw;

        raw = Get_Adc(ADC_CH_CUR_B);
        g_cur_b_sum += raw;

        raw = Get_Adc(ADC_CH_CUR_C);
        g_cur_c_sum += raw;

        g_sample_count++;
    }
}

static void Sensor_Process(void)
{
    float avg_volt;

    if (g_sample_count == 0) return;

    avg_volt = ADC_TO_VOLT((float)g_vib_sum / g_sample_count);
    g_sensor.vib_rms = avg_volt / ADC_VREF * VIB_FULLSCALE;
    if (g_sensor.vib_rms > VIB_FULLSCALE) g_sensor.vib_rms = VIB_FULLSCALE;

    avg_volt = ADC_TO_VOLT((float)g_temp_sum / g_sample_count);
    g_sensor.temperature = avg_volt / ADC_VREF * TEMP_FULLSCALE;
    if (g_sensor.temperature > TEMP_FULLSCALE) g_sensor.temperature = TEMP_FULLSCALE;

    avg_volt = ADC_TO_VOLT((float)g_cur_a_sum / g_sample_count);
    g_sensor.cur_a = avg_volt / ADC_VREF * CUR_FULLSCALE;
    if (g_sensor.cur_a > CUR_FULLSCALE) g_sensor.cur_a = CUR_FULLSCALE;

    avg_volt = ADC_TO_VOLT((float)g_cur_b_sum / g_sample_count);
    g_sensor.cur_b = avg_volt / ADC_VREF * CUR_FULLSCALE;
    if (g_sensor.cur_b > CUR_FULLSCALE) g_sensor.cur_b = CUR_FULLSCALE;

    avg_volt = ADC_TO_VOLT((float)g_cur_c_sum / g_sample_count);
    g_sensor.cur_c = avg_volt / ADC_VREF * CUR_FULLSCALE;
    if (g_sensor.cur_c > CUR_FULLSCALE) g_sensor.cur_c = CUR_FULLSCALE;
}

static void OLED_ShowMain(void)
{
    char buf[16];

    /* Line 0: Vibration + Temperature */
    OLED_ShowString(0, 0, "V:", 16);
    sprintf(buf, "%4.1f", (double)g_sensor.vib_rms);
    OLED_ShowString(2 * 8, 0, (u8 *)buf, 16);
    OLED_ShowString(7 * 8, 0, "T:", 16);
    sprintf(buf, "%4.1f", (double)g_sensor.temperature);
    OLED_ShowString(9 * 8, 0, (u8 *)buf, 16);

    /* Line 2: Current A + B */
    OLED_ShowString(0, 2, "Ia:", 16);
    sprintf(buf, "%4.2f", (double)g_sensor.cur_a);
    OLED_ShowString(3 * 8, 2, (u8 *)buf, 16);
    OLED_ShowString(8 * 8, 2, "Ib:", 16);
    sprintf(buf, "%4.2f", (double)g_sensor.cur_b);
    OLED_ShowString(11 * 8, 2, (u8 *)buf, 16);

    /* Line 4: Current C + Status */
    OLED_ShowString(0, 4, "Ic:", 16);
    sprintf(buf, "%4.2f", (double)g_sensor.cur_c);
    OLED_ShowString(3 * 8, 4, (u8 *)buf, 16);

    if (g_alarm_flag)
    {
        if (g_blink_state)
            OLED_ShowString(8 * 8, 4, "ALARM!", 16);
        else
            OLED_ShowString(8 * 8, 4, "      ", 16);
    }
    else
    {
        OLED_ShowString(8 * 8, 4, "Normal", 16);
    }

    /* Line 6: blank (used for blink clearance) */
    OLED_ShowString(0, 6, "                ", 16);
}

static void OLED_ShowSetVib(void)
{
    char buf[20];
    OLED_ShowString(0, 0, "Set Vibration  ", 16);
    OLED_ShowString(0, 2, "Threshold:     ", 16);
    sprintf(buf, "%4.1f mm/s", (double)g_config.vib_threshold);
    OLED_ShowString(0, 4, (u8 *)buf, 16);
    OLED_ShowString(0, 6, "KEY2+  KEY3-   ", 16);
}

static void OLED_ShowSetTemp(void)
{
    char buf[20];
    OLED_ShowString(0, 0, "Set Temperature", 16);
    OLED_ShowString(0, 2, "Threshold:     ", 16);
    sprintf(buf, "%4.1f C    ", (double)g_config.temp_threshold);
    OLED_ShowString(0, 4, (u8 *)buf, 16);
    OLED_ShowString(0, 6, "KEY2+  KEY3-   ", 16);
}

static void OLED_ShowSetCur(void)
{
    char buf[20];
    OLED_ShowString(0, 0, "Set Current    ", 16);
    OLED_ShowString(0, 2, "Threshold:     ", 16);
    sprintf(buf, "%4.2f A   ", (double)g_config.cur_threshold);
    OLED_ShowString(0, 4, (u8 *)buf, 16);
    OLED_ShowString(0, 6, "KEY2+  KEY3-   ", 16);
}

static void OLED_ShowSetWifi(void)
{
    char buf[20];
    OLED_ShowString(0, 0, "Set WiFi       ", 16);
    OLED_ShowString(0, 2, "Upload Intv:   ", 16);
    sprintf(buf, "%d sec   ", g_config.upload_interval);
    OLED_ShowString(0, 4, (u8 *)buf, 16);
    OLED_ShowString(0, 6, "KEY2+  KEY3-   ", 16);
}

static void OLED_Update(void)
{
    if (g_display_page != g_last_display_page)
    {
        OLED_Clear();
        g_last_display_page = g_display_page;
    }

    switch (g_display_page)
    {
    case PAGE_MAIN:
        OLED_ShowMain();
        break;
    case PAGE_SET_VIB:
        OLED_ShowSetVib();
        break;
    case PAGE_SET_TEMP:
        OLED_ShowSetTemp();
        break;
    case PAGE_SET_CUR:
        OLED_ShowSetCur();
        break;
    case PAGE_SET_WIFI:
        OLED_ShowSetWifi();
        break;
    default:
        g_display_page = PAGE_MAIN;
        break;
    }
}

static void Key_Process(void)
{
    u8 key = KEY_Scan(0);

    if (key == 0) return;

    switch (key)
    {
    case KEY0_PRES:
        if (g_display_page != PAGE_MAIN)
            g_display_page = PAGE_MAIN;
        break;

    case KEY1_PRES:
        g_display_page++;
        if (g_display_page >= PAGE_MAX)
            g_display_page = PAGE_SET_VIB;
        if (g_display_page != PAGE_MAIN)
            OLED_Update();
        break;

    case KEY2_PRES:
        switch (g_display_page)
        {
        case PAGE_SET_VIB:
            g_config.vib_threshold += 0.5f;
            if (g_config.vib_threshold > 20.0f) g_config.vib_threshold = 20.0f;
            break;
        case PAGE_SET_TEMP:
            g_config.temp_threshold += 1.0f;
            if (g_config.temp_threshold > 150.0f) g_config.temp_threshold = 150.0f;
            break;
        case PAGE_SET_CUR:
            g_config.cur_threshold += 0.1f;
            if (g_config.cur_threshold > 10.0f) g_config.cur_threshold = 10.0f;
            break;
        case PAGE_SET_WIFI:
            g_config.upload_interval += 1;
            if (g_config.upload_interval > 60) g_config.upload_interval = 60;
            break;
        default:
            break;
        }
        if (g_display_page != PAGE_MAIN)
            OLED_Update();
        break;

    case KEY3_PRES:
        switch (g_display_page)
        {
        case PAGE_SET_VIB:
            g_config.vib_threshold -= 0.5f;
            if (g_config.vib_threshold < 0.5f) g_config.vib_threshold = 0.5f;
            break;
        case PAGE_SET_TEMP:
            g_config.temp_threshold -= 1.0f;
            if (g_config.temp_threshold < 0.0f) g_config.temp_threshold = 0.0f;
            break;
        case PAGE_SET_CUR:
            g_config.cur_threshold -= 0.1f;
            if (g_config.cur_threshold < 0.0f) g_config.cur_threshold = 0.0f;
            break;
        case PAGE_SET_WIFI:
            g_config.upload_interval -= 1;
            if (g_config.upload_interval < 1) g_config.upload_interval = 1;
            break;
        default:
            break;
        }
        if (g_display_page != PAGE_MAIN)
            OLED_Update();
        break;

    default:
        break;
    }
}

static void WiFi_Upload(void)
{
    char buf[100];
    sprintf(buf, "{\"vib\":%.1f,\"tmp\":%.1f,\"Ia\":%.2f,\"Ib\":%.2f,\"Ic\":%.2f,\"alm\":%d}\r\n",
            (double)g_sensor.vib_rms,
            (double)g_sensor.temperature,
            (double)g_sensor.cur_a,
            (double)g_sensor.cur_b,
            (double)g_sensor.cur_c,
            g_alarm_flag);
    USART3_SendString((u8 *)buf);
}

int main(void)
{
    uint32_t last_display_tick = 0;
    uint32_t last_upload_tick = 0;

    delay_init();

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "Motor Monitor", 16);
    OLED_ShowString(0, 2, "Init...", 16);

    Adc_Init();
    Key_Init();
    Alarm_Init();
    usart3_init(9600);
    TIM3_Init();

    g_config.vib_threshold = VIB_THRESHOLD_DEFAULT;
    g_config.temp_threshold = TEMP_THRESHOLD_DEFAULT;
    g_config.cur_threshold = CUR_THRESHOLD_DEFAULT;
    g_config.upload_interval = UPLOAD_INTERVAL_DEFAULT;

    delay_ms(1000);

    USART3_SendString((u8 *)"Motor Monitor System Ready\r\n");
    OLED_Update();

    while (1)
    {
        if (g_sys_tick_ms - last_display_tick >= 50)
        {
            last_display_tick = g_sys_tick_ms;

            Sensor_SampleBlock();
            Sensor_Process();
            Alarm_Check();

            if (g_display_page == PAGE_MAIN)
                OLED_Update();

            g_blink_tick++;
            if (g_blink_tick >= 4)
            {
                g_blink_tick = 0;
                g_blink_state = !g_blink_state;
                if (g_alarm_flag && g_display_page == PAGE_MAIN)
                    OLED_Update();
            }
        }

        Key_Process();

        if (g_display_page == PAGE_MAIN &&
            g_sys_tick_ms - last_upload_tick >= (uint32_t)g_config.upload_interval * 1000)
        {
            last_upload_tick = g_sys_tick_ms;
            WiFi_Upload();
        }
    }
}
