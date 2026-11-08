#include "serial.h"
#include "twi.h"

#include <avr/io.h>

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

const u8 HD_FIRST_LINE = 0;
const u8 HD_SECOND_LINE = 0x40; // 64
const u8 HD_THIRD_LINE = 0x14; // 20
const u8 HD_FOURTH_LINE = 0x54; // 84
const u8 HD_LINE_ADDRESSED[4] = {
    HD_FIRST_LINE, HD_SECOND_LINE,
    HD_THIRD_LINE, HD_FOURTH_LINE
};

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

// Send a command to the lcd thorugh TWI.
void lcd_send_command(u8 command, bool is_instruction) {
    for (u8 count = 0; count < 2; count++) {
        u8 data = (count == 1) ? command << 4 : command & 0b11110000;
        data |= HD_ENA | HD_BL_ENA | (is_instruction ? 0 : 1);
        twi_send_byte(data);

        // Every data byte is terminated with a 0. HD_BL_ENA's pin isn't
        // connected to the display manager, but to a seperate circuit that
        // handles the backlight, so that should stay high.
        twi_send_byte(0 | HD_BL_ENA);
    }
}

// Send a command buffer to the lcd thorugh TWI.
// Commands must be an array of u8's. Every even element (0, 2, 4...) should
// contain true if the odd indexed element after it is an instruction
// (like clearing the screen) or a data call (like writing into DD or CG ram).
void lcd_send_commands(u8* commands, u16 sizeof_commands) {
    for (u16 i = 1; i < sizeof_commands; i += 2) {
        lcd_send_command(commands[i], commands[i - 1]);
    }
}

void lcd_display_control(bool enable_display, bool show_cursor, bool blink_cursor) {
    u8 data = HdDisplayControl
        | ((enable_display ? 1 : 0)<<HD_DISPLAY_CONTROL_DISPLAY_ENABLE)
        | ((show_cursor ? 1 : 0)<<HD_DISPLAY_CONTROL_CURSOR)
        | ((blink_cursor ? 1 : 0)<<HD_DISPLAY_CONTROL_BLINK);
    lcd_send_command(data, true);
}

void lcd_clear_screen() {
    lcd_send_command(HdCls, true);
}

void lcd_return_home() {
    lcd_send_command(HdReturnHome, true);
}

// Jump to a position on the screen. Both line and col are 0 indexed.
//
// False is returned if invalid input is provided: if line is bigger than 3
// or if col is bigger than 23.
bool lcd_jump_to(u8 line, u8 col) {
    if (line > 3 || col > 23) return false;

    lcd_send_command(HdSetDDRamAddress | (HD_LINE_ADDRESSED[line] + col), true);
    return true;
}

// Writes the passed in string to the lcd and handles wrapping.
void lcd_write_string(char* string) {
    char* byte = string;
    while (*byte != '\0') {
        lcd_send_command(*byte, false);
        byte++;
    }
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

    // == Say hello world ==
    lcd_clear_screen();
    lcd_return_home();
    lcd_jump_to(1, (20 / 2 - 2) - 1);
    lcd_write_string("Hello");
    lcd_jump_to(2, (20 / 2 - 2) - 1);
    lcd_write_string("World!");

    serial_write("Demo sent", 0);

eternal_suffering:
    // Make sure we never return (this is optimized out so that's a lie)
    while (true);
}