#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#include "pthread.h" 
  
#define PORT_1 9065
#define PORT_2 9066
#define PORT_3 9067
#define PORT_4 9068
#define PORT_5 9069
#define PACKET_SIZE 512

typedef struct Node_Creation_Args {
    int index;
    int port;
    int sock_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
} Node_Creation_Args;

typedef struct Small_Flit {
    unsigned char packet_header_0;
    unsigned char packet_header_1;
} Small_Flit;

typedef union Data {
    Small_Flit small_flit;
    unsigned char buffer[PACKET_SIZE];
} Data;

typedef struct Node_Tx_Args {
    int index;
    int sock_fd;
    struct sockaddr_in network_addr;
    Data tx_data;
} Node_Tx_Args;

typedef struct Node_Rx_Args {
    int index;
    int sock_fd;
    struct sockaddr_in network_addr;
    Data rx_data;
} Node_Rx_Args;

void node_server_create(void* arguments);
void node_client_create(void* arguments);
void node_tx(void* arguments);
void node_rx(void* arguments);
unsigned int bit_read_index(unsigned int num, int index);
unsigned int bit_write_index(unsigned int num, int index);

int main()
{
    Node_Creation_Args node_creation_arguments[10];

    pthread_t sim;

    //Creation of a ring of 5 nodes. (pentagon)

    //Node 1 "server" port
    memset(&node_creation_arguments[0], 0, sizeof(node_creation_arguments[0]));
    node_creation_arguments[0].index = 1;
    node_creation_arguments[0].port = PORT_1;

    //Node 1 "client" port
    memset(&node_creation_arguments[1], 0, sizeof(node_creation_arguments[1]));    
    node_creation_arguments[1].index = 1;
    node_creation_arguments[1].port = PORT_5;

    //Node 2 "server" port
    memset(&node_creation_arguments[2], 0, sizeof(node_creation_arguments[2]));
    node_creation_arguments[2].index = 2;
    node_creation_arguments[2].port = PORT_2;

    //Node 2 "client" port
    memset(&node_creation_arguments[3], 0, sizeof(node_creation_arguments[3]));
    node_creation_arguments[3].index = 2;
    node_creation_arguments[3].port = PORT_1;

    //Node 3 "server" port
    memset(&node_creation_arguments[4], 0, sizeof(node_creation_arguments[4]));
    node_creation_arguments[4].index = 3;
    node_creation_arguments[4].port = PORT_3;

    //Node 3 "client" port
    memset(&node_creation_arguments[5], 0, sizeof(node_creation_arguments[5]));
    node_creation_arguments[5].index = 3;
    node_creation_arguments[5].port = PORT_2;

    //Node 4 "server" port
    memset(&node_creation_arguments[6], 0, sizeof(node_creation_arguments[6]));
    node_creation_arguments[6].index = 4;
    node_creation_arguments[6].port = PORT_4;

    //Node 4 "client" port
    memset(&node_creation_arguments[7], 0, sizeof(node_creation_arguments[7]));
    node_creation_arguments[7].index = 4;
    node_creation_arguments[7].port = PORT_3;

    //Node 5 "server" port
    memset(&node_creation_arguments[8], 0, sizeof(node_creation_arguments[8]));
    node_creation_arguments[8].index = 5;
    node_creation_arguments[8].port = PORT_5;

    //Node 5 "client" port
    memset(&node_creation_arguments[9], 0, sizeof(node_creation_arguments[9]));
    node_creation_arguments[9].index = 5;
    node_creation_arguments[9].port = PORT_4;

    //Node 1 to Node 2
    pthread_create(&sim, NULL, node_server_create, (void*) &node_creation_arguments[0]); //Node 1 "server" port
    pthread_create(&sim, NULL, node_client_create, (void*) &node_creation_arguments[3]); //Node 2 "client" port

    //Node 2 to Node 3
    pthread_create(&sim, NULL, node_server_create, (void*) &node_creation_arguments[2]); //Node 2 "server" port
    pthread_create(&sim, NULL, node_client_create, (void*) &node_creation_arguments[5]); //Node 3 "client" port
   
    //Node 3 to Node 4
    pthread_create(&sim, NULL, node_server_create, (void*) &node_creation_arguments[4]); //Node 3 "server" port
    pthread_create(&sim, NULL, node_client_create, (void*) &node_creation_arguments[7]); //Node 4 "client" port

    //Node 4 to Node 5
    pthread_create(&sim, NULL, node_server_create, (void*) &node_creation_arguments[6]); //Node 4 "server" port
    pthread_create(&sim, NULL, node_client_create, (void*) &node_creation_arguments[9]); //Node 5 "client" port

    //Node 5 to Node 1
    pthread_create(&sim, NULL, node_server_create, (void*) &node_creation_arguments[8]); //Node 5 "server" port
    pthread_create(&sim, NULL, node_client_create, (void*) &node_creation_arguments[1]); //Node 1 "client" port

    //Beginning of Transactions
    printf("End of Thread Creation.\n");
    
    //Initialization of Structure
    Node_Tx_Args tx_arguments;
    Node_Rx_Args rx_arguments;
    memset(&tx_arguments, 0, sizeof(tx_arguments));
    memset(&rx_arguments, 0, sizeof(rx_arguments));

    tx_arguments.index = 2;
    rx_arguments.index = 1;
    tx_arguments.sock_fd = node_creation_arguments[3].sock_fd;
    rx_arguments.sock_fd = node_creation_arguments[0].sock_fd;
    memcpy(&(tx_arguments.network_addr), &(node_creation_arguments[3].server_addr), sizeof(node_creation_arguments[3].server_addr));
    memcpy(&(rx_arguments.network_addr), &(node_creation_arguments[0].client_addr), sizeof(node_creation_arguments[3].client_addr));

    memset(tx_arguments.tx_data.buffer, 0xDEADBEEF, 1);
    memset(tx_arguments.tx_data.buffer + 1, 0xAB, 2);

    node_tx(&tx_arguments);
    node_rx(&rx_arguments);

    printf("%x\n", (rx_arguments.rx_data.buffer));
    printf("%x %x\n", &(rx_arguments.rx_data.small_flit.packet_header_0), &(rx_arguments.rx_data.small_flit.packet_header_1));

    printf("%x %x %x %x\n", *(rx_arguments.rx_data.buffer), *(rx_arguments.rx_data.buffer + 1), *(rx_arguments.rx_data.buffer + 2), *(rx_arguments.rx_data.buffer + 3));
    printf("%x %x\n", (rx_arguments.rx_data.small_flit.packet_header_0), (rx_arguments.rx_data.small_flit.packet_header_1));

    printf("%d\n", sizeof(*(rx_arguments.rx_data.buffer)));
    printf("%d\n", sizeof(unsigned char));

    int i;
    for (i = 0; i < 10; i++)
        close(node_creation_arguments[i].sock_fd); 

    pthread_exit(NULL);
    return(0);
}

