#ifndef I2C_H
#define	I2C_H

#include <xc.h>

// Defining this macro will enable debug code that logs the I2C interrupts and operations.
//#define _I2C_TRACE

///
/// Holds information about a write-with-callback operation.
struct WriteCallbackContext
{
    uint8_t id; ///< A caller-defined id for the operation.
    uint16_t count; ///< A callback counter, incremented by the I2C handler.
    
    uint8_t data; ///< The next byte that should be sent, set by the callback handler.
};

///
/// The function signature for the callback method used with a write-with-callback operation.
/// @param context Holds information about the operation.
/// @returns 1 when passing back data to be written, 0 if there is no data to be written.
typedef uint8_t (WriteCallback)(struct WriteCallbackContext* context);

///
/// Initializes the I2C pins and peripherals as a host device.
void I2C_Host_Init(void);

/// Writes data to a client.
///
/// @param address The I2C address of the client.
/// @param data A pointer to the data to send.
/// @param len The length of the data, in bytes.
/// @NOTE This call blocks until the write completes or fails.
void I2C_Write(uint8_t address, const void* data, uint8_t len);

/// Starts a write to a client with data supplied by a callback function.
///
/// @param address The I2C address of the client.
/// @param callback A pointer to the callback function.
/// @param context A pointer to a context structure containing information about the operation.
/// @NOTE This call blocks until the write completes or fails.
/// @NOTE Callbacks are called in the interrupt context as data is needed.
void I2C_WriteWithCallback(uint8_t address, WriteCallback* callback, struct WriteCallbackContext* context);

/// Reads from a client.
///
/// @param address The I2C address of the client.
/// @param data A pointer to a buffer that will be filled with the data read.
/// @param len The length of the buffer, in bytes.
/// @NOTE This call blocks until the read completes or fails.
void I2C_Read(uint8_t address, void* data, uint8_t len);

/// Writes data to a client, then reads back a response.
///
/// @param address The I2C address of the client.
/// @param writeData A pointer to the data to send.
/// @param writeLen The length of the data, in bytes.
/// @param readData A pointer to a buffer that will be filled with the data read.
/// @param readLen The length of the buffer, in bytes.
/// @NOTE This call blocks until the write/read completes or fails.
///
/// @NOTE The write and read I2C operations are separated with a restart command.
void I2C_WriteRead(uint8_t address, const void* writeData, uint8_t writeLen, void* readData, uint8_t readLen);

///
/// Called by the ISR to process I2C interrupts.
void I2C_HandleInterrupt(void);

///
/// @returns The cumulative error count.
uint8_t I2C_GetErrorCount(void);

///
/// @returns The cumulative reset count.
uint8_t I2C_GetResetCount(void);

#endif	/* I2C_H */

