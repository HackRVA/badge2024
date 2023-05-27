#if !defined(NEW_BADGE_MONSTERS_IR_H)
#define NEW_BADGE_MONSTERS_IR_H

#include "ir.h"

typedef void (*ir_callback)(const IR_DATA *);

void register_ir_packet_callback(ir_callback callback);
void unregister_ir_packet_callback(ir_callback callback);
unsigned short get_payload(IR_DATA* packet);
void process_packet(IR_DATA* packet);
void check_for_incoming_packets(void);
void ir_packet_callback(const IR_DATA *data);
void build_and_send_packet(unsigned char address, unsigned short badge_id, unsigned short payload);

/*
 * We have 16 bits of payload. Let's say the high order 4 bits are the opcode.
 * That gives us 16 opcodes, with 12 bits of payload per opcode for single
 * packet opcodes (we can have multi-packet opcodes if needed).
 * All integer values are transmitted little endian (low order byte first).
 *
 * Badge packet is 32-bits:
 * 1 start bit
 * 1 cmd bits
 * 5 address bits (like port number)
 * 9 badge id bits
 * 16 payload bits
 *
 */

extern const IR_APP_ID BADGE_IR_GAME_ADDRESS;
extern const int BADGE_IR_BROADCAST_ID;
extern const unsigned char OPCODE_XMIT_MONSTER;

#endif

