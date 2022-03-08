//
// Created by Samuel Jones on 2/21/22.
//

#include "ir.h"
#include <stdio.h>


void ir_init(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}

bool ir_add_callback(__attribute__((unused)) ir_data_callback data_cb, __attribute__((unused)) IR_APP_ID app_id) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return true;
}

bool ir_remove_callback(__attribute__((unused)) ir_data_callback data_cb, __attribute__((unused)) IR_APP_ID app_id) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return true;
}

bool ir_transmitting(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return false;
}

// Note that sending > 3 data bytes at the start will block; to prevent this, use ir_send_partial message and check the
// return value to see how far it got.
void ir_send_complete_message(__attribute__((unused)) const IR_DATA *data) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
}

// Returns the number of data packets that were queued to send, instead of blocking to send out all data.
uint8_t ir_send_partial_message(__attribute__((unused)) const IR_DATA *data, __attribute__((unused)) uint8_t starting_sequence_num) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return data->data_length;
}

// TODO maybe we can track this outside of the HAL and in the badge system files somewhere
bool ir_messages_seen(__attribute__((unused)) bool reset) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}

int ir_message_count(void) {
    printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
    return 0;
}
