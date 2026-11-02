#ifndef __COOL_SERIAL_HEADER_GUARD__
#define __COOL_SERIAL_HEADER_GUARD__

#include "types.h"

typedef enum {
    BAUDRATE_DEFAULT = 7, // Same as 250k
    BAUDRATE_2400 = 832,
    BAUDRATE_4800 = 416,
    BAUDRATE_9600 = 207,
    BAUDRATE_14_4K = 138,
    BAUDRATE_19_2k = 103,
    BAUDRATE_28_8K = 68,
    BAUDRATE_38_4K = 51,
    BAUDRATE_57_6K = 34,
    BAUDRATE_76_8K = 25,
    BAUDRATE_115_2K = 16,
    BAUDRATE_230_4K = 8,
    BAUDRATE_250K = 7,
    BAUDRATE_500K = 3,
    BAUDRATE_1M = 1
} Baudrate;

// Initialize the serial communication over USB. This must be called before
// any other serial_ function is called.
void serial_init(Baudrate baudrate);

// Writes size amount of bytes from data into the serial port. If
// (and only if) size is 0 data is writen until a nullterminator
// (aka a byte that is 0) is encountered.
//
// -1 is returned if serial_init wasn't called before this.
i8 serial_write(const void* data, u32 size);

// Read the serial port into the buffer given buffer and return how many
// bytes were read. The read will stop if the given buffer runs out of space,
// or when `use_delim` is true and `delim` is encountered.
//
// The buffer is not null terminated by this function.
//
// 0 is returned if buffer_size is 0, or `serial_init()` wasn't called before this.
u32 serial_read(void* buffer, u32 buffer_size, bool use_delim, u8 delim);

#endif // __COOL_SERIAL_HEADER_GUARD__