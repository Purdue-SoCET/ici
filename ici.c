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

#define RESET_CREDIT 20

typedef struct Socket_Credit {
    int high_priority_credit;
    int low_priority_credit;
    int sync_credit;
    int async_credit;
    int multicast_credit;
    int port0_credit;
    int port1_credit;
    //int port2_credit;
    //int port3_credit;
} Socket_Credit;

typedef struct Node_Creation_Args {
    int index;
    //Networking Fields
    int port;
    int sock_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    Socket_Credit socket_credit;
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
    Socket_Credit* curr_socket_credit;
} Node_Tx_Args;

typedef struct Node_Rx_Args {
    int index;
    int sock_fd;
    struct sockaddr_in network_addr;
    Data rx_data;
    Socket_Credit* curr_socket_credit;
} Node_Rx_Args;

void node_server_create(void* arguments);
void node_client_create(void* arguments);
void node_tx(void* arguments);
void node_rx(void* arguments);
void set_tx_arguments(int is_server, Node_Creation_Args* tx, Node_Tx_Args* tx_args, unsigned char* packet);
void set_rx_arguments(int is_server, Node_Creation_Args* rx, Node_Rx_Args* rx_args);
void fabric_init(Node_Creation_Args** nodes);
unsigned char bit_read_index(unsigned char num, char index);
void bit_write_index(unsigned char* num, char index);

