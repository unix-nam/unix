#ifndef BATTLESHIP_H
#define BATTLESHIP_H

#define BOARD_SIZE 10
#define PORT 12345

typedef struct {
    int x, y;       // 공격 좌표
} Attack;

// 보드 초기화 함수
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]);

// 보드 출력 함수
void print_board(char board[BOARD_SIZE][BOARD_SIZE]);

#endif
