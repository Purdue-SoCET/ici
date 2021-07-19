#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <math.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>

#include "pthread.h" 
  
#define PORT_0 9065
#define PORT_1 9066
#define PORT_2 9067
#define PORT_3 9068
#define PORT_4 9069
#define PORT_5 9070
#define PORT_6 9071
#define PORT_7 9072
#define PORT_8 9073
#define PORT_9 9074
#define PORT_10 9075
#define PORT_11 9076
#define PORT_12 9077
#define PORT_13 9078
#define PORT_14 9079
#define PORT_15 9080
#define PORT_16 9081
#define PORT_17 9082
#define PORT_18 9083
#define PORT_19 9084
#define PORT_20 9085
#define PORT_21 9086
#define PORT_22 9087
#define PORT_23 9088
#define PORT_24 9089
#define PORT_25 9090
#define PORT_26 9091
#define PORT_27 9092
#define PORT_28 9093
#define PORT_29 9094
#define PORT_30 9095
#define PORT_31 9096

#define PACKET_SIZE 512

#define NUM_BUFFER_SLOTS 3

#define RESET_CREDIT 20

#define N0_N1 0
#define N0_N31 1
#define N1_N2 2
#define N1_N0 3
#define N2_N3 4
#define N2_N1 5
#define N3_N4 6
#define N3_N2 7
#define N4_N5 8
#define N4_N3 9
#define N5_N6 10
#define N5_N4 11
#define N6_N7 12
#define N6_N5 13
#define N7_N8 14
#define N7_N6 15
#define N8_N9 16
#define N8_N7 17
#define N9_N10 18
#define N9_N8 19
#define N10_N11 20
#define N10_N9 21
#define N11_N12 22
#define N11_N10 23
#define N12_N13 24
#define N12_N11 25
#define N13_N14 26
#define N13_N12 27
#define N14_N15 28
#define N14_N13 29
#define N15_N16 30
#define N15_N14 31
#define N16_N17 32
#define N16_N15 33
#define N17_N18 34
#define N17_N16 35
#define N18_N19 36
#define N18_N17 37
#define N19_N20 38
#define N19_N18 39
#define N20_N21 40
#define N20_N19 41
#define N21_N22 42
#define N21_N20 43
#define N22_N23 44
#define N22_N21 45
#define N23_N24 46
#define N23_N22 47
#define N24_N25 48
#define N24_N23 49
#define N25_N26 50
#define N25_N24 51
#define N26_N27 52
#define N26_N25 53
#define N27_N28 54
#define N27_N26 55
#define N28_N29 56
#define N28_N27 57
#define N29_N30 58
#define N29_N28 59
#define N30_N31 60
#define N30_N29 61
#define N31_N0 62
#define N31_N30 63

#define FLIT_TARGET_0 96

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
    uint32_t node_index;
    uint32_t nodes_connected_to;
    uint32_t nodes_in_fabric;

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
    //uint64_t buffer[PACKET_SIZE];
} Data;

typedef struct Node_Tx_Args {
    int index;
    uint32_t node_index;
    uint32_t nodes_connected_to;
    uint32_t nodes_in_fabric;

    int sock_fd;
    struct sockaddr_in network_addr;
    Data tx_data;
    Socket_Credit* curr_socket_credit;
} Node_Tx_Args;

typedef struct Node_Rx_Args {
    int index;
    uint32_t node_index;
    uint32_t nodes_connected_to;
    uint32_t nodes_in_fabric;

    int sock_fd;
    struct sockaddr_in network_addr;
    Data rx_data;
    Socket_Credit* curr_socket_credit;
} Node_Rx_Args;

//This is because we are threading this function
typedef struct Continuous_Rx_Args {
    void* arguments;
    void* creation_arguments;
} Continuous_Rx_Args;

//Global Buffer Pointers
unsigned char*** node_rx_buffers;

int circle_count = 0; //Duck tape variable
int discovery_done = 0; //^^

void create_bitmap_message_1(Node_Creation_Args* node_creation_arguments, uint32_t index, uint32_t target, uint32_t nodes_found, uint64_t* hop_bitmap, char* message);
void node_server_create(void* arguments);
void node_client_create(void* arguments);
void node_tx(void* arguments);
void node_rx(void* arguments);
void continuous_rx(void* args);
void set_node_creation_arguments(Node_Creation_Args* node_creation_arguments, int index, int nodes_connected_to, int nodes_in_fabric, int node_index, int port);
void set_tx_arguments(int is_server, Node_Creation_Args* tx, Node_Tx_Args* tx_args, unsigned char* packet);
void set_rx_arguments(int is_server, Node_Creation_Args* rx, Node_Rx_Args* rx_args);
void fabric_init(Node_Creation_Args** nodes);
unsigned char bit_read_index(unsigned char num, char index);
void bit_write_index(unsigned char* num, char index);
void write_buffers();
unsigned char hex_to_ascii(unsigned char);
void node_create_function(int index, Node_Creation_Args* node_args, uint32_t node_index, uint32_t connected_node, uint32_t fabric, int port);
void update_hop_bitmap(int curr_node, uint64_t* hop_bitmap);

