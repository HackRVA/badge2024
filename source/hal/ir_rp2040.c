//
// Created by Samuel Jones on 2/10/22.
//

#include "ir.h"
#include "nec_transmit.h"
#include "nec_receive.h"
#include "pinout_rp2040.h"
#include "hardware/irq.h"
#include "delay.h"

#define IR_MAX_HANDLERS_PER_ID (3)

#define IR_PIO (pio0)

#define START_BIT (1<<15)
#define DATA_CONTINUATION_BIT (1<<14)

#define START_RECIPIENT_ADDRESS_BITS (10)
#define START_RECIPIENT_ADDRESS_SHIFT (5)
#define START_RECIPIENT_ADDRESS_MASK (((1<<START_RECIPIENT_ADDRESS_BITS)-1) << START_RECIPIENT_ADDRESS_SHIFT)

#define START_APP_ID_BITS (5)
#define START_APP_ID_MASK ((1<<START_APP_ID_BITS)-1)

#define DATA_SEQUENCE_NUM_BITS (6)
#define DATA_SEQUENCE_NUM_SHIFT (8)
#define DATA_SEQUENCE_NUM_MASK (((1<<DATA_SEQUENCE_NUM_BITS)-1) << DATA_SEQUENCE_NUM_SHIFT)

#define DATA_PAYLOAD_BITS (8)
#define DATA_PAYLOAD_MASK ((1<<DATA_PAYLOAD_BITS)-1)

#define CONTINUATION_MASK (1<<14)

static int active_callbacks = 0;

static uint8_t current_rx_data[MAX_IR_MESSAGE_SIZE];
static IR_DATA current_rx_message = {
    .data = current_rx_data,
};

static int tx_sm;
static int rx_sm;
static int message_count;
static bool receiving_message = false;

ir_data_callback cb[IR_MAX_ID][IR_MAX_HANDLERS_PER_ID] = {};

typedef struct {
    uint8_t command;
    uint8_t address;
} NEC_IR_PACKET;

static uint32_t get_raw_nec_data(uint16_t payload) {
    uint8_t *data = (uint8_t*) &payload;
    return nec_encode_frame(data[0], data[1]);
}

static bool get_payload(uint32_t raw_nec_data, uint16_t* payload) {
    uint8_t *data = (uint8_t*) payload;
    return nec_decode_frame(raw_nec_data, &data[0], &data[1]);
}

#if IR_DEBUG
uint32_t last_packets[20];
static int pkt_idx;
#endif

