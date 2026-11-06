#pragma once

#ifndef __COOL_TWI_HEADER_GUARD__
#define __COOL_TWI_HEADER_GUARD__

#include "types.h"

typedef enum {
    TWI_RESULT_GENERAL_ERROR = -10,
    TWI_RESULT_CLOCK_NOT_SET = -2,
    TWI_RESULT_TRANS_ERROR = -1,
    TWI_RESULT_OK = 0,
    TWI_RESULT_REPSTART = 1,
    TWI_RESULT_NACK = 2,
} TwiResult;

// Sets the clock rate of the TWI hardware. Without this other TWI
// calls will not work.
//
// Bitrate is a number between 0 and 255 (0 disables it). Prescaler must be either:
// 1, 4, 16, 64 (if it's not one of those 1 is used).
//
// SCL frequency = 16 000 000 / (16 + 2 * TWBR * PRESCALER)
void twi_set_clock(u8 bitrate, u8 prescaler);

// Starts a start, or a repeated start condition
// (if later TWI_RESULT_REPSTART is returned) on the TWI.
//
// If TWI_RESULT_OK is returned that means an normal start
// condition has been transmitted successfully.
TwiResult twi_send_start();

// Sends a stop condition.
//
// TWI_RESULT_OK or TWI_RESULT_CLOCK_NOT_SET is returned.
TwiResult twi_send_stop();

// Address a slave by sending a SLA+R/W packet and seeing if the
// slave acknowledges it (in which case TWI_RESULT_OK is returned,
// if the slave doesn't ack the addressing TWI_RESULT_NACK is returned)
//
// Should be sent only after the clock is set and a start condition
// is transmitted.
TwiResult twi_address_slave(u8 slave_address, bool should_write);

// Transmits 1 byte of data.
//
// TWI_RESULT_TRANS_ERROR is returned when the byte wans't
// transmitted successfully, and TWI_RESULT_CLOCK_NOT_SET when
// the clock rate wasn't set yet.
TwiResult twi_send_byte(u8 byte);

// Sends `size` bytes of `data` through the TWI interface, writing out how
// much bytes have been transfered into bytes_transmitted. Bytes transmitted
// is also an offset indicator.
//
// TWI_RESULT_CLOCK_NOT_SET is returned and no data is sent when the clockrate
// wasn't set yet (see `twi_set_clock`).
TwiResult twi_send_bytes(const void* data, u32 size, u32* bytes_transmitted);

#endif // __COOL_TWI_HEADER_GUARD__