int main()
{
    Node_Creation_Args* node_creation_arguments;
    node_creation_arguments = malloc(10 * sizeof(*node_creation_arguments));

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
    node_server_create((void*) &node_creation_arguments[0]); //Node 1 "server" port
    node_client_create((void*) &node_creation_arguments[3]); //Node 2 "client" port

    //Node 2 to Node 3
    node_server_create((void*) &node_creation_arguments[2]); //Node 2 "server" port
    node_client_create((void*) &node_creation_arguments[5]); //Node 3 "client" port
   
    //Node 3 to Node 4
    node_server_create((void*) &node_creation_arguments[4]); //Node 3 "server" port
    node_client_create((void*) &node_creation_arguments[7]); //Node 4 "client" port

    //Node 4 to Node 5
    node_server_create((void*) &node_creation_arguments[6]); //Node 4 "server" port
    node_client_create((void*) &node_creation_arguments[9]); //Node 5 "client" port

    //Node 5 to Node 1
    node_server_create((void*) &node_creation_arguments[8]); //Node 5 "server" port
    node_client_create((void*) &node_creation_arguments[1]); //Node 1 "client" port
    
    //Initialization of Argument Structures
    Node_Tx_Args tx_arguments;
    Node_Rx_Args rx_arguments;
    memset(&tx_arguments, 0, sizeof(tx_arguments));
    memset(&rx_arguments, 0, sizeof(rx_arguments));

    //Initialize the Ring of Nodes
    fabric_init(&node_creation_arguments);

    //Temporary code
    char message[PACKET_SIZE];
    memset(message, 0, sizeof(message));

    printf("%x %x\n", message[1], message[0]);
   
    set_tx_arguments(0, &node_creation_arguments[3], &tx_arguments, message);
    set_rx_arguments(1, &node_creation_arguments[0], &rx_arguments);

    node_tx(&tx_arguments);
    node_rx(&rx_arguments);

    printf("%x ", rx_arguments.rx_data.small_flit.packet_header_1);
    printf("%x\n", rx_arguments.rx_data.small_flit.packet_header_0);

    set_tx_arguments(1, &node_creation_arguments[0], &tx_arguments, message);
    set_rx_arguments(0, &node_creation_arguments[3], &rx_arguments);

    pthread_create(&sim, NULL, node_rx, (void*) &rx_arguments);
    node_tx(&tx_arguments);
    //node_rx(&rx_arguments);
    printf("Socket Index %d Credit: High Priority: %d Low Priority: %d Synchronous: %d Asynchronous: %d\n", 0, node_creation_arguments[0].socket_credit.high_priority_credit, node_creation_arguments[0].socket_credit.low_priority_credit, node_creation_arguments[0].socket_credit.sync_credit, node_creation_arguments[0].socket_credit.async_credit);

    /* Miscellaneous code for memory testing of the structures
    printf("%x\n", (rx_arguments.rx_data.buffer));
    printf("%x %x\n", &(rx_arguments.rx_data.small_flit.packet_header_0), &(rx_arguments.rx_data.small_flit.packet_header_1));

    printf("%x %x %x %x\n", *(rx_arguments.rx_data.buffer), *(rx_arguments.rx_data.buffer + 1), *(rx_arguments.rx_data.buffer + 2), *(rx_arguments.rx_data.buffer + 3));
    printf("%x %x\n", (rx_arguments.rx_data.small_flit.packet_header_0), (rx_arguments.rx_data.small_flit.packet_header_1));

    printf("%d\n", sizeof(*(rx_arguments.rx_data.buffer)));
    printf("%d\n", sizeof(unsigned char));
    */

    printf("%x\n", *(rx_arguments.rx_data.buffer));

    //End of Temporary Code

    //Close All Threads and Free Memory
    int i;

    for (i = 0; i < 10; i++)
        close(node_creation_arguments[i].sock_fd); 

    free(node_creation_arguments);

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

    //Credit Return:
    //Updates current node credit
    unsigned char packet_header_0 = rx_arguments -> rx_data.small_flit.packet_header_0;
    unsigned char packet_header_1 = rx_arguments -> rx_data.small_flit.packet_header_1;
    unsigned char credit_return_priority = packet_header_0 >> 6 & 0x3 | packet_header_1 << 2 & 0x4; //Isolates bit 8, bit 7, and bit 6 from packet header
    unsigned char credit_return_class = packet_header_1 >> 1 & 0x7; //Isolates bit 11, bit 10, and bit 9 from packet header

    printf("credit return: %x\n", credit_return_priority);

    switch(credit_return_priority)
    {
    	case 0x0: break;
	case 0x1: break;
	case 0x2: rx_arguments -> curr_socket_credit -> high_priority_credit += 4; break;
	case 0x3: rx_arguments -> curr_socket_credit -> high_priority_credit += 8; break;
	case 0x4: rx_arguments -> curr_socket_credit -> low_priority_credit += 4; break;
	case 0x5: rx_arguments -> curr_socket_credit -> low_priority_credit += 8; break;
	case 0x6: rx_arguments -> curr_socket_credit -> high_priority_credit += 4; rx_arguments -> curr_socket_credit -> low_priority_credit += 4; break;
	case 0x7: rx_arguments -> curr_socket_credit -> high_priority_credit += 8; rx_arguments -> curr_socket_credit -> low_priority_credit += 8; break;
	default: break;
    }

    switch(credit_return_class)
    {
    	case 0x0: break;
	case 0x1: break;
	case 0x2: rx_arguments -> curr_socket_credit -> async_credit += 4; break;
	case 0x3: rx_arguments -> curr_socket_credit -> async_credit += 8; break;
	case 0x4: rx_arguments -> curr_socket_credit -> sync_credit += 4; break;
	case 0x5: rx_arguments -> curr_socket_credit -> sync_credit += 8; break;
	case 0x6: rx_arguments -> curr_socket_credit -> async_credit += 4; rx_arguments -> curr_socket_credit -> sync_credit += 4; break;
	case 0x7: rx_arguments -> curr_socket_credit -> async_credit += 8; rx_arguments -> curr_socket_credit -> sync_credit += 8; break;
	default: break;
    }


    /* *NOTE: This code reflects a previous version the ICI
    //High and Low priority credits
    if (bit_read_index(packet_header_0, 6)) //If Credit Return: Priority 4/8 is set
    {
        rx_arguments -> curr_socket_credit -> high_priority_credit += bit_read_index(packet_header_0, 7) ? 8 : bit_read_index(packet_header_0, 0) ? 0 : 2;
        rx_arguments -> curr_socket_credit -> low_priority_credit += bit_read_index(packet_header_1, 0) ? 8 : 0 ;
    }
    else
    {
        rx_arguments -> curr_socket_credit -> high_priority_credit += bit_read_index(packet_header_0, 7) ? 4 : bit_read_index(packet_header_1, 0) ? 0 : 2;
        rx_arguments -> curr_socket_credit -> low_priority_credit += bit_read_index(packet_header_1, 0) ? 4 : 0;
    }

    //Asynchronous and Synchronous credits
    if (bit_read_index(packet_header_1, 1)) //If Credit Return: Class 4/8 is set
    {
        rx_arguments -> curr_socket_credit -> async_credit += bit_read_index(packet_header_1, 2) ? 8 : bit_read_index(packet_header_1, 3) ? 0 : 2;
        rx_arguments -> curr_socket_credit -> sync_credit += bit_read_index(packet_header_1, 3) ? 8 : 0 ;
    }
    else
    {
        rx_arguments -> curr_socket_credit -> async_credit += bit_read_index(packet_header_1, 2) ? 4 : bit_read_index(packet_header_1, 3) ? 0 : 2;
        rx_arguments -> curr_socket_credit -> sync_credit += bit_read_index(packet_header_1, 3) ? 4 : 0;
    }
    */

 
    return; 
}

