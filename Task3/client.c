#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define BUF_SIZE 1024

struct file_node{
    long file_size;
    unsigned char file_name[32];
};

void error_handling(char* message);
void file_handler(int process); //프로그램 전체를 관장하는 함수
void receive_file_list(int sd, struct file_node* file_node_list); //ls, cd를 위한 함수
void receive_file(int sd, struct file_node* file_node_list, int choose); //down을 위한 함수
void send_file(int sd, char* file_name); //up을 위한 함수

int main(int argc, char* argv[]){
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;

    if(argc != 3){
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        error_handling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("connect() error");
    }else{
        puts("Connected............");
    }

    file_handler(sock);
    //receive_file_list(sock);
    //이동, 다운로드, 업로드

    close(sock);

    return 0;
}

void error_handling(char* message){
    printf("%s\n", message);
    exit(1);
}

void file_handler(int sd){
    printf("\n프로그램을 시작합니다..\n\n");
    while(1){
        printf("===  HELP  ==================================\n\n");
        printf("- cd [dir_path] : dir_path에 접속\n");
        printf("- ls : 현재 디렉토리의 파일들 확인\n");
        printf("- down [file_name] : file_name 다운로드\n");
        printf("- up [file_name] : file_name 업로드\n");
        printf("- q or Q : 프로그램 종료\n");
        printf("명령어를 입력해주세요 : ");

        //입력 값 통째로 전송
        char send_msg[BUF_SIZE];
        fgets(send_msg, BUF_SIZE, stdin);
        send_msg[strlen(send_msg) - 1] = '\0';

        int size = strlen(send_msg);
        write(sd, &size, sizeof(int));
        write(sd, &send_msg, sizeof(char) * size);

        //토큰나이즈
        char* command[2];
        int idx = 0;
        char* ptr = strtok(send_msg, " ");
        while(ptr != NULL || idx < 2){
            command[idx++] = ptr;
            ptr = strtok(NULL, " ");
        }
        if(idx != 2){
            printf("잘못된 값을 입력하셨습니다.\n");
        }

        struct file_node file_nodes[BUF_SIZE];

        if((strcmp(command[0], "Q") == 0) || (strcmp(command[0], "q") == 0)){ 
            printf("프로그램을 종료합니다..(서버와의 연결이 종료됩니다)\n");
            break;
        }else{
            //여기서 상황별 함수 동작할 수 있도록
            if((strcmp(command[0], "cd") == 0) || (strcmp(command[0], "ls") == 0)){
                receive_file_list(sd, file_nodes);
            }else if(strcmp(command[0], "down") == 0){
                int choose = *command[1] - '0';
                receive_file(sd, file_nodes, choose);
            }else if(strcmp(command[0], "up") == 0){
                send_file(sd, command[1]);
            }
        }
    }
}

void receive_file_list(int sd, struct file_node* file_node_list){
    int idx;
    read(sd, &idx, sizeof(int));
    read(sd, file_node_list, sizeof(struct file_node) * idx);
    printf("파일 목록: \n");
    for(int i=0; i<idx; i++){
        printf("[%d] 파일 이름 : %s | 파일 사이즈 : %ld\n", i+1, file_node_list[i].file_name, file_node_list[i].file_size);
    }
    printf("\n");
}

void receive_file(int sd, struct file_node* file_node_list, int choose){
    choose--;
    long file_cnt;
    read(sd, &file_cnt, sizeof(long));

    FILE* fp;
    fp = fopen(file_node_list[choose].file_name, "wb");
    char buf[BUF_SIZE];
    int read_cnt = 0;
    int len;

    // while(read_cnt < file_cnt){
    //     len = read(sd, buf, sizeof(char)*BUF_SIZE);
    //     read_cnt += len;
    //     fwrite(buf, 1, len, fp);
    // }
    while(read_cnt < file_cnt){
        len = read(sd, buf, sizeof(char)*BUF_SIZE);
        read_cnt += len;
        if(len < BUF_SIZE){
            fwrite(buf, 1, len, fp);
        }
        fwrite(buf, 1, BUF_SIZE, fp);
    }
    fclose(fp);
    printf("다운로드 완료!!\n\n");
}

void send_file(int sd, char* file_name){
    //사이즈 출력
    struct stat st;
    char path[64];
    strcpy(path, "./");
    strcat(path, file_name);
    stat(path, &st);
    long file_cnt = st.st_size;

    //사이즈 값 전송
    write(sd, &file_cnt, sizeof(long));

    //파일 전송
    FILE* fp;
    fp = fopen(file_name, "rb");
    char buf[BUF_SIZE];

    int read_cnt;

    while(1){
        read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
        if(read_cnt < BUF_SIZE){
            write(sd, buf, sizeof(char) * read_cnt);
            break;
        }
        write(sd, buf, BUF_SIZE);
    }
    fclose(fp);
    printf("업로드 완료!!\n\n");
}