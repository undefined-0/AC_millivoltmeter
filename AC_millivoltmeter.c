#include "ti_msp_dl_config.h"
#include "bsp/oled.h"
#include "stdio.h"

#define RESULT_SIZE 2048

volatile bool ADC_conv_cplt_flag; // ADC转换完成标志

// ADC采集数组
volatile uint16_t AdcValue[RESULT_SIZE]; // 采集硬件部分输出的直流电压值

// 为在OLED上显示结果而设的缓冲区
char buffer[50];

int main(void)
{
    SYSCFG_DL_init();
    NVIC_EnableIRQ(ADC0_INST_INT_IRQN); // 使能ADC中断

    ADC_conv_cplt_flag  = false;
    uint16_t i = 0;

    OLED_Init();
    OLED_CLS();

    DL_ADC12_startConversion(ADC0_INST); // 已配置成多通道连续转换模式，只需要start一次

    while (1) 
    {
        // ADC未采样完成时就一直等待
        while (ADC_conv_cplt_flag == false) 
        { 
            __WFE();
        }

        // ADC采样完成时将结果写入数组
        AdcValue[i] = DL_ADC12_getMemResult(ADC0_INST, DL_ADC12_MEM_IDX_0);

        /*-----------------------继电器控制逻辑（可能需根据硬件情况再更改）-----------------------*/
        if(AdcValue[i]>1000) // 所测信号为大信号，直接短接不放大
        {
            DL_GPIO_setPins(GPIO_RELAY_CTRL_PORT,GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN);
        }
        else // 所测信号为小信号，经过放大器
        {
            DL_GPIO_clearPins(GPIO_RELAY_CTRL_PORT,GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN);
        }
        /*-----------------------继电器控制逻辑（可能需根据硬件情况再更改）-----------------------*/
        
        // 将当前ADC值格式化为字符串并显示在OLED上
        sprintf(buffer, "ADC Value: %4d", AdcValue[i]);
        OLED_ShowString(1, 1, buffer, 1);

        i++; // 采样点计数变量自增
        ADC_conv_cplt_flag = false; // 将采样完成标志置false

        if (i >= RESULT_SIZE) 
        {
            i = 0; // 循环写入
        }
    }
}

void ADC0_INST_IRQHandler(void) // ADC转换完成中断处理函数
{
    switch (DL_ADC12_getPendingInterrupt(ADC0_INST)) 
    {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED: // mem0中断，需要与syscfg中的配置相配合
            ADC_conv_cplt_flag = true; // 在中断中将采样完成标志置true
            break;
        default:
            break;
    }
}
