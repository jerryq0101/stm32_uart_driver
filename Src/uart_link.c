/*
 * uart_link.c
 *
 *  Created on: Feb 18, 2026
 *      Author: jerryqi
 */

#include "uart_link.h"

// ----------------------------
// Internal helpers
// ----------------------------

// Finding the current written to index
static inline uint16_t dma_wr_idx(const uart_link_t *l)
{
    // DMA NDTR = remaining transfers until wrap/end
    // bytes written so far = sz - NDTR
    return (uint16_t)(l->rx_dma_sz - __HAL_DMA_GET_COUNTER(l->huart->hdmarx));
}

// increment ring buffer index safely
static inline uint16_t inc_idx(uint16_t idx, uint16_t sz)
{
    // Works for any sz (power-of-two not required)
    idx++;
    if (idx >= sz) {
    	idx = 0;
    }
    return idx;
}

// ----------------------------
// RX / TX tasks
// ----------------------------

static void uart_rx_task_entry(void *arg)
{
    uart_link_t *l = (uart_link_t *)arg;
    Frame f;

    for (;;)
    {
        // Wait until ISR/callback tells us new bytes are available (IDLE interrupt)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Written to index
        uint16_t wr = dma_wr_idx(l);

        // Drain ring until we catch up to DMA write index
        while (l->rx_rd != wr)
        {
            uint8_t b = l->rx_dma[l->rx_rd];
            l->rx_rd = inc_idx(l->rx_rd, l->rx_dma_sz);

            // parser_feed returns true only when a full frame is complete
            if (parser_feed(&l->parser, b, &f))
            {
            	xQueueSend(l->frame_q, &f, 0);	// Send onto the frame
                // else drop
            }
        }
    }
}

static void uart_tx_task_entry(void *arg)
{
    uart_link_t *l = (uart_link_t *)arg;
    Frame f;
    uint8_t out[128];

    for (;;)
    {
    	// There is something on the dispatch queue
        if (xQueueReceive(l->tx_q, &f, portMAX_DELAY) == pdTRUE)
        {
            // Build wire bytes into provided txbuf
            size_t n = frame_build(out, f.ver, f.msg, f.seq, f.flags, f.payload, f.len);

            // Blocking transmit in TX task (single writer)
            (void)HAL_UART_Transmit(l->huart, out, (uint16_t)n, 100);
        }
    }
}

// ----------------------------
// Public API
// ----------------------------

bool uart_link_init(uart_link_t *l,
                    UART_HandleTypeDef *huart,
                    uint8_t *rx_dma_buf, uint16_t rx_dma_sz,
                    UBaseType_t rx_prio, uint16_t rx_stack_words,
                    UBaseType_t tx_prio, uint16_t tx_stack_words,
                    UBaseType_t tx_q_len)
{
    if (!l || !huart || !rx_dma_buf || rx_dma_sz == 0){
    	return false;
    }

    l->huart      = huart;

    l->rx_dma     = rx_dma_buf;
    l->rx_dma_sz  = rx_dma_sz;
    l->rx_rd      = 0;

    parser_init(&l->parser);

    l->frame_q = xQueueCreate(tx_q_len, sizeof(Frame));
    l->tx_q = xQueueCreate(tx_q_len, sizeof(Frame));

    if (!l->tx_q) {
        return false;
    }

    // Create RX task
    if (xTaskCreate(uart_rx_task_entry, "uart_rx",
                    rx_stack_words, l, rx_prio, &l->rx_task) != pdPASS)
    {
        return false;
    }

    // Create TX task
    if (xTaskCreate(uart_tx_task_entry, "uart_tx",
                    tx_stack_words, l, tx_prio, &l->tx_task) != pdPASS)
    {
        return false;
    }

    return true;
}

bool uart_link_start(uart_link_t *l, bool disable_half_transfer_it)
{
    if (!l || !l->huart || !l->huart->hdmarx){
        return false;
    }

    // Kick RX using ReceiveToIdle DMA.
    // IMPORTANT: Configure DMA RX as CIRCULAR in CubeMX for ring behavior.
    if (HAL_UARTEx_ReceiveToIdle_DMA(l->huart, l->rx_dma, l->rx_dma_sz) != HAL_OK){
        return false;
    }

    if (disable_half_transfer_it)
    {
        __HAL_DMA_DISABLE_IT(l->huart->hdmarx, DMA_IT_HT);
    }

    return true;
}

void uart_link_rx_event_isr(uart_link_t *l, UART_HandleTypeDef *huart, uint16_t size)
{
    if (!l || !huart) {
    	return;
    }
    if (huart != l->huart) {
    	return;
    }

    BaseType_t hpw = pdFALSE;
    vTaskNotifyGiveFromISR(l->rx_task, &hpw);
    portYIELD_FROM_ISR(hpw);
}

// Put a frame onto dispatch queue for sending
bool uart_link_send(uart_link_t *l, const Frame *f, TickType_t to)
{
    if (!l || !l->tx_q || !f) {
    	return false;
    }
    return (xQueueSend(l->tx_q, f, to) == pdTRUE);
}


