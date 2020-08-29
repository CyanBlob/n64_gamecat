#ifndef CONSTS
#define CONSTS

#define UART_NUM    UART_DEVICE_3

// 0-7 are multiplexed for read low bits/write (depending on READ state)
#define AD0         0
#define AD1         1
#define AD2         2
#define AD3         3
#define AD4         4
#define AD5         5
#define AD6         6
#define AD7         7

#define AD0_PIN     FUNC_GPIOHS0
#define AD1_PIN     FUNC_GPIOHS1
#define AD2_PIN     FUNC_GPIOHS2
#define AD3_PIN     FUNC_GPIOHS3
#define AD4_PIN     FUNC_GPIOHS4
#define AD5_PIN     FUNC_GPIOHS5
#define AD6_PIN     FUNC_GPIOHS6
#define AD7_PIN     FUNC_GPIOHS7

// AD0-7 are fully separated on my circuit, so we'll read with the previous set of pins
// and write those values on this set, manipulating the values as desired
#define AD0_OUT         21
#define AD1_OUT         22
#define AD2_OUT         23
#define AD3_OUT         24
#define AD4_OUT         25
#define AD5_OUT         26
#define AD6_OUT         27
#define AD7_OUT         28

#define AD0_PIN_OUT     FUNC_GPIOHS21
#define AD1_PIN_OUT     FUNC_GPIOHS22
#define AD2_PIN_OUT     FUNC_GPIOHS23
#define AD3_PIN_OUT     FUNC_GPIOHS24
#define AD4_PIN_OUT     FUNC_GPIOHS25
#define AD5_PIN_OUT     FUNC_GPIOHS26
#define AD6_PIN_OUT     FUNC_GPIOHS27
#define AD7_PIN_OUT     FUNC_GPIOHS28

// 8-15 are for address high bits. They're connected to the MCU, console, and cartridge.
// The cartridge doesn't send values back along them, so the MCU is just here to spy
#define AD8         8
#define AD9         9
#define AD10        10
#define AD11        11
#define AD12        12
#define AD13        13
#define AD14        14
#define AD15        15

#define AD8_PIN     FUNC_GPIOHS8
#define AD9_PIN     FUNC_GPIOHS9
#define AD10_PIN    FUNC_GPIOHS10
#define AD11_PIN    FUNC_GPIOHS11
#define AD12_PIN    FUNC_GPIOHS12
#define AD13_PIN    FUNC_GPIOHS13
#define AD14_PIN    FUNC_GPIOHS14
#define AD15_PIN    FUNC_GPIOHS15

// address latch low - when pulled low (from up), AD0-7 hold the low address bits
#define ALE_L       17
#define ALE_L_PIN   FUNC_GPIOHS17

// address latch high - when pulled low (from up), AD8-15 hold the high address bits
#define ALE_H       18
#define ALE_H_PIN   FUNC_GPIOHS18

// when pulled low when ALE_L/H, we need to write the first 8 bits of the value at the given
// address. It will then be pulled high for a short period, then low again. At that point,
// write the upper 8 bits. Both writes use AD0-7
#define READ        19
#define READ_PIN    FUNC_GPIOHS19

// haven't figured this one out yet due to missing timing diagrams; I'm working on it. It most
// likely works just like READ (two pulses, 8 bits saved on each
#define WRITE       20
#define WRITE_PIN   FUNC_GPIOHS20

#endif