int main()
{
    Node_Creation_Args* node_creation_arguments;
    //Node_Creation_Args* node_identity_information;
    node_creation_arguments = malloc(64 * sizeof(*node_creation_arguments));
    //node_identity_information = malloc(5 * sizeof(*node_identity_information));
    pthread_t sim;

    //Creation of a ring of 5 nodes. (pentagon)
    //void node_create_function(int index, Node_Creation_Args* node_args, uint32_t node_index, uint32_t connected_node, uint32_t fabric, int port);
    
    //Node 0 "server" port
    memset(&node_creation_arguments[N0_N1], 0, sizeof(node_creation_arguments[N0_N1]));
    node_create_function(0, &(node_creation_arguments[N0_N1]), 0x1, 0x80000002, 0xFFFFFFFF, PORT_0);

    //Node 0 "client" port
    memset(&node_creation_arguments[N0_N31], 0, sizeof(node_creation_arguments[N0_N31]));
    node_create_function(0, &(node_creation_arguments[N0_N31]), 0x1, 0x80000002, 0xFFFFFFFF, PORT_31);

    //Node 1 "server" port
    memset(&node_creation_arguments[N1_N2], 0, sizeof(node_creation_arguments[N1_N2]));
    node_create_function(1, &(node_creation_arguments[N1_N2]), 0x2, 0x5, 0xFFFFFFFF, PORT_1);

    //Node 1 "client" port
    memset(&node_creation_arguments[N1_N0], 0, sizeof(node_creation_arguments[N1_N0]));   
    node_create_function(1, &(node_creation_arguments[N1_N0]), 0x2, 0x5, 0xFFFFFFFF, PORT_0); 

    //Node 2 "server" port
    memset(&node_creation_arguments[N2_N3], 0, sizeof(node_creation_arguments[N2_N3]));
    node_create_function(2, &(node_creation_arguments[N2_N3]), 0x4, 0xA, 0xFFFFFFFF, PORT_2); 

    //Node 2 "client" port
    memset(&node_creation_arguments[N2_N1], 0, sizeof(node_creation_arguments[N2_N1]));
    node_create_function(2, &(node_creation_arguments[N2_N1]), 0x4, 0xA, 0xFFFFFFFF, PORT_1); 

    //Node 3 "server" port
    memset(&node_creation_arguments[N3_N4], 0, sizeof(node_creation_arguments[N3_N4]));
    node_create_function(3, &(node_creation_arguments[N3_N4]), 0x8, 0x14, 0xFFFFFFFF, PORT_3);

    //Node 3 "client" port
    memset(&node_creation_arguments[N3_N2], 0, sizeof(node_creation_arguments[N3_N2]));
    node_create_function(3, &(node_creation_arguments[N3_N2]), 0x8, 0x14, 0xFFFFFFFF, PORT_2); 

    //Node 4 "server" port
    memset(&node_creation_arguments[N4_N5], 0, sizeof(node_creation_arguments[N4_N5]));
    node_create_function(4, &(node_creation_arguments[N4_N5]), 0x10, 0x28, 0xFFFFFFFF, PORT_4); 

    //Node 4 "client" port
    memset(&node_creation_arguments[N4_N3], 0, sizeof(node_creation_arguments[N4_N3]));
    node_create_function(4, &(node_creation_arguments[N4_N3]), 0x10, 0x28, 0xFFFFFFFF, PORT_3);

    //Node 5 "server" port
    memset(&node_creation_arguments[N5_N6], 0, sizeof(node_creation_arguments[N5_N6]));
    node_create_function(5, &(node_creation_arguments[N5_N6]), 0x20, 0x50, 0xFFFFFFFF, PORT_5);

    //Node 5 "client" port
    memset(&node_creation_arguments[N5_N4], 0, sizeof(node_creation_arguments[N5_N4]));
    node_create_function(5, &(node_creation_arguments[N5_N4]), 0x20, 0x50, 0xFFFFFFFF, PORT_4);

    //Node 6 "server" port
    memset(&node_creation_arguments[N6_N7], 0, sizeof(node_creation_arguments[N6_N7]));
    node_create_function(6, &(node_creation_arguments[N6_N7]), 0x40, 0xA0, 0xFFFFFFFF, PORT_6);

    //Node 6 "client" port
    memset(&node_creation_arguments[N6_N5], 0, sizeof(node_creation_arguments[N6_N5]));
    node_create_function(6, &(node_creation_arguments[N6_N5]), 0x40, 0xA0, 0xFFFFFFFF, PORT_5);

    //Node 7 "server" port
    memset(&node_creation_arguments[N7_N8], 0, sizeof(*node_creation_arguments));
    node_create_function(7, &(node_creation_arguments[N7_N8]), 0x80, 0x140, 0xFFFFFFFF, PORT_7);

    //Node 7 "client" port
    memset(&node_creation_arguments[N7_N6], 0, sizeof(*node_creation_arguments));
    node_create_function(7, &(node_creation_arguments[N7_N6]), 0x80, 0x140, 0xFFFFFFFF, PORT_6);

    //Node 8 "server" port
    memset(&node_creation_arguments[N8_N9], 0, sizeof(*node_creation_arguments));
    node_create_function(8, &(node_creation_arguments[N8_N9]), 0x100, 0x280, 0xFFFFFFFF, PORT_8);

    //Node 8 "client" port
    memset(&node_creation_arguments[N8_N7], 0, sizeof(*node_creation_arguments));
    node_create_function(8, &(node_creation_arguments[N8_N7]), 0x100, 0x280, 0xFFFFFFFF, PORT_7);

    //Node 9 "server" port
    memset(&node_creation_arguments[N9_N10], 0, sizeof(*node_creation_arguments));
    node_create_function(9, &(node_creation_arguments[N9_N10]), 0x200, 0x500, 0xFFFFFFFF, PORT_9);

    //Node 9 "client" port
    memset(&node_creation_arguments[N9_N8], 0, sizeof(*node_creation_arguments));
    node_create_function(9, &(node_creation_arguments[N9_N8]), 0x200, 0x500, 0xFFFFFFFF, PORT_8);

    //Node 10 "server" port
    memset(&node_creation_arguments[N10_N11], 0, sizeof(*node_creation_arguments));
    node_create_function(10, &(node_creation_arguments[N10_N11]), 0x400, 0xA00, 0xFFFFFFFF, PORT_10);

    //Node 10 "client" port
    memset(&node_creation_arguments[N10_N9], 0, sizeof(*node_creation_arguments));
    node_create_function(10, &(node_creation_arguments[N10_N9]), 0x400, 0xA00, 0xFFFFFFFF, PORT_9);

    //Node 11 "server" port
    memset(&node_creation_arguments[N11_N12], 0, sizeof(*node_creation_arguments));
    node_create_function(11, &(node_creation_arguments[N11_N12]), 0x800, 0x1400, 0xFFFFFFFF, PORT_11);

    //Node 11 "client" port
    memset(&node_creation_arguments[N11_N10], 0, sizeof(*node_creation_arguments));
    node_create_function(11, &(node_creation_arguments[N11_N10]), 0x800, 0x1400, 0xFFFFFFFF, PORT_10);

    //Node 12 "server" port
    memset(&node_creation_arguments[N12_N13], 0, sizeof(*node_creation_arguments));
    node_create_function(12, &(node_creation_arguments[N12_N13]), 0x1000, 0x2800, 0xFFFFFFFF, PORT_12);

    //Node 12 "client" port
    memset(&node_creation_arguments[N12_N11], 0, sizeof(*node_creation_arguments));
    node_create_function(12, &(node_creation_arguments[N12_N11]), 0x1000, 0x2800, 0xFFFFFFFF, PORT_11);

    //Node 13 "server" port
    memset(&node_creation_arguments[N13_N14], 0, sizeof(*node_creation_arguments));
    node_create_function(13, &(node_creation_arguments[N13_N14]), 0x2000, 0x5000, 0xFFFFFFFF, PORT_13);

    //Node 13 "client" port
    memset(&node_creation_arguments[N13_N12], 0, sizeof(*node_creation_arguments));
    node_create_function(13, &(node_creation_arguments[N13_N12]), 0x2000, 0x5000, 0xFFFFFFFF, PORT_12);

    //Node 14 "server" port
    memset(&node_creation_arguments[N14_N15], 0, sizeof(*node_creation_arguments));
    node_create_function(14, &(node_creation_arguments[N14_N15]), 0x4000, 0xA000, 0xFFFFFFFF, PORT_14);

    //Node 14 "client" port
    memset(&node_creation_arguments[N14_N13], 0, sizeof(*node_creation_arguments));
    node_create_function(14, &(node_creation_arguments[N14_N13]), 0x4000, 0xA000, 0xFFFFFFFF, PORT_13);

    //Node 15 "server" port
    memset(&node_creation_arguments[N15_N16], 0, sizeof(*node_creation_arguments));
    node_create_function(15, &(node_creation_arguments[N15_N16]), 0x8000, 0x14000, 0xFFFFFFFF, PORT_15);

    //Node 15 "client" port
    memset(&node_creation_arguments[N15_N14], 0, sizeof(*node_creation_arguments));
    node_create_function(15, &(node_creation_arguments[N15_N14]), 0x8000, 0x14000, 0xFFFFFFFF, PORT_14);

    //Node 16 "server" port
    memset(&node_creation_arguments[N16_N17], 0, sizeof(*node_creation_arguments));
    node_create_function(16, &(node_creation_arguments[N16_N17]), 0x10000, 0x28000, 0xFFFFFFFF, PORT_16);

    //Node 16 "client" port
    memset(&node_creation_arguments[N16_N15], 0, sizeof(*node_creation_arguments));
    node_create_function(16, &(node_creation_arguments[N16_N15]), 0x10000, 0x28000, 0xFFFFFFFF, PORT_15);

    //Node 17 "server" port
    memset(&node_creation_arguments[N17_N18], 0, sizeof(*node_creation_arguments));
    node_create_function(17, &(node_creation_arguments[N17_N18]), 0x20000, 0x50000, 0xFFFFFFFF, PORT_17);

    //Node 17 "client" port
    memset(&node_creation_arguments[N17_N16], 0, sizeof(*node_creation_arguments));
    node_create_function(17, &(node_creation_arguments[N17_N16]), 0x20000, 0x50000, 0xFFFFFFFF, PORT_16);

    //Node 18 "server" port
    memset(&node_creation_arguments[N18_N19], 0, sizeof(*node_creation_arguments));
    node_create_function(18, &(node_creation_arguments[N18_N19]), 0x40000, 0xA0000, 0xFFFFFFFF, PORT_18);

    //Node 18 "client" port
    memset(&node_creation_arguments[N18_N17], 0, sizeof(*node_creation_arguments));
    node_create_function(18, &(node_creation_arguments[N18_N17]), 0x40000, 0xA0000, 0xFFFFFFFF, PORT_17);

    //Node 19 "server" port
    memset(&node_creation_arguments[N19_N20], 0, sizeof(*node_creation_arguments));
    node_create_function(19, &(node_creation_arguments[N19_N20]), 0x80000, 0x140000, 0xFFFFFFFF, PORT_19);

    //Node 19 "client" port
    memset(&node_creation_arguments[N19_N18], 0, sizeof(*node_creation_arguments));
    node_create_function(19, &(node_creation_arguments[N19_N18]), 0x80000, 0x140000, 0xFFFFFFFF, PORT_18);

    //Node 20 "server" port
    memset(&node_creation_arguments[N20_N21], 0, sizeof(*node_creation_arguments));
    node_create_function(20, &(node_creation_arguments[N20_N21]), 0x100000, 0x280000, 0xFFFFFFFF, PORT_20);

    //Node 20 "client" port
    memset(&node_creation_arguments[N20_N19], 0, sizeof(*node_creation_arguments));
    node_create_function(20, &(node_creation_arguments[N20_N19]), 0x100000, 0x280000, 0xFFFFFFFF, PORT_19);

    //Node 21 "server" port
    memset(&node_creation_arguments[N21_N22], 0, sizeof(*node_creation_arguments));
    node_create_function(21, &(node_creation_arguments[N21_N22]), 0x200000, 0x500000, 0xFFFFFFFF, PORT_21);

    //Node 21 "client" port
    memset(&node_creation_arguments[N21_N20], 0, sizeof(*node_creation_arguments));
    node_create_function(21, &(node_creation_arguments[N21_N20]), 0x200000, 0x500000, 0xFFFFFFFF, PORT_20);

    //Node 22 "server" port
    memset(&node_creation_arguments[N22_N23], 0, sizeof(*node_creation_arguments));
    node_create_function(22, &(node_creation_arguments[N22_N23]), 0x400000, 0xA00000, 0xFFFFFFFF, PORT_22);

    //Node 22 "client" port
    memset(&node_creation_arguments[N22_N21], 0, sizeof(*node_creation_arguments));
    node_create_function(22, &(node_creation_arguments[N22_N21]), 0x400000, 0xA00000, 0xFFFFFFFF, PORT_21);

    //Node 23 "server" port
    memset(&node_creation_arguments[N23_N24], 0, sizeof(*node_creation_arguments));
    node_create_function(23, &(node_creation_arguments[N23_N24]), 0x800000, 0x1400000, 0xFFFFFFFF, PORT_23);

    //Node 23 "client" port
    memset(&node_creation_arguments[N23_N22], 0, sizeof(*node_creation_arguments));
    node_create_function(23, &(node_creation_arguments[N23_N22]), 0x800000, 0x1400000, 0xFFFFFFFF, PORT_22);

    //Node 24 "server" port
    memset(&node_creation_arguments[N24_N25], 0, sizeof(*node_creation_arguments));
    node_create_function(24, &(node_creation_arguments[N24_N25]), 0x1000000, 0x2800000, 0xFFFFFFFF, PORT_24);

    //Node 24 "client" port
    memset(&node_creation_arguments[N24_N23], 0, sizeof(*node_creation_arguments));
    node_create_function(24, &(node_creation_arguments[N24_N23]), 0x1000000, 0x2800000, 0xFFFFFFFF, PORT_23);

    //Node 25 "server" port
    memset(&node_creation_arguments[N25_N26], 0, sizeof(*node_creation_arguments));
    node_create_function(25, &(node_creation_arguments[N25_N26]), 0x2000000, 0x5000000, 0xFFFFFFFF, PORT_25);

    //Node 25 "client" port
    memset(&node_creation_arguments[N25_N24], 0, sizeof(*node_creation_arguments));
    node_create_function(25, &(node_creation_arguments[N25_N24]), 0x2000000, 0x5000000, 0xFFFFFFFF, PORT_24);

    //Node 26 "server" port
    memset(&node_creation_arguments[N26_N27], 0, sizeof(*node_creation_arguments));
    node_create_function(26, &(node_creation_arguments[N26_N27]), 0x4000000, 0xA000000, 0xFFFFFFFF, PORT_26);

    //Node 26 "client" port
    memset(&node_creation_arguments[N26_N25], 0, sizeof(*node_creation_arguments));
    node_create_function(26, &(node_creation_arguments[N26_N25]), 0x4000000, 0xA000000, 0xFFFFFFFF, PORT_25);

    //Node 27 "server" port
    memset(&node_creation_arguments[N27_N28], 0, sizeof(*node_creation_arguments));
    node_create_function(27, &(node_creation_arguments[N27_N28]), 0x8000000, 0x14000000, 0xFFFFFFFF, PORT_27);

    //Node 27 "client" port
    memset(&node_creation_arguments[N27_N26], 0, sizeof(*node_creation_arguments));
    node_create_function(27, &(node_creation_arguments[N27_N26]), 0x8000000, 0x14000000, 0xFFFFFFFF, PORT_26);

    //Node 28 "server" port
    memset(&node_creation_arguments[N28_N29], 0, sizeof(*node_creation_arguments));
    node_create_function(28, &(node_creation_arguments[N28_N29]), 0x10000000, 0x28000000, 0xFFFFFFFF, PORT_28);

    //Node 28 "client" port
    memset(&node_creation_arguments[N28_N27], 0, sizeof(*node_creation_arguments));
    node_create_function(28, &(node_creation_arguments[N28_N27]), 0x10000000, 0x28000000, 0xFFFFFFFF, PORT_27);

    //Node 29 "server" port
    memset(&node_creation_arguments[N29_N30], 0, sizeof(*node_creation_arguments));
    node_create_function(29, &(node_creation_arguments[N29_N30]), 0x20000000, 0x50000000, 0xFFFFFFFF, PORT_29);

    //Node 29 "client" port
    memset(&node_creation_arguments[N29_N28], 0, sizeof(*node_creation_arguments));
    node_create_function(29, &(node_creation_arguments[N29_N28]), 0x20000000, 0x50000000, 0xFFFFFFFF, PORT_28);

    //Node 30 "server" port
    memset(&node_creation_arguments[N30_N31], 0, sizeof(*node_creation_arguments));
    node_create_function(30, &(node_creation_arguments[N30_N31]), 0x40000000, 0xA0000000, 0xFFFFFFFF, PORT_30);

    //Node 30 "client" port
    memset(&node_creation_arguments[N30_N29], 0, sizeof(*node_creation_arguments));
    node_create_function(30, &(node_creation_arguments[N30_N29]), 0x40000000, 0xA0000000, 0xFFFFFFFF, PORT_29);

    //Node 31 "server" port
    memset(&node_creation_arguments[N31_N0], 0, sizeof(*node_creation_arguments));
    node_create_function(31, &(node_creation_arguments[N31_N0]), 0x80000000, 0x40000001, 0xFFFFFFFF, PORT_31);

    //Node 31 "client" port
    memset(&node_creation_arguments[N31_N30], 0, sizeof(*node_creation_arguments));
    node_create_function(31, &(node_creation_arguments[N31_N30]), 0x80000000, 0x40000001, 0xFFFFFFFF, PORT_30);

    //Node 0 to Node 1
    node_server_create((void*) &node_creation_arguments[N0_N1]); //Node 0 "server" port
    node_client_create((void*) &node_creation_arguments[N1_N0]); //Node 1 "client" port

    //Node 1 to Node 2
    node_server_create((void*) &node_creation_arguments[N1_N2]); //Node 1 "server" port
    node_client_create((void*) &node_creation_arguments[N2_N1]); //Node 2 "client" port

    //Node 2 to Node 3
    node_server_create((void*) &node_creation_arguments[N2_N3]); //Node 2 "server" port
    node_client_create((void*) &node_creation_arguments[N3_N2]); //Node 3 "client" port
   
    //Node 3 to Node 4
    node_server_create((void*) &node_creation_arguments[N3_N4]); //Node 3 "server" port
    node_client_create((void*) &node_creation_arguments[N4_N3]); //Node 4 "client" port

    //Node 4 to Node 5
    node_server_create((void*) &node_creation_arguments[N4_N5]); //Node 4 "server" port
    node_client_create((void*) &node_creation_arguments[N5_N4]); //Node 5 "client" port

    //Node 5 to Node 6
    node_server_create((void*) &node_creation_arguments[N5_N6]); //Node 5 "server" port
    node_client_create((void*) &node_creation_arguments[N6_N5]); //Node 6 "client" port

    //Node 6 to Node 7
    node_server_create((void*) &node_creation_arguments[N6_N7]); //Node 6 "server" port
    node_client_create((void*) &node_creation_arguments[N7_N6]); //Node 7 "client" port

    //Node 7 to Node 8
    node_server_create((void*) &node_creation_arguments[N7_N8]); //Node 7 "server" port
    node_client_create((void*) &node_creation_arguments[N8_N7]); //Node 8 "client" port
    
    //Node 8 to Node 9
    node_server_create((void*) &node_creation_arguments[N8_N9]); //Node 8 "server" port
    node_client_create((void*) &node_creation_arguments[N9_N8]); //Node 9 "client" port

    //Node 9 to Node 10
    node_server_create((void*) &node_creation_arguments[N9_N10]); //Node 9 "server" port
    node_client_create((void*) &node_creation_arguments[N10_N9]); //Node 10 "client" port

    //Node 10 to Node 11
    node_server_create((void*) &node_creation_arguments[N10_N11]); //Node 10 "server" port
    node_client_create((void*) &node_creation_arguments[N11_N10]); //Node 11 "client" port

    //Node 11 to Node 12
    node_server_create((void*) &node_creation_arguments[N11_N12]); //Node 11 "server" port
    node_client_create((void*) &node_creation_arguments[N12_N11]); //Node 12 "client" port

    //Node 12 to Node 13
    node_server_create((void*) &node_creation_arguments[N12_N13]); //Node 12 "server" port
    node_client_create((void*) &node_creation_arguments[N13_N12]); //Node 13 "client" port

    //Node 13 to Node 14
    node_server_create((void*) &node_creation_arguments[N13_N14]); //Node 13 "server" port
    node_client_create((void*) &node_creation_arguments[N14_N13]); //Node 14 "client" port

    //Node 14 to Node 15
    node_server_create((void*) &node_creation_arguments[N14_N15]); //Node 14 "server" port
    node_client_create((void*) &node_creation_arguments[N15_N14]); //Node 15 "client" port

    //Node 15 to Node 16
    node_server_create((void*) &node_creation_arguments[N15_N16]); //Node 15 "server" port
    node_client_create((void*) &node_creation_arguments[N16_N15]); //Node 16 "client" port

    //Node 16 to Node 17
    node_server_create((void*) &node_creation_arguments[N16_N17]); //Node 16 "server" port
    node_client_create((void*) &node_creation_arguments[N17_N16]); //Node 17 "client" port

    //Node 17 to Node 18
    node_server_create((void*) &node_creation_arguments[N17_N18]); //Node 17 "server" port
    node_client_create((void*) &node_creation_arguments[N18_N17]); //Node 18 "client" port

    //Node 18 to Node 19
    node_server_create((void*) &node_creation_arguments[N18_N19]); //Node 18 "server" port
    node_client_create((void*) &node_creation_arguments[N19_N18]); //Node 19 "client" port

    //Node 19 to Node 20
    node_server_create((void*) &node_creation_arguments[N19_N20]); //Node 19 "server" port
    node_client_create((void*) &node_creation_arguments[N20_N19]); //Node 20 "client" port

    //Node 20 to Node 21
    node_server_create((void*) &node_creation_arguments[N20_N21]); //Node 20 "server" port
    node_client_create((void*) &node_creation_arguments[N21_N20]); //Node 21 "client" port

    //Node 21 to Node 22
    node_server_create((void*) &node_creation_arguments[N21_N22]); //Node 21 "server" port
    node_client_create((void*) &node_creation_arguments[N22_N21]); //Node 22 "client" port

    //Node 22 to Node 23
    node_server_create((void*) &node_creation_arguments[N22_N23]); //Node 22 "server" port
    node_client_create((void*) &node_creation_arguments[N23_N22]); //Node 23 "client" port

    //Node 23 to Node 24
    node_server_create((void*) &node_creation_arguments[N23_N24]); //Node 23 "server" port
    node_client_create((void*) &node_creation_arguments[N24_N23]); //Node 24 "client" port

    //Node 24 to Node 25
    node_server_create((void*) &node_creation_arguments[N24_N25]); //Node 24 "server" port
    node_client_create((void*) &node_creation_arguments[N25_N24]); //Node 25 "client" port

    //Node 25 to Node 26
    node_server_create((void*) &node_creation_arguments[N25_N26]); //Node 25 "server" port
    node_client_create((void*) &node_creation_arguments[N26_N25]); //Node 26 "client" port

    //Node 26 to Node 27
    node_server_create((void*) &node_creation_arguments[N26_N27]); //Node 26 "server" port
    node_client_create((void*) &node_creation_arguments[N27_N26]); //Node 27 "client" port

    //Node 27 to Node 28
    node_server_create((void*) &node_creation_arguments[N27_N28]); //Node 27 "server" port
    node_client_create((void*) &node_creation_arguments[N28_N27]); //Node 28 "client" port

    //Node 28 to Node 29
    node_server_create((void*) &node_creation_arguments[N28_N29]); //Node 28 "server" port
    node_client_create((void*) &node_creation_arguments[N29_N28]); //Node 29 "client" port

    //Node 29 to Node 30
    node_server_create((void*) &node_creation_arguments[N29_N30]); //Node 29 "server" port
    node_client_create((void*) &node_creation_arguments[N30_N29]); //Node 30 "client" port

    //Node 30 to Node 31
    node_server_create((void*) &node_creation_arguments[N30_N31]); //Node 30 "server" port
    node_client_create((void*) &node_creation_arguments[N31_N30]); //Node 31 "client" port

    //Node 31 to Node 0
    node_server_create((void*) &node_creation_arguments[N31_N0]); //Node 31 "server" port
    node_client_create((void*) &node_creation_arguments[N0_N31]); //Node 0 "client" port

    //Initialize the globally available buffer
    node_rx_buffers = malloc(32 * sizeof(*node_rx_buffers)); //array of 32, 1 for each node
    int n, m;
    for (n = 0; n < 32; n++)
    {
        node_rx_buffers[n] = malloc(NUM_BUFFER_SLOTS * sizeof(**node_rx_buffers));
        for (m = 0; m < NUM_BUFFER_SLOTS; m++)
        {
            node_rx_buffers[n][m] = malloc(PACKET_SIZE * sizeof(***node_rx_buffers));
        }
    }
    
    //Initialization of Argument Structures
    Node_Tx_Args tx_arguments;
    //Node_Rx_Args rx_arguments;
    Node_Rx_Args* rx_arguments;
    memset(&tx_arguments, 0, sizeof(tx_arguments));
    //memset(&rx_arguments, 0, sizeof(rx_arguments));
    rx_arguments = malloc(10 * sizeof(*rx_arguments));

    //Initialize the Ring of Nodes
    fabric_init(&node_creation_arguments);

    Continuous_Rx_Args* args;
    args = malloc(64 * sizeof(*args));

    int node_count;
    for (node_count = 0; node_count <= 62; node_count += 2)
    {
        memset(&(rx_arguments[node_count]), 0, sizeof(rx_arguments[node_count]));
        set_rx_arguments(1, &node_creation_arguments[node_count], &(rx_arguments[node_count]));
        args[node_count].arguments = (void*) &(rx_arguments[node_count]);
        args[node_count].creation_arguments = (void*) node_creation_arguments;
        pthread_create(&sim, NULL, continuous_rx, (void*) &(args[node_count]));
    }

    for (node_count = 1; node_count <= 63; node_count += 2)
    {
        memset(&(rx_arguments[node_count]), 0, sizeof(rx_arguments[node_count]));
        set_rx_arguments(0, &node_creation_arguments[node_count], &(rx_arguments[node_count]));
        args[node_count].arguments = (void*) &(rx_arguments[node_count]);
        args[node_count].creation_arguments = (void*) node_creation_arguments;
        pthread_create(&sim, NULL, continuous_rx, (void*) &(args[node_count]));
    }

    //Temporary code (TESTING)
    char message[PACKET_SIZE];
    memset(message, 0, PACKET_SIZE * sizeof(*message));
    uint64_t* hop_bitmap;
    hop_bitmap = malloc(3 * sizeof(*hop_bitmap));
    hop_bitmap[0] = 0;
    hop_bitmap[1] = 0;
    hop_bitmap[2] = 0;
    create_bitmap_message_1(node_creation_arguments, N1_N2, 1, 0, hop_bitmap, message);
    set_tx_arguments(0, &node_creation_arguments[N1_N2], &tx_arguments, message);
    node_tx(&tx_arguments);

    // create_bitmap_message_1(node_creation_arguments, N5_N4, 3, 0, 0, message);
    // set_tx_arguments(0, &node_creation_arguments[N5_N4], &tx_arguments, message);
    // node_tx(&tx_arguments);
    // create_bitmap_message_1(node_creation_arguments, N5_N4, 2, 0, 0, message);
    // set_tx_arguments(0, &node_creation_arguments[N5_N4], &tx_arguments, message);
    // node_tx(&tx_arguments);

    // create_bitmap_message_1(node_creation_arguments, N1_N2, 2, message);
    // set_tx_arguments(1, &node_creation_arguments[N1_N2], &tx_arguments, message);
    // node_tx(&tx_arguments);

    // create_bitmap_message_1(node_creation_arguments, N4_N5, 5, message);
    // set_tx_arguments(1, &node_creation_arguments[N4_N5], &tx_arguments, message);
    // node_tx(&tx_arguments);

    // create_bitmap_message_1(node_creation_arguments, N5_N4, 4, message);
    // set_tx_arguments(0, &node_creation_arguments[N5_N4], &tx_arguments, message);
    // node_tx(&tx_arguments);

    // create_bitmap_message_1(node_creation_arguments, N3_N2, 2, message);
    // set_tx_arguments(0, &node_creation_arguments[N3_N2], &tx_arguments, message);
    // node_tx(&tx_arguments);

    // printf("%x\n", rx_arguments[0].rx_data.small_flit.packet_header_1);
    // printf("%x\n", rx_arguments[0].rx_data.small_flit.packet_header_0);

    //set_tx_arguments(1, &node_creation_arguments[0], &tx_arguments, message);
    //set_rx_arguments(0, &node_creation_arguments[3], &rx_arguments);

    //pthread_create(&sim, NULL, node_rx, (void*) &rx_arguments);
    //node_tx(&tx_arguments);
    //node_rx(&rx_arguments);
    //printf("Socket Index %d Credit: High Priority: %d Low Priority: %d Synchronous: %d Asynchronous: %d\n", 0, node_creation_arguments[0].socket_credit.high_priority_credit, node_creation_arguments[0].socket_credit.low_priority_credit, node_creation_arguments[0].socket_credit.sync_credit, node_creation_arguments[0].socket_credit.async_credit);

    /* Miscellaneous code for memory testing of the structures
    printf("%x\n", (rx_arguments.rx_data.buffer));
    printf("%x %x\n", &(rx_arguments.rx_data.small_flit.packet_header_0), &(rx_arguments.rx_data.small_flit.packet_header_1));

    printf("%x %x %x %x\n", *(rx_arguments.rx_data.buffer), *(rx_arguments.rx_data.buffer + 1), *(rx_arguments.rx_data.buffer + 2), *(rx_arguments.rx_data.buffer + 3));
    printf("%x %x\n", (rx_arguments.rx_data.small_flit.packet_header_0), (rx_arguments.rx_data.small_flit.packet_header_1));

    printf("%d\n", sizeof(*(rx_arguments.rx_data.buffer)));
    printf("%d\n", sizeof(unsigned char));
    */

    //printf("%x\n", *(rx_arguments.rx_data.buffer));

    //End of Temporary Code

    //Close All Threads and Free Memory
    sleep(1);
    int i , j;

    pthread_exit(NULL);

    for (i = 0; i < 64; i++)
        close(node_creation_arguments[i].sock_fd); 
    
    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < NUM_BUFFER_SLOTS; j++)
        {
            free(node_rx_buffers[i][j]);
        }
        free(node_rx_buffers[i]);
    }
    free(node_rx_buffers);

    free(node_creation_arguments);
    free(rx_arguments);
    free(args);

    return(0);
}

