#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "battleship.h"
#define NUM_SHIPS 5

// 보드
char board[BOARD_SIZE][BOARD_SIZE];       // 자신의 배치 보드
char enemy_board[BOARD_SIZE][BOARD_SIZE]; // 상대방 공격 결과 보드

// 배 정보
Ship ships[] = {
    {"항공모함", 5},
    {"전함", 4},
    {"순양함", 3},
    {"잠수함", 3},
    {"구축함", 2}
};



// 두 개의 보드를 나란히 출력 (기존 내용 지우고 새로 출력)
void print_combined_board() {
    // 콘솔 화면 초기화 (ANSI 이스케이프 시퀀스)
    printf("\033[H\033[J"); // 화면 초기화 및 커서 이동

    printf("\n내 배 보드           | 공격 결과 보드\n");
    printf("  ");
    for (int i = 0; i < BOARD_SIZE; i++) printf("%d ", i);
    printf("      ");
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


// 배를 배치하고 서버로 보드 전송
void place_and_send_ships(int sock) {
    printf("배를 배치하세요:\n");
    for (int i = 0; i < NUM_SHIPS; i++) {
        int x, y;
        char orientation;

        while (1) {
            printf("%s (크기: %d칸)을 배치합니다.\n", ships[i].name, ships[i].size);
            printf("배치 시작 좌표 (x, y)와 방향(H: 가로, V: 세로)을 입력하세요: ");
            scanf("%d %d %c", &x, &y, &orientation);

            if (is_valid_placement(board, x, y, ships[i].size, orientation)) {
                place_ship(board, x, y, ships[i].size, orientation);
                print_board(board);
                break;
            } else {
                printf("유효하지 않은 배치입니다. 다시 시도하세요.\n");
            }
        }
    }

    // 서버로 보드 전송
    printf("서버로 보드 정보를 전송합니다...\n");
    if (send(sock, board, sizeof(board), 0) == -1) {
        perror("보드 전송 실패");
        exit(EXIT_FAILURE);
    }
    printf("보드 전송 완료.\n");
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 소켓 생성
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

    // 서버 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }

    printf("서버 연결 완료. 보드 초기화...\n");
    initialize_board(board);
    initialize_board(enemy_board);
    place_and_send_ships(sock);

    // 게임 루프
    while (1) {
        Attack attack;
        char result[100];

        printf("공격할 좌표 입력 (x, y): ");

        // 입력 값 검증
        if (scanf("%d %d", &attack.x, &attack.y) != 2) {
            printf("유효하지 않은 입력입니다. 숫자를 입력해주세요.\n");
            while (getchar() != '\n'); // 입력 버퍼 비우기
            continue;
        }

        // 좌표 유효성 검사
        if (attack.x < 0 || attack.x >= BOARD_SIZE || attack.y < 0 || attack.y >= BOARD_SIZE) {
            printf("유효하지 않은 좌표입니다. 보드 범위 내의 좌표를 입력해주세요 (0-%d).\n", BOARD_SIZE - 1);
            continue;
        }

        // 서버로 공격 좌표 전송
        send(sock, &attack, sizeof(Attack), 0);

        // 서버로부터 결과 수신
        int bytes_received = recv(sock, result, sizeof(result), 0);

        if (bytes_received == 0) {
            printf("서버가 연결을 종료했습니다.\n");
            break;
        }

        // **상대방 종료 메시지 처리**
        if (strstr(result, "상대방이 게임을 종료했습니다.")) {
            printf("%s\n", result); // 메시지 출력
            break; // 게임 종료
        }

        // 공격 결과 표시
        if (strstr(result, "명중")) {
            enemy_board[attack.x][attack.y] = 'O'; // 명중
        } else {
            enemy_board[attack.x][attack.y] = 'X'; // 실패
        }

        printf("결과: %s\n", result);

        // 보드 출력
        print_combined_board();

        // 승리 또는 패배 메시지 처리
        if (strstr(result, "승리하였습니다.") || strstr(result, "패배하였습니다.")) {
            printf("%s\n", result);
            break;
        }
    }


    close(sock);
    return 0;
}
