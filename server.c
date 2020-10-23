#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT 6969 
#define MAXLINE 1024

//EXAMPLE FILE COPIED FROM INTERNET

int main()
{ 
    int sock_fd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from server"; 
    struct sockaddr_in server_addr, client_addr; 
      
    //Creating socket file descriptor. 
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&server_addr, 0, sizeof(server_addr)); 
    memset(&client_addr, 0, sizeof(client_addr)); 
      
    //Filling server information 
    server_addr.sin_family = AF_INET; // IPv4 
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(PORT); 
      
    //Bind the socket with the server address 
    if (bind(sock_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    int len, n; 
  
    len = sizeof(client_addr);  //len is value/result 
  
    n = recvfrom(sock_fd, (char*) buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &client_addr, &len); 
    buffer[n] = '\0'; 
    printf("Client : %s\n", buffer); 
    sendto(sock_fd, (const char*)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr*) &client_addr, len); 
    printf("Hello message sent.\n");  
      
    return 0; 
}  
