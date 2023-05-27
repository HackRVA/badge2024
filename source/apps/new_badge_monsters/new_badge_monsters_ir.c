#include <string.h>
#ifdef __linux__
#include <stdio.h>
#endif

#include "new_badge_monsters_ir.h"
#include "new_badge_monsters.h"
#include "init.h"

/* These need to be protected from interrupts. */
#define QUEUE_SIZE 5
#define QUEUE_DATA_SIZE 4

const IR_APP_ID BADGE_IR_GAME_ADDRESS = IR_APP2;
const int BADGE_IR_BROADCAST_ID = 0;
const unsigned char OPCODE_XMIT_MONSTER = 0x01;

static int queue_in=0;
static int queue_out=0;
static IR_DATA packet_queue[QUEUE_SIZE] = { {0} };
static uint8_t packet_data[QUEUE_SIZE][QUEUE_DATA_SIZE] = {{0}};


void register_ir_packet_callback(void (*callback)(const IR_DATA *))
{
    ir_add_callback(callback, BADGE_IR_GAME_ADDRESS);
}

void unregister_ir_packet_callback(void (*callback)(const IR_DATA *))
{
    ir_remove_callback(callback, BADGE_IR_GAME_ADDRESS);
}

void build_and_send_packet(uint8_t address, uint16_t badge_id, uint16_t payload)
{
    uint8_t byte_payload[2] = {payload >> 8, payload & 0xFF};
    IR_DATA ir_packet = {
            .data_length = 2,
            .recipient_address = badge_id,
            .app_address = address,
            .data = byte_payload
    };

    ir_send_complete_message(&ir_packet);

}

uint16_t get_payload(IR_DATA* packet)
{
    return packet->data[0] << 8 | packet->data[1];
}

void process_packet(IR_DATA* packet)
{
    unsigned int payload;
    unsigned char opcode;

    payload = get_payload(packet);
    opcode = payload >> 12;

    printf("Got IR packet: %04x\n", payload);

    if(opcode == OPCODE_XMIT_MONSTER){
        printf("Enabling monster %u!\n", payload &0xFF);
        enable_monster(payload & 0x0ff);
    }
}

void check_for_incoming_packets(void)
{
    IR_DATA *new_packet;
    int next_queue_out;
    uint32_t interrupt_state = hal_disable_interrupts();
    while (queue_out != queue_in) {
        next_queue_out = (queue_out + 1) % QUEUE_SIZE;
        new_packet = &packet_queue[queue_out];
        queue_out = next_queue_out;
        hal_restore_interrupts(interrupt_state);
        process_packet(new_packet);
        interrupt_state = hal_disable_interrupts();
    }
    hal_restore_interrupts(interrupt_state);
}

void ir_packet_callback(const IR_DATA *data)
{
    // This is called in an interrupt context!
	int next_queue_in;

	next_queue_in = (queue_in + 1) % QUEUE_SIZE;
	if (next_queue_in == queue_out) {
        /* queue is full, drop packet */
		return;
    }
    size_t data_size = data->data_length;
    if (QUEUE_DATA_SIZE < data_size) {
        data_size = QUEUE_DATA_SIZE;
    }
    memcpy(&packet_data[queue_in], data->data, data_size);
	memcpy(&packet_queue[queue_in], data, sizeof(packet_queue[0]));
    packet_queue[queue_in].data = packet_data[queue_in];

	queue_in = next_queue_in;
}
