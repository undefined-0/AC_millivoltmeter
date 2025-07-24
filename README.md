# AC_millivoltmeter
本仓库用于存放小项目“可调档位交流毫伏表”的软件程序。



## 软件部分的功能

1. ADC采样硬件部分输出的直流值
2. 判断直流值是否过小，并根据直流值的大小控制GPIO以完成档位切换
3. 将采样值显示在OLED上（暂不要求采样值->实际值的换算）



## MSPM0G3507接线方法

* 四线0.96寸OLED屏幕

  * PA0 - I2C0 SDA

  * PA1 - I2C0 SCL

  * 3V3、GND两根电源线


* ADC采样IO

  * PA27 - ADC12 Channel 0 Pin（AdcResult_U）
