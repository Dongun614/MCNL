#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <termios.h>

void error_handling(char* message);
void* client_recv(void* sd);
void* client_send(void* sd);
void clrscr();
void gotoxy(int x, int y);
int getch();

#define BUF_SIZE 1024
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct word{
    int count;
    char name[128];
};

char msg[BUF_SIZE];
int str_len = 0;

int main(int argc, char* argv[]){
    int sock;
    struct sockaddr_in serv_adr;
    pthread_t recv, send;
    
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

    pthread_mutex_lock(&mutex);
    clrscr();
    fflush(stdout);
    gotoxy(1,1);
    printf("search word: ");
    gotoxy(1,2);
    printf("----------------------------------------------");
    gotoxy(13, 1);
    fflush(stdout);
    pthread_mutex_unlock(&mutex);

    pthread_create(&send, NULL, client_send, (void*)&sock);
    pthread_detach(send);
    pthread_create(&recv, NULL, client_recv, (void*)&sock);
    pthread_detach(recv);


    while(1);

    close(sock);

    return 0;
}

void error_handling(char* message){
    printf("%s\n", message);
    exit(1);
}

void* client_recv(void* sd){
    int cl_sock = *((int*)sd);

    while(1){
        int word_cnt;
        int recv_idx;
        read(cl_sock, &recv_idx, sizeof(int));
        if(recv_idx != 0){
            read(cl_sock, &word_cnt, sizeof(int));
            struct word* wordlist = (struct word*)malloc(sizeof(struct word)*word_cnt);
            read(cl_sock, wordlist, sizeof(struct word)*word_cnt);
            
            pthread_mutex_lock(&mutex);
            gotoxy(1,3);
            printf("\033[0J");
            fflush(stdout);

            for(int i=0; i<word_cnt; i++){
                gotoxy(1, 3+i);
                printf("\033[0K");
                char* start = strstr(wordlist[i].name, msg);
                for(char* c = wordlist[i].name; *c != '\0'; c++){
                    if(c == start){
                        printf("\033[0;31m");
                        for(int i=0; i<recv_idx; i++){
                            printf("%c", c[i]);
                        }
                        printf("\033[0m");
                        c += recv_idx - 1;
                    } else{
                        printf("%c", *c);
                    }
                }
            }
            gotoxy(13 + recv_idx, 1);
            fflush(stdout);
            pthread_mutex_unlock(&mutex);
        }else{
            pthread_mutex_lock(&mutex);
            gotoxy(1,3);
            printf("\033[0J");
            fflush(stdout);
            gotoxy(13 + recv_idx, 1);
            fflush(stdout);
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

void* client_send(void* sd){
    int cl_sock = *((int*)sd);
    int input_size = 0;

    while(1){
        char ch = getch();
        if((ch == 8 || ch == 127) && str_len > 0){
            msg[--str_len] = '\0';
            if(str_len == 0){
                write(cl_sock, &str_len, sizeof(int));
            }else{
                write(cl_sock, &str_len, sizeof(int));
                write(cl_sock, msg, sizeof(char)*str_len);
            }
        }else{
            msg[str_len++] = ch;
            msg[str_len] = '\0';
            input_size++;
            write(cl_sock, &str_len, sizeof(int));
            write(cl_sock, msg, sizeof(char)*str_len);
        }
        
        pthread_mutex_lock(&mutex);
        gotoxy(13, 1);
        fflush(stdout);
        printf("%s", msg);
        printf(" ");
        fflush(stdout);
        
        gotoxy(13 + str_len, 1);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void clrscr(){
	fprintf(stdout, "\033[2J\033[0;0f");
	fflush(stdout);
}

void gotoxy(int x,int y){
    printf("%c[%d;%df",0x1B,y,x);
}

int getch(){
	int c = 0;
	struct termios oldattr, newattr;

	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	newattr.c_cc[VMIN] = 1;
	newattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
	c = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

	return c;
}