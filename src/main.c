#include "serial.h"
#include "twi.h"

#include <avr/io.h>

void mewo() {
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
    const u8 HD_FUNCTION_SET_5x10_FONT = 1<< 2;

    typedef enum {
        HdCls = 1,
        HdReturnHome = 2,
        HdSetEntryMode = 4,
        HdDisplayControl = 8,
        HdCursorShift = 16,
        HdFunctionSet = 32,
        HdSetCGRamAddress = 64,
        HdSetDDRamAddress = 128
    } HdCommand;

    // == Say hello wold ==
    u8 commands[] = {
    //  CMD?    Value
        true,   HdFunctionSet | HD_FUNCTION_SET_4BIT_MODE | HD_FUNCTION_SET_2LINE_MODE,
        true,   HdCls,
        true,   HdReturnHome,
        true,   HdDisplayControl | (1<<HD_DISPLAY_CONTROL_CURSOR) | (1<<HD_DISPLAY_CONTROL_DISPLAY_ENABLE),
        true,   HdSetDDRamAddress,
        false,  0b01001000, // H
        false,  0b01100101, // e
        false,  0b01101100, // l
        false,  0b01101100, // l
        false,  0b01101111, // o
    };
    usize cmd_size = sizeof(commands);

    u8 msg[cmd_size * 2] = {};
    u16 msg_index = 0;

    for (u16 i = 1; i < cmd_size; i += 2) {
        // char thing[100];
        // sprintf(thing, "%i. %x\n\r", i, commands[i]);
        // serial_write(thing, 0);

        for (u8 count = 0; count < 2; count++) {
            u8 data = count == 1 ? commands[i] << 4 : commands[i] & 0b11110000;
            msg[msg_index] = data | HD_ENA | HD_BL_ENA | (!commands[i - 1] ? 1 : 0);
            msg_index++;

            // thing[0] = 0;
            // sprintf(thing, "\tI: %i - Count: %i. Data: %x  msg[msg_index]: %x\n\r", i, count, data, msg[msg_index]);
            // serial_write(thing, 0);

            msg[msg_index] = 0 | HD_BL_ENA;
            msg_index++;
        }
    }

    TwiResult res = twi_send_bytes(msg, sizeof(msg));
    // twi_send_stop();
    serial_write("Demo sent", 0);
}

int main(void) {
    // Turn off the onboard LED
    DDRB = 0<<PB5;

    serial_init(0);
    serial_write("Welcome to hell!\n\r", 0);

    // Synchronous TWI implementation (no interrupts)
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
            goto eternal_suffering;
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

    // // Send enable + 0000 for data bit upper first
    // twi_send_byte(x);

    // // Send no enable
    // twi_send_byte(0 | HDBLENA);

    // // Send enable + 1DCB
    // twi_send_byte(x | 0b11110000);

    // // Send no enable
    // twi_send_byte(0 | HDBLENA);

eternal_suffering:
    // Make sure we never return
    while (true);
}