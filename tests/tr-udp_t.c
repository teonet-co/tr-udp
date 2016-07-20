/*
 * File:   tr-udp_t.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on Jun 1, 2016, 12:36:15 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CUnit/Basic.h>

#include "trudp.h"
#include "packet.h"   
#include "trudp_stat.h"

// For tests use only
static inline uint32_t trudpGetNewId(trudpChannelData *td) {
    return td->sendId++;
}

#undef NO_MESSAGES
#define NO_MESSAGES 1

/*
 * CUnit Test Suite
 */

int init_suite(void);
int clean_suite(void);

/**
 * Create and Destroy TR-UDP
 */
static void create_test() {
    
    // Create TR-UDP
    trudpData *td = trudpInit(0, 0, NULL, NULL);
    CU_ASSERT_PTR_NOT_NULL(td);
    
    // Destroy TR-UDP
    trudpDestroy(td);
}

/**
 * Send data test
 */
static void send_data_test() {

    // Create TR-UDP
    trudpData *td = trudpInit(0, 0, NULL, NULL);
    trudpChannelData *tcd = trudp_ChannelNew(td, "0", 8000, 0);
    CU_ASSERT_PTR_NOT_NULL(tcd);

    // Send data
    char *data = "HelloTR-UDP!";
    size_t data_length = strlen(data) + 1;
    CU_ASSERT(trudp_ChannelSendData(tcd, data, data_length) > 0);
    CU_ASSERT(trudpQueueSize(tcd->sendQueue->q) == 1); // Send Queue should contain 1 element
    CU_ASSERT_PTR_NOT_NULL(trudpPacketQueueFindById(tcd->sendQueue, 0)); // Send Queue should contain an element with ID 0
    
    // Destroy TR-UDP
    trudp_ChannelDestroy(tcd);
    trudpDestroy(td);
}


///**
// * Process received DATA packets data callback function
// * 
// * @param data
// * @param data_length
// * @param user_data
// */
//static void trudpProcessDataCb(void *td, void *data, size_t data_length, void *user_data) {
//    
//    #if !NO_MESSAGES
//    void *packet = trudpPacketGetPacket(data);
//    double tt = (trudpGetTimestamp() - trudpPacketGetTimestamp(packet) )/1000.0;
//    printf("\n%s DATA: \"%s\" processed %.3f ms ...", user_data ? (char*)user_data : "", (char*)data, tt);
//    printf("\n");
//    #endif
//}

///**
// * Process received ACK packets data callback function
// * 
// * @param data
// * @param data_length
// * @param user_data
// */
//static void trudpProcessAckCb(void *td, void *data, size_t data_length, void *user_data) {
//    
//    #if !NO_MESSAGES
//    void *packet = trudpPacketGetPacket(data);
//    double tt = (trudpGetTimestamp() - trudpPacketGetTimestamp(packet) )/1000.0;
//    printf("\n%s ACK processed %.3f ms ...", (char*)user_data, tt );
//    #endif
//    
//}
                
/**
 * Show process result
 * 
 * @param rv
 */
static void _showProcessResult(void *rv, void* user_data) {
    #if !NO_MESSAGES
    if(!rv) printf("\n%s Packet was processed, no DATA present (ACK or RESET packet) ...", user_data ? (char*)user_data : "");
    else if(rv == (void*)-1) printf("\n%s Wrong DATA packet, skipped ...", user_data ? (char*)user_data : "");
    else printf("\n%s Packet was processed, DATA was send to callback ...", user_data ? (char*)user_data : "");    
    //printf("\n");
    #endif
}

/**
 * Process received packet
 */
