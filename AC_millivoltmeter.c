#include "ti_msp_dl_config.h"
#include "bsp/oled.h"
#include "stdio.h"

#define RESULT_SIZE 64

volatile bool ADC_conv_cplt_flag; // ADC转换完成标志
uint16_t VoutValue = 0; // 程序开始时，为设置继电器状态而采的，Vin未经放大时交流毫伏表模块输出的直流电压值
float Vout_DC = 0; // 交流毫伏表模块输出的直流电压值（单位mV）
float Vin_mVpp = 0; // 推算所得的输入正弦波峰峰值（单位mV）
float AdcValue_avg = 0; // ADC采样数组的平均值
uint32_t pin_temp;

// ADC采集数组
volatile uint16_t AdcValue[RESULT_SIZE]; // 采集硬件部分输出的直流电压值

// 为在OLED上显示结果而设的缓冲区
char buffer[20];

// 若以单个采样值做判断，输入信号在阈值附近时继电器会产生振荡。故先求平均，再判断
/**
  * @brief  计算数组的平均值（用于平滑数据、获取更稳定的直流电压值）
  * @param  array  指向uint16_t类型数组的指针
  * @param  size   数组大小
  * @retval 计算得到的平均值，类型为float
  */
float calculateAverage(volatile uint16_t *array, int size) {
    uint32_t sum = 0;
    for (uint16_t i = 0; i < size; i++) {
        sum += array[i];
    }
    return (float)sum / (float)size;
}

int main(void)
{
    SYSCFG_DL_init();
    NVIC_EnableIRQ(ADC0_INST_INT_IRQN); // 使能ADC中断

    ADC_conv_cplt_flag = false;
    uint16_t i = 0;

    OLED_Init();
    OLED_CLS();

    // 继电器控制IO（PA2）默认为拉低（默认接进来的是大信号，不进行放大，以免对大信号进行放大而造成失真）

    // // 延时稳定（让模拟电路响应）
    // for(volatile int i = 0; i < 10000; i++);

    // 启动ADC转换
    DL_ADC12_startConversion(ADC0_INST);

    // 等待第一次采样完成
    while (ADC_conv_cplt_flag == false) { __WFE();}
    VoutValue = DL_ADC12_getMemResult(ADC0_INST, DL_ADC12_MEM_IDX_0);
    // 输入信号为100mVpp时，实测最大ADC采样值小于300，故阈值设为300
    if(VoutValue < 300) // 若所测信号为小信号（0~100mVpp），则拉高PA2，信号经过放大器（x10）后接入交流毫伏表模块
    {
        DL_GPIO_setPins(GPIO_RELAY_CTRL_PORT, GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN);
    }
    else // 若所测信号为大信号（100mVpp~1Vpp），则拉低PA2，不进行放大，信号直接接入交流毫伏表模块
    {
        DL_GPIO_clearPins(GPIO_RELAY_CTRL_PORT, GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN);
    }

    // // 延时让系统稳定
    // for(volatile int i = 0; i < 10000; i++);

    // // 正常循环开始
    // uint16_t i = 0;

    while (1) 
    {
        // ADC未采样完成时就一直等待
        while (ADC_conv_cplt_flag == false) 
        { 
            __WFE();
        }

        // ADC采样完成时将结果写入数组
        AdcValue[i] = DL_ADC12_getMemResult(ADC0_INST, DL_ADC12_MEM_IDX_0);
        ADC_conv_cplt_flag = false; // 将采样完成标志置false

        // 将当前ADC采样原始值显示在OLED上
        sprintf(buffer, "ADC Value: %4d", AdcValue[i]);
        OLED_ShowString(1, 1, buffer, 1);

        // 计算ADC采样数组平均值
        AdcValue_avg = calculateAverage((volatile uint16_t*)AdcValue, RESULT_SIZE);

        /*-----------------------↓↓↓推算原电压的逻辑↓↓↓-----------------------*/
        // Vout_DC = (float)AdcValue[i]*3.3/4096.0*1000; // 交流毫伏表模块输出的直流电压值（单位mV）
        Vout_DC = AdcValue_avg*3.3/4096.0*1000; // 交流毫伏表模块输出的直流电压值（单位mV）
        // 将Vout_DC显示在OLED上
        sprintf(buffer, "Vout(DC): %.2f mV", Vout_DC);
        OLED_ShowString(3, 1, buffer, 1);

        // if(DL_GPIO_readPins(GPIO_RELAY_CTRL_PORT, GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN))
        // if(DL_GPIO_readPins(GPIO_RELAY_READ_PORT, GPIO_RELAY_READ_PIN_RELAY_READ_PIN))
        pin_temp = DL_GPIO_readPins(GPIO_RELAY_READ_PORT, GPIO_RELAY_READ_PIN_RELAY_READ_PIN);
        if(pin_temp)
        // 当前处于放大模式（x10增益），将结果除以10。
        {
            Vin_mVpp = (Vout_DC*0.997 + 8.9198) /10.0 *1.4142;
            sprintf(buffer, "11111111"); // 调试用，意为“当前处于放大模式”
        }
        else // 当前未放大，结果不除以10。
        {
            Vin_mVpp = (Vout_DC*1.0134 + 31.132 - 24) *1.4142 + 5;
            sprintf(buffer, "00000000"); // 调试用，意为“当前未放大”
        }
        OLED_ShowString(7, 1, buffer, 1);

        //将推算所得的输入正弦波峰峰值显示在OLED上
        sprintf(buffer, "Vin(Vpp): %.2f mVpp", Vin_mVpp);
        OLED_ShowString(5, 1, buffer, 1);
        /*-----------------------↑↑↑推算原电压的逻辑↑↑↑-----------------------*/


        /*-----------------------↓↓↓继电器控制逻辑↓↓↓-----------------------*/
        if (Vin_mVpp < 100.0) 
        {  // 推算出输入信号<200mVpp，是小信号
            DL_GPIO_setPins(GPIO_RELAY_CTRL_PORT, GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN); // 放大
        } 
        else if (Vin_mVpp > 130.0) 
        { // 输入信号>500mVpp时才放大，留出迟滞
            DL_GPIO_clearPins(GPIO_RELAY_CTRL_PORT, GPIO_RELAY_CTRL_PIN_RELAY_CTRL_PIN); // 不放大
        }
        /*-----------------------↑↑↑继电器控制逻辑↑↑↑-----------------------*/


        i++; // 采样点计数变量自增
        // ADC_conv_cplt_flag = false; // 将采样完成标志置false（本句已上移至ADC等待语句后）

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
