#include "adc.h"
#include "delay.h"

void Adc_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    // ADC时钟 72/6 = 12MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    // PA0~PA4 模拟输入
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0 |
        GPIO_Pin_1 |
        GPIO_Pin_2 |
        GPIO_Pin_3 |
        GPIO_Pin_4;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ADC_DeInit(ADC1);

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;

    // 单通道
    ADC_InitStructure.ADC_NbrOfChannel = 1;

    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    // 校准
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

u16 Get_Adc(u8 ch)
{
    // 配置通道
    ADC_RegularChannelConfig(
        ADC1,
        ch,
        1,
        ADC_SampleTime_239Cycles5);

    // 开始转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // 等待完成
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    return ADC_GetConversionValue(ADC1);
}

u16 Get_Adc_Average(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc(ch);
    }

    return temp_val / times;
}