static void process_received_packet_test() {

    // Create TR-UDP
    trudpData *td = trudpInit(0, 0, 0, NULL);
    trudpChannelData *tcd = trudp_ChannelNew(td, "0", 8000, 0);
    CU_ASSERT_PTR_NOT_NULL(tcd);
    
    // Create DATA packets 
    uint32_t id = 0;
    char *data[] = { "Hello TR-UDP test 0!", "Hello TR-UDP test 1!", "Hello TR-UDP test 2!", "Hello TR-UDP test 3!" };
    size_t packetLength[4], 
           processedData_length, 
           data_length[] = { strlen(data[0]) + 1, strlen(data[1]) + 1, strlen(data[2]) + 1, strlen(data[3]) + 1 };
    void *rv,
         *packetDATA[] = { 
            trudpPacketDATAcreateNew(trudpGetNewId(tcd), 0, (void*)data[0], data_length[0], &packetLength[0]), 
            trudpPacketDATAcreateNew(trudpGetNewId(tcd), 0, (void*)data[1], data_length[1], &packetLength[1]),
            trudpPacketDATAcreateNew(trudpGetNewId(tcd), 0, (void*)data[2], data_length[2], &packetLength[2]),
            trudpPacketDATAcreateNew(trudpGetNewId(tcd), 0, (void*)data[3], data_length[3], &packetLength[3])
         };
    CU_ASSERT(trudpPacketCheck(packetDATA[0], packetLength[0]));
    CU_ASSERT(trudpPacketCheck(packetDATA[1], packetLength[1]));
    CU_ASSERT(trudpPacketCheck(packetDATA[2], packetLength[2]));
    CU_ASSERT(trudpPacketCheck(packetDATA[3], packetLength[3]));

    // Process received packet (id 0) test
    id = 0;
    rv = trudp_ChannelProcessReceivedPacket(tcd, packetDATA[id], packetLength[id], &processedData_length);
    _showProcessResult(rv, TD(tcd)->user_data);
    CU_ASSERT_FATAL(rv && rv != (void*)-1);
    CU_ASSERT_STRING_EQUAL(data[id], rv);
    
    // Process received packet (id 1) test
    id = 1;
    rv = trudp_ChannelProcessReceivedPacket(tcd, packetDATA[id], packetLength[id], &processedData_length);
    _showProcessResult(rv, TD(tcd)->user_data);
    CU_ASSERT_FATAL(rv && rv != (void*)-1);
    CU_ASSERT_STRING_EQUAL(data[id], rv);
    
    // Process received packet (id 3) test
    id = 3;
    rv = trudp_ChannelProcessReceivedPacket(tcd, packetDATA[id], packetLength[id], &processedData_length);
    _showProcessResult(rv, TD(tcd)->user_data);
    CU_ASSERT_FATAL(rv == NULL); // The trudpProcessReceivedPacket save this packet to receiveQueue and return NULL
    
    // Process received packet (id 2) test
    id = 2;
    rv = trudp_ChannelProcessReceivedPacket(tcd, packetDATA[id], packetLength[id], &processedData_length);
    _showProcessResult(rv, TD(tcd)->user_data);
    CU_ASSERT_FATAL(rv && rv != (void*)-1);
    CU_ASSERT_STRING_EQUAL(data[3], rv); // The trudpProcessReceivedPacket process this package, and package from queue, and return data of last processed - id = 3
    
    // Process wrong packet
    void *wrongPacket = "Some wrong packet ...";
    size_t wrongPacketLength = strlen(wrongPacket) + 1;
    rv = trudp_ChannelProcessReceivedPacket(tcd, wrongPacket, wrongPacketLength, &processedData_length);
    _showProcessResult(rv, TD(tcd)->user_data);
    CU_ASSERT_FATAL(rv == (void*)-1);
    
    // Free packets
    int i; for(i=0; i < 4; i++) trudpPacketCreatedFree(packetDATA[i]);
    
    // Destroy TR-UDP
    trudp_ChannelDestroy(tcd);
    trudpDestroy(td);
}


trudpChannelData *tcd_A, *tcd_B;

void td_A_eventCb(void *tcd_ptr, int event, void *packet, size_t packet_length, void *user_data) {
    
    switch(event) {
        
        case PROCESS_SEND: {
            
            /*int type = */trudpPacketGetType(packet);
            #if !NO_MESSAGES
            printf("\n%s_writeCb %s %s ...", user_data ? (char*)user_data : "", type == 0x0 ? "DATA" : type == 0x1 ? "ACK" : "RESET", type == 0x0 ? (char*)trudpPacketGetData(packet) : "");
            #endif

            // Receive data by B TR-UDP
            size_t processedData_length;
            void *rv = trudp_ChannelProcessReceivedPacket(tcd_B, packet, packet_length, &processedData_length);
//            _showProcessResult(rv, td_B->user_data);
            CU_ASSERT(!rv || (rv && rv != (void*)-1));
        }
    }
}

