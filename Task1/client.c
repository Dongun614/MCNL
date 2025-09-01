#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char* message);

struct file_node{
    long file_size;
    unsigned char file_name[32];
};

int main(int argc, char* argv[]){
    //잘못된 명령어 예외 처리
    if(argc != 3){
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    //소캣 생성 과정
    int sd;
    struct sockaddr_in serv_adr;
    sd = socket(PF_INET, SOCK_STREAM, 0);

    //메모리 세팅
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    printf("프로그램을 시작합니다..\n");
    FILE *fp;
    struct file_node file_node_list[BUF_SIZE];
    int con = 1;
    while(1){
        int idx;

        read(sd, &idx, sizeof(int));
        int test = read(sd, file_node_list, sizeof(struct file_node) * idx);
        printf("파일 목록: \n");
        for(int i=0; i<idx; i++){
            printf("[%d] 파일 이름 : %s | 파일 사이즈 : %ld\n", i+1, file_node_list[i].file_name, file_node_list[i].file_size);
        }

        int choose;
        printf("원하시는 파일을 선택해주세요(0 입력 시 종료): ");
        scanf("%d", &choose);
        choose--;

        if(choose == -1){
            printf("프로그램을 종료합니다..\n");
            break;
        }

        write(sd, &choose, sizeof(int));

        long file_cnt;
        read(sd, &file_cnt, sizeof(long));
        
        fp = fopen(file_node_list[choose].file_name, "wb");
        char buf[1024];

        int read_cnt = 0;
        int len;

        while(read_cnt < file_cnt){
            len = read(sd, buf, BUF_SIZE);
            read_cnt += len;
            fwrite((void*)buf, 1, len, fp);
        }

        fclose(fp);


        puts("Received file data\n");

        printf("continue? (1: cont, 0: exit)> ");
        scanf("%d", &con);
        write(sd, &con, sizeof(int));
        if(con == 0){
            printf("프로그램을 종료합니다..\n");
            break;
        }
    }

    close(sd);
    return 0;
}


void error_handling(char* message){
    printf("%s", message);
}