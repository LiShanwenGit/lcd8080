# LCD8080

#### 介绍
该源码为常用的小尺寸并行接口的intel8080液晶屏实现Linux驱动  

## 支持控制器种类
- ILI9341 
- ST7789V


## 添加设备树 

```
lcd8080@0 {
    status = "okay";
    compatible   = "lcd8080-ili9341";
    reg          = <0>;
    data-width   = <8>;
    height       = <240>;
    width        = <320>;
    rotate       = <0>;
    fps          = <30>;
    dma-enable;
    bgr ;

    cs-gpios     = <&pio 6 2 GPIO_ACTIVE_HIGH>; /* PG2 */
    rd-gpios     = <&pio 5 0 GPIO_ACTIVE_HIGH>; /* PF0 */
    reset-gpios  = <&pio 5 1 GPIO_ACTIVE_HIGH>; /* PF1 */
    wr-gpios     = <&pio 5 2 GPIO_ACTIVE_HIGH>; /* PF2 */
    dc-gpios     = <&pio 5 3 GPIO_ACTIVE_HIGH>; /* PF3 */
    backlight-gpios = <&pio 5 3 GPIO_ACTIVE_HIGH>; /* PF3 */
    data-gpios   = <&pio 4 0 GPIO_ACTIVE_HIGH>, /* PE0 */
                   <&pio 4 1 GPIO_ACTIVE_HIGH>, /* PE1 */
                   <&pio 4 2 GPIO_ACTIVE_HIGH>, /* PE2 */
                   <&pio 4 3 GPIO_ACTIVE_HIGH>, /* PE3 */
                   <&pio 4 4 GPIO_ACTIVE_HIGH>, /* PE4 */
                   <&pio 4 5 GPIO_ACTIVE_HIGH>, /* PE5 */
                   <&pio 4 6 GPIO_ACTIVE_HIGH>, /* PE6 */
                   <&pio 4 7 GPIO_ACTIVE_HIGH>; /* PE7 */
};
```
## 说明  
为了很好的利用FBTFT的驱动初始化代码，该源码中使用的框架尽可能与FBTFT一致，这样的好处是方便用户自己添加还没有支持的LCD控制器  

## 添加新的控制器 