void node_create_function(int index, Node_Creation_Args* node_args, uint32_t node_index, uint32_t connected_node, uint32_t fabric, int port) {
    node_args -> index = index;
    node_args -> nodes_connected_to = connected_node;
    node_args -> nodes_in_fabric = fabric;
    node_args -> node_index = node_index;
    node_args -> port = port;
    printf("Index %d has node index of %x\n", node_args -> index, node_args -> node_index);
}

void create_bitmap_message_1(Node_Creation_Args* node_creation_arguments, uint32_t index, uint32_t target, uint32_t nodes_found, uint64_t* hop_bitmap, char* message)
{
    //NOTE: THE REASON IT IS PUSHED 4 TIMES EACH ELEMENT IS BECAUSE MESSAGE IS A CHAR (8BITS) WHILE TRYING TO STORE A 32 BITS IN EACH INDEX
    message[0] = 0x000000FF & node_creation_arguments[index].node_index; //index of the node 
    message[1] = (0x0000FF00 & node_creation_arguments[index].node_index) >> 8;
    message[2] = (0x00FF0000 & node_creation_arguments[index].node_index) >> 16;
    message[3] = (0xFF000000 & node_creation_arguments[index].node_index) >> 24;
    message[4] = 0x000000FF & node_creation_arguments[index].nodes_connected_to; //show the index of the nodes that the current nodes is connected to
    message[5] = (0x0000FF00 & node_creation_arguments[index].nodes_connected_to) >> 8;
    message[6] = (0x00FF0000 & node_creation_arguments[index].nodes_connected_to) >> 16;
    message[7] = (0xFF000000 & node_creation_arguments[index].nodes_connected_to) >> 24;
    message[8] = 0x000000FF & node_creation_arguments[index].nodes_in_fabric; //index of fabric manager
    message[9] = (0x0000FF00 & node_creation_arguments[index].nodes_in_fabric) >> 8;
    message[10] = (0x00FF0000 & node_creation_arguments[index].nodes_in_fabric) >> 16;
    message[11] = (0xFF000000 & node_creation_arguments[index].nodes_in_fabric) >> 24;
    message[12] = 0x000000FF & target; //the target node it is trying to return
    message[13] = (0x0000FF00 & target) >> 8;
    message[14] = (0x00FF0000 & target) >> 16;
    message[15] = (0xFF000000 & target) >> 24;
    message[16] = 0x000000FF & nodes_found;
    message[17] = (0x0000FF00 & nodes_found) >> 8;
    message[18] = (0x00FF0000 & nodes_found) >> 16;
    message[19] = (0xFF000000 & nodes_found) >> 24;
    int i = 0;
    for (i = 0; i < 3; i++) {
        message[20 + (8 * i)] = 0x000000FF & hop_bitmap[i]; //contains the order of the passed nodes 
        message[21 + (8 * i)] = (0x0000FF00 & hop_bitmap[i]) >> 8;
        message[22 + (8 * i)] = (0x00FF0000 & hop_bitmap[i]) >> 16;
        message[23 + (8 * i)] = (0xFF000000 & hop_bitmap[i]) >> 24;
        message[24 + (8 * i)] = (0xFF00000000 & hop_bitmap[i]) >> 32;
        message[25 + (8 * i)] = (0xFF0000000000 & hop_bitmap[i]) >> 40;
        message[26 + (8 * i)] = (0xFF000000000000 & hop_bitmap[i]) >> 48;
        message[27 + (8 * i)] = (0xFF00000000000000 & hop_bitmap[i]) >> 56;
        //Instead of storing a single hop_bitmap, create an array of 5 hop_bitmaps each element of uint32t 
    }
    //TODO: Expand from 5 node hop bit map to 32 node hop bitmap (160 bits) (DONE WITH THE SPACE)
    //TODO: Keep track of which port the discovery message came in on for each node (a trace of ports connected too)
    return;
}


