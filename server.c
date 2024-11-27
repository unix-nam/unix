#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "battleship.h"

char boards[2][BOARD_SIZE][BOARD_SIZE]; // 두 플레이어의 보드

void handle_attack(int attacker, int defender, int client_sockets[]) {
    Attack attack;
    recv(client_sockets[attacker], &attack, sizeof(Attack), 0);

    char result[20];
    if (boards[defender][attack.x][attack.y] == 'S') {
        boards[defender][attack.x][attack.y] = 'X'; // 명중 표시
        strcpy(result, "명중!");
    } else {
        strcpy(result, "실패.");
    }
    send(client_sockets[attacker], result, sizeof(result), 0);

    // 디버그: 현재 보드 출력
    printf("플레이어 %d의 보드:\n", defender + 1);
    print_board(boards[defender]);
}

int main() {
    int server_fd, client_sockets[2];
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("바인딩 실패");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("리스닝 실패");
        exit(EXIT_FAILURE);
    }

    printf("플레이어 대기 중...\n");

    // 두 클라이언트 연결
    for (int i = 0; i < 2; i++) {
        client_sockets[i] = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        printf("플레이어 %d 연결 완료\n", i + 1);
        initialize_board(boards[i]);
    }

    // 게임 루프
    int turn = 0;
    while (1) {
        handle_attack(turn % 2, (turn + 1) % 2, client_sockets);
        turn++;
    }

    // 소켓 종료
    for (int i = 0; i < 2; i++) close(client_sockets[i]);
    close(server_fd);

    return 0;
}
