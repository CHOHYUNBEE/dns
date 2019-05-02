#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#define BUFSIZE 1024
#define MAXCACHESIZE 100


void error_handling(char *message);
void save_file(char *message);
int load_file(char **cache);
void print_hostinfo(char *message);
int domainorip(char *src);

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUFSIZE];
    char **cache;
    int str_len;
    int cnt = 0;
    int i = 0;

    if(argc!=3)
    {
        printf("Usage: %s <IP> <port> \n",argv[0]);
        exit(1);
    }

    cache = (char**)calloc(MAXCACHESIZE,sizeof(char*));
    cnt = load_file(cache);

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

        int check = 0;
        for(int i=0; i < cnt; i++) {
            char *token = NULL;
            char temp[BUFSIZE] = "";
            strcpy(temp, cache[i]);

            token = strtok(temp, "\t");
            //캐시에 등록되어있는 도메인이나 ip라면
            if(!strcmp(temp, message)) {
                check = 1;
                printf("\ncached data\n\n");
                token = strtok(NULL, "\t");
                print_hostinfo(token);
            }
        }
        if(check) {
            memset(message, 0, BUFSIZE);
            continue;
        }

        write(sock,message,strlen(message));
        char key[BUFSIZE] = "";
        strcpy(key, message);

        memset(message, 0, BUFSIZE);
        read(sock, message, BUFSIZE);
        message[strlen(message)] = 0;

        if(strlen(message) <= 0) {
            printf("read error\nretry\n");
            continue;
        }

        if(!strcmp(message, "host not found\n")) {
            printf("%s\n", message);
            continue;
        }

        // 호스트 정보 저장
        strcat(key, "\t");
        strcat(key, message);
        cache[cnt] = (char*)calloc(strlen(key)+1, sizeof(char));
        strcpy(cache[cnt], key);
        cnt++;

        //호스트 출력
        print_hostinfo(message);
        save_file(key);
    }

    free(cache);
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

//캐시 파일에 저장하기 위한 함수 배열로 [입력한 도메인 또는 ip, host_name, hosttype, hostlengh,aliases,address_list의 정보]
int load_file(char **cache) {
    FILE *f = fopen("cache.dat", "r");
    char line[BUFSIZE] = "";
    int cnt = 0;
    if(!f) return cnt;

    // 파일 한줄씩 읽기
    while(!feof(f)) {
        fgets(line,BUFSIZE-1,f);
        if(line[0] == 0) return 0;

        cache[cnt] = (char*)calloc(strlen(line)+1, sizeof(char));
        strcpy(cache[cnt], line);
        cnt++;
    }

    fclose(f);
    return --cnt;
}

//호스트의 정보를 배열로 받아와 보기 좋게 출력하는 함수
void print_hostinfo(char *message) {
    char *token = NULL;
    int i=0,j=0;
    if(message == NULL) return;

    token = strtok(message, " ");
    printf("\nofficial name: %s\n", token);
    token = strtok(NULL," ");
    printf("host address type: %s\n", token);
    token = strtok(NULL," ");
    printf("lengh of host address: %s\n", token);

    while(token = strtok(NULL," ")) {
        if(!token) continue;
        if(!strcmp(token, "\n")) continue;
        if (domainorip(token)) {
            printf("aliases[%d]: %s\n", i ,token);
            i++;
        } else{
            printf("address_list[%d]:%s\n", j,token);
            j++;
        }
    }

    printf("\n");
}

//printhostinfo 함수에서 aliases와 address의 출력을 위해 비교하는 함수
int domainorip(char *src) {
    int count = 0;

    for(int i=0; src[i]; i++) {
        if(src[i] == '.') {
            count++;
            continue;
        }
        if(isalpha(src[i]) == 0) {
            count++;
        }
    }
    if(strlen(src) == count) return 0;
    return 1;
}