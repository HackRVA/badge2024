//
// Created by Samuel Jones on 2/10/22.
//

#ifndef BADGE2022_C_IR_H
#define BADGE2022_C_IR_H

#include <stddef.h>
#include <stdint.h>

// This follows the NEC packet format and encoding, which has built-in redundancy.
// TODO app-level IR packets should be restructured to fit this.
// It'll likely wind up better for us to just use the library as intended.
typedef struct {
    uint8_t address;
    uint8_t command;
} IR_PACKET;

typedef void (*ir_data_callback)(IR_PACKET packet);

void ir_init(void);

void ir_set_callback(ir_data_callback data_cb);

void ir_enqueue(IR_PACKET packet);


#endif //BADGE2022_C_IR_H
