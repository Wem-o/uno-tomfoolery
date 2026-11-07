#include "serial.h"
#include "twi.h"

#include <avr/io.h>
#include <stdio.h>

// Register selector
const u8 HD_REGSEL = 0;
// Enable writing
const u8 HD_ENA = 1<<2;
// Enable bakclight (in hardware)
const u8 HD_BL_ENA = 1<<3;

const u8 HD_DISPLAY_CONTROL_DISPLAY_ENABLE = 2;
const u8 HD_DISPLAY_CONTROL_CURSOR = 1;
const u8 HD_DISPLAY_CONTROL_BLINK = 0;

const u8 HD_FUNCTION_SET_4BIT_MODE = 1<<4;
const u8 HD_FUNCTION_SET_2LINE_MODE = 1<<3;
const u8 HD_FUNCTION_SET_5x10_FONT = 1<<2;

const u8 HD_SHIFT_DISPLAY = 1<<3;
const u8 HD_SHIFT_CURSOR = 0;

const u8 HD_SHIFT_RIGHT = 1<<2;
const u8 HD_SHIFT_LEFT = 0;

typedef enum {
    HdCls = 1,
    HdReturnHome = 2,
    HdSetEntryMode = 4,
    HdDisplayControl = 8,
    HdShift = 16,
    HdFunctionSet = 32,
    HdSetCGRamAddress = 64,
    HdSetDDRamAddress = 128
} HdCommand;


// Commands must be an array of u8's. Every even element (0, 2, 4...) should
// contain true if the odd indexed element after it is an instruction
// (like clearing the screen) or a data call (like writing into DD or CG ram).
void lcd_send_commands(u8* commands, u16 sizeof_commands) {
    for (u16 i = 1; i < sizeof_commands; i += 2) {
        for (u8 count = 0; count < 2; count++) {
            u8 data = (count == 1) ? commands[i] << 4 : commands[i] & 0b11110000;
            data |= HD_ENA | HD_BL_ENA | (!commands[i - 1] ? 1 : 0);
            twi_send_byte(data);

            // Every data byte is terminated with a 0. HD_BL_ENA's pin isn't
            // connected to the display manager, but to a seperate circuit that
            // handles the backlight, so that should stay high.
            twi_send_byte(0 | HD_BL_ENA);
        }
    }
}

void mewo() {
    // == Say hello wold ==
    u8 commands[] = {
    //  CMD?    Value
        true,   HdDisplayControl | (1<<HD_DISPLAY_CONTROL_CURSOR) | (1<<HD_DISPLAY_CONTROL_DISPLAY_ENABLE),
        true,   HdCls,
        true,   HdReturnHome,

        false,  0b01001000, // H
        false,  0b01100101, // e
        false,  0b01101100, // l
        false,  0b01101100, // l
        false,  0b01101111, // o

        // 2nd line
        true, HdSetDDRamAddress | 0x40,
        false,  0b01010111, // W
        false,  0b01101111, // o
        false,  0b01110010, // r
        false,  0b01101100, // l
        false,  0b01100100, // d
        false,  0b00100001, // !
    };

    lcd_send_commands(commands, sizeof(commands));

    serial_write("Demo sent", 0);
}

int main(void) {
    // Turn off the onboard LED
    DDRB = 0<<PB5;

    serial_init(0);
    serial_write("Welcome to hell!\n\r", 0);


    unsigned char slave_addresss = 0x27;

    // Set up clock generator and prescaler
    // To get around 5k hertz 100 TWBR and 16 prescaling is used
    twi_set_clock(100, 16);
    switch (twi_send_start()) {
        case TWI_RESULT_OK:
            serial_write("TWI: START signal has been transmitted successfully!\n\r", 0);
            break;
        case TWI_RESULT_REPSTART:
            serial_write("TWI: A REPEATED START signal has been transmitted successfully\n\r",0 );
            break;
        case TWI_RESULT_TRANS_ERROR:
            serial_write("TWI: Failed to send START/REPEATED START signal!\n\r", 0);
        default:
            goto eternal_suffering;
    }
    // Check whether slave ACK-d addressing
    if (twi_address_slave(slave_addresss, true) != TWI_RESULT_OK) {
        serial_write("TWI: Slave didn't ACK (ACK-N) addressing!\n\r", 0);
        goto eternal_suffering;
    }
    serial_write("TWI: Slave ACK-d addressing!\n\r", 0);

    mewo();

eternal_suffering:
    // Make sure we never return (this is optimized out so that's a lie)
    while (true);
}