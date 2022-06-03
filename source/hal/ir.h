//
// Created by Samuel Jones on 2/10/22.
//

#ifndef BADGE2022_C_IR_H
#define BADGE2022_C_IR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define IR_BADGE_ID_BROADCAST (0)
#define MAX_IR_MESSAGE_SIZE (64)

typedef enum {

    IR_LED,
    IR_LIVEAUDIO,
    IR_PING,

    IR_APP0,
    IR_APP1,
    IR_APP2,
    IR_APP3,
    IR_APP4,
    IR_APP5,
    IR_APP6,
    IR_APP7,

    IR_MAX_ID

} IR_APP_ID;

// This follows the NEC packet encoding, which has built-in redundancy. It sends 2 bytes of information for every
// 4-byte transmission. Normally this is in the form of an address and command byte, but the badge IR has the following
// format to support a little bit longer messages:

// A sequence of packets is sent to send a message. The first packet of any message has the MS bit set to indicate it
// is a start packet, followed by 10 bits of device address, followed by 5 bits of app ID. This lets the receiver
// know if it should handle the rest of the message or not, and which app to send it to.
//
// In summary, this looks like:
// | 1 (START, 1 bit) - recipient address (10 bit) - app ID (5 bit) |
//
// Subsequent bytes start with the MS bit /unset/, followed by a 1 if there are more bytes coming in the message, or
// 0 if this is the last byte. After this, there is a 6-bit message sequence number, starting from 0. Finally there are
// 8 bits of packet data. This is done so a receiver can know that it has received every required part of the message
// and when the message is over.
//
// Packets that aren't the last one in a message look like:
// | 0 (not start, 1 bit) - 1 (more message after this, 1 bit) - message sequence number (6 bits) - data (8 bits) |
//
// And packets that are the last look like:
// | 0 (not start, 1 bit) - 0 (no more message, 1 bit) - message sequence number (6 bits) - data (8 bits) |
//

// This structure is not the encoded packet format but just a data format for ease of use.
typedef struct {
    uint16_t recipient_address;
    uint8_t app_address;
    uint8_t data_length;
    uint8_t *data;
} IR_DATA;

typedef void (*ir_data_callback)(const IR_DATA* data);

void ir_init(void);

bool ir_add_callback(ir_data_callback data_cb, IR_APP_ID app_id);
bool ir_remove_callback(ir_data_callback data_cb, IR_APP_ID app_id);

bool ir_transmitting(void);
bool ir_listening(void);

// Note that sending > 3 data bytes at the start will block; to prevent this, use ir_send_partial message and check the
// return value to see how far it got.
void ir_send_complete_message(const IR_DATA *data);

// Returns the number of data packets that were queued to send, instead of blocking to send out all data.
uint8_t ir_send_partial_message(const IR_DATA *data, uint8_t starting_sequence_num);

// TODO maybe we can track this outside of the HAL and in the badge system files somewhere
bool ir_messages_seen(bool reset);
int ir_message_count(void);


#endif //BADGE2022_C_IR_H
