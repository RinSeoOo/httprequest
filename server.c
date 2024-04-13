#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
#include <stdlib.h>
#include <signal.h>


#pragma comment(lib, "ws2_32.lib") //Winsock Library

#define NOT_FOUND_CONTENT       "<h1>404 Not Found</h1>\n"
#define SERVER_ERROR_CONTENT    "<h1>500 Internal Server Error</h1>\n"
#define SERVERPORT 8080
#define BUFSIZE 400000
#define HEADER_FMT "HTTP/1.1 %d %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n"


void fill_header(char *header, int status, long len, char *type){
    char status_txt[40];
    switch(status){
        case 200:
            strcpy(status_txt, "OK");
        break;
        case 404:
            strcpy(status_txt, "Not Found");
            break;
        case 506:
            strcpy(status_txt, "Unassigned");
        break;
        case 500:
        default:
            strcpy(status_txt, "Internal Server Error");
            break;
    }
    sprintf(header, HEADER_FMT, status, status_txt, len, type);
}



int main(int argc, char* argv[]) {
    WSADATA wsaData;
    int servsock;
    SOCKADDR_IN servaddr;
    char hello[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<doctype !html><html><head><title>Hello World</title></head>"
    "<body><h1>Hello world! my name is rinrinrn</h1></body></html>\r\n";
    char szBuff[BUFSIZE];



    // port 입력
    if (argc < 2) {
        printf("Usage: %s {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    printf("[INFO] The server will listen to port: %d.\n", port);



    // WSA 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }


    // create socket
    servsock = socket(AF_INET, SOCK_STREAM, 0);
    if (servsock == INVALID_SOCKET) {
        perror("[ERROR] can not create socket");
        closesocket(servsock);
        WSACleanup();
        exit(EXIT_FAILURE);
    }


    // socket structure
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;



    // binding
    if (bind(servsock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        fprintf(stderr, "Bind failed with error: %d\n", WSAGetLastError());
        closesocket(servsock);
        WSACleanup();
        return -1;
    }
    printf("bind() success\n");



    // listen
    if (listen(servsock, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(servsock);
        WSACleanup();
        return -1;
    }
    printf("socket() success\n");



    // signal(SIGCHLD, SIG_IGN);

    // accept
    SOCKET clisock;
    struct sockaddr_in cliaddr;
    int addrlen = sizeof(struct sockaddr_in);
    // FILE *index_html;
    int cli_len;



    char header[BUFSIZE];
    printf("\n++++Waiting for new connection ++++\n\n");
    while (1) {
        clisock = accept(servsock, (struct sockaddr *)&cliaddr, &addrlen);
        if (clisock == INVALID_SOCKET) {
            printf("Accept failed with error: %d\n", WSAGetLastError());
            closesocket(servsock);
            WSACleanup();
            return -1;
        }
        printf("client accept() success\n");

        if(clisock == -1){
            perror("Unable to accept conneciton.\n");
            continue;
        }

        printf("accepted connection from %s, port %d\n",
        inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));


        cli_len = recv(clisock, szBuff, sizeof(szBuff), 0);       
        
        char header2[BUFSIZE];
        // FILE *html_file = fopen("index.html", "r");

        if (strstr(szBuff, "GET /index.html") != NULL) {
            FILE *html_file = fopen("index.html", "r");
        
            if (!html_file) {
                printf("Failed to open file\n");
                fill_header(header2, cli_len, 0, "text/html");
                send(clisock, header2, strlen(header2), 0);
            } else {
                fseek(html_file, 0, SEEK_END);
                long content_length = ftell(html_file);
                fseek(html_file, 0, SEEK_SET);

                cli_len = send(clisock, header2, strlen(header2), 0);
                fill_header(header2, cli_len, content_length, "text/html");

                size_t bytes_read;
                while ((bytes_read = fread(szBuff, 1, sizeof(szBuff), html_file)) > 0) {
                    cli_len = send(clisock, szBuff, bytes_read, 0);
                }
                // while (!feof(html_file)) {
                //     size_t bytes_read = fread(szBuff, 1, sizeof(szBuff), html_file);
                //     send(clisock, szBuff, bytes_read, 0);
                // }
                fclose(html_file);
            }
            printf("\nheader: %s\n", header2);
        }
        else {
            // size_t bytes_read = read(szBuff, 1, sizeof(szBuff), hello);
            // cli_len = send(clisock, szBuff, bytes_read, 0);
            cli_len = send(clisock, hello, sizeof(hello) - 1, 0);
            fill_header(header, cli_len, sizeof(szBuff), "text/html");
            printf("\nheader: %s\n", header);
        }

        
        printf("Bytes Received: %d, message: %s from %s\n", cli_len, szBuff, inet_ntoa(cliaddr.sin_addr));
        
        
        
        if(cli_len == 0){
            printf("Client closed connection\n");
            closesocket(clisock);
            return -1;
        }
        else if(cli_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }

        if(cli_len == 0){
            printf("Client closed connection\n");
            closesocket(clisock);
            return -1;
        }
        

        // Close client socket
        closesocket(clisock);
        // fclose(html_file);
    }


    // Cleanup
    closesocket(servsock);
    WSACleanup();
    return 0;
}
