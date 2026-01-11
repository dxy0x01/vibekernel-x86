#include "command.h"
#include "../drivers/screen.h"
#include "../string/string.h"
#include "../memory/heap/kheap.h"

#define MAX_COMMANDS 50
#define MAX_ARGS 10

static struct command commands[MAX_COMMANDS];
static int command_count = 0;

void command_init() {
    // Commands will be registered here or in kernel main
}

void command_register(const char* name, const char* description, command_handler_t handler) {
    if (command_count < MAX_COMMANDS) {
        commands[command_count].name = name;
        commands[command_count].description = description;
        commands[command_count].handler = handler;
        command_count++;
    }
}

void help_handler(int argc, char** argv) {
    print_string("Available commands:\n");
    for (int i = 0; i < command_count; i++) {
        print_string("  ");
        print_string(commands[i].name);
        print_string(" - ");
        print_string(commands[i].description);
        print_string("\n");
    }
}

void command_run(char* input) {
    if (strlen(input) == 0) return;

    char* argv[MAX_ARGS];
    int argc = 0;

    // Simple manual parsing
    char* p = input;
    while (*p && argc < MAX_ARGS) {
        // Skip whitespace
        while (*p == ' ') p++;
        if (*p == '\0') break;

        argv[argc++] = p;

        // Find end of token
        while (*p && *p != ' ') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }

    if (argc == 0) return;

    for (int i = 0; i < command_count; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    print_string("Unknown command: ");
    print_string(argv[0]);
    print_string("\n");
}
