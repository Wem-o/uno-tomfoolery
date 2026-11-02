#include "types.h"
#include "serial.h"

#include <avr/io.h>
#include <stdbool.h>

/// The passed in baud rate should greater than 0 and less than 2^12-1
void UART_init(unsigned int UBRR, bool should_use_double_speed) {
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
void serial_init(Baudrate baudrate) {
    // U2X (double speed) is always enabled as it gives smaller
    // rounding errors for 16mhz cpu speeds
    UART_init(baudrate ? baudrate : BAUDRATE_DEFAULT, true);
}

// Writes size amount of bytes from data into the serial port. If
// (and only if) size is 0 data is writen until a nullterminator
// (aka a byte that is 0) is encountered.
i8 serial_write(const void* data, u32 size) {
    // Check if serial communication and data transmittion is enabled
    if (!UART_is_tx_enabled()) return -1;

    if (size != 0) {
        for (u32 i = 0; i < size; ++i) {
            // Wait for the buffer to become empty (aka transmitted)
            while (!(UCSR0A & (1<<UDRE0)));

            UDR0 = *((u8*)data + i);
        }
    } else {
        for (u32 i = 0; *((u8*)data + i) != '\0'; i++) {
            // Wait for the buffer to become empty
            while (!(UCSR0A & (1<<UDRE0)));

            UDR0 = *((u8*)data + i);
        }
    }

    return 0;
}

// Read the serial port into the buffer given buffer and return how many
// bytes were read. The read will stop if the given buffer runs out of space,
// or when `use_delim` is true and `delim` is encountered.
//
// The buffer is not null terminated by this function.
//
// 0 is returned if buffer_size is 0, or `serial_init()` wasn't called before this.
u32 serial_read(void* buffer, u32 buffer_size, bool use_delim, u8 delim) {
    if (buffer_size == 0 || !UART_is_rx_enabled()) return 0;

    u32 read_bytes = 0;
    for (; buffer_size; --buffer_size) {
        // Busy wait until the byte is received
        while (!(UCSR0A & (1<<RXC0)));

        u8* buff = (u8*)buffer + read_bytes;

        *buff = UDR0;
        read_bytes++;
        if (use_delim && *buff == delim) {
            return read_bytes;
        }
    }
    return read_bytes;
}