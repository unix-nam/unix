// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/select.h>
#include <stdint.h>
#include <errno.h>
#include "battleship.h"

#define MAX_PLAYERS 2
#define TURN_TIMEOUT 30 // 공격 제한 시간
#define BUFFER_SIZE 1024 

char boards[MAX_PLAYERS][BOARD_SIZE][BOARD_SIZE];
int client_sockets[MAX_PLAYERS];
int game_over = 0;

// 모든 바이트를 전송하는 함수
int send_all(int sock, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *ptr = buffer;
    while (total_sent < length) {
        ssize_t sent = send(sock, ptr + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            return -1; // 에러 발생
        }
        total_sent += sent;
    }
    return 0; // 성공
}

// 메시지 전송 함수
int send_message(int client_socket, const char *message) {

    char msg_with_newline[BUFFER_SIZE];
    snprintf(msg_with_newline, sizeof(msg_with_newline), "%s\n", message);

    if (send_all(client_socket, msg_with_newline, strlen(msg_with_newline)) == -1) {
        perror("메시지 전송 실패");
        return -1;
    }
    printf("서버 -> 클라이언트: %s", msg_with_newline);
    return 0;
}

// 클라이언트 종료 알림
void notify_other_client(int disconnected_client) {
    int other_client = (disconnected_client == 0) ? 1 : 0;
    if (client_sockets[other_client] != -1) {
        printf("플레이어 %d가 게임을 포기하였습니다. 플레이어 %d가 승리하였습니다.\n", disconnected_client + 1, other_client + 1);
        send_message(client_sockets[other_client], "VICTORY");
        save_game_result(other_client + 1, disconnected_client + 1); // 게임 결과 저장
        // 송신 방향 종료
        shutdown(client_sockets[other_client], SHUT_WR);
        // 메시지가 전송될 시간을 확보하기 위해 잠시 대기
        sleep(1);
        close(client_sockets[other_client]);
        client_sockets[other_client] = -1;
    }
}

// 모든 바이트를 수신하는 함수
int recv_all(int sock, void *buffer, size_t length) {
    size_t total_received = 0;
    char *ptr = buffer;
    while (total_received < length) {
        ssize_t received = recv(sock, ptr + total_received, length - total_received, 0);
        if (received <= 0) {
            return -1;
        }

        total_received += received;
    }
    return 0;
}

// 클라이언트로부터 보드 수신
int receive_board(int client_socket, int player) {
    printf("플레이어 %d로부터 보드 정보를 수신합니다...\n", player + 1);
    if (recv_all(client_socket, boards[player], sizeof(boards[player])) == -1) {
        perror("보드 수신 실패");
        return -1;
    }
    printf("플레이어 %d 보드 수신 완료:\n", player + 1);
    print_board(boards[player]);
    return 0;
}

