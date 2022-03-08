//
// Created by Samuel Jones on 2/10/22.
//

#include "cli_ir.h"
#include "cli.h"
#include "ir.h"
#include "rtc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static IR_DATA last_ir_packet;
static uint8_t last_ir_data[MAX_IR_MESSAGE_SIZE];
static uint32_t last_rx_time;

void ir_handler(const IR_DATA* pkt) {
    last_ir_packet = *pkt;
    memcpy(last_ir_data, pkt->data, pkt->data_length);
    last_ir_packet.data = last_ir_data;
    last_rx_time = rtc_get_ms_since_boot();
}

int run_ir_send(char *args) {

    char * dest_string = cli_get_token(&args);
    if (!dest_string) {
        puts("No destination address specified");
        return 1;
    }
    int dest = strtol(dest_string, NULL, 10);
    if (dest < 0 || dest >= 1024) {
        puts("Destination must be in the range [0-1023]");
        return 1;
    }

    char * app_string = cli_get_token(&args);
    if (!app_string) {
        puts("No app specified");
        return 1;
    }

    int app = strtol(app_string, NULL, 10);
    if (app < 0 || app >= 64) {
        puts("App must be in the range [0-63]");
        return 1;
    }

    char *data = cli_get_token(&args);
    if (!data) {
        puts("No data provided");
        return 1;
    }

    size_t data_hex_len = strlen(data);
    if (data_hex_len % 2) {
        puts("An even byte boundary is required");
        return 1;
    }
    if (data_hex_len >= MAX_IR_MESSAGE_SIZE*2) {
        puts("Data is too long - a maximum of 64 binary bytes is supported");
        return 1;
    }

    uint8_t ir_data[MAX_IR_MESSAGE_SIZE];
    for (size_t i=0; i<data_hex_len; i+=2) {
        char byte_str[3] = {data[i], data[i+1], '\0'};
        ir_data[i/2] = strtoul(byte_str, NULL, 16);
    }

    IR_DATA ir_transmit = {
        .recipient_address = dest,
        .app_address = app,
        .data_length = data_hex_len/2,
        .data = ir_data
    };

    ir_send_complete_message(&ir_transmit);

    return 0;
}

int run_ir_handler(char *args) {

    char * handler_string = cli_get_token(&args);
    if (!handler_string) {
        puts("No handler ID specified");
        return 1;
    }
    int handler_id = strtol(handler_string, NULL, 10);
    printf("Adding handler to app ID: %u\n", handler_id);

    ir_add_callback(ir_handler, handler_id);
    return 0;
}


int run_ir_last(__attribute__((unused)) char *args) {

    if (!last_rx_time) {
        puts("No IR packets received by handler (make sure handler is installed).");
        return 0;
    }

    printf("Last packet (rx at %u ms) : badge %u, app %u\n",
           last_rx_time, last_ir_packet.recipient_address, last_ir_packet.app_address);
    printf("  Data: ");
    for (int i=0; i<last_ir_packet.data_length; i++) {
        printf("%02x", last_ir_packet.data[i]);
    }
    printf("\n");

#if IR_DEBUG
    extern uint32_t last_packets[20];
    for (int i=0; i<20; i++) {
        printf("\t%08x\n", last_packets[i]);
    }
#endif

    return 0;
}

static const CLI_COMMAND ir_subcommands[] = {
        {.name="send", .process=run_ir_send,
                .help="usage: ir send [dest, 0-1023] [app 0-63] [data (hex string up to 64 bytes)]"},
        {.name="handler", .process=run_ir_handler,
                .help="usage: ir handler [app_num] - Install packet reception interrupt handler."},
        {.name="last", .process=run_ir_last,
                .help="usage: ir last - Show last packet received by the packet handler."},
        {}
};


const CLI_COMMAND ir_command = {
        .name="ir", .subcommands=(CLI_COMMAND *) ir_subcommands,
        .help="usage: ir subcommand [[args...]]\n"
              "valid subcommands: send handler last"
};
