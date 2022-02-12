//
// Created by Samuel Jones on 2/10/22.
//

#include "ir.h"
#include "nec_transmit.h"
#include "nec_receive.h"
#include "pinout_rp2040.h"
#include "hardware/irq.h"

#define IR_PIO (pio0)

#define START_MASK (1<<15)
#define START_DEVICE_ADDRESS_BITS (10)
#define START_DEVICE_ADDRESS_SHIFT (5)
#define START_DEVICE_ADDRESS_MASK (((1<<START_DEVICE_ADDRESS_BITS)-1) << START_DEVICE_ADDRESS_SHIFT)

#define START_APP_ID_BITS (5)
#define START_APP_ID_MASK ((1<<5)-1)

#define CONTINUATION_MASK (1<<14)

static int tx_sm;
static int rx_sm;

ir_data_callback cb[IR_MAX_ID] = {};

typedef struct {
    uint8_t command;
    uint8_t address;
} NEC_IR_PACKET;

void irq_fifo_handler(void) {
    while (!pio_sm_is_rx_fifo_empty(IR_PIO, rx_sm)) {
        //uint32_t rx_data =
                pio_sm_get(IR_PIO, rx_sm);

        //NEC_IR_PACKET rx_packet;
        //bool success = nec_decode_frame(rx_data, &rx_packet.address, &rx_packet.command);
        //uint16_t raw_packet = rx_packet.address << 8 | rx_packet.command;

        // TODO parse incoming packets!
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

void ir_set_callback(ir_data_callback data_cb, IR_APP_ID app_id) {
    cb[app_id] = data_cb;
}

bool ir_transmitting(void) {
    return pio_sm_is_tx_fifo_empty(IR_PIO, tx_sm);
}

static void ir_send_start_packet(const IR_DATA *data) {
    // TODO implement
    //uint16_t packet_data = 0;
    //NEC_IR_PACKET packet;

    //uint32_t raw_data = nec_encode_frame(packet.address, packet.command);
    //pio_sm_put(IR_PIO, tx_sm, raw_data);
}

void ir_send_complete_message(const IR_DATA *data) {
    // TODO implement
}

void ir_send_partial_message(const IR_DATA *data, uint8_t starting_sequence_num, bool is_end) {
    // TODO implement
}


bool ir_messages_seen(bool reset) {
    // TODO implement
    return false;
}

int ir_message_count(void) {
    // TODO implement
    return 0;
}