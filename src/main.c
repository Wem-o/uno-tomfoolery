#include "serial.h"
#include "types.h"

#include <avr/io.h>

int main(void) {
    // Turn off the onboard LED
    DDRB |= 0<<PB5;

    serial_init();
    serial_write("Welcome to hell!\n\r", 0);

    // Synchronous TWI implementation (no interrupts)
    unsigned char slave_addresss = 0x27;

    // // Set up clock generator and prescaler
    // // To get around 5k hertz 100 TWBR and 16 prescaling is used
    TWBR = 100;
    TWSR |= (1<<TWPS0);

    // Enable TWI, queue up and send start condition (send is done by writing 1 to TWINT)
    TWCR = (1<<TWEN) | (1<<TWSTA) | (1<<TWINT);
    // Wait for the start condition to be transmitted
    while (!(TWCR & (1<<TWINT)));
    // Check if start was sent successfully
    unsigned char status = (TWSR & 0xF8);
    if (status == 0x08) {
        serial_write("TWI: START signal has been transmitted successfully!\n\r", 0);

    } else if (status == 0x10) {
        serial_write("TWI: A REPEATED START signal has been transmitted successfully\n\r",0 );
    }
    else {
        serial_write("TWI: Failed to send START/REPEATED START signal!\n\r", 0);
        goto eternal_suffering;
    }

    // Prepare SLA+W (in the TWDR)
    TWDR = (slave_addresss<<1) | 0;

    // Address the slave
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Check whether slave ACK-d addressing
    if ((TWSR & 0xF8) != 0x18) {
        serial_write("TWI: Slave didn't ACK (ACK-N) addressing!\n\r", 0);

        goto eternal_suffering;
    }

    serial_write("TWI: Slave ACK-d addressing!\n\r", 0);

    // Make sure we never return
eternal_suffering:
    for (;;) {}
    return 0;
}