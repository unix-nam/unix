// battleship.h
#ifndef BATTLESHIP_H
#define BATTLESHIP_H

#include <stdint.h>

#define BOARD_SIZE 10
#define PORT 12345

typedef struct {
    char *name;
    int size;
} Ship;

typedef struct {
    int32_t x, y;       // 공격 좌표 (고정 크기)
} Attack;

// 함수 선언
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]);
void print_board(char board[BOARD_SIZE][BOARD_SIZE]);
int is_valid_placement(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, char orientation);
void place_ship(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, char orientation);

#endif