void write_buffers()
{
    FILE* fptr = fopen("rx_buffers.txt", "w");

    int i, j, q;

    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < NUM_BUFFER_SLOTS; j++)
        {
            for (q = PACKET_SIZE - 1; q >= 0; q--)
            {
                fputc(hex_to_ascii((node_rx_buffers[i][j][q] >> 4)), fptr);
                fputc(hex_to_ascii(node_rx_buffers[i][j][q] & 0xF), fptr);
            }
            fputc('\n', fptr);
        }
        fputc('\n', fptr);
        fputc('\n', fptr);
        fputc('\n', fptr);
    }

    fclose(fptr);

    return;
}

unsigned char hex_to_ascii(unsigned char in)
{
    unsigned char result;

    if ((0xF & in) < 0xa)
    {
        result = (0xF & in) + 48;
    }
    else
    {
        result = (0xF & in) + 55;
    }

    if (((0xF0 & in) >> 4) < 0xa)
    {
        result |= (((0xF0 & in) >> 4) + 48) << 4;
    }
    else
    {
        result |= (((0xF0 & in) >> 4) + 55) << 4;
    }

    return(result);
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

    n = -1;
    
    Node_Rx_Args* rx_arguments = arguments;

    len = sizeof(rx_arguments -> network_addr);
    printf("node index %x\n", rx_arguments -> node_index);
    while (n == -1)
    {
        n = recvfrom(rx_arguments -> sock_fd, rx_arguments -> rx_data.buffer, PACKET_SIZE, MSG_WAITALL, (struct sockaddr *) &(rx_arguments -> network_addr), &len);
        sleep(1);
    }

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

void update_hop_bitmap(int curr_node, uint64_t* hop_bitmap) {   
    //NOTE: THIS IS ASSUMING THE LOWEST NUMBER NODE IS 0 
    int i = 0;
    int update_hop_value;
    uint64_t bit_mask; 
    int control;
    for (i = 0; i < 32; i++) {
        if (i < 12) {
            if (((hop_bitmap[0] >> (5 * i)) && 0x1F) != 0)
                update_hop_value = ((hop_bitmap[0] >> (5 * i) && 0x1F) + 1) << (5 * i);
                bit_mask = ~(0x1F << (5 * i));
                hop_bitmap[0] &= bit_mask;
                hop_bitmap[0] |= update_hop_value;
        }
        else if (i < 24){
            if (((hop_bitmap[1] >> (5 * (i - 12))) && 0x1F) != 0) {
                update_hop_value = ((hop_bitmap[1] >> (5 * (i - 12)) && 0x1F) + 1) << (5 * (i - 12));
                bit_mask = ~(0x1F << (5 * (i - 12)));
                hop_bitmap[1] &= bit_mask;
                hop_bitmap[1] |= update_hop_value;
            }
        }
        else {
            if (((hop_bitmap[2] >> (5 * (i - 24))) && 0x1F) != 0) {
                update_hop_value = ((hop_bitmap[2] >> (5 * (i - 24)) && 0x1F) + 1) << (5 * (i - 24));
                bit_mask = ~(0x1F << (5 * (i - 24)));
                hop_bitmap[2] &= bit_mask;
                hop_bitmap[2] |= update_hop_value;
             }
        }
    }
    int bitmap_update_index = curr_node / 12;
    int shift_number = 5 * (curr_node / (bitmap_update_index + 1));
    hop_bitmap[bitmap_update_index] |= 1 << shift_number;

}

//void forward_message(int index, Node_Creation_Args* temp_node_creation_arguments, Node_Creation_Args* node_creation_arguments, Node_Rx_Args* rx_arguments, Node_Tx_Args tx_arguments);

void continuous_rx(void* args)
    {
        int pointer = 0;

        //Used for forwarding messages
        Node_Tx_Args tx_arguments;
        Continuous_Rx_Args* function_args = args;
        Node_Creation_Args temp_node_creation_arguments;

        Node_Creation_Args* node_creation_arguments = function_args -> creation_arguments;
        Node_Rx_Args* rx_arguments = function_args -> arguments;
        uint64_t hop_bitmap[3] = {0};
        while(1)
        {
            printf("Waiting...\n");
            int i;
            node_rx(rx_arguments);
            for (i = 0; i < PACKET_SIZE; i++)
            {
                int temp = log10(rx_arguments -> node_index) / log10(2) - 1;
                //node_rx_buffers[temp][pointer][i] = rx_arguments -> rx_data.buffer[i];
                memcpy(&(node_rx_buffers[temp][pointer][i]), &(rx_arguments -> rx_data.buffer[i]), sizeof(rx_arguments -> rx_data.buffer[i]));
            }

            //Locate nodes found bitmap, check if current nodes bit is set, if it has already been set, already visited every node in ring
            //Stop sending discovery messages
            //If bit is not set, set it and (but not yet)
            //For each found node, increment its hop value (use index of found nodes that have a 1 to find the corresponding bits in the hop map, (5 * (i + 1) - 1:5 * i) is value of hop distance)
            //ASSUMPTION: using a ring configuration, so do not need to worry about sequence number because each hop value is unique i.e. no two hop values will be the same
            //Now set found bit for current node
            //To create a forwarding message if this is not the destination node
            int dest = rx_arguments -> rx_data.buffer[FLIT_TARGET_0 / (sizeof(char) * 8)] & 0x1F;
            int index = log10(rx_arguments -> node_index) / log10(2);
            circle_count = index == 1 ? circle_count + 1 : circle_count; //TEMP

            int curr_nodes_found;
            //int curr_hop_values;
            curr_nodes_found = (rx_arguments -> rx_data.buffer)[16];
            curr_nodes_found |= (rx_arguments -> rx_data.buffer)[17] << 8;

            /*
            curr_hop_values = (rx_arguments -> )rx_data.buffer[20];
            curr_hop_values |= (rx_arguments -> rx_data.buffer)[21] << 8;
            curr_hop_values |= (rx_arguments -> rx_data.buffer)[22] << 16;
            curr_hop_values |= (rx_arguments -> rx_data.buffer)[23] << 24;*/
            for (i = 0; i < 3; i++) {
                hop_bitmap[i] = (uint64_t)(rx_arguments -> rx_data.buffer)[20 + (8 * i)];
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[21 + (8 * i)] << 8;
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[22 + (8 * i)] << 16;
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[23 + (8 * i)] << 24;
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[24 + (8 * i)] << 32;
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[25 + (8 * i)] << 40;
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[26 + (8 * i)] << 48;
                hop_bitmap[i] |= (uint64_t)(rx_arguments -> rx_data.buffer)[27 + (8 * i)] << 56;
            }
        
            if ((curr_nodes_found >> index) & 0x1) 
            {
                discovery_done = 1;
            }
            if (!discovery_done) 
            {
                int loop_control;
                //CHANGE FROM 1 to 5 -> 0 to 32
                update_hop_bitmap(rx_arguments -> node_index, hop_bitmap);
                /*for (loop_control = 0; loop_control < 5; loop_control++)
                {   
                    
                    int hop_increment = (curr_nodes_found >> loop_control) & 0x1;
                    if (!hop_increment)
                    {
                        int temp = 0x1F; //So that an integer conversion happens
                        int hop_value = (curr_hop_values >> (loop_control * 5)) & temp; //(5 * (i + 1) - 1:5 * i)
                        hop_value++;
                        curr_hop_values = curr_hop_values & ~(temp << (loop_control * 5));
                        curr_hop_values = curr_hop_values | (hop_value << (loop_control * 5)); 
                        
                        //printf("%x\n", curr_hop_values);
                    }
                }*/
                curr_nodes_found |= (0x1 << index); //assume this is 
                /*
                (rx_arguments -> rx_data.buffer)[16] = 0xFF & curr_nodes_found;
                (rx_arguments -> rx_data.buffer)[17] = (0xFF00 & curr_nodes_found) << 8;
                (rx_arguments -> rx_data.buffer)[20] = 0xFF & curr_hop_values;
                (rx_arguments -> rx_data.buffer)[21] = (0xFF00 & curr_hop_values) << 8;
                (rx_arguments -> rx_data.buffer)[22] = (0xFF0000 & curr_hop_values) << 16;
                (rx_arguments -> rx_data.buffer)[23] = (0xFF000000 & curr_hop_values) << 24;*/
                for (i = 0; i < 3; i++) {
                    (rx_arguments -> rx_data.buffer)[20 + (8 * i)] = 0xFF && hop_bitmap[i];
                    (rx_arguments -> rx_data.buffer)[20 + (8 * i)] = 0x000000FF & hop_bitmap[i]; //contains the order of the passed nodes 
                    (rx_arguments -> rx_data.buffer)[21 + (8 * i)] = (0x0000FF00 & hop_bitmap[i]) >> 8;
                    (rx_arguments -> rx_data.buffer)[22 + (8 * i)] = (0x00FF0000 & hop_bitmap[i]) >> 16;
                    (rx_arguments -> rx_data.buffer)[23 + (8 * i)] = (0xFF000000 & hop_bitmap[i]) >> 24;
                    (rx_arguments -> rx_data.buffer)[24 + (8 * i)] = (0xFF00000000 & hop_bitmap[i]) >> 32;
                    (rx_arguments -> rx_data.buffer)[25 + (8 * i)] = (0xFF0000000000 & hop_bitmap[i]) >> 40;
                    (rx_arguments -> rx_data.buffer)[26 + (8 * i)] = (0xFF000000000000 & hop_bitmap[i]) >> 48;
                    (rx_arguments -> rx_data.buffer)[27 + (8 * i)] = (0xFF00000000000000 & hop_bitmap[i]) >> 56;
                }
            }

            if (dest != index | !discovery_done) //(circle_count < 2)
            {
                printf("Forwarding Message at Node %d\n", index);
                //To set next destination of message
                switch (index)
                {
                case 0:
                    if ((dest == 31) | (dest == 30) & 0)
                    {
                        //Node 0 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N0_N31]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 0 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N0_N1]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 1:
                    if ((dest == 31) | (dest == 0) & 0)
                    {
                        //Node 0 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N1_N0]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 0 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N1_N2]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 2:
                    if (((dest == 1) | (dest == 0)) & 0)
                    {
                        //Node 2 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 2, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N2_N1]), sizeof(node_creation_arguments[N2_N1]));
                        set_tx_arguments(0,&temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 2 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 2, 0, 0, 0, PORT_2);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N2_N3]), sizeof(node_creation_arguments[N2_N3]));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 3:
                    if (((dest == 1) | (dest == 2)) & 0)
                    {
                        //Node 3 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 3, 0, 0, 0, PORT_2);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N3_N2]), sizeof(node_creation_arguments[N3_N2]));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 3 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 3, 0, 0, 0, PORT_3);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N3_N4]), sizeof(node_creation_arguments[N3_N4]));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 4:
                    if (((dest == 3) | (dest == 2)) & 0)
                    {
                        //Node 4 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 4, 0, 0, 0, PORT_3);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N4_N3]), sizeof(node_creation_arguments[N4_N3]));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 4 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 4, 0, 0, 0, PORT_4);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N4_N5]), sizeof(node_creation_arguments[N4_N5]));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 5:
                    if (((dest == 4) | (dest == 3)) & 0)
                    {
                        //Node 5 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 5, 0, 0, 0, PORT_4);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N5_N4]), sizeof(node_creation_arguments[N5_N4]));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 5 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 5, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N5_N6]), sizeof(node_creation_arguments[N5_N6]));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 6:
                    if ((dest == 5) | (dest == 4) & 0)
                    {
                        //Node 6 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N6_N5]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 6 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N6_N7]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 7:
                    if ((dest == 5) | (dest == 6) & 0)
                    {
                        //Node 7 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N7_N6]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 7 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N7_N8]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 8:
                    if ((dest == 6) | (dest == 7) & 0)
                    {
                        //Node 6 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N8_N7]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 6 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N8_N9]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;

                case 9:
                    if ((dest == 8) | (dest == 7) & 0)
                    {
                        //Node 9 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N9_N8]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 9 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N9_N10]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 10:
                    if ((dest == 9) | (dest == 8) & 0)
                    {
                        //Node 10 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N10_N9]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 10 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N10_N11]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 11:
                    if ((dest == 9) | (dest == 10) & 0)
                    {
                        //Node 11 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N11_N10]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 11 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N11_N12]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 12:
                    if ((dest == 10) | (dest == 11) & 0)
                    {
                        //Node 12 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N11_N10]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 12 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N12_N13]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 13:
                    if ((dest == 12) | (dest == 11) & 0)
                    {
                        //Node 13 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N13_N12]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 12 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N13_N14]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                
                case 14:
                    if ((dest == 13) | (dest == 12) & 0)
                    {
                        //Node 14 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N14_N13]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 14 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N14_N15]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;

                case 15:
                    if ((dest == 14) | (dest == 13) & 0)
                    {
                        //Node 15 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N15_N14]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 15 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N15_N16]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;

                case 16:
                    if ((dest == 14) | (dest == 15) & 0)
                    {
                        //Node 16 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N16_N15]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 16 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N16_N17]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 17:
                    if ((dest == 16) | (dest == 15) & 0)
                    {
                        //Node 17 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N17_N16]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 17 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N17_N18]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 18:
                    if ((dest == 16) | (dest == 17) & 0)
                    {
                        //Node 18 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N18_N17]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 18 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N18_N19]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 19:
                    if ((dest == 18) | (dest == 17) & 0)
                    {
                        //Node 19 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N19_N18]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 19 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N19_N20]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 20:
                    if ((dest == 18) | (dest == 19) & 0)
                    {
                        //Node 20 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N20_N19]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 20 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N20_N21]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 21:
                    if ((dest == 20) | (dest == 19) & 0)
                    {
                        //Node 21 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N21_N20]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 21 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N21_N22]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 22:
                    if ((dest == 20) | (dest == 21) & 0)
                    {
                        //Node 22 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N22_N21]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 22 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N22_N23]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 23:
                    if ((dest == 22) | (dest == 21) & 0)
                    {
                        //Node 23 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N23_N22]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 23 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N23_N24]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 24:
                    if ((dest == 23) | (dest == 22) & 0)
                    {
                        //Node 24 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N24_N23]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 24 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N24_N25]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 25:
                    if ((dest == 23) | (dest == 24) & 0)
                    {
                        //Node 24 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N25_N24]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 24 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N25_N26]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 26:
                    if ((dest == 25) | (dest == 24) & 0)
                    {
                        //Node 26 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N26_N25]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 26 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N26_N27]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 27:
                    if ((dest == 25) | (dest == 26) & 0)
                    {
                        //Node 27 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N27_N26]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 27 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N27_N28]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 28:
                    if ((dest == 27) | (dest == 26) & 0)
                    {
                        //Node 28 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N28_N27]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 28 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N28_N29]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 29:
                    if ((dest == 27) | (dest == 28) & 0)
                    {
                        //Node 29 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N29_N28]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 29 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N29_N30]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 30:
                    if ((dest == 29) | (dest == 28) & 0)
                    {
                        //Node 30 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N30_N29]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 30 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N30_N31]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                case 31:
                    if ((dest == 29) | (dest == 30) & 0)
                    {
                        //Node 31 "client" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_5);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N31_N30]), sizeof(*node_creation_arguments));
                        set_tx_arguments(0, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    else
                    {
                        //Node 31 "server" port
                        //set_node_creation_arguments(&temp_node_creation_arguments, 1, 0, 0, 0, PORT_1);
                        memcpy(&temp_node_creation_arguments, &(node_creation_arguments[N31_N0]), sizeof(*node_creation_arguments));
                        set_tx_arguments(1, &temp_node_creation_arguments, &tx_arguments, rx_arguments -> rx_data.buffer);
                    }
                    break;
                default:
                    printf("AN ERROR HAS OCCURRED IN FORWARDING!\n");
                    break;
                }
                node_tx(&tx_arguments);
            }
            else
            {
                pointer = pointer + 1;
                pointer = pointer > NUM_BUFFER_SLOTS - 1 ? 0 : pointer;
                write_buffers();
                printf("Buffer at Node %d Written (Pointer at %d)\n", index, pointer);
            }
        }
    }

void set_node_creation_arguments(Node_Creation_Args* node_creation_arguments, int index, int nodes_connected_to, int nodes_in_fabric, int node_index, int port)
{
    memset(node_creation_arguments, 0, sizeof(node_creation_arguments));    
    node_creation_arguments -> index = index;
    node_creation_arguments -> nodes_connected_to = nodes_connected_to;
    node_creation_arguments -> nodes_in_fabric = nodes_in_fabric;
    node_creation_arguments -> node_index = node_index;
    node_creation_arguments -> port = port;
}

void set_tx_arguments(int is_server, Node_Creation_Args* tx, Node_Tx_Args* tx_args, unsigned char* packet)
{
    tx_args -> index = tx -> index;
    tx_args -> node_index = tx -> node_index;
    tx_args -> node_index = tx -> nodes_connected_to;
    tx_args -> nodes_in_fabric = tx -> nodes_in_fabric;
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
 
    memcpy(&(tx_args -> tx_data.buffer), packet, PACKET_SIZE);

    return;
}

void set_rx_arguments(int is_server, Node_Creation_Args* rx, Node_Rx_Args* rx_args)
{
    rx_args -> index = rx -> index;
    rx_args -> node_index = rx -> node_index;
    rx_args -> nodes_connected_to = rx -> nodes_connected_to;
    rx_args -> nodes_in_fabric = rx -> nodes_in_fabric;
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

    while(server_index <= 62)
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
        client_index = client_index == 63 ? 1 : client_index + 2;
    }

    return;
}

unsigned char bit_read_index(unsigned char num, char index)
{
    unsigned char result;

    if(index >= 8)
    {
        return 0;
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

