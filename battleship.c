// battleship.c
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "battleship.h"
#include <time.h>

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

// 게임 결과 저장
void save_game_result(int winner, int loser) {
    FILE* file = fopen("game_result.txt", "a"); // "a" 모드 사용해 파일에 데이터를 추가
    if (!file) {
        printf("파일을 열 수 없습니다. %s\n", strerror(errno));
    }

    // 현재 시간을 기록
    time_t currentTime = time(NULL);
    char* timeStr = ctime(&currentTime);
    timeStr[strcspn(timeStr, "\n")] = 0; // ctime이 반환하는 문자열의 끝에 있는 개행 제거

    // 결과를 파일에 저장
    fprintf(file, "게임 결과 (%s):\n", timeStr);
    fprintf(file, "승자: %d\n", winner);
    fprintf(file, "패자: %d\n", loser);
    fprintf(file, "========================\n");

    fclose(file);
    printf("게임 결과가 game_result.txt 파일에 저장되었습니다.\n");
}
