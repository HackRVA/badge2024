#ifndef BADGE_MONSTERS_H__
#define BADGE_MONSTERS_H__

int badge_monsters_cb(void);

/* We have 16 bits of payload. Let's say the high order 4 bits are the opcode.
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

#define BADGE_IR_GAME_ADDRESS IR_APP2
#define BADGE_IR_BROADCAST_ID 0

#define OPCODE_XMIT_MONSTER 0x01

#endif
