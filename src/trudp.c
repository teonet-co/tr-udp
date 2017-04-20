/**
 * The MIT License
 *
 * Copyright 2016 Kirill Scherba <kirill@scherba.ru>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \file   tr-udp.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Created on May 31, 2016, 1:44 AM
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "trudp.h"
#include "trudp_channel.h"
#include "trudp_utils.h"

#include "packet.h"
#include "trudp_stat.h"
#include "packet_queue.h"

// Basic module functions ====================================================

/**
 * Initialize TR-UDP
 *
 * @param fd File descriptor to read write data
 * @param port Port (optional)
 * @param event_cb Event callback
 * @param user_data User data which will send to most library function
 *
 * @return
 */
trudpData *trudpInit(int fd, int port, trudpEventCb event_cb, void *user_data) {

    trudpData* trudp = (trudpData*) malloc(sizeof(trudpData));
    memset(trudp, 0, sizeof(trudpData));

    trudp->map = trudpMapNew(MAP_SIZE_DEFAULT, 1);
    trudp->psq_data = NULL;
    trudp->user_data = user_data;
    trudp->port = port;
    trudp->fd = fd;

    // Initialize statistic data
    trudpStatInit(trudp);
    trudp->started = trudpGetTimestampFull();

    // Set event callback
    trudp->evendCb = event_cb;

    // Set trudpData label
    trudp->trudp_data_label[0] = 0x77557755;
    trudp->trudp_data_label[1] = 0x55775577;

    // Send INITIALIZE event
    trudpEventSend((void*)trudp, INITIALIZE, NULL, 0, NULL);

    return trudp;
}

/**
 * Destroy TR-UDP
 *
 * @param trudp
 */
void trudpDestroy(trudpData* td) {

    if(td) {
        trudpEventSend((void*)td, DESTROY, NULL, 0, NULL);
        trudpMapDestroy(td->map);
        free(td);
    }
}

/**
 * Execute trudpEventCb callback
 *
 * @param t_pointer Pointer to trudpData or to trudpChannelData
 * @param event
 * @param data
 * @param data_length
 * @param user_data
 * @param cb
 */
void trudpEventSend(void *t_pointer, int event, void *data,
        size_t data_length, void *user_data) {

    trudpData *td = (trudpData *)t_pointer;

    if(td->trudp_data_label[0] == 0x77557755 &&
       td->trudp_data_label[1] == 0x55775577) {

        trudpEventCb cb = td->evendCb;
        if(cb != NULL) cb(t_pointer, event, data, data_length, td->user_data);
    }
    else {
        
        trudpChannelData *tcd = (trudpChannelData *) t_pointer;

        trudpEventCb cb = TD(tcd)->evendCb;
        if(cb != NULL) cb((void*)tcd, event, data, data_length, TD(tcd)->user_data);
    }
}

/**
 * Execute evetrudpEventCbnt callback with event GOT_DATA when data packet received
 * 
 * @param t_pointer Pointer to trudpChannelData
 * @param packet Pointer to received packet
 * @param data_length [out] Packets data length (NULL if not need to return it
 * 
 * @return  Pointer to packet data
 */
void *trudpEventGotDataSend(void *t_pointer, void *packet, 
        size_t *data_length) {
    
    void *data = trudpPacketGetData(packet);
    size_t data_len = trudpPacketGetDataLength(packet);
    trudpEventSend(t_pointer, GOT_DATA, data, data_len, NULL);
    if(data_length) *data_length = data_len;
    
    return data;
}

/**
 * Destroy all trudp channels
 *
 * @param tcd Pointer to trudpData
 */
void trudp_ChannelDestroyAll(trudpData *td) {

    trudpMapElementData *el;
    trudpMapIterator *it;
    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)
                    trudpMapIteratorElementData(el, NULL);
            trudp_ChannelDestroy(tcd);
        }
        trudpMapIteratorDestroy(it);
    }
}

