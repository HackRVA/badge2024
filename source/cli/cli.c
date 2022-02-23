//
// Created by Samuel Jones on 11/9/21.
//

#include "cli.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "delay.h"
#include "ir.h"

#if TARGET_PICO
#include "pico/stdio.h"
#endif

#define MAX_LINE_LEN 200
void cli_run(const CLI_COMMAND *cmd) {
    char line[MAX_LINE_LEN];
    while (true) {
        int cmd_result = 0;
        int result = cli_get_line("Enter command:", line, MAX_LINE_LEN);
        if (result > 0) {
            cmd_result = cli_process_line(NULL, cmd, line);
        }
        if (result < 0 || cmd_result < 0) {
            break;
        }
    }
}

int cli_get_line(const char *prompt, char* line, size_t len) {

    if (prompt) {
        puts(prompt);
    }
    printf("> ");

    int cur_len = 0;
    bool too_long = false;
    while (1) {

#if TARGET_PICO
        int raw_input = getchar_timeout_us(1000);
        if (raw_input == PICO_ERROR_TIMEOUT) {
            continue;
        }
#elif TARGET_SIMULATOR
        int raw_input = getchar();
#endif
        char input = (char) raw_input;

        // If we get EOF, signal this so we can exit command loop
        if (((int)input) == EOF) {
            return -1;
        }

        // echo to display command back in term
        putchar(input);

        // backspace
        if (input == '\b' && cur_len) {
            cur_len--;
        // normal input
        } else {
            line[cur_len] = input;
            if (cur_len < len) {
                cur_len++;
            } else {
                too_long = true;
            }
        }

        // EOL detection
        if (line[cur_len-1] == '\r') {
            // null terminate the line
            line[cur_len-1] = '\0';
            putchar('\n');
            break;
        }
    }

    if (too_long) {
        printf("Command was too long - maximum length is %u", (unsigned int)len);
        return 0;
    }

    return cur_len;
}

char* cli_get_token(char** line) {

    // discard leading whitespace
    while (**line == ' ') {
        (*line)++;
    }

    if (**line == '\0') {
        return NULL;
    }

    char *token_start = *line;

    while (**line != ' ' && **line != '\0') {
        (*line)++;
    }
    if (**line == ' ') {
        **line = '\0';
        (*line)++;
    }
    return token_start;
}

int cli_process_line(const CLI_COMMAND *parent, const CLI_COMMAND *cmd, char* line) {

    int cmd_index = 0;
    char *token = cli_get_token(&line);

    while (cmd[cmd_index].name != NULL) {
        const CLI_COMMAND *cur_cmd = &cmd[cmd_index];
        if (strcmp(token, cur_cmd->name) == 0) {
            if (cur_cmd->subcommands) {
                return cli_process_line(cur_cmd, (const CLI_COMMAND *) cur_cmd->subcommands, line);
            }
            else if (cur_cmd->process) {
                int result = cur_cmd->process(line);
                if (result > 0) {
                    puts(cur_cmd->help);
                }
                return result;
            }
        }
        cmd_index++;
    }

    // No command match / print parent
    if (parent && parent->help) {
        puts(parent->help);
    } else {
        puts("No command matched.");
    }
    return 1;
}
