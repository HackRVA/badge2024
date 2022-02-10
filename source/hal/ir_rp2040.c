//
// Created by Samuel Jones on 2/10/22.
//

#include "ir.h"
#include "nec_transmit.h"
#include "nec_receive.h"
#include "pinout_rp2040.h"
#include "hardware/irq.h"

#define IR_PIO (pio0)

static int tx_sm;
static int rx_sm;

ir_data_callback cb = NULL;

void irq_fifo_handler(void) {
    while (!pio_sm_is_rx_fifo_empty(IR_PIO, rx_sm)) {
        uint32_t rx_data = pio_sm_get(IR_PIO, rx_sm);

        IR_PACKET rx_packet;
        bool success = nec_decode_frame(rx_data, &rx_packet.address, &rx_packet.command);

        if (success && cb) {
            cb(rx_packet);
        }
    }
}

void ir_init(void) {
    rx_sm = nec_rx_init(IR_PIO, BADGE_GPIO_IR_RX);
    tx_sm = nec_tx_init(IR_PIO, BADGE_GPIO_IR_TX);

    irq_add_shared_handler(PIO0_IRQ_1, irq_fifo_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    enum pio_interrupt_source irq_source = pis_sm0_rx_fifo_not_empty + rx_sm;
    pio_set_irq1_source_enabled(IR_PIO, irq_source, true);
    irq_set_enabled(PIO0_IRQ_1, true);
}

void ir_set_callback(ir_data_callback data_cb) {
    cb = data_cb;
}

void ir_enqueue(IR_PACKET packet) {
    uint32_t packet_data = nec_encode_frame(packet.address, packet.command);
    pio_sm_put(IR_PIO, tx_sm, packet_data);
}
