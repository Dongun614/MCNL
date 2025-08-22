#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 30
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
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_sz;

    struct sockaddr_in serv_adr, from_adr;
    if(argc != 3){
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        error_handling("socket() error\n");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_adr.sin_port=htons(atoi(argv[2]));

    int cnt;
    struct file_node file_node_list[BUF_SIZE];
    int con = 1;
    while(con){
        FILE *fp;

        sendto(sock, &con, sizeof(int), 0, 
            (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        printf("프로그램을 시작합니다..\n");
        //파일 개수 받기
        adr_sz = sizeof(from_adr);
        recvfrom(sock, &cnt, sizeof(int), 0,
            (struct sockaddr*)&from_adr, &adr_sz);
        sendto(sock, &con, sizeof(int), 0, 
            (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        //폴더 내용 받기
        adr_sz = sizeof(from_adr);
        recvfrom(sock, &file_node_list, sizeof(struct file_node)*cnt, 0,
            (struct sockaddr*)&from_adr, &adr_sz);
        sendto(sock, &con, sizeof(int), 0, 
            (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        printf("파일 목록: \n");
        for(int i=0; i<cnt; i++){
            printf("[%d] 파일 이름 : %s | 파일 사이즈 : %ld\n", i+1, file_node_list[i].file_name, file_node_list[i].file_size);
        }

        //원하는 파일 입력받기
        int choose;
        printf("원하시는 파일을 선택해주세요(0 입력 시 종료): ");
        scanf("%d", &choose);
        choose--;

        if(choose == -1){
            printf("프로그램을 종료합니다..\n");
            break;
        }

        //파일 인덱스 보내기
        sendto(sock, &choose, sizeof(int), 0,
            (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        //파일 사이즈 받기
        long file_cnt;
        adr_sz = sizeof(from_adr);
        recvfrom(sock, &file_cnt, sizeof(long), 0,
            (struct sockaddr*)&from_adr, &adr_sz);
        sendto(sock, &con, sizeof(int), 0, 
            (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        //해당 파일 받기
        fp = fopen(file_node_list[choose].file_name, "wb");
        struct pkt pak;
        int read_cnt = 0;
        int count = 0;
        while(read_cnt < file_cnt){
            adr_sz = sizeof(from_adr);
            struct ack ak;
            recvfrom(sock, &pak, sizeof(struct pkt), 0,
                (struct sockaddr*)&from_adr, &adr_sz);
            ak.seq = pak.seq;
            read_cnt += pak.data_length;
            fwrite((void*)pak.data, 1, pak.data_length, fp);
            printf("read_cnt: %d\n", read_cnt);
            sendto(sock, &ak, sizeof(struct ack), 0, 
                (struct sockaddr*)&serv_adr, sizeof(serv_adr));
        }
        printf("종료하시겠습니까? (종료하려면 0, 계속하시려면 1): ");
        scanf("%d", &con);
        sendto(sock, &con, sizeof(int), 0,
            (struct sockaddr*)&serv_adr, sizeof(serv_adr));
        fclose(fp);
        if(con == 0){
            printf("프로그램을 종료합니다..\n");
            break;
        }
    }

    close(sock);
    return 0;
}

void error_handling(char* message){
    printf("%s\n", message);
}