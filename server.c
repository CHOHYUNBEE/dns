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
int domainorip(char *src);
void logfile(char *client_ip, char *msg);

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
            //log 정보 파일 (클라이언트가 검색을 시작했을 때)
            logfile(inet_ntoa(clnt_addr.sin_addr), "connect client");
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

                // 도메인인지 아이피인지 확인하여 정보 가져오기
                if(domainorip(message)) {
                    host = gethostbyname(message);
                }
                else {
                    memset(&addr, 0, sizeof(addr));
                    addr.sin_addr.s_addr = inet_addr(message);
                    host = gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);
                }
                // 호스트의 값이 없으면
                if(host == NULL) {
                    memset(message, 0, BUFSIZE);
                    strcpy(message, "host not found\n");
                    write(clnt_sock, message, strlen(message));
                    memset(message, 0, BUFSIZE);
                    continue;
                }

                //hostent에 있는 이름 주소타입 등의 정보를 result 배열에 저장
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
            // log 정보 파일 (클라이언트가 검색을 끝냈을 때)
            logfile(inet_ntoa(clnt_addr.sin_addr), "disconnect client");
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

//로그 파일을 만들기 위한 함수 [클라이언트의 방문 년월일과 시간, IP와 연결이 끊어졌을 때의 시간 및 정보]
void logfile(char *client_ip, char *msg) {
    FILE *f = fopen("log.dat", "a");

    if(!f) return;

    char dateandtime[30] = "";
    struct tm *t;

    time_t curdatetime;
    time(&curdatetime);
    t = localtime(&curdatetime);

    strftime(dateandtime, 30, "%Y.%m.%d %H:%M:%S" ,t);
    fprintf(f, "%s %s %s\n", dateandtime, client_ip, msg);
    printf("%s %s %s\n", dateandtime, client_ip, msg);

    fclose(f);
}
