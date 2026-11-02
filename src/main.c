#include <avr/io.h>

#include <stdio.h>
#include <stddef.h>
#include <string.h>

/// The passed in baud rate should greater than 0 and less than 2^12-1
void UART_init(unsigned int UBRR, char should_use_double_speed) {
    // Disable interrupts while enabling USART
    SREG &= 0b01111111;

    UBRR0H = (unsigned char)(UBRR >> 8);
    UBRR0L = (unsigned char)UBRR;

    if (should_use_double_speed) {
        UCSR0A |= 1<<U2X0;
    }

    // Enable receiver and transmitter
    UCSR0B |= (1<<RXEN0) | (1<<TXEN0);
    // Set the mode to async usart, parity mode to disabled
    // stop bit to be a single one, and frame size to 8 bits
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

short UART_is_tx_enabled() {
    if (UCSR0B & (1<<TXEN0)) return 1;
    else return 0;
}

short UART_is_rx_enabled() {
    if (UCSR0B & (1<<RXEN0)) return 1;
    else return 0;
}

/// Initialize the serial communication over USB.
void serial_init() {
    // 3 or 7 if u2xn is enabled which for 250k baudrate
    // (with 0% rounding error for clock generation)
    UART_init(7, 1);
}

// Write size bytes to serial from data. If size is 0 bytes will be writen
// until \0 is encountered.
char write_serial(const char* data, size_t size) {
    // Check if serial communication and data transmittion is enabled
    if (!UART_is_tx_enabled()) return -1;

    if (size != 0) {
        for (size_t i = 0; i < size; ++i) {
            // Wait for the buffer to become empty
            while (!(UCSR0A & (1<<UDRE0)));
            UDR0 = *(data + i);
        }
    } else {
        unsigned int i = 0;
        const char* byte = data;
        while (*byte != '\0') {
            // Wait for the buffer to become empty
            while (!(UCSR0A & (1<<UDRE0)));
            UDR0 = *byte;
            i++;
            byte = data + i;
        }
    }

    return 0;
}

/// Read the serial connection into the buffer until delim is
// encountered and return the amount of read bytes or -1 on error.
long int read_serial_until(char* buffer, long int buffer_size, char delim, char include_delim) {
    if (buffer_size <= 1) return 0;
    if (!UART_is_rx_enabled()) return -1;

    long int read_bytes = 0;
    for (; buffer_size != 1; --buffer_size) {
        // Busy wait until the byte is received
        while (!(UCSR0A & (1<<RXC0)));

        read_bytes++;
        *buffer = UDR0;
        if (*buffer == delim) {
            *(buffer + (include_delim ? 1 : 0)) = '\0';
            return read_bytes;
        }

        buffer++;
    }
    *buffer = '\0';
    return read_bytes;
}

int main(void) {
    // Turn off the onboard LED
    DDRB |= 0<<PB5;

    serial_init();
    write_serial("Welcome to hell!\n\r", 0);

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
        write_serial("TWI: START signal has been transmitted successfully!\n\r", 0);

    } else if (status == 0x10) {
        write_serial("TWI: A REPEATED START signal has been transmitted successfully\n\r",0 );
    }
    else {
        write_serial("TWI: Failed to send START/REPEATED START signal!\n\r", 0);
        goto eternal_suffering;
    }

    // Prepare SLA+W (in the TWDR)
    TWDR = (slave_addresss<<1) | 0;

    // Address the slave
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Check whether slave ACK-d addressing
    if ((TWSR & 0xF8) != 0x18) {
        write_serial("TWI: Slave didn't ACK (ACK-N) addressing!\n\r", 0);

        goto eternal_suffering;
    }

    write_serial("TWI: Slave ACK-d addressing!\n\r", 0);

    // Make sure we never return
eternal_suffering:
    for (;;) {}
    return 0;
}