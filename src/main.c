#include "serial.h"
#include "twi.h"

#include <avr/io.h>

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

    const u8 HDRS = 0;
    const u8 HDRW = 1;
    const u8 HDEN = 2;
    const u8 HDBLENA = (1<<3);
    const u8 HDDATA_MASK = 0b11110000;

    // == Blink cursor ==
    u8 x = (1<<HDEN);
    u8 msg[] = {x, 0 | HDBLENA, x | 0b11110000, 0 | HDBLENA};
    twi_send_bytes(msg, sizeof(msg), 0);
    twi_send_stop();
    serial_write("Blinky sent", 0);


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