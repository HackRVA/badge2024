//
// Created by Samuel Jones on 2/10/22.
//

#include "cli_ir.h"
#include "cli.h"
#include "ir.h"
#include "rtc.h"
#include <stdlib.h>
#include <stdio.h>

static IR_DATA last_ir_packet;
static uint32_t last_rx_time;

void ir_handler(const IR_DATA* pkt) {
    last_ir_packet = *pkt;
    last_rx_time = rtc_get_ms_since_boot();
}

int run_ir_send(char *args) {

    char * addr_string = cli_get_token(&args);
    if (!addr_string) {
        puts("No address specified");
        return 1;
    }
    int address = strtol(addr_string, NULL, 10);
    if (address < 0 || address >= 256) {
        puts("Address must be an 8-bit number");
        return 1;
    }

    char * command_string = cli_get_token(&args);
    if (!command_string) {
        puts("No command specified");
        return 1;
    }
    int command = strtol(command_string, NULL, 10);
    if (command < 0 || command >= 256) {
        puts("Command must be an 8-bit number");
        return 1;
    }
#if 0
    IR_PACKET pkt = {
        .address = address,
        .command = command,
    };

    ir_enqueue(pkt);
#endif

    return 0;
}

int run_ir_handler(char *args) {

    //ir_set_callback(ir_handler, 0);
    return 0;
}


int run_ir_last(char *args) {

    if (!last_rx_time) {
        puts("No IR packets received.");
        return 0;
    }

    printf("Last packet (rx at %u ms) : addr 0x%02x, command 0x%02x\n",
           last_rx_time, last_ir_packet.app_address, last_ir_packet.data[0]);
    return 0;
}

static const CLI_COMMAND ir_subcommands[] = {
        {.name="send", .process=run_ir_send,
                .help="usage: ir send [address, 0-255] [command 0-255]"},
        {.name="handler", .process=run_ir_handler,
                .help="usage: ir handler - Install packet reception interrupt handler."},
        {.name="last", .process=run_ir_last,
                .help="usage: ir last - Show last packet received by the packet handler."},
        {}
};


const CLI_COMMAND ir_command = {
        .name="ir", .subcommands=(CLI_COMMAND *) ir_subcommands,
        .help="usage: ir subcommand [[args...]]\n"
              "valid subcommands: send handler last"
};