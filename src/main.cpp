/** N64 GameCat source by Andrew Thomas (aka CyanBlob)
 *  For use with the Sipeed MAiX BiT MCU (Kendryte K210 chip)
 */

#include <stdio.h>
#include <cstring>

#include "fpioa.h"
#include "uart.h"
#include "gpiohs.h"
#include "sysctl.h"

// this includes all the pin mappings
#include "consts.h"

// last read address
uint16_t read = 0;

// this is a very simplified implementation of the planned cheat system. It will allow overriding
// the value of a single address
uint32_t overwrite_addr = 0x8011A604; // rupees (https://cloudmodding.com/zelda/oot)
uint16_t overwrite_val = 0x0045;      // nice

bool writing = false;

void init_read();
void init_write();
void init();

void interruptTest()
{
    char buf[20];
    memset(buf, '0', sizeof(buf));
    snprintf(buf, sizeof(buf), "INTERRUPT TOGGLE\n");
    buf[sizeof(buf) - 1] = '\0';
    uart_send_data(UART_NUM, buf, strlen(buf));
}

void read_upper()
{
    if (writing)
    {
        init_read();
    }

    // clear upper 16 bits of `read`
    read &= 0x0000FFFF;

    read |= gpiohs_get_pin(AD0) << 16;
    read |= gpiohs_get_pin(AD1) << 17;
    read |= gpiohs_get_pin(AD2) << 18;
    read |= gpiohs_get_pin(AD3) << 19;
    read |= gpiohs_get_pin(AD4) << 20;
    read |= gpiohs_get_pin(AD5) << 21;
    read |= gpiohs_get_pin(AD6) << 22;
    read |= gpiohs_get_pin(AD7) << 23;
    read |= gpiohs_get_pin(AD8)  << 24;
    read |= gpiohs_get_pin(AD9)  << 25;
    read |= gpiohs_get_pin(AD10) << 26;
    read |= gpiohs_get_pin(AD11) << 27;
    read |= gpiohs_get_pin(AD12) << 28;
    read |= gpiohs_get_pin(AD13) << 29;
    read |= gpiohs_get_pin(AD14) << 30;
    read |= gpiohs_get_pin(AD15) << 31;
}

void read_lower()
{
    if (writing)
    {
        init_read();
    }

    // clear lower 16 bits of `read`
    read &= 0xFFFF0000;

    read |= gpiohs_get_pin(AD0);
    read |= gpiohs_get_pin(AD1) << 1;
    read |= gpiohs_get_pin(AD2) << 2;
    read |= gpiohs_get_pin(AD3) << 3;
    read |= gpiohs_get_pin(AD4) << 4;
    read |= gpiohs_get_pin(AD5) << 5;
    read |= gpiohs_get_pin(AD6) << 6;
    read |= gpiohs_get_pin(AD7) << 7;
    read |= gpiohs_get_pin(AD8)  << 8;
    read |= gpiohs_get_pin(AD9)  << 9;
    read |= gpiohs_get_pin(AD10) << 10;
    read |= gpiohs_get_pin(AD11) << 11;
    read |= gpiohs_get_pin(AD12) << 12;
    read |= gpiohs_get_pin(AD13) << 13;
    read |= gpiohs_get_pin(AD14) << 14;
    read |= gpiohs_get_pin(AD15) << 15;
}

