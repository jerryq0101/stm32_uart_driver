/*
 * egse_uart.h
 *
 *  Created on: Feb 17, 2026
 *      Author: jerryqi
 */

#ifndef INC_EGSE_UART_H_
#define INC_EGSE_UART_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "uart_frame.h"   // Frame type

// Call once after MX_* init
void egse_uart_init(void);

// Non-blocking: returns true if a complete frame is available
bool egse_uart_try_pop(Frame *out);

// Non-blocking send (uses HAL_UART_Transmit for now)
bool egse_uart_send(uint8_t ver, uint8_t msg, uint8_t seq, uint8_t flags,
                    const uint8_t *payload, uint8_t len);

#endif /* INC_EGSE_UART_H_ */
