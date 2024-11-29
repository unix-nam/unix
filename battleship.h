#ifndef BATTLESHIP_H
#define BATTLESHIP_H

#define BOARD_SIZE 10
#define PORT 12345

typedef struct {
    char *name;
    int size;
} Ship;

typedef struct {
    int x, y;       // 공격 좌표
} Attack;

// 함수 선언
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]);
void print_board(char board[BOARD_SIZE][BOARD_SIZE]);
int is_valid_placement(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, char orientation);
void place_ship(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, char orientation);
int is_number(char *str);
#endif
