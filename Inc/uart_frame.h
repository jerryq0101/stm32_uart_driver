/*
 * uart_frame.h
 *
 *  Created on: Feb 17, 2026
 *      Author: jerryqi
 */

#ifndef INC_UART_FRAME_H_
#define INC_UART_FRAME_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SOF1 0xAA
#define SOF2 0x55
#define FRAME_MAX_PAYLOAD 64	// 64 bytes max size

typedef struct {
	uint8_t ver, msg, len, seq, flags;	// Header
	uint8_t payload[FRAME_MAX_PAYLOAD];	// Payload
	uint16_t crc;						// CRC
} Frame;


// Frame Parsing state
// Need two states for SOF as first waiting for it
typedef enum {
	ST_SOF1, ST_SOF2, ST_HDR, ST_PAYLOAD, ST_CRC1, ST_CRC2
} ParseState;

typedef struct {
	ParseState st;
	uint8_t hdr[5];	// Header
	uint8_t hdr_i;	// how many of the 5 header bytes collected so far
	uint8_t pay_i;	// how many payload bytes so far...
	Frame cur;
	uint16_t crc_calc;
	uint8_t crc_lo;
} Parser;

void parser_init(Parser* p);
bool parser_feed(Parser* p, uint8_t b, Frame* out);

uint16_t crc16_ccitt_false(const uint8_t* data, uint32_t len);
size_t frame_build(uint8_t* out, uint8_t ver, uint8_t msg, uint8_t seq, uint8_t flags, const uint8_t* payload, uint8_t len);



#endif /* INC_UART_FRAME_H_ */
