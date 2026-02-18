/*
 * protocol.h
 *
 *  Created on: Feb 17, 2026
 *      Author: jerryqi
 */

#ifndef INC_PROTOCOL_H_
#define INC_PROTOCOL_H_

// Common Communication message types

typedef enum {
		MSG_CMD    = 0x10,	// Command request (payload = op and params)
		MSG_CFG    = 0x11,	// Configuration / set param (payload = key + value)
		MSG_PING   = 0x12,	// Liveness Check (Payload optional)
		MSG_TIME   = 0x13,	// Time sync (Payload = unix/us)

		MSG_ACK    = 0x20,	// ACK for CMD/CFG/PING/TIME (payload = status + code)
		MSG_TELEM  = 0x21,	// telemetry packet (payload = stream_id + data)
		MSG_EVENT  = 0x22,	// async event/log (payload = event_id + data)
		MSG_ERR    = 0x23,	// async error/fault (payload = err_code + context)
} msg_type_t;

typedef enum {
		ACK_ACCEPTED	= 0x01,	// Received and queued
		ACK_REJECTED	= 0x02,	// invalid / not allowed
		ACK_BUSY		= 0x03,	// Cannot accept right now..
		ACK_DONE_OK		= 0x04,	// Completed successfully
		ACK_DONE_FAIL	= 0x05,	// Completed with failure
		ACK_TIMEOUT		= 0x06,	// Downstream timeout
		ACK_PING		= 0x07	// respond to a ping
} ack_status_t;


#endif /* INC_PROTOCOL_H_ */
