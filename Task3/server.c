#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024

struct file_node{
    long file_size;
    unsigned char file_name[32];
};

void error_handling(char* buf);
//해당 디렉토리에 있는 파일 리스트 생성
void make_file_list(int clnt_sd, struct file_node* file_node_list);
//사용자가 선택한 파일 보내기
void send_file(int clnt_sd, struct file_node* file_node_list, int choose);
//사용자가 업로드한 파일 받기
void receive_file(int clnt_sd, char* file_name);

int main(int argc, char* argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    fd_set reads, cpy_reads;

    socklen_t adr_sz;
    int fd_max, str_len, fd_num, i;
    char buf[BUF_SIZE];
    if(argc != 2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("bind() error");
    }
    if(listen(serv_sock, 5) == -1){
        error_handling("listen() error");
    }

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    while(1){
        cpy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;

        if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == - 1){
            break;
        }
        if(fd_num == 0){
            continue;
        }

        for(i=0; i<fd_max+1; i++){
            if(FD_ISSET(i, &cpy_reads)){
                if(i == serv_sock){
                    adr_sz = sizeof(clnt_adr);
                    clnt_sock =
                        accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                    FD_SET(clnt_sock, &reads);
                    if(fd_max < clnt_sock){
                        fd_max = clnt_sock;
                    }
                    printf("%d client와 연결되었습니다..\n", clnt_sock);
                }else{
                    int size;
                    char receive_msg[BUF_SIZE];
                    read(i, &size, sizeof(int));
                    read(i, receive_msg, sizeof(char) * size);
                    receive_msg[size] = '\0'; //예를 들어 cd ../ 를 저장한 상태
                    struct file_node file_nodes[BUF_SIZE];

                    //토큰나이즈
                    char* command[2];
                    int idx = 0;
                    char* ptr = strtok(receive_msg, " ");
                    while(ptr != NULL || idx < 2){
                        command[idx++] = ptr;
                        ptr = strtok(NULL, " ");
                    }
                    if(idx != 2){
                        printf("잘못된 값을 입력히였습니다\n");
                    }

                    if((strcmp(command[0], "Q") == 0) || (strcmp(command[0], "q") == 0)){
                        printf("%d client와 연결을 종료합니다\n", i);
                        FD_CLR(i, &reads);
                    } else if(strcmp(command[0], "cd") == 0){
                        chdir(command[1]);
                        make_file_list(i, file_nodes);
                    } else if(strcmp(command[0], "ls") == 0){
                        make_file_list(i, file_nodes);
                    } else if(strcmp(command[0], "down") == 0){
                        int choose = *command[1] - '0';
                        send_file(i, file_nodes, choose);
                    } else if(strcmp(command[0], "up") == 0){
                        receive_file(i, command[1]);
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}

void error_handling(char* buf){
    printf("%s\n", buf);
    exit(1);
}

void make_file_list(int clnt_sd, struct file_node* file_node_list){
    DIR* dp = opendir(".");

    if(dp == NULL){
        error_handling("Can not open file..");
        exit(1);
    }

    struct dirent* entry;
    int idx = 0;

    while((entry = readdir(dp)) != NULL){
        //.. 또는 . 으로 된 파일들 건너뛰기
        if(strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0){
            continue;
        }
        //파일 이름 저장
        strcpy(file_node_list[idx].file_name, entry->d_name);

        //사이즈 측정을 위해 stat을 사용, 이를 위해 path값 만들기
        struct stat st;
        char path[64];

        strcpy(path, "./");
        strcat(path, entry->d_name);
        stat(path, &st);
        //off_t를 long으로 캐스팅하여 파일 사이즈 저장
        file_node_list[idx++].file_size = (long) st.st_size;
    }
    write(clnt_sd, &idx, sizeof(int));
    write(clnt_sd, file_node_list, sizeof(struct file_node)*idx);
}

void send_file(int clnt_sd, struct file_node* file_node_list, int choose){
    choose--;
    write(clnt_sd, &(file_node_list[choose].file_size), sizeof(long));

    FILE* fp;
    fp = fopen(file_node_list[choose].file_name, "rb");
    char buf[BUF_SIZE];
    int read_cnt;
    while(1){
        read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
        if(read_cnt < BUF_SIZE){
            write(clnt_sd, buf, sizeof(char)*read_cnt);
            break;
        }
        write(clnt_sd, buf, sizeof(char)*BUF_SIZE);
    }
    fclose(fp);
}

void receive_file(int clnt_sd, char* file_name){
    long file_cnt;
    read(clnt_sd, &file_cnt, sizeof(long));

    FILE* fp;
    fp = fopen(file_name, "wb");
    char buf[BUF_SIZE];
    int read_cnt = 0;
    int len;

    while(read_cnt < file_cnt){
        len = read(clnt_sd, buf, BUF_SIZE);
        read_cnt += len;
        fwrite(buf, 1, len, fp);
    }
    fclose(fp);
    printf("다운로드 된 파일이 있습니다..\n");
}