void td_B_eventCb(void *tcd_ptr, int event, void *packet, size_t packet_length, void *user_data) {
    
    switch(event) {
        
        case PROCESS_SEND: {
            
            /*int type = */trudpPacketGetType(packet);
            #if !NO_MESSAGES
            printf("\n%s_writeCb %s %s ...", user_data ? (char*)user_data : "", type == 0x0 ? "DATA" : type == 0x1 ? "ACK" : "RESET", type == 0x0 ? (char*)trudpPacketGetData(packet) : "");
            #endif

            // Receive data by A TR-UDP
            size_t processedData_length;
            void *rv = trudp_ChannelProcessReceivedPacket(tcd_A, packet, packet_length, &processedData_length);
//            _showProcessResult(rv, td_A->user_data);
            CU_ASSERT(!rv || (rv && rv != (void*)-1));
        }
    }
}

/**
 * Send data / process received packet test
 */
static void send_process_received_packet_test() {

    // Create sender TR-UDP
    trudpData *td_A = trudpInit(0, 0, td_A_eventCb, "td_A");
    tcd_A = trudp_ChannelNew(td_A, "0", 8000, 0);
    CU_ASSERT_PTR_NOT_NULL(tcd_A);
    
    // Create receiver TR-UDP
    trudpData *td_B = trudpInit(0, 0, td_B_eventCb, "td_B");
    tcd_B = trudp_ChannelNew(td_B, "0", 8001, 0);
    CU_ASSERT_PTR_NOT_NULL(tcd_B);
    
    // Create DATA packets 
    uint32_t idx, num_packets = 4;
    char *data[] = { "Hello TR-UDP test 0!", "Hello TR-UDP test 1!", "Hello TR-UDP test 2!", "Hello TR-UDP test 3!" };
    size_t data_length[] = { strlen(data[0]) + 1, strlen(data[1]) + 1, strlen(data[2]) + 1, strlen(data[3]) + 1 };
    
    #if !NO_MESSAGES
    printf("\n");
    #endif
    
    // Send data from A to B, packet data idx = 0
    idx = 0;
    CU_ASSERT(trudp_ChannelSendData(tcd_A, data[idx], data_length[idx]) > 0);    
    CU_ASSERT(trudp_ChannelSendData(tcd_B, data[num_packets-idx-1], data_length[num_packets-idx-1]) > 0);
    
    idx = 1;
    // Test send queue retrieves
//       void *wc = td_A->sendCb; td_A->sendCb = NULL; // Stop send callback
       CU_ASSERT(trudp_ChannelSendData(tcd_A, data[idx], data_length[idx]) > 0);
//       usleep(tcd_A->triptime); td_A->sendCb = wc; // Sleep and restore callback
       #if !NO_MESSAGES
       printf("\ntrudpProcessSendQueue begin");
       #endif    
       /*int r = */trudp_SendQueueProcessChannel(tcd_A, trudpGetTimestamp(), NULL);
       #if !NO_MESSAGES
       printf("send queue processed times: %d ...\ntrudpProcessSendQueue end\n", r);
       #endif
    // end test send queue retrieves
    CU_ASSERT(trudp_ChannelSendData(tcd_B, data[num_packets-idx-1], data_length[num_packets-idx-1]) > 0);
     
    idx = 3;
    CU_ASSERT(trudp_ChannelSendData(tcd_A, data[idx], data_length[idx]) > 0);
    CU_ASSERT(trudp_ChannelSendData(tcd_B, data[num_packets-idx-1], data_length[num_packets-idx-1]) > 0);
    
    idx = 2;
    CU_ASSERT(trudp_ChannelSendData(tcd_A, data[idx], data_length[idx]) > 0);
    CU_ASSERT(trudp_ChannelSendData(tcd_B, data[num_packets-idx-1], data_length[num_packets-idx-1]) > 0);
    
//    char *stat_js = trudpStatGet(TD(tcd_A), JSON_TYPE, NULL);
//    puts(stat_js);
//    free(stat_js);
    
    char *stat_str = ksnTRUDPstatShowStr(TD(tcd_A));
    puts(stat_str);
    free(stat_str);
    
//    stat_str = ksnTRUDPstatShowStr(TD(tcd_B));
//    puts(stat_str);
//    free(stat_str);
    
    // Destroy TR-UDP
    trudp_ChannelDestroy(tcd_A);
    trudp_ChannelDestroy(tcd_B);
}    

int trUdpSuiteAdd() {
    
    CU_pSuite pSuite = NULL;

    /* Add a suite to the registry */
    pSuite = CU_add_suite("TR-UDP main module", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "create test", create_test)) 
     || (NULL == CU_add_test(pSuite, "send data test", send_data_test))
     || (NULL == CU_add_test(pSuite, "process received packet test", process_received_packet_test))  
     || (NULL == CU_add_test(pSuite, "send data / process received packet test", send_process_received_packet_test))
    
            ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
