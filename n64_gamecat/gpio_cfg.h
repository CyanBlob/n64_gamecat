#ifndef N64_GAMECAT_GPIO_CFG_H
#define N64_GAMECAT_GPIO_CFG_H

#define PIN_MIN  2
#define PIN_MAX  39

#define CO_AD0  10
#define CO_AD1  8
#define CO_AD2  6
#define CO_AD3  4
#define CO_AD4  2
#define CO_AD5  34
#define CO_AD6  32
#define CO_AD7  30
#define CO_AD8  31
#define CO_AD9  33
#define CO_AD10 35
#define CO_AD11 3
#define CO_AD12 5
#define CO_AD13 7
#define CO_AD14 9
#define CO_AD15 11

#define CA_AD0  28
#define CA_AD1  26
#define CA_AD2  24
#define CA_AD3  22
#define CA_AD4  20
#define CA_AD5  18
#define CA_AD6  16
#define CA_AD7  14
#define CA_AD8  15
#define CA_AD9  17
#define CA_AD10 19
#define CA_AD11 21
#define CA_AD12 23
#define CA_AD13 25
#define CA_AD14 27
#define CA_AD15 29

uint console_pins[] = {

        CO_AD0,
        CO_AD1,
        CO_AD2,
        CO_AD3,
        CO_AD4,
        CO_AD5,
        CO_AD6,
        CO_AD7,
        CO_AD8,
        CO_AD9,
        CO_AD10,
        CO_AD11,
        CO_AD12,
        CO_AD13,
        CO_AD14,
        CO_AD15,
};
uint cartridge_pins[] = {
        CA_AD0,
        CA_AD1,
        CA_AD2,
        CA_AD3,
        CA_AD4,
        CA_AD5,
        CA_AD6,
        CA_AD7,
        CA_AD8,
        CA_AD9,
        CA_AD10,
        CA_AD11,
        CA_AD12,
        CA_AD13,
        CA_AD14,
        CA_AD15,
};

#define ALE_H 36
#define ALE_L 37
#define WRITE 38
#define READ  39

#endif //N64_GAMECAT_GPIO_CFG_H
