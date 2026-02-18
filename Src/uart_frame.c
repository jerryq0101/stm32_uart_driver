/*
 * uart_frame.c
 *
 *  Created on: Feb 17, 2026
 *      Author: jerryqi
 */
#include <uart_frame.h>
#include <string.h>


// TODO: Understand how CRC works
static uint16_t crc16_update(uint16_t crc, uint8_t b) {
  crc ^= (uint16_t)b << 8;
  for (int i = 0; i < 8; i++) crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  return crc;
}

uint16_t crc16_ccitt_false(const uint8_t* data, uint32_t len) {
  uint16_t crc = 0xFFFF;
  for (uint32_t i = 0; i < len; i++) crc = crc16_update(crc, data[i]);
  return crc;
}

void parser_init(Parser* p) {
	memset(p, 0, sizeof(*p));
	p->st = ST_SOF1;
}

// Parser* p - the parser's existing state
// uint8_t b  - next byte from UART
// Frame* out - the parser puts the completed frame here (once completed)

// ST_SOF1: waiting for SOF1
// ST_SOF2: waiting for SOF2
		// Once we see SOF2 as a next bit, we move to the next one
bool parser_feed(Parser* p, uint8_t b, Frame* out) {
	switch (p->st) {
	case ST_SOF1:
		if (b == SOF1) {
			p->st = ST_SOF2;
		}
		break;
	case ST_SOF2:
		if (b == SOF2) {
			p->st = ST_HDR;
			p->hdr_i = 0;
			p->crc_calc = 0xFFFF;
		} else {
			// Expecting SOF2 but its not SOF2, therefore broken frame
			// Go back to looking for a new frame.
			p->st = ST_SOF1;
		}
		break;
	case ST_HDR:
		p->hdr[p->hdr_i++] = b;
		p->crc_calc = crc16_update(p->crc_calc, b);

		if (p->hdr_i == 5) {
			p->cur.ver = p->hdr[0];
			p->cur.msg = p->hdr[1];
			p->cur.len = p->hdr[2];
			p->cur.seq = p->hdr[3];
			p->cur.flags = p->hdr[4];

			// If too long, we bin this and look for new frame
			if (p->cur.len > FRAME_MAX_PAYLOAD) {
				p->st = ST_SOF1;
				break;
			}

			p->pay_i = 0;

			// If there is no payload, go straight to looking for CRC bytes, otherwise read payload
			p->st = (p->cur.len == 0) ? ST_CRC1 : ST_PAYLOAD;
		}
		break;
	case ST_PAYLOAD:
		p->cur.payload[p->pay_i++] = b;
		p->crc_calc = crc16_update(p->crc_calc, b);

		// If we collected payload to length already
		if (p->pay_i == p->cur.len) {
			p->st = ST_CRC1;
		}
		break;
	case ST_CRC1:
		p->crc_lo = b;
		p->st = ST_CRC2;
		break;
	case ST_CRC2: {
		uint16_t crc_rx = (uint16_t) p->crc_lo | ((uint16_t) b << 8);

		// If sender's crc calculation is equal to ours, then we good.
		if (crc_rx == p->crc_calc) {
			p->cur.crc = crc_rx;
			*out = p->cur;	// Full struct copy, so won't get overwritten by the next message if fast.
			p->st = ST_SOF1;
			return true;
		}
		// CRC failed bin this
		p->st = ST_SOF1;
		break;
	}
	}
	return false;
}


// Builder: Takes a frame and turns it into raw bytes to send over UART
	// The above parser had received bytes and turns it into a frame
	// frame_build takes frame and turns it into raw bytes to send to UART
// Before sending ACK(seq = 5), must convert it into a on wire format
// out - where the UART frame will be written
// ver, msg, seq, flags, are the headers
// payload is the payload buffer being sent (no modification in this function)
// len is how many bytes in the payload
size_t frame_build(uint8_t *out, uint8_t ver, uint8_t msg, uint8_t seq, uint8_t flags, const uint8_t *payload, uint8_t len) {
	out[0] = SOF1;
	out[1] = SOF2;
	out[2] = ver;
	out[3] = msg;
	out[4] = len;
	out[5] = seq;
	out[6] = flags;

	// Copying in payload's length bytes to UART frame
	if (len && payload) {
		memcpy(&out[7], payload, len);
	}

	// Computing the crc on our end
	uint16_t crc = crc16_ccitt_false(&out[2], 5+len);

	// Bottom CRC first and top part second (Little endian)
	out[7 + len] = (uint8_t) (crc & 0xFF);
	out[8 + len] = (uint8_t) ((crc >> 8) & 0xFF);
	return 9 + len;
}





