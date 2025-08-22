#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
void error_handling(char* message);

struct file_node{
    long file_size;
    unsigned char file_name[32];
};

int main(int argc, char* argv[]){
    //잘못된 명령어 예외 처리
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //소캣 생성 과정
    int serv_sd, clnt_sd;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    //메모리 세팅
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    listen(serv_sd, 5);

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    FILE* fp;
    struct file_node file_node_list[BUF_SIZE];
    int con = 1;
    printf("프로그램을 시작합니다..\n");
    while(con){
        //여기에 파일 관련
        DIR* dp = opendir(".");

        if(dp == NULL){
            error_handling("Can not open file..");
            return -1;
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
        
        int choose;

        read(clnt_sd, &choose, sizeof(int));

        if(choose == -1){
            printf("프로그램을 종료합니다..");
            break;
        }

        write(clnt_sd, &(file_node_list[choose].file_size), sizeof(long));

        fp = fopen(file_node_list[choose].file_name, "rb");

        char buf[BUF_SIZE];

        int total  = 0;

        while(1){ //파일 사이즈 만큼 받을 수 있게
            int read_size = fread((void*)buf, 1, BUF_SIZE, fp);
            if(read_size < BUF_SIZE){
                write(clnt_sd, buf, sizeof(char)*read_size);
                break;
            }
            write(clnt_sd, buf, sizeof(char)*BUF_SIZE);
        }
        fclose(fp);

        read(clnt_sd, &con, sizeof(int));
        if(con == 0){
            printf("프로그램을 종료합니다..\n");
        }
    }

    close(clnt_sd);
    close(serv_sd);
    
    return 0;
}

void error_handling(char* message){
    printf("%s", message);
}