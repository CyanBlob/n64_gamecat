#ifndef N64_GAMECAT_GPIO_CFG_H
#define N64_GAMECAT_GPIO_CFG_H

#define PIN_MIN  2
#define PIN_MAX  39

#define CA_AD0  2
#define CA_AD1  3
#define CA_AD2  4
#define CA_AD3  5
#define CA_AD4  6
#define CA_AD5  7
#define CA_AD6  8
#define CA_AD7  9
#define CA_AD8  10
#define CA_AD9  11
#define CA_AD10 12
#define CA_AD11 13
#define CA_AD12 14
#define CA_AD13 15
#define CA_AD14 16
#define CA_AD15 17

#define CO_AD0  18
#define CO_AD1  19
#define CO_AD2  20
#define CO_AD3  21
#define CO_AD4  22
#define CO_AD5  23
#define CO_AD6  24
#define CO_AD7  25
#define CO_AD8  26
#define CO_AD9  27
#define CO_AD10 28
#define CO_AD11 29
#define CO_AD12 30
#define CO_AD13 31
#define CO_AD14 32
#define CO_AD15 33

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

#define ALE_H 34
#define ALE_L 35
#define WRITE 36
#define READ  37

#define FLAG_ALE_H 40
#define FLAG_ALE_L 41
#define FLAG_WRITE 42
#define FLAG_READ 43

#endif //N64_GAMECAT_GPIO_CFG_H
