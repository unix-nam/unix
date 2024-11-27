#include <stdio.h>
#include <string.h>
#include "battleship.h"

// 보드 초기화
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = '.'; // 초기값을 '.'으로 설정
        }
    }
}

// 보드 출력
void print_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    printf("\n  ");
    for (int i = 0; i < BOARD_SIZE; i++) printf("%d ", i); // 열 번호 출력
    printf("\n");

    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%d ", i); // 행 번호 출력
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c ", board[i][j]); // 보드 상태 출력
        }
        printf("\n");
    }
    fflush(stdout); // 출력 강제 플러시
}
