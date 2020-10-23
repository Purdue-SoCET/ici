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
    char *hello = "Hello from client"; 
    struct sockaddr_in server_addr; 
  
    //Creating socket file descriptor 
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0 )
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&server_addr, 0, sizeof(server_addr)); 
      
    // Filling server information 
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(PORT); 
    server_addr.sin_addr.s_addr = INADDR_ANY; 
      
    int n, len; 
      
    sendto(sock_fd, (const char*) hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr*) &server_addr, sizeof(server_addr)); 
    printf("Hello message sent.\n"); 
          
    n = recvfrom(sock_fd, (char*) buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &server_addr, &len); 
    buffer[n] = '\0'; 
    printf("Server : %s\n", buffer); 
  
    close(sock_fd); 
    return 0; 
} 
