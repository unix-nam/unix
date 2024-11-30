// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include "battleship.h"

#define NUM_SHIPS 5
#define BUFFER_SIZE 1024 // Increased buffer size

// 보드
char board[BOARD_SIZE][BOARD_SIZE];         // 자신의 배치 보드
char enemy_board[BOARD_SIZE][BOARD_SIZE];   // 상대방 공격 결과 보드

// 소켓 파일 디스크립터 (전역 변수)
int sock = 0;

// 최근 공격 좌표 (전역 변수)
int last_attack_x = -1;
int last_attack_y = -1;

// 배 정보
Ship ships[] = {
    {"항공모함", 5},
    {"전함", 4},
    {"순양함", 3},
    {"잠수함", 3},
    {"구축함", 2}
};

// 함수 프로토타입 선언
void print_combined_board();
void initialize_boards();
void place_and_send_ships(int sock);
int send_all(int sock, const void *buffer, size_t length);
void handle_sigint(int sig);
int read_line_from_buffer(char *buffer, int *start, int end, char *line);

// 두 개의 보드를 나란히 출력
void print_combined_board() {
    // 콘솔 화면 초기화 (ANSI 이스케이프 시퀀스)
    printf("\033[H\033[J"); // 화면 초기화 및 커서 이동

    printf("\n내 배 보드               | 공격 결과 보드\n");
    printf("  ");
    for (int i = 0; i < BOARD_SIZE; i++) printf("%d ", i);
    printf("   |     ");
    for (int i = 0; i < BOARD_SIZE; i++) printf("%d ", i);
    printf("\n");

    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("   |   ");
        printf("%d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c ", enemy_board[i][j]);
        }
        printf("\n");
    }

    fflush(stdout); // 출력 강제 플러시
}

// 보드 초기화
void initialize_boards() {
    initialize_board(board);
    initialize_board(enemy_board);
}

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

