#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE 1024
#define MAXCACHESIZE 100

void error_handling(char *message);
void save_file(char *message);
void load_file(char **message, int *cnt);
void print_host(char *message);

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUFSIZE];
    char cache[MAXCACHESIZE][BUFSIZE] = {""};
    int str_len;
    int cnt = 0;
    int i = 0;

    if(argc!=3)
    {
        printf("Usage: %s <IP> <port> \n",argv[0]);
        exit(1);
    }

    load_file(cache, &cnt);

    sock=socket(PF_INET,SOCK_STREAM,0);
    if(sock==-1) error_handling("socket() error");

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
        error_handling("connet() error");

    while(1)
    {
        memset(message, 0, BUFSIZE);
        //메시지 입력 및 전송
        fputs("전송할 메세지를 입력하세요 (q to quit):" ,stdout);
        fgets(message,BUFSIZE,stdin);
        message[strlen(message)-1] = 0;

        if(!strcmp(message,"q")) break;
        if(!strcmp(message, "show array")) {
            for(int i=0; i < cnt; i++) {
                printf("%s\n", cache[i]);
            }
            continue;
        }

        // search array
        int check = 0;
        for(int i=0; i < cnt; i++) {
            char *token = NULL;
            char temp[BUFSIZE] = "";
            strcpy(temp, cache[i]);

            token = strtok(temp, "\t");
            if(!strcmp(temp, message)) {
                check = 1;
                printf("\ncached data\n\n");
                token = strtok(NULL, "\t");
                print_host(token);
            }
        }
        if(check) {
            memset(message, 0, BUFSIZE);
            continue;
        }

        write(sock,message,strlen(message));
        char key[BUFSIZE] = "";
        strcpy(key, message);

        read(sock, message, BUFSIZE);

        if(strlen(message) <= 0) {
            printf("read error\nretry\n");
            continue;
        }

        if(!strcmp(message, "host not found\n")) {
            printf("%s\n", message);
            continue;
        }

        // host info save
        strcat(key, "\t");
        strcat(key, message);
        strcpy(cache[cnt++], key);

        // print host
        print_host(message);
        save_file(key);
    }


    close(sock);
    return 0;
}


void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void save_file(char *message) {
    FILE *f = fopen("cache.dat", "a");
    if(!f) return;

    fprintf(f, "%s\n", message);
    fclose(f);
}

void load_file(char **message, int *cnt) {
    FILE *f = fopen("cache.dat", "a");
    if(!f) return;

    // 파일 한줄씩 읽기

    while(1) {
        fgets(~~);
        cnt++;
    }


    fclose(f);
}

void print_host(char *message) {
    char *token = NULL;

    if(message == NULL) return;

    token = strtok(message, " ");
    printf("\n%s\n", token);

    while(token = strtok(NULL," ")) {
        printf("%s\n", token);
    }

    printf("\n");
}