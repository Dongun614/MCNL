#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
void error_handling(char* message);
void* server_do(void* clnt_sock);
int compare(const void* a, const void* b);

struct word{
    int count;
    char name[128];
};

int main(int argc, char* argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

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
        error_handling("bind() error\n");
    }
    if(listen(serv_sock, 10) == -1){
        error_handling("listen() error");
    }

    clnt_adr_sz = sizeof(clnt_adr);
    while(1){
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(clnt_sock == -1){
            error_handling("accept() error");
        }
        else{
            pthread_t pid;
            printf("Connected client %d\n", clnt_sock);
            int* client_num = malloc(sizeof(int));
            *client_num = clnt_sock;
            pthread_create(&pid, NULL, server_do, (void*)client_num);
            pthread_detach(pid);
        }
    }

    close(serv_sock);
    return 0;
}

void error_handling(char* message){
    printf("%s\n", message);
    exit(1);
}

void* server_do(void* clnt_sock){
    int clnt_sd = *((int*)clnt_sock);
    char msg[BUF_SIZE];
    int recv_idx;
    int idx = 0;
    //일치하는 값들 담을 문자열


    while(1){
        read(clnt_sd, &recv_idx, sizeof(int));
        if(recv_idx < idx){
            for(int i=recv_idx; i<idx; i++){
                msg[i] = '\0';
                idx--;
            }
        }else{
            idx = recv_idx;
        }
        if(idx < 0){
            idx = 0;
        }
        read(clnt_sd, msg, sizeof(char)*idx);
        write(clnt_sd, &idx, sizeof(int));
        printf("idx: %d\n", idx);
        if(idx > 0){
            struct word* wordlist = (struct word*)malloc(sizeof(struct word)*100);
            int word_idx = 0;

            //파일 작업
            FILE* fp_read;
            fp_read = fopen("text.txt", "rb");

            //같은 문자열 있으면 저장
            char buf[BUF_SIZE];
            while(fgets(buf, BUF_SIZE, fp_read) != NULL){
                if(strstr(buf, msg) != NULL){
                    //여기서 토크나이즈해서 구조체에 넣기
                    strcpy(wordlist[word_idx].name, strtok(buf, "|"));
                    wordlist[word_idx++].count = atoi(strtok(NULL, " "));
                }
            }

            qsort(wordlist, word_idx, sizeof(struct word), compare);

            write(clnt_sd, &word_idx, sizeof(int));
            write(clnt_sd, wordlist, sizeof(struct word)*word_idx);
            fflush(fp_read);
        }
    }

    close(clnt_sd);
    return NULL;
}

int compare(const void* a, const void* b){
    struct word w1 = *((struct word*)a);
    struct word w2 = *((struct word*)b);

    if(w1.count > w2.count){
        return -1;
    } else if(w1.count < w2.count){
        return 1;
    } else {
        return 0;
    }
}