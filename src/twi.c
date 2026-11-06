#include "twi.h"
#include "types.h"

#include <avr/io.h>

// Sets the clock rate of the TWI hardware. Without this other TWI
// calls will not work.
//
// Bitrate is a number between 0 and 255 (0 disables it). Prescaler must be either:
// 1, 4, 16, 64 (if it's not one of those 1 is used).
//
// SCL frequency = 16 000 000 / (16 + 2 * TWBR * PRESCALER)
void twi_set_clock(u8 bitrate, u8 prescaler) {
    TWBR = bitrate;

    switch (prescaler) {
        case 4:
            TWSR |= (1<<TWPS0);
            break;
        case 16:
            TWSR |= (1<<TWPS1);
            break;
        case 64:
            TWSR |= (1<<TWPS0) | (1<<TWPS1);
            break;
        default:
            TWSR &= 0b11111100;
            break;
    }
}

bool twi_is_clock_set() {
    return TWBR;
}

// Starts a start, or a repeated start condition
// (if later TWI_RESULT_REPSTART is returned) on the TWI.
//
// If TWI_RESULT_OK is returned that means an normal start
// condition has been transmitted successfully.
TwiResult twi_send_start() {
    if (!twi_is_clock_set()) return TWI_RESULT_CLOCK_NOT_SET;

    // Queue up and enable start condition
    TWCR = (1<<TWEN) | (1<<TWSTA) | (1<<TWINT);
    // Wait for the start condition to be transmitted
    while (!(TWCR & (1<<TWINT)));

    // Check if start was sent successfully
    u8 status = (TWSR & 0xF8);
    if      (status == 0x08)    return TWI_RESULT_OK;
    else if (status == 0x10)    return TWI_RESULT_REPSTART;
    else                        return TWI_RESULT_GENERAL_ERROR;
}

// Sends a stop condition.
//
// TWI_RESULT_OK or TWI_RESULT_CLOCK_NOT_SET is returned.
TwiResult twi_send_stop() {
    if (!twi_is_clock_set()) return TWI_RESULT_CLOCK_NOT_SET;

    // Queue up and enable start condition
    TWCR = (1<<TWEN) | (1<<TWSTO) | (1<<TWINT);
    // Wait for the start condition to be transmitted
    while (!(TWCR & (1<<TWINT)));

    return TWI_RESULT_OK;
}

// Address a slave by sending a SLA+R/W packet and seeing if the
// slave acknowledges it (in which case TWI_RESULT_OK is returned,
// if the slave doesn't ack the addressing TWI_RESULT_NACK is returned)
//
// Should be sent only after the clock is set and a start condition
// is transmitted.
TwiResult twi_address_slave(u8 slave_address, bool should_write) {
    if (!twi_is_clock_set()) return TWI_RESULT_CLOCK_NOT_SET;

    // Prepare SLA+R/W (0 is write, 1 is read)
    TWDR = (slave_address<<1) | (should_write ? 0 : 1);

    // Address the slave
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Check whether slave (N)ACK-d addressing
    unsigned char status = (TWSR & 0xF8);
    if      (status == 0x18)    return TWI_RESULT_OK;
    else if (status == 0x20)    return TWI_RESULT_NACK;
    else                        return TWI_RESULT_TRANS_ERROR;
}

// Transmits 1 byte of data.
//
// TWI_RESULT_TRANS_ERROR is returned when the byte wans't
// transmitted successfully, and TWI_RESULT_CLOCK_NOT_SET when
// the clock rate wasn't set yet and general error may also be
// returned.
TwiResult twi_send_byte(u8 byte) {
    if (!twi_is_clock_set()) return TWI_RESULT_CLOCK_NOT_SET;

    TWDR = byte;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    u8 status = TWSR & 0xF8;
    if      (status == 0x28)     return TWI_RESULT_OK;
    else if (status == 0x30)     return TWI_RESULT_TRANS_ERROR;
    else                         return TWI_RESULT_GENERAL_ERROR;
}

// Sends `size` bytes of `data` through the TWI interface, writing out how
// much bytes have been transfered into bytes_transmitted. Bytes transmitted
// is also an offset indicator.
//
// TWI_RESULT_CLOCK_NOT_SET is returned and no data is sent when the clockrate
// wasn't set yet (see `twi_set_clock`).
TwiResult twi_send_bytes(const void* data, u32 size) {
    if (!twi_is_clock_set()) return TWI_RESULT_CLOCK_NOT_SET;

    u32 x = 0;

    TwiResult result;
    for (; size; size--) {
        result = twi_send_byte(*((u8*)data + x));
        switch (result) {
            case TWI_RESULT_OK:
                x++;
                continue;
            case TWI_RESULT_TRANS_ERROR:
            case TWI_RESULT_GENERAL_ERROR:
                return result;
            default:
                // This shouldn't be possible to reach!
                break;
        }
    }

    return 0;
}