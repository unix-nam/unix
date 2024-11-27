#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "battleship.h"

char board[BOARD_SIZE][BOARD_SIZE];

void place_ships() {
    printf("함선을 배치하세요 (5개).\n");
    for (int i = 0; i < 5; i++) {
        int x, y;
        printf("배치할 좌표 (x, y): ");
        scanf("%d %d", &x, &y);
        if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && board[x][y] == '.') {
            board[x][y] = 'S'; // 함선 배치
        } else {
            printf("유효하지 않은 좌표입니다. 다시 입력하세요.\n");
            i--; // 다시 배치 시도
        }
    }
    printf("배치 완료. 현재 보드 상태:\n");
    print_board(board);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("유효하지 않은 주소");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }

    printf("서버 연결 완료. 보드 초기화...\n");
    initialize_board(board);
    place_ships();

    while (1) {
        Attack attack;
        char result[20];

        printf("공격할 좌표 입력 (x, y): ");
        scanf("%d %d", &attack.x, &attack.y);
        send(sock, &attack, sizeof(Attack), 0);

        recv(sock, result, sizeof(result), 0);
        printf("결과: %s\n", result);

        // 상대방 보드 상태 출력
        print_board(board);
    }

    close(sock);
    return 0;
}
