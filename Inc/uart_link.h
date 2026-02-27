/*
 * uart_link.h
 *
 *  Created on: Feb 18, 2026
 *      Author: jerryqi
 */

#ifndef INC_UART_LINK_H_
#define INC_UART_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#include "uart_frame.h"
#include "stm32l4xx_hal.h"	// TODO: Switch particular STM board
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef void (*uart_link_frame_cb_t)(void *user, const Frame *f);

typedef struct {
	UART_HandleTypeDef *huart;

        // Utilities
        Parser parser;

	// RX DMA ring
	uint8_t *rx_dma;
	uint16_t rx_dma_sz;			// dma buffer size
	volatile uint16_t rx_rd;	// where did we read until

        // RX (queue)
        QueueHandle_t frame_q;

	// TX (queue)
	QueueHandle_t tx_q;

	// Tasks
	TaskHandle_t rx_task;
	TaskHandle_t tx_task;
} uart_link_t;

// Create Tasks/queues, initialize parser, doesn't start DMA yet
bool uart_link_init(uart_link_t *l,
                    UART_HandleTypeDef *huart,
                    uint8_t *rx_dma_buf, uint16_t rx_dma_sz,
                    UBaseType_t rx_prio, uint16_t rx_stack_words,
                    UBaseType_t tx_prio, uint16_t tx_stack_words,
                    UBaseType_t tx_q_len);

// Arms ReceiveToIdle DMA + disables HT interrupt if needed
bool uart_link_start(uart_link_t *l, bool disable_half_transfer_it);

// Call from HAL_UARTEx_RxEventCallback (ISR Context), job of waking up rx task (and context switching if high priority)
void uart_link_rx_event_isr(uart_link_t *l, UART_HandleTypeDef *huart, uint16_t size);

// Enqueue a frame for TX (Task context)
bool uart_link_send(uart_link_t *l, const Frame *f, TickType_t to);

#endif /* INC_UART_LINK_H_ */
