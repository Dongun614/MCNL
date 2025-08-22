#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#define BUF_SIZE 1024
void error_handling(char* message);

struct pkt{
    unsigned int seq;
    unsigned int data_length;
    unsigned char data[1024];
};

struct ack{
    unsigned int seq;
};

struct file_node{
    long file_size;
    unsigned char file_name[32];
};

int main(int argc, char* argv[]){
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_adr_sz;

    struct sockaddr_in serv_adr, clnt_adr;
    if(argc != 2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(serv_sock == -1){
        error_handling("UDP socket creation error");
    }

    //time set
    struct timeval tv_timeo = {5, 0};
    setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_timeo, sizeof(tv_timeo));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("bind() error");
    }

    printf("프로그램을 시작합니다..\n");

    int con = 1;
    int ret;
    
    while(con){
        //파일 관련
        FILE* fp;
        struct file_node file_node_list[BUF_SIZE];
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
        closedir(dp);

        //폴더 내 파일 개수 보내기
        sendto(serv_sock, &idx, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, clnt_adr_sz);
        clnt_adr_sz = sizeof(clnt_adr);
        ret = recvfrom(serv_sock, &con, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(ret == -1){
            sendto(serv_sock, &idx, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, clnt_adr_sz);
        }

        //폴더 내용 보내기
        sendto(serv_sock, &file_node_list, sizeof(struct file_node)*idx, 0,
            (struct sockaddr*)&clnt_adr, clnt_adr_sz);
        clnt_adr_sz = sizeof(clnt_adr);
        ret = recvfrom(serv_sock, &con, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(ret == -1){
            sendto(serv_sock, &file_node_list, sizeof(struct file_node)*idx, 0,
            (struct sockaddr*)&clnt_adr, clnt_adr_sz);
        }

        //파일 인덱스 받기
        int choose;
        clnt_adr_sz = sizeof(clnt_adr);
        ret = recvfrom(serv_sock, &choose, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        
        //해당 파일 사이즈 보내기
        sendto(serv_sock, &(file_node_list[choose].file_size), sizeof(long), 0,
            (struct sockaddr*)&clnt_adr, clnt_adr_sz);
        clnt_adr_sz = sizeof(clnt_adr);
        ret = recvfrom(serv_sock, &con, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(ret == -1){
            sendto(serv_sock, &(file_node_list[choose].file_size), sizeof(long), 0,
            (struct sockaddr*)&clnt_adr, clnt_adr_sz);
        }

        //해당 파일 보내기
        clock_t start = clock();
        fp = fopen(file_node_list[choose].file_name, "rb");
        struct pkt pak;
        struct ack ak;
        pak.seq = 0;
        int total_read_size = 0;
        while(1){
            int read_size = fread((void*)pak.data, 1, BUF_SIZE, fp);
            pak.data_length = read_size;
            if(read_size < BUF_SIZE){
                sendto(serv_sock, &pak, sizeof(struct pkt), 0, 
                    (struct sockaddr*)&clnt_adr, clnt_adr_sz);
                clnt_adr_sz = sizeof(clnt_adr);
                ret = recvfrom(serv_sock, &ak, sizeof(struct ack), 0,
                    (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if(ret == -1){
                    printf("재전송 합니다..\n");
                    sendto(serv_sock, &pak, sizeof(struct pkt), 0, 
                    (struct sockaddr*)&clnt_adr, clnt_adr_sz);
                }else{
                    pak.seq++;
                }
                break;
            }
            sendto(serv_sock, &pak, sizeof(struct pkt), 0, 
                (struct sockaddr*)&clnt_adr, clnt_adr_sz);
            clnt_adr_sz = sizeof(clnt_adr);
            printf("total : %d\n", total_read_size+=read_size);
            ret = recvfrom(serv_sock, &ak, sizeof(struct ack), 0,
                (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
            if(ret == -1){
                printf("재전송 합니다..\n");
                sendto(serv_sock, &pak, sizeof(struct pkt), 0, 
                (struct sockaddr*)&clnt_adr, clnt_adr_sz);
            }else{
                pak.seq++;
            }
        }
        clock_t end = clock();
        printf("ThroughPut: %lf\n", file_node_list[choose].file_size /(double)(end - start));
        clnt_adr_sz = sizeof(clnt_adr);
        ret = recvfrom(serv_sock, &con, sizeof(int), 0,
            (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        fclose(fp);
        if(con == 0){
            printf("프로그램을 종료합니다..\n");
            break;
        }
    }

    close(serv_sock);
    return 0;
}

void error_handling(char* message){
    printf("%s\n", message);
    exit(1);
}