/**
 * Destroy trudp channel by Address, port and channel number
 *
 * @param td Pointer to trudpData
 * @param addr String with IP address
 * @param port Port number
 * @param channel Cannel number 0-15
 */
inline void trudp_ChannelDestroyAddr(trudpData *td, char *addr, int port, int channel) {

        size_t key_length;
        char *key = trudpMakeKey(addr, port, channel, &key_length);
        trudpChannelData *tcd = (trudpChannelData *)trudpMapGet(td->map, key, key_length, NULL);
        if(tcd && tcd != (void *)-1) trudp_ChannelDestroy(tcd);
}

/**
 * Add channel to the trudpData map
 * 
 * @param td
 * @param key
 * @param key_length
 * @param tcd
 * @return 
 */
trudpChannelData *trudp_ChannelAddToMap(void *td, char *key, size_t key_length, 
        trudpChannelData *tcd) {
    
    return trudpMapAdd(((trudpData *)td)->map, key, key_length, tcd, 
            sizeof(trudpChannelData));
}

// ===========================================================================


/**
 * Create RESET packet and send it to all channels
 *
 * @param td
 */
void trudpSendResetAll(trudpData *td) {

    trudpMapElementData *el;
    trudpMapIterator *it;
    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)
                    trudpMapIteratorElementData(el, NULL);

            trudp_ChannelSendRESET(tcd, NULL, 0);
        }
        trudpMapIteratorDestroy(it);
    }
}

/**
 * Default TR-UDP process read data from UDP
 * 
 * @param td
 * @param data
 * @param data_length
 */
void trudpProcessReceive(trudpData *td, void *data, size_t data_length) {

    struct sockaddr_in remaddr; // remote address
    socklen_t addr_len = sizeof(remaddr);
    ssize_t recvlen = trudpUdpRecvfrom(td->fd, data, data_length,
            (__SOCKADDR_ARG)&remaddr, &addr_len);

    // Process received packet
    if(recvlen > 0) {
        size_t data_length;
        trudpChannelData *tcd = trudpGetChannelCreate(td, (__CONST_SOCKADDR_ARG) &remaddr, 0);
        if(tcd == (void *)-1 ||
           trudp_ChannelProcessReceivedPacket(tcd, data, recvlen, &data_length) == (void *)-1) {

            if(tcd == (void *)-1) printf("!!! can't PROCESS_RECEIVE_NO_TRUDP\n");
            else
            trudpEventSend(tcd, PROCESS_RECEIVE_NO_TRUDP, data, recvlen, NULL);
        }
    }
}

/**
 * Send the same data to all connected peers
 *
 * @param td Pointer to trudpData
 * @param data Pointer to data
 * @param data_length Data length
 *
 * @return Number of peers
 */
size_t trudpSendDataToAll(trudpData *td, void *data, size_t data_length) {

    int rv = 0;

    trudpMapIterator *it;
    trudpMapElementData *el;
    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)
                    trudpMapIteratorElementData(el, NULL);

            if(tcd->connected_f) {
                // drop packets if send queue > 100 \todo move it to Send Data
                // function or something else
                //if(trudpSendQueueSize(tcd->sendQueue) < 100) {
                    if(trudp_ChannelSendData(tcd, data, data_length) < 0) break;
                    rv++;
                //}
            }
        }
        trudpMapIteratorDestroy(it);
    }

    return rv;
}

/**
 * Keep connection at idle line
 *
 * @param td
 * @return
 */
size_t trudpProcessKeepConnection(trudpData *td) {

    int rv = -1;

    trudpMapIterator *it;
    trudpMapElementData *el;
    uint64_t ts = trudpGetTimestampFull();
    while(rv == -1 && (it = trudpMapIteratorNew(td->map))) {
        rv = 0;
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)
                trudpMapIteratorElementData(el, NULL);

            if(tcd->connected_f && ts - tcd->lastReceived > SEND_PING_AFTER) {
                if(trudp_ChannelCheckDisconnected(tcd, ts) == -1) {
                    
                    rv = -1;
                    break;
                }
                trudp_ChannelSendPING(tcd, "PING", 5);
                rv++;
            }
        }
        trudpMapIteratorDestroy(it);
    }

    return rv;
}

/**
 * Get maximum receive queue size of all channels
 *
 * @param td Pointer to trudpData
 * @return Maximum send queue size of all channels or zero if all queues is empty
 */
size_t trudpGetReceiveQueueMax(trudpData *td) {

    int rv = 0;

    trudpMapIterator *it;
    trudpMapElementData *el;
    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)
                    trudpMapIteratorElementData(el, NULL);
            int size = trudpReceiveQueueSize(tcd->receiveQueue);
            if(size > rv) rv = size;
        }
        trudpMapIteratorDestroy(it);
    }

    return rv;
}

/**
 * Get trudpChannelData by address, port and channel number
 *
 * @param td Pointer to trudpData
 * @param addr Pointer to address string
 * @param port Port number
 * @param channel Channel number
 *
 * @return Pointer to trudpChannelData or (void*)-1 if not found
 */
trudpChannelData *trudpGetChannelAddr(trudpData *td, char *addr, int port,
        int channel) {

    size_t data_length, key_length;
    char *key = trudpMakeKey(addr, port, channel, &key_length);
    trudpChannelData *tcd = (trudpChannelData *)trudpMapGet(td->map, key,
        key_length, &data_length);

    return tcd;
}

/**
 * Get trudpChannelData by socket address and channel number
 *
 * @param td Pointer to trudpData
 * @param addr Pointer to address structure
 * @param channel Channel number
 *
 * @return Pointer to trudpChannelData or (void*)-1 if not found
 */
trudpChannelData *trudpGetChannel(trudpData *td, __CONST_SOCKADDR_ARG addr,
        int channel) {

    int port;
    char *addr_str = trudpUdpGetAddr(addr, &port);

    return trudpGetChannelAddr(td, addr_str, port, channel);
}

/**
 * Get trudpChannelData by socket address and channel number, create channel if 
 * not exists
 *
 * @param td Pointer to trudpData
 * @param remaddr Pointer to sockaddr_in remote address
 * @param addr_length Remote address length
 * @param channel TR-UDP channel
 *
 * @return Pointer to trudpChannelData or (void*)-1 at error
 */
trudpChannelData *trudpGetChannelCreate(trudpData *td,
        __CONST_SOCKADDR_ARG addr, int channel) {

    int port;
    char *addr_str = trudpUdpGetAddr((__CONST_SOCKADDR_ARG)addr, &port);
    trudpChannelData *tcd = trudpGetChannelAddr(td, addr_str, port, channel);

    if(tcd == (void*)-1) {
        tcd = trudp_ChannelNew(td, addr_str, port, channel);
        if(tcd != (void*)-1)
            trudpEventSend(tcd, CONNECTED, NULL, 0, NULL);
    }

    if(tcd != (void*)-1) tcd->connected_f = 1;

    return tcd;
}

// Send queue functions =======================================================

/**
 * Get minimum timeout from all trudp cannel send queue
 *
 * @param td
 * @param current_time Timestamp, usually current time
 *
 * @return Minimum timeout or UINT32_MAX if send queue is empty
 */
uint32_t trudp_SendQueueGetTimeout(trudpData *td, uint64_t current_time) {

    trudpMapIterator *it;
    trudpMapElementData *el;
    uint32_t min_timeout_sq = UINT32_MAX;

    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)trudpMapIteratorElementData(el, NULL);
            uint32_t timeout_sq = trudp_ChannelSendQueueGetTimeout(tcd, current_time);
            if(timeout_sq < min_timeout_sq) min_timeout_sq = timeout_sq;
            if(!min_timeout_sq) break;
        }
        trudpMapIteratorDestroy(it);
    }

    return min_timeout_sq;
}

/**
 * Get sum of all send queues size
 * 
 * @param td
 * @return 
 */
size_t trudp_SendQueueSize(trudpData *td) {
    
    uint32_t sz = 0;
    trudpMapIterator *it;
    trudpMapElementData *el;

    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            trudpChannelData *tcd = (trudpChannelData *)trudpMapIteratorElementData(el, NULL);
            sz += trudpSendQueueSize(tcd->sendQueue);
        }
        trudpMapIteratorDestroy(it);
    }
    
    return sz;
}

/**
 * Check all peers send Queue elements and resend elements with expired time
 *
 * @param td Pointer to trudpData
 * @param next_et [out] Next expected time
 *
 * @return Number of resend packets
 */
int trudp_SendQueueProcess(trudpData *td, uint64_t *next_et) {

    int retval, rv = 0;
    uint64_t ts = trudpGetTimestampFull(), min_expected_time, next_expected_time;
    do {
        retval = 0;
        trudpMapIterator *it;
        trudpMapElementData *el;
        min_expected_time = UINT64_MAX;
        if((it = trudpMapIteratorNew(td->map))) {
            while((el = trudpMapIteratorNext(it))) {
                trudpChannelData *tcd = (trudpChannelData *)trudpMapIteratorElementData(el, NULL);
                retval = trudp_SendQueueProcessChannel(tcd, ts, &next_expected_time);
                if(retval < 0) break;
                if(retval > 0) rv += retval;
                if(next_expected_time && next_expected_time < min_expected_time)
                    min_expected_time = next_expected_time;
            }
            trudpMapIteratorDestroy(it);
        }
    } while(retval == -1 || (retval > 0 && min_expected_time <= ts));

    if(next_et) *next_et = (min_expected_time != UINT64_MAX) ? min_expected_time : 0;

    return rv;
}

///**
// * Get maximum send queue size of all channels
// *
// * @param td Pointer to trudpData
// * @return Maximum send queue size of all channels or zero if all queues is empty
// */
//size_t trudp_SendQueueGetSizeMax(trudpData *td) {
//
//    int rv = 0;
//
//    trudpMapIterator *it;
//    trudpMapElementData *el;
//    if((it = trudpMapIteratorNew(td->map))) {
//        while((el = trudpMapIteratorNext(it))) {
//            trudpChannelData *tcd = (trudpChannelData *)
//                    trudpMapIteratorElementData(el, NULL);
//            int size = trudpPacketQueueSize(tcd->sendQueue);
//            if(size > rv) rv = size;
//        }
//        trudpMapIteratorDestroy(it);
//    }
//
//    return rv;
//}

// Write queue functions ======================================================

/**
 * Check all peers write Queue elements and write one first packet
 *
 * @param td Pointer to trudpData
 *
 * @return Size of send packets
 */
size_t trudp_WriteQueueProcess(trudpData *td) {

    int i = 0;
    size_t retval = 0;
    trudpMapElementData *el;
    trudpMapIterator *it;
    if((it = trudpMapIteratorNew(td->map))) {
        while(!retval && (el = trudpMapIteratorNext(it))) {
            if(i++ < td->writeQueueIdx) continue;
            trudpChannelData *tcd = (trudpChannelData *)
                    trudpMapIteratorElementData(el, NULL);
            retval = trudp_ChannelWriteQueueProcess(tcd);
            td->writeQueueIdx++;
        }
        trudpMapIteratorDestroy(it);
        if(!retval) td->writeQueueIdx = 0;
    }

    return retval;
}

/**
 * Get number of elements in all Write queues
 * 
 * @param td
 * @return 
 */
size_t trudp_WriteQueueSizeAll(trudpData *td) {

    size_t retval = 0;
    trudpMapElementData *el;
    trudpMapIterator *it;
    if((it = trudpMapIteratorNew(td->map))) {
        while((el = trudpMapIteratorNext(it))) {
            size_t data_lenth;
            trudpChannelData *tcd = (trudpChannelData *)
                    trudpMapIteratorElementData(el, &data_lenth);
            retval += trudpWriteQueueSize(tcd->writeQueue);
        }
        trudpMapIteratorDestroy(it);
    }

    return retval;
}