void set_tx_arguments(int is_server, Node_Creation_Args* tx, Node_Tx_Args* tx_args, unsigned char* packet)
{
    tx_args -> index = tx -> index;
    tx_args -> sock_fd = tx -> sock_fd;

    tx_args -> curr_socket_credit = &(tx -> socket_credit);

    if (is_server)
    {
        memcpy(&(tx_args -> network_addr), &(tx -> client_addr), sizeof(tx -> client_addr));
    }
    else
    {
        memcpy(&(tx_args -> network_addr), &(tx -> server_addr), sizeof(tx -> server_addr));
    }

    //Temporary line of code for testing
    bit_write_index(packet, 7);
 
    memcpy(&(tx_args -> tx_data.buffer), packet, PACKET_SIZE);

    return;
}

void set_rx_arguments(int is_server, Node_Creation_Args* rx, Node_Rx_Args* rx_args)
{
    rx_args -> index = rx -> index;
    rx_args -> sock_fd = rx -> sock_fd;

    rx_args -> curr_socket_credit = &(rx -> socket_credit);

    if (is_server)
    {
        memcpy(&(rx_args -> network_addr), &(rx -> client_addr), sizeof(rx -> client_addr));
    }
    else
    {
        memcpy(&(rx_args -> network_addr), &(rx -> server_addr), sizeof(rx -> server_addr));
    }

    return;
}

void fabric_init(Node_Creation_Args** nodes)
{ 
    //Initialization of Argument Structures
    Node_Tx_Args tx_arguments;
    Node_Rx_Args rx_arguments;
    memset(&tx_arguments, 0, sizeof(tx_arguments));
    memset(&rx_arguments, 0, sizeof(rx_arguments));

    //Set Initialization Message to 0
    unsigned char init_message[PACKET_SIZE];
    int temp = 0;
    memset(init_message, temp, sizeof(init_message));

    int server_index, client_index, tx_index, rx_index;
    
    server_index = 0;
    client_index = 3;

    while(server_index <= 8)
    {
        (*nodes)[client_index].socket_credit.high_priority_credit = RESET_CREDIT;
        (*nodes)[client_index].socket_credit.low_priority_credit = RESET_CREDIT;
        (*nodes)[client_index].socket_credit.sync_credit = RESET_CREDIT;
        (*nodes)[client_index].socket_credit.async_credit = RESET_CREDIT;
        (*nodes)[server_index].socket_credit.high_priority_credit = RESET_CREDIT;
        (*nodes)[server_index].socket_credit.low_priority_credit = RESET_CREDIT;
        (*nodes)[server_index].socket_credit.sync_credit = RESET_CREDIT;
        (*nodes)[server_index].socket_credit.async_credit = RESET_CREDIT;

        set_tx_arguments(0, &(*nodes)[client_index], &tx_arguments, init_message);
        set_rx_arguments(1, &(*nodes)[server_index], &rx_arguments);

        node_tx(&tx_arguments);
        node_rx(&rx_arguments);

        memcpy(&((*nodes)[server_index].client_addr), &(rx_arguments.network_addr), sizeof((*nodes)[server_index].client_addr));

        //Validation print statement
        printf("Connection from Node %d to Node %d initializated.\n", (*nodes)[server_index].index, (*nodes)[client_index].index);

        //Index Update Code
        server_index += 2;
        client_index = client_index == 9 ? 1 : client_index + 2;
    }

    return;
}

unsigned char bit_read_index(unsigned char num, char index)
{
    unsigned char result;

    if(index >= 8)
    {
        return(NULL);
    }

    result = num >> index;
    result &= 0x1;

    return(result);
}

void bit_write_index(unsigned char* num, char index)
{
    unsigned char temp;
    if(index >= 8)
    {
        return;
    }

    temp = 1 << index;

    if(num != 0)
    {
        *num = *num | temp;
    }
    else
    {
        *num = *num & ~temp;
    }

    return;
}

