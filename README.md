# AC_millivoltmeter
本仓库用于存放小项目“10~100mVpp可调档位交流毫伏表”的软件程序。



## 软件部分的功能

1. ADC采样硬件部分输出的直流值
2. 判断直流值是否过小，并根据直流值的大小控制GPIO，进而控制继电器模块以完成档位切换
   * 所测信号为大信号（100mVpp~1Vpp），不进行放大，直接接入交流毫伏表模块
   * 所测信号为小信号（10~100mVpp），经过放大器（x10）后接入交流毫伏表模块
3. 将采样值显示在OLED上
4. （新增功能）反推出输入信号的峰峰值，显示在OLED上



## MSPM0G3507接线方法

* 四线0.96寸OLED屏幕

  * PA0 - I2C0 SDA

  * PA1 - I2C0 SCL

  * 3V3、GND两根电源线


* ADC采样IO

  * PA27 - ADC12 Channel 0 Pin（AdcResult_U）

* 继电器控制IO
  * PA2 - 连接继电器模块的GPIO引脚
  
  * B24 - 紧邻PA2，用跳帽与PA2连接
  
    * PA2默认拉低（默认接进来的是大信号，不进行放大，以免对大信号进行放大而造成失真）
  
  
    >PA2拉低 - $Y_{1、2}$与$A_{1、2}$短接，大信号，不放大
    >PA2拉高 - $Y_{1、2}$与$B_{1、2}$短接，小信号，放大
  
    * B24：用于读取PA2电平状态的引脚
      * 直接使用`DL_GPIO_readPins()`来读取被配置为Output的引脚PA2时，无论是拉高或拉低，读取结果总是为0，无法顺利进入“不放大”模式下的电压计算逻辑。可能「读取Output引脚」是个不被允许的操作。
      * 鉴于此情况，我们另外使能了一个紧邻PA2的IO口B24来读取当前PA2的电平。