void node_server_create(void* arguments) //Server Side
{
    Data tx_data; 

    Node_Creation_Args* node_arguments = arguments;
      
    //Creating socket file descriptor and ensuring it is valid. 
    node_arguments -> sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (node_arguments -> sock_fd < 0)
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }  
      
    //Filling in server_addr structure.
    node_arguments -> server_addr.sin_family = AF_INET; // IPv4 
    node_arguments -> server_addr.sin_addr.s_addr = INADDR_ANY; 
    node_arguments -> server_addr.sin_port = htons(node_arguments -> port); 
      
    //Bind socket with the server address. 
    if (bind(node_arguments -> sock_fd, (const struct sockaddr*) &(node_arguments -> server_addr), sizeof(node_arguments -> server_addr)) < 0)
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    /*
    int len, n; 
  
    len = sizeof(client_addr);  //len is value/result 
  
    n = recvfrom(sock_fd, tx_data.buffer, PACKET_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr, &len);  
    printf("NODE %d: Received this: %x\n", node_arguments -> index, *(tx_data.buffer)); 

    memset(tx_data.buffer, node_arguments -> index, sizeof(int)); 

    sendto(sock_fd, tx_data.buffer, PACKET_SIZE, MSG_CONFIRM, (const struct sockaddr*) &client_addr, len); 
    printf("NODE %d: Packet sent.\n", node_arguments -> index);
    */
      
    return; 
}

void node_client_create(void* arguments) //Client Side
{
    Data rx_data;

    Node_Creation_Args* node_arguments = arguments;
  
    //Creating socket file descriptor and ensuring it is valid. 
    node_arguments -> sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (node_arguments -> sock_fd < 0 )
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    //Filling in server_addr structure. 
    node_arguments -> server_addr.sin_family = AF_INET; 
    node_arguments -> server_addr.sin_port = htons(node_arguments -> port); 
    node_arguments -> server_addr.sin_addr.s_addr = INADDR_ANY; 
    
    /*
    int n, len; 

    memset(rx_data.buffer, node_arguments -> index, sizeof(int));
      
    sendto(sock_fd, rx_data.buffer, PACKET_SIZE, MSG_CONFIRM, (const struct sockaddr*) &server_addr, sizeof(server_addr)); 
    printf("NODE %d: Packet sent.\n", node_arguments -> index); 
          
    n = recvfrom(sock_fd, rx_data.buffer, PACKET_SIZE, MSG_WAITALL, (struct sockaddr *) &server_addr, &len); 
    printf("NODE %d: Received this: %x\n", node_arguments -> index, *(rx_data.buffer)); 
    */
  
    return; 
}

void node_tx(void* arguments)
{
    Node_Tx_Args* tx_arguments = arguments;

    sendto(tx_arguments -> sock_fd, tx_arguments -> tx_data.buffer, PACKET_SIZE, MSG_CONFIRM, (const struct sockaddr*) &(tx_arguments -> network_addr), sizeof(tx_arguments -> network_addr));

    return;
}

void node_rx(void* arguments)
{
    int len, n;
    
    Node_Rx_Args* rx_arguments = arguments;

    len = sizeof(rx_arguments -> network_addr);

    n = recvfrom(rx_arguments -> sock_fd, rx_arguments -> rx_data.buffer, PACKET_SIZE, MSG_WAITALL, (struct sockaddr *) &(rx_arguments -> network_addr), &len);
 
    return; 
}

unsigned int bit_read_index(unsigned int num, int index)
{
    unsigned int result;

    if(index >= 32)
    {
        return(NULL);
    }

    result = num >> index;
    result = 0x00000001 & index;

    return(result);
}

unsigned int bit_write_index(unsigned int num, int index)
{
    unsigned int result;
    if(index >= 32)
    {
        return(NULL);
    }

    result = 1 << index;

    if(num != 0)
    {
        result = num | result;
    }
    else
    {
        result = num & ~result;
    }

    return(result);
}

