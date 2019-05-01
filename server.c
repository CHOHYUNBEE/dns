#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <time.h>

#define BUFSIZE 1024

void error_handling(char *message);
void z_handler(int sig);
int domain_check(char *src);
void save_log(char *client_ip, char *msg);

struct sockaddr_in addr;

int main(int argc, char **argv)
{
    int serv_sock;
    int clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;

    struct sigaction act;
    int addr_size, str_len, state;
    pid_t pid;
    char buf[BUFSIZE] = "";
    char message[BUFSIZE] = "";
    char *token = NULL;

    if(argc!=2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    act.sa_handler=z_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;

    state=sigaction(SIGCHLD, &act, 0);                              /* 시그널 핸들러 등록 */
    if(state != 0){
        puts("sigaction() error");
        exit(1);
    }

    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
        error_handling("bind() error");

    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    while(1)
    {
        addr_size=sizeof(clnt_addr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr, &addr_size);
        if(clnt_sock==-1)
            continue;

        /* 클라이언트와의 연결을 독립적으로 생성 */
        if( (pid=fork()) ==-1) {                /* fork 실패 시 */
            close(clnt_sock);
            continue;
        }

        else if( pid>0 ) {                             /* 부모 프로세스인 경우 */
            save_log(inet_ntoa(clnt_addr.sin_addr), "connect client");
            close(clnt_sock);
            continue;
        }
        else {                                       /* 자식 프로세스의 경우 */
            close(serv_sock);
            /* 자식 프로세스의 처리영역 : 데이터 수신 및 전송 */

            while(1) {
                str_len = read(clnt_sock, message, BUFSIZE);
                if(str_len <= 0) break;

                struct hostent *host = NULL; // Test

                strcat(buf,message);

                char result[BUFSIZE] = "";
                char buf[BUFSIZE] = "";

                // domain or ip
                if(domain_check(message)) {
                    host = gethostbyname(message);
                }
                else {
                    memset(&addr, 0, sizeof(addr));
                    addr.sin_addr.s_addr = inet_addr(message);
                    host = gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);
                }

                if(host == NULL) {
                    memset(message, 0, BUFSIZE);
                    strcpy(message, "host not found\n");
                    write(clnt_sock, message, strlen(message));
                    memset(message, 0, BUFSIZE);
                    continue;
                }
                strcat(result, host->h_name);
                strcat(result, " ");

                sprintf(buf, "%d ", host->h_addrtype);
                strcat(result, buf);

                memset(buf, 0, BUFSIZE);
                sprintf(buf, "%d ", host->h_length);
                strcat(result, buf);


                if(host->h_aliases[0] != NULL) {
                    for(int i=0; host->h_aliases[i]; i++) {
                        strcat(result, host->h_aliases[i]);
                        strcat(result, " ");
                    }
                }
                if(host->h_addr_list[0] != NULL) {
                    for(int i=0; host->h_addr_list[i]; i++) {
                        strcat(result, inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
                        strcat(result, " ");
                    }
                }
                memset(message, 0, BUFSIZE);
                strcpy(message, result);

                write(clnt_sock, message, strlen(message));


                memset(message, 0, BUFSIZE);
            }
            save_log(inet_ntoa(clnt_addr.sin_addr), "disconnect client");
            close(clnt_sock);
            exit(0);
        }
    }
    return 0;
}

void z_handler(int sig)
{
    pid_t pid;
    int rtn;

    pid=waitpid(-1, &rtn, WNOHANG);
}
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int domain_check(char *src) {
    for(int i=0; src[i]; i++) {
        if(src[i] == '.') {
            continue;
        }
        if(isalpha(src[i]) == 0) return 0;
    }
    return 1;
}

void save_log(char *client_ip, char *msg) {
    FILE *f = fopen("log.dat", "a");

    if(!f) return;

    char datetime[30] = "";
    struct tm *t;

    time_t curdatetime;
    time(&curdatetime);
    t = localtime(&curdatetime);

    strftime(datetime, 30, "%Y%m%d %H:%M:%S" ,t);
    fprintf(f, "%s %s %s\n", datetime, client_ip, msg);
    printf("%s %s %s\n", datetime, client_ip, msg);

    fclose(f);
}