void irq_fifo_handler(void) {

    while (!pio_sm_is_rx_fifo_empty(IR_PIO, rx_sm)) {
        uint32_t rx_data = pio_sm_get(IR_PIO, rx_sm);

        uint16_t payload = 0;

#if IR_DEBUG
        last_packets[pkt_idx++] = rx_data;
        if (pkt_idx >= 20) {
            pkt_idx = 0;
        }
#endif
        bool success = get_payload(rx_data, &payload);
        if (!success) {
            continue;
        }

        if (payload & START_BIT) {
            current_rx_message.data_length = 0;
            current_rx_message.recipient_address = (payload & START_RECIPIENT_ADDRESS_MASK) >> START_RECIPIENT_ADDRESS_SHIFT;
            current_rx_message.app_address = (payload & START_APP_ID_MASK);
            receiving_message = true;
            continue;
        }
        if (!receiving_message) {
            continue;
        }

        uint8_t sequence_number = (payload & DATA_SEQUENCE_NUM_MASK) >> DATA_SEQUENCE_NUM_SHIFT;
        if (sequence_number != current_rx_message.data_length) {
            continue;
        }
        current_rx_message.data[current_rx_message.data_length++] = payload & DATA_PAYLOAD_MASK;
        if (!(payload & CONTINUATION_MASK)) {
            receiving_message = false;
            // End of message!
            if (current_rx_message.recipient_address != IR_BADGE_ID_BROADCAST
                /* OR recipient address isn't Badge address TODO*/) {
                continue;
            }
            if (current_rx_message.app_address >= IR_MAX_ID) {
                continue;
            }
            bool message_processed = false;
            for (int i=0; i<IR_MAX_HANDLERS_PER_ID; i++) {
                if (cb[current_rx_message.app_address][i] == NULL) {
                    // No handler
                    break;
                }
                cb[current_rx_message.app_address][i](&current_rx_message);
                message_processed = true;
            }
            if (message_processed) {
                message_count++;
            }
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

bool ir_add_callback(ir_data_callback data_cb, IR_APP_ID app_id) {
    for (int i=0; i<IR_MAX_HANDLERS_PER_ID; i++) {
        if (cb[app_id][i] == data_cb) {
            return true; // already registered
        } else if (cb[app_id][i] == NULL) {
            cb[app_id][i] = data_cb;
            active_callbacks++;
            return true;
        }
    }
    return false;
}

bool ir_remove_callback(ir_data_callback data_cb, IR_APP_ID app_id) {
    bool removed = false;
    for (int i=0; i<IR_MAX_HANDLERS_PER_ID; i++) {
        // Once we identify the handler to remove, shuffle all later ones to the left
        if (removed || (cb[app_id][i] == data_cb)) {
            if (i != IR_MAX_HANDLERS_PER_ID) {
                cb[app_id][i] = cb[app_id][i+1];
            } else {
                cb[app_id][i] = NULL;
            }
            if (active_callbacks) {
                active_callbacks--;
            }
            removed = true;
        }
    }
    return removed;
}

bool ir_transmitting(void) {
    return pio_sm_is_tx_fifo_empty(IR_PIO, tx_sm);
}

bool ir_listening(void) {
    return (bool) active_callbacks;
}

static void ir_send_start_packet(const IR_DATA *data) {

    uint16_t packet_data = START_BIT;
    packet_data |= (data->recipient_address << START_RECIPIENT_ADDRESS_SHIFT) & START_RECIPIENT_ADDRESS_MASK;
    packet_data |= (data->app_address) & START_APP_ID_MASK;
    pio_sm_put_blocking(IR_PIO, tx_sm, get_raw_nec_data(packet_data));

}

static void ir_send_data_packet(const IR_DATA *data, int index) {


    uint16_t packet_data = 0;
    if (index < data->data_length-1) {
        // More data coming
        packet_data |= DATA_CONTINUATION_BIT;
    }
    packet_data |= (index << DATA_SEQUENCE_NUM_SHIFT) & DATA_SEQUENCE_NUM_MASK;
    packet_data |= (data->data[index]);
    pio_sm_put_blocking(IR_PIO, tx_sm, get_raw_nec_data(packet_data));

}

void ir_send_complete_message(const IR_DATA *data) {

    irq_set_enabled(PIO0_IRQ_1, false);
    enum pio_interrupt_source irq_source = pis_sm0_rx_fifo_not_empty + rx_sm;
    pio_set_irq1_source_enabled(IR_PIO, irq_source, false);
    // Pause receive while transmitting
    pio_sm_set_enabled(IR_PIO, rx_sm, false);
    // not sure why, but sometimes the first packet received after idle time is basically garbage. Sending something
    // we can discard helps
    pio_sm_put_blocking(IR_PIO, tx_sm, 0xa55aa55a);

    ir_send_start_packet(data);
    for (int i=0; i<data->data_length; i++) {
        ir_send_data_packet(data, i);
    }

    // todo: do this perhaps instead on a FIFO empty interrupt?
    while (ir_transmitting()) {
        sleep_ms(1);
    }
    sleep_ms(200);

    pio_sm_clear_fifos(IR_PIO, rx_sm);
    irq_set_enabled(PIO0_IRQ_1, true);
    pio_set_irq1_source_enabled(IR_PIO, irq_source, true);
    pio_sm_set_enabled(IR_PIO, rx_sm, true);
}

uint8_t ir_send_partial_message(const IR_DATA *data, uint8_t starting_sequence_num) {
    if (starting_sequence_num == 0) {
        ir_send_start_packet(data);
    }

    for (int i=starting_sequence_num; i<data->data_length; i++) {
        if (pio_sm_is_tx_fifo_full(IR_PIO, tx_sm)) {
            return i - starting_sequence_num;
        }
        ir_send_data_packet(data, i);
    }
    // Finished sending message
    return data->data_length - starting_sequence_num;
}


bool ir_messages_seen(bool reset) {
    bool result = message_count != 0;
    if (reset) {
        message_count = 0;
    }
    return result;
}

int ir_message_count(void) {
    return message_count;
}