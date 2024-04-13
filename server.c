#include <stdio.h>
#include <winsock2.h>
#include <malloc.h>


#pragma comment(lib, "ws2_32.lib") //Winsock Library

#define BUFSIZE 100000
#define BUFSIZE2 400000
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
    "<doctype !html><html lang=\"ko\"><head><title>Hello World</title><link rel=\"shortcut icon\" href=\"#\"></head>"
    "<body><h1>Hello world!</h1></br>my name is seoyoung. this is my WebServer!</body></html>\r\n";
    char szBuff[BUFSIZE2] = "\0";



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


    // accept
    SOCKET clisock;
    struct sockaddr_in cliaddr;
    int addrlen = sizeof(struct sockaddr_in);
    int cli_len;
    
    // char header[BUFSIZE] = "\0";

    printf("\n++++Waiting for new connection ++++\n\n");
    while (1) {
        // header[BUFSIZE] = 0;
        clisock = accept(servsock, (struct sockaddr *)&cliaddr, &addrlen);
        if (clisock == INVALID_SOCKET) {
            printf("Accept failed with error: %d\n", WSAGetLastError());
            closesocket(servsock);
            WSACleanup();
            return -1;
        }
        printf("client accept() success\n");

        printf("accepted connection from %s, port %d\n",
        inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));


        cli_len = recv(clisock, szBuff, sizeof(szBuff), 0);

        // 종료 조건 
        if(cli_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }
        if(clisock == 0){
            printf("Client closed connection\n");
            closesocket(clisock);
            break; // 클라이언트가 연결을 닫으면 서버 종료
        }

        char header[BUFSIZE2] = "\0";
        if (strstr(szBuff, "GET /index.html") != NULL) {
            FILE *html_file = fopen("index.html", "r");
        
            if (!html_file) {
                printf("Failed to open file\n");
                fill_header(header, 200, 0, "text/html");
                send(clisock, header, strlen(header), 0);
            } else {
                fseek(html_file, 0, SEEK_END);
                long content_length = ftell(html_file);
                fseek(html_file, 0, SEEK_SET);
                
                fill_header(header, 200, content_length, "text/html");
                cli_len = send(clisock, header, strlen(header), 0);

                size_t bytes_read;
                char chunk_header[30] = "\0";
                while ((bytes_read = fread(szBuff, 1, sizeof(szBuff), html_file)) > 0) {
                    //cli_len = send(clisock, szBuff, bytes_read, 0);
                    // 청크 헤더 작성
                    sprintf(chunk_header, "%d\r\n", bytes_read);
                    send(clisock, chunk_header, strlen(chunk_header), 0);
                    
                    // 청크 데이터 전송
                    send(clisock, szBuff, bytes_read, 0);
                    send(clisock, "\r\n", 1, 0);
                }
                // 마지막 청크 전송 (빈 청크)
                send(clisock, "0\r\n\r\n", 5, 0);

            }
            fclose(html_file);
        }
        else if(strstr(szBuff, "GET / ") != NULL) {
            cli_len = send(clisock, hello, sizeof(hello), 0);
            fill_header(header, 200, sizeof(szBuff), "text/html");
        }
        else if (strstr(szBuff, "GET /favicon.ico") != NULL) {
            // Ignore favicon requests and send a 404 Not Found response
            cli_len = 1;
            fill_header(header, 404, 0, "text/html");
            send(clisock, header, strlen(header), 0);
        }
        else{
            cli_len = 1;
            send(clisock, header, strlen(header), 0);
            continue;
        }

        
        printf("\nheader: %s\n", header);
        printf("Bytes Received: %d, message: %s from %s\n\n", cli_len, szBuff, inet_ntoa(cliaddr.sin_addr));
        // header[BUFSIZE] = "\0";
        
        if(cli_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
            WSACleanup();
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
