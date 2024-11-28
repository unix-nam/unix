#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include "battleship.h"

char boards[2][BOARD_SIZE][BOARD_SIZE];
int client_sockets[2];
int game_over = 0;

// 클라이언트 종료 알림
void notify_other_client(int disconnected_client) {
    int other_client = (disconnected_client == 0) ? 1 : 0;
    char msg[] = "상대방이 게임을 종료했습니다. 게임을 종료합니다.\n";
    send(client_sockets[other_client], msg, sizeof(msg), 0);
    close(client_sockets[other_client]);
}

// 클라이언트로부터 보드 수신
void receive_board(int client_socket, int player) {
    printf("플레이어 %d로부터 보드 정보를 수신합니다...\n", player + 1);
    if (recv(client_socket, boards[player], sizeof(boards[player]), 0) == -1) {
        perror("보드 수신 실패");
        exit(EXIT_FAILURE);
    }
    printf("플레이어 %d 보드 수신 완료:\n", player + 1);
    print_board(boards[player]);
}

// 공격 처리
int handle_attack(int attacker, int defender) {
    Attack attack;
    char result[60];

    while (1) { // 유효한 입력이 들어올 때까지 반복
        int bytes_received = recv(client_sockets[attacker], &attack, sizeof(Attack), 0);

        // 연결 종료 감지
        if (bytes_received == 0) {
            printf("플레이어 %d 연결 끊김.\n", attacker + 1);
            notify_other_client(attacker);
            return 1;
        }

        // 유효한 입력이면 루프 종료
        break;
    }

    // 공격 처리
    printf("%d %d %d \n", defender, attack.x, attack.y);
    if (boards[defender][attack.x][attack.y] == 'S') {
        boards[defender][attack.x][attack.y] = 'X';
        strcpy(result, "명중");
    } else {
        strcpy(result, "실패");
    }

    send(client_sockets[attacker], result, sizeof(result), 0);

    printf("플레이어 %d의 공격: (%d, %d)\n", attacker + 1, attack.x, attack.y);

    // 게임 종료 조건 검사
    game_over = 1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (boards[defender][i][j] == 'S') {
                game_over = 0;
            }
        }
    }

    char msg[30];
    if (game_over == 1) {
        // 승리 메시지 설정
        strcpy(msg, "승리하였습니다.");
        send(client_sockets[attacker], msg, sizeof(msg), 0);

        // 패배 메시지 설정
        strcpy(msg, "패배하였습니다.");
        send(client_sockets[defender], msg, sizeof(msg), 0);
    }

    return 0;
}


int main() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // 소켓 생성
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("바인딩 실패");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("리스닝 실패");
        exit(EXIT_FAILURE);
    }

    printf("플레이어 대기 중...\n");

    // 두 클라이언트 연결 및 보드 수신
    for (int i = 0; i < 2; i++) {
        client_sockets[i] = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        printf("플레이어 %d 연결 완료\n", i + 1);
        receive_board(client_sockets[i], i);
    }

    // 게임 루프
    int turn = 0;
    while (!game_over) {
        if (handle_attack(turn % 2, (turn + 1) % 2)) {
            game_over = 1; // 한쪽 클라이언트가 종료되면 게임 종료
        }
        turn++;
    }

    // 소켓 종료
    for (int i = 0; i < 2; i++) {
        close(client_sockets[i]);
    }
    close(server_fd);

    printf("서버 종료.\n");
    return 0;
}