// 공격 처리
int handle_attack(int attacker, int defender) {
    Attack attack;
    char result[BUFFER_SIZE];   
    memset(&attack, 0, sizeof(Attack));
    memset(result, 0, sizeof(result));

    // 공격 좌표 수신을 위한 select 설정
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(client_sockets[attacker], &read_fds);

    timeout.tv_sec = TURN_TIMEOUT; // 제한 시간 설정
    timeout.tv_usec = 0;

    printf("플레이어 %d의 턴입니다. 공격 좌표를 기다립니다 (제한 시간: %d초).\n", attacker + 1, TURN_TIMEOUT);

    int sel = select(client_sockets[attacker] + 1, &read_fds, NULL, NULL, &timeout);

    if (sel == -1) {
        perror("select 실패");
        notify_other_client(attacker);
        return 1; // 게임 종료
    } else if (sel == 0) {
        // 시간 초과 처리
        printf("플레이어 %d의 턴이 시간 초과되었습니다.\n", attacker + 1);
        send_message(client_sockets[attacker], "TIMEOUT_DEFEAT");
        send_message(client_sockets[defender], "TIMEOUT_VICTORY");
        printf("플레이어 %d가 시간 초과로 패배하였습니다.\n", attacker + 1);
        save_game_result(defender + 1, attacker + 1);
        return 1; // 게임 종료
    }

    // 공격 좌표 수신
    if (recv_all(client_sockets[attacker], &attack, sizeof(Attack)) == -1) {
        perror("공격 수신 실패");
        notify_other_client(attacker);
        return 1; // 게임 종료
    }

    // Convert from network byte order to host byte order
    attack.x = ntohl(attack.x);
    attack.y = ntohl(attack.y);

    printf("플레이어 %d의 공격: (%d, %d)\n", attacker + 1, attack.x, attack.y);

    // 좌표 유효성 검사
    if (attack.x < 0 || attack.x >= BOARD_SIZE || attack.y < 0 || attack.y >= BOARD_SIZE) {
        send_message(client_sockets[attacker], "INVALID_ATTACK");
        return 0; // 게임 계속
    }

    // 공격 처리
    if (boards[defender][attack.x][attack.y] == 'S') {
        boards[defender][attack.x][attack.y] = 'X';
        send_message(client_sockets[attacker], "HIT");
    } else if (boards[defender][attack.x][attack.y] == '.' || boards[defender][attack.x][attack.y] == 'O') {
        boards[defender][attack.x][attack.y] = 'O';
        send_message(client_sockets[attacker], "MISS");
    } else {
        send_message(client_sockets[attacker], "INVALID_ATTACK");
        return 0; // 게임 계속
    }

    // 게임 종료 조건 검사
    int ships_remaining = 0;
    for (int i = 0; i < BOARD_SIZE && !ships_remaining; i++) {
        for (int j = 0; j < BOARD_SIZE && !ships_remaining; j++) {
            if (boards[defender][i][j] == 'S') {
                ships_remaining = 1;
            }
        }
    }

    if (!ships_remaining) {
        // 공격자 승리
        send_message(client_sockets[attacker], "VICTORY");
        // 피공격자 패배
        send_message(client_sockets[defender], "DEFEAT");
        printf("플레이어 %d가 게임에서 승리했습니다.\n", attacker + 1);
        save_game_result(attacker + 1, defender + 1); // 게임 결과 저장
        return 1; // 게임 종료
    }

    // 공격 결과를 피공격자에게 전달
    char opp_attack_msg[BUFFER_SIZE];
    char *attack_result = (boards[defender][attack.x][attack.y] == 'X') ? "HIT" : "MISS"; // 공격 성공 or 실패
    snprintf(opp_attack_msg, sizeof(opp_attack_msg), "OPPONENT_ATTACK %d %d %s", attack.x, attack.y, attack_result);
    send_message(client_sockets[defender], opp_attack_msg);

    return 0; // 게임 계속
}

int main() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // 클라이언트 소켓 초기화
    for (int i = 0; i < MAX_PLAYERS; i++) {
        client_sockets[i] = -1;
    }

    // 소켓 생성
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    // 소켓 옵션 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 주소 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("바인딩 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 리스닝
    if (listen(server_fd, MAX_PLAYERS) < 0) {
        perror("리스닝 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("플레이어 대기 중...\n");

    // 두 클라이언트 연결 및 보드 수신
    for (int i = 0; i < MAX_PLAYERS; i++) {
        client_sockets[i] = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_sockets[i] == -1) {
            perror("클라이언트 연결 실패");
            
            // 이미 연결된 클라이언트들 종료
            for (int j = 0; j < i; j++) {
                close(client_sockets[j]);
            }
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        printf("플레이어 %d 연결 완료\n", i + 1);
        if (receive_board(client_sockets[i], i) == -1) {
            notify_other_client(i);
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("모든 플레이어가 연결되었습니다. 게임을 시작합니다.\n");

    // 초기 턴 설정 (플레이어 1부터 시작)
    int turn = 0;

    // 첫 번째 턴을 공격자에게 알림
    if (send_message(client_sockets[turn % MAX_PLAYERS], "YOUR_TURN") == -1) {
        notify_other_client(turn % MAX_PLAYERS);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (send_message(client_sockets[(turn + 1) % MAX_PLAYERS], "WAIT_TURN") == -1) {
        notify_other_client((turn + 1) % MAX_PLAYERS);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 게임 루프
    while (!game_over) {
        int attacker = turn % MAX_PLAYERS;
        int defender = (turn + 1) % MAX_PLAYERS;

        if (handle_attack(attacker, defender)) {
            game_over = 1;
            break;
        }

        // 다음 턴으로 전환
        turn++;

        // 턴 관리 알림
        if (!game_over) {
            if (send_message(client_sockets[turn % MAX_PLAYERS], "YOUR_TURN") == -1) {
                notify_other_client(turn % MAX_PLAYERS);
                break;
            }
            if (send_message(client_sockets[(turn + 1) % MAX_PLAYERS], "WAIT_TURN") == -1) {
                notify_other_client((turn + 1) % MAX_PLAYERS);
                break;
            }
        }
    }

    // 소켓 종료
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (client_sockets[i] != -1) {
            close(client_sockets[i]);
        }
    }
    close(server_fd);

    printf("서버 종료.\n");
    return 0;
}