// 배를 배치하고 서버로 보드 전송
void place_and_send_ships(int sock) {
    printf("배를 배치하세요:\n");
    for (int i = 0; i < NUM_SHIPS; i++) {
        int x, y;
        char orientation;

        while (1) {
            printf("%s (크기: %d칸)을 배치합니다.\n", ships[i].name, ships[i].size);
            printf("배치 시작 좌표 (x y)와 방향 (H: 가로, V: 세로)을 입력하세요: ");
            int inputs = scanf("%d %d %c", &x, &y, &orientation);
            while (getchar() != '\n'); // 입력 버퍼 비우기

            if (inputs != 3) {
                printf("잘못된 입력입니다. 다시 시도하세요.\n");
                continue;
            }

            // 대문자로 변환
            orientation = toupper(orientation);

            if (orientation != 'H' && orientation != 'V') {
                printf("유효하지 않은 방향입니다. H 또는 V를 입력하세요.\n");
                continue;
            }

            if (is_valid_placement(board, x, y, ships[i].size, orientation)) {
                place_ship(board, x, y, ships[i].size, orientation);
                print_combined_board();
                break;
            } else {
                printf("유효하지 않은 배치입니다. 다시 시도하세요.\n");
            }
        }
    }

    // 서버로 보드 전송
    printf("서버로 보드 정보를 전송합니다...\n");
    if (send_all(sock, board, sizeof(board)) == -1) {
        perror("보드 전송 실패");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("보드 전송 완료.\n");
}

// SIGINT 핸들러
void handle_sigint(int sig) {
    printf("\n프로그램을 종료합니다. 게임을 포기하셨습니다.\n");
    // 서버에 패배 메시지 전송
    char msg[] = "DEFEAT\n";
    send_all(sock, msg, strlen(msg));
    close(sock);
    exit(EXIT_SUCCESS);
}

// 메시지 버퍼에서 한 줄을 읽어오는 함수
int read_line_from_buffer(char *buffer, int *start, int end, char *line) {
    int i = *start;
    while (i < end && buffer[i] != '\n') {
        i++;
    }

    if (i >= end) {
        // 개행 문자를 찾지 못함
        return 0;
    }

    // 한 줄 복사
    int line_length = i - *start;
    memcpy(line, buffer + *start, line_length);
    line[line_length] = '\0'; // 문자열 종료
    *start = i + 1; // 다음 시작 위치 설정
    return 1;
}

int main() {
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    int buffer_start = 0;
    int buffer_end = 0;

    // SIGINT 핸들러 설정
    signal(SIGINT, handle_sigint);

    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 서버 IP 주소 설정 (필요에 따라 변경)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("유효하지 않은 주소");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 서버 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("서버 연결 실패");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("서버 연결 완료. 보드 초기화...\n");
    initialize_boards();
    place_and_send_ships(sock);

    // 게임 루프
    while (1) {
        // 데이터를 수신하여 버퍼에 저장
        int bytes_received = recv(sock, buffer + buffer_end, sizeof(buffer) - buffer_end - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("서버가 연결을 종료했습니다.\n");
            } else {
                perror("메시지 수신 실패");
            }
            break;
        }
        buffer_end += bytes_received;

        // 버퍼에서 메시지를 추출하여 처리
        char line[BUFFER_SIZE];
        while (read_line_from_buffer(buffer, &buffer_start, buffer_end, line)) {
            // 메시지 처리
            if (strcmp(line, "YOUR_TURN") == 0) {
                // 자신의 턴일 때 공격 입력
                printf("당신의 턴입니다. 30초 안에 공격할 좌표를 입력하세요.\n");
                Attack attack;
                int valid_attack = 0;

                while (!valid_attack) {
                    printf("공격할 좌표 입력 (x y): ");
                    if (scanf("%d %d", (int *)&attack.x, (int *)&attack.y) != 2) {
                        printf("유효하지 않은 입력입니다. 숫자를 입력해주세요.\n");
                        while (getchar() != '\n'); // 입력 버퍼 비우기
                        continue;
                    }
                    while (getchar() != '\n'); // 입력 버퍼 비우기

                    // 좌표 유효성 검사
                    if (attack.x < 0 || attack.x >= BOARD_SIZE || attack.y < 0 || attack.y >= BOARD_SIZE || enemy_board[attack.x][attack.y] != '.') {
                        printf("유효하지 않은 좌표입니다. 보드 범위 내의 좌표를 입력해주세요 (0-%d).\n", BOARD_SIZE - 1);
                        continue;
                    }

                    // 최근 공격 좌표 저장
                    last_attack_x = attack.x;
                    last_attack_y = attack.y;

                    // Convert to network byte order before sending
                    attack.x = htonl(attack.x);
                    attack.y = htonl(attack.y);

                    // 공격 좌표 전송
                    if (send_all(sock, &attack, sizeof(Attack)) == -1) {
                        perror("공격 좌표 전송 실패");
                        close(sock);
                        exit(EXIT_FAILURE);
                    }
                    valid_attack = 1;
                }
            }
            else if (strcmp(line, "HIT") == 0) {
                // 공격이 명중했을 때
                printf("명중!\n");
                if (last_attack_x != -1 && last_attack_y != -1) {
                    enemy_board[last_attack_x][last_attack_y] = 'X';
                }
                print_combined_board();
            }
            else if (strcmp(line, "MISS") == 0) {
                // 공격이 실패했을 때
                printf("실패.\n");
                if (last_attack_x != -1 && last_attack_y != -1) {
                    enemy_board[last_attack_x][last_attack_y] = 'O';
                }
                print_combined_board();
            }
            else if (strcmp(line, "INVALID_ATTACK") == 0) {
                // 유효하지 않은 공격
                printf("이미 공격한 좌표입니다. 다시 시도하세요.\n");
                // 최근 공격 좌표 초기화
                last_attack_x = -1;
                last_attack_y = -1;
            }
            else if (strncmp(line, "OPPONENT_ATTACK", 15) == 0) {
                // 상대방의 공격 결과를 처리
                int opp_x, opp_y;
                char opp_result[10];
                int parsed = sscanf(line, "OPPONENT_ATTACK %d %d %s", &opp_x, &opp_y, opp_result);
                if (parsed == 3) {
                    if (strcmp(opp_result, "HIT") == 0) {
                        board[opp_x][opp_y] = 'X';
                        printf("상대방이 (%d, %d)을 공격했습니다. 명중!\n", opp_x, opp_y);
                    }
                    else if (strcmp(opp_result, "MISS") == 0) {
                        board[opp_x][opp_y] = 'O';
                        printf("상대방이 (%d, %d)을 공격했습니다. 실패.\n", opp_x, opp_y);
                    }
                    print_combined_board();
                } else {
                    printf("잘못된 OPPONENT_ATTACK 메시지 형식: %s\n", line);
                }
            }
            else if (strcmp(line, "VICTORY") == 0) {
                // 승리 메시지
                printf("축하합니다! 당신이 승리하였습니다.\n");
                break;
            }
            else if (strcmp(line, "DEFEAT") == 0) {
                // 패배 메시지
                printf("안타깝습니다. 당신이 패배하였습니다.\n");
                break;
            }
            else if (strcmp(line, "TIMEOUT_VICTORY") == 0) {
                // 시간 초과로 인한 승리
                printf("상대방이 턴을 내지 않아 당신이 승리하였습니다.\n");
                break;
            }
            else if (strcmp(line, "TIMEOUT_DEFEAT") == 0) {
                // 시간 초과로 인한 패배
                printf("턴 시간 초과로 인해 패배하였습니다.\n");
                break;
            }
            else if (strcmp(line, "WAIT_TURN") == 0) {
                // 자신의 턴이 아님을 알림
                printf("상대방의 턴입니다. 기다려주세요.\n");
            }
            else {
                // 기타 메시지 처리
                printf("서버 메시지: %s\n", line);
            }

            // 보드 출력
            print_combined_board();
        }

        // 버퍼를 정리하여 사용하지 않은 데이터를 앞으로 이동
        if (buffer_start == buffer_end) {
            // 버퍼를 모두 사용했으므로 인덱스 초기화
            buffer_start = 0;
            buffer_end = 0;
        } else if (buffer_start > 0) {
            // 사용되지 않은 데이터를 버퍼의 시작으로 이동
            memmove(buffer, buffer + buffer_start, buffer_end - buffer_start);
            buffer_end -= buffer_start;
            buffer_start = 0;
        }
    }

    close(sock);
    return 0;
}