void write_value()
{
    static int8_t i = 1; // track whether to write high (1) or low (0) bits

    if (!writing)
    {
        init_write();
    }

    // both writes read the same 16 cartridge pins; the cartridge should switch values
    // when READ pulses
    // NOTE: if the cartridge is much slower than this MCU,we may need to stall a bit here (60ns?)
    if (read == overwrite_addr)
    {
        if (i == 1)
        {
            // TODO: this is temporary until I get new hardware. I can only write the bottom
            // 8 bits in each write operation, so I'm only going to bother with bits 0-7 for now
            gpiohs_set_pin(AD0_OUT, gpiohs_get_pin(AD0));
            gpiohs_set_pin(AD1_OUT, gpiohs_get_pin(AD1));
            gpiohs_set_pin(AD2_OUT, gpiohs_get_pin(AD2));
            gpiohs_set_pin(AD3_OUT, gpiohs_get_pin(AD3));
            gpiohs_set_pin(AD4_OUT, gpiohs_get_pin(AD4));
            gpiohs_set_pin(AD5_OUT, gpiohs_get_pin(AD5));
            gpiohs_set_pin(AD6_OUT, gpiohs_get_pin(AD6));
            gpiohs_set_pin(AD7_OUT, gpiohs_get_pin(AD7));
        }

        else if (i == 0)
        {
            gpiohs_set_pin(AD0_OUT, static_cast<gpio_pin_value_t>((1 << (0 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD1_OUT, static_cast<gpio_pin_value_t>((1 << (1 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD2_OUT, static_cast<gpio_pin_value_t>((1 << (2 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD3_OUT, static_cast<gpio_pin_value_t>((1 << (3 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD4_OUT, static_cast<gpio_pin_value_t>((1 << (4 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD5_OUT, static_cast<gpio_pin_value_t>((1 << (5 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD6_OUT, static_cast<gpio_pin_value_t>((1 << (6 + i*8)) & overwrite_val));
            gpiohs_set_pin(AD7_OUT, static_cast<gpio_pin_value_t>((1 << (7 + i*8)) & overwrite_val));

            i = 1;
        }
        printf("%d\n", i);
    }
    else
    {
        gpiohs_set_pin(AD0_OUT, gpiohs_get_pin(AD0));
        gpiohs_set_pin(AD1_OUT, gpiohs_get_pin(AD1));
        gpiohs_set_pin(AD2_OUT, gpiohs_get_pin(AD2));
        gpiohs_set_pin(AD3_OUT, gpiohs_get_pin(AD3));
        gpiohs_set_pin(AD4_OUT, gpiohs_get_pin(AD4));
        gpiohs_set_pin(AD5_OUT, gpiohs_get_pin(AD5));
        gpiohs_set_pin(AD6_OUT, gpiohs_get_pin(AD6));
        gpiohs_set_pin(AD7_OUT, gpiohs_get_pin(AD7));
    }
}

inline void init_read()
{
    gpiohs_set_drive_mode(AD0, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD1, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD2, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD3, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD4, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD5, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD6, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(AD7, GPIO_DM_INPUT);

    writing = false;
}

inline void init_write()
{
    writing = true;

    gpiohs_set_drive_mode(AD0, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD1, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD2, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD3, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD4, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD5, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD6, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(AD7, GPIO_DM_OUTPUT);
}

void init_gpio(uint8_t number, fpioa_function_t pin, gpio_drive_mode_t mode)
{
    fpioa_set_function(number, pin);
    gpiohs_set_drive_mode(number, mode);
}

void init_control(uint8_t number, fpioa_function_t pin, gpio_drive_mode_t mode, void(*callback)())
{
    fpioa_set_function(number, pin);
    gpiohs_set_drive_mode(number, mode);
    gpiohs_set_pin_edge(number, GPIO_PE_BOTH);
    gpiohs_set_irq(number, 1, callback);
}

void init()
{
    uart_init(UART_NUM);
    uart_configure(UART_NUM, 115200, UART_BITWIDTH_8BIT, UART_STOP_1, UART_PARITY_NONE);

    init_gpio(AD0, AD0_PIN, GPIO_DM_INPUT);
    init_gpio(AD1, AD1_PIN, GPIO_DM_INPUT);
    init_gpio(AD2, AD2_PIN, GPIO_DM_INPUT);
    init_gpio(AD3, AD3_PIN, GPIO_DM_INPUT);
    init_gpio(AD4, AD4_PIN, GPIO_DM_INPUT);
    // TODO: re-enable when not using UART
    //init_gpio(AD5, AD5_PIN, GPIO_DM_INPUT);
    init_gpio(AD6, AD6_PIN, GPIO_DM_INPUT);
    init_gpio(AD7, AD7_PIN, GPIO_DM_INPUT);
    init_gpio(AD8, AD8_PIN, GPIO_DM_INPUT);
    init_gpio(AD9, AD9_PIN, GPIO_DM_INPUT);
    init_gpio(AD10, AD10_PIN, GPIO_DM_INPUT);
    init_gpio(AD11, AD11_PIN, GPIO_DM_INPUT);
    init_gpio(AD12, AD12_PIN, GPIO_DM_INPUT);
    init_gpio(AD13, AD13_PIN, GPIO_DM_INPUT);
    init_gpio(AD14, AD14_PIN, GPIO_DM_INPUT);
    init_gpio(AD15, AD15_PIN, GPIO_DM_INPUT);

    // control pins
    sysctl_enable_irq();

    init_control(ALE_L, ALE_L_PIN, GPIO_DM_INPUT, read_lower);
    init_control(ALE_H, ALE_H_PIN, GPIO_DM_INPUT, read_upper);
    init_control(READ, READ_PIN, GPIO_DM_INPUT,   write_value);

    // TODO: handle writes
    init_control(WRITE, WRITE_PIN, GPIO_DM_INPUT, interruptTest);
}

int main()
{
    init();

    char buf['30'];
    memset(buf, '0', sizeof(buf));
    snprintf(buf, sizeof(buf), "Booted\n");
    buf[sizeof(buf) - 1] = '\0';
    uart_send_data(UART_NUM, buf, strlen(buf));

    while(1)
    {
    }

    return 0;
}
