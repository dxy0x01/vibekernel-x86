#include "screen.h"

int get_cursor_offset();
void set_cursor_offset(int offset);
int print_char(char c, int col, int row, char attr);
int get_offset(int col, int row);
int get_offset_row(int offset);
int get_offset_col(int offset);

// Global cursor offset (simple version, not reading from VGA ports yet)
int cursor_offset = 0;

void print_at(char* message, int col, int row) {
    if (col >= 0 && row >= 0)
        cursor_offset = get_offset(col, row);
        
    int i = 0;
    while (message[i] != 0) {
        cursor_offset = print_char(message[i++], col, row, WHITE_ON_BLACK);
    }
}

void print_string(char* message) {
    print_at(message, -1, -1);
}

void clear_screen() {
    int screen_size = MAX_COLS * MAX_ROWS;
    char* screen = (char*) VIDEO_ADDRESS;
    int i;
    for (i = 0; i < screen_size; i++) {
        screen[i*2] = ' ';
        screen[i*2+1] = WHITE_ON_BLACK;
    }
    cursor_offset = 0;
}

// Private kernel functions

int print_char(char c, int col, int row, char attr) {
    char* vidmem = (char*) VIDEO_ADDRESS;
    if (!attr) attr = WHITE_ON_BLACK;

    if (col >= 0 && row >= 0) {
        cursor_offset = get_offset(col, row);
    }

    int i = cursor_offset;

    if (c == '\n') {
        int rows = cursor_offset / (2 * MAX_COLS);
        cursor_offset = get_offset(0, rows + 1);
    } else {
        vidmem[cursor_offset] = c;
        vidmem[cursor_offset+1] = attr;
        cursor_offset += 2;
    }
    
    // Check if we need to scroll (not implemented yet, just wrap)
    // if (cursor_offset >= MAX_ROWS * MAX_COLS * 2) ...

    return cursor_offset;
}

int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }
int get_offset_row(int offset) { return offset / (2 * MAX_COLS); }
int get_offset_col(int offset) { return (offset - (get_offset_row(offset)*2*MAX_COLS))/2; }
