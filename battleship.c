#include <stdio.h>
#include <string.h>
#include "battleship.h"
#include <ctype.h>

// 보드 초기화
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = '.'; // 기본값 설정
        }
    }
}

// 보드 출력
void print_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    printf("\n  ");
    for (int i = 0; i < BOARD_SIZE; i++) printf("%d ", i);
    printf("\n");

    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
    fflush(stdout); // 출력 강제 플러시
}

// 숫자인지 검증
int is_number(char *str) {
    
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    
    return 1;
}

// 배치 유효성 검사
int is_valid_placement(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, char orientation) {    
    if (orientation == 'H') {
        if (y + size > BOARD_SIZE) return 0;
        for (int i = 0; i < size; i++) {
            if (board[x][y + i] != '.') return 0;
        }
    } else if (orientation == 'V') {
        if (x + size > BOARD_SIZE) return 0;
        for (int i = 0; i < size; i++) {
            if (board[x + i][y] != '.') return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

// 배 배치
void place_ship(char board[BOARD_SIZE][BOARD_SIZE], int x, int y, int size, char orientation) {
    if (orientation == 'H') {
        for (int i = 0; i < size; i++) {
            board[x][y + i] = 'S';
        }
    } else if (orientation == 'V') {
        for (int i = 0; i < size; i++) {
            board[x + i][y] = 'S';
        }
    }
}
