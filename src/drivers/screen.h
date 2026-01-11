#ifndef SCREEN_H
#define SCREEN_H

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f

// Screen i/o functions
void print_set_colour(char color);
void clear_screen();
void print_string(const char* message);
void print_at(const char* message, int col, int row);

#define print print_string

#endif
