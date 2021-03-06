/*
 * The MIT License
 *
 * Copyright 2016-2020 Kirill Scherba <kirill@scherba.ru>.
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
 */

// Channel functions ==========================================================

#include "trudp_channel.h"

#include <stdlib.h>
#include <string.h>

#include "teobase/types.h"

#include "teoccl/memory.h"
#include "teobase/logging.h"

#include "trudp_stat.h"
#include "trudp_utils.h"

#include "packet.h"
#include "trudp_receive_queue.h"
#include "trudp_send_queue.h"

// Local function
static trudpChannelData *_trudpChannelAddToMap(trudpData *td,
                                               trudpChannelData *tcd);
static uint64_t _trudpChannelCalculateExpectedTime(trudpChannelData *tcd,
                                                   uint64_t current_time,
                                                   int retransmit);
static void _trudpChannelCalculateTriptime(trudpChannelData *tcd, void *packet,
                                           size_t send_data_length);
static void _trudpChannelFree(trudpChannelData *tcd);
static uint32_t _trudpChannelGetId(trudpChannelData *tcd);
static uint32_t _trudpChannelGetNewId(trudpChannelData *tcd);
static void _trudpChannelIncrementStatSendQueueSize(trudpChannelData *tcd);
static void _trudpChannelIncrementStatWriteQueueSize(trudpChannelData *tcd);
static void _trudpChannelReset(trudpChannelData *tcd);
static void _trudpChannelSendACK(trudpChannelData *tcd, trudpPacket *packet);
static void _trudpChannelSendACKtoPING(trudpChannelData *tcd, trudpPacket* packet);
static void _trudpChannelSendACKtoRESET(trudpChannelData *tcd, trudpPacket* packet);
static size_t _trudpChannelSendPacket(trudpChannelData *tcd,
                                      trudpPacket *packetDATA,
                                      size_t packetLength,
                                      int save_to_send_queue);
static void _trudpChannelSetDefaults(trudpChannelData *tcd);
static void _trudpChannelSetLastReceived(trudpChannelData *tcd);

// importing debug option flag
extern bool trudpOpt_DBG_dumpDataPacketHeaders;
extern int64_t trudpOpt_CORE_disconnectTimeoutDelay_us;

void trudp_ChannelSendReset(trudpChannelData *tcd) {
  trudpChannelSendRESET(tcd, NULL, 0);
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
static trudpChannelData *_trudpChannelAddToMap(trudpData *td,
                                               trudpChannelData *tcd) {

  return (trudpChannelData*)teoMapAdd(td->map, (uint8_t*)tcd->channel_key,
    tcd->channel_key_length, (uint8_t*)tcd, sizeof(trudpChannelData));
}

/**
 * Get channel send queue timeout
 *
 * @param tcd Pointer to trudpChannelData
 * @param current_t Current time
 *
 * @return Send queue timeout (may by 0) or UINT32_MAX if send queue is empty
 */
uint32_t trudpChannelSendQueueGetTimeout(trudpChannelData *tcd,
                                         uint64_t current_t) {

  return trudpSendQueueGetTimeout(tcd->sendQueue, current_t);
}

/**
 * Set default trudpChannelData values
 *
 * @param tcd Pointer to trudpChannelData
 */
static void _trudpChannelSetDefaults(trudpChannelData *tcd) {

  tcd->sendId = 0;
  tcd->triptime = 0;
  tcd->triptimeFactor = 1.5;
  tcd->outrunning_cnt = 0;
  tcd->receiveExpectedId = 0;
  tcd->lastReceived = teoGetTimestampFull();
  tcd->lastSentPing = 0;  //  Never sent ping before
  tcd->triptimeMiddle = START_MIDDLE_TIME;
  tcd->read_buffer = NULL;
  tcd->read_buffer_ptr = 0;
  tcd->read_buffer_size = 0;
  tcd->last_packet_ptr = 0;

  // Initialize statistic
  trudpStatChannelInit(tcd);
}

/**
 * Free trudp channel
 *
 * @param tcd Pointer to trudpChannelData
 */
static void _trudpChannelFree(trudpChannelData *tcd) {

  tcd->td->stat.sendQueue.size_current -= trudpSendQueueSize(tcd->sendQueue);
  tcd->td->stat.writeQueue.size_current -= trudpWriteQueueSize(tcd->writeQueue);

  if (tcd->read_buffer != NULL) {
    free(tcd->read_buffer);
    tcd->read_buffer = NULL;
  }

  trudpSendQueueFree(tcd->sendQueue);
  trudpWriteQueueFree(tcd->writeQueue);
  trudpReceiveQueueFree(tcd->receiveQueue);
  _trudpChannelSetDefaults(tcd);
}

// ============================================================================

/**
 * Create trudp channel
 *
 * @param td Pointer to trudpData
 * @param remote_address
 * @param remote_port_i
 * @param channel
 * @return
 */
trudpChannelData *trudpChannelNew(struct trudpData *td, const char *remote_address,
                                  int remote_port_i, int channel) {

  trudpChannelData tcd;
  memset(&tcd, 0, sizeof(tcd));

  tcd.td = td;
  tcd.sendQueue = trudpSendQueueNew();
  tcd.writeQueue = trudpWriteQueueNew();
  tcd.receiveQueue = trudpReceiveQueueNew();
  trudpUdpMakeAddr(remote_address, remote_port_i, (__SOCKADDR_ARG)&tcd.remaddr, &tcd.addrlen);
  tcd.channel = channel;

  tcd.connected_f = 0;

  // Set other defaults
  _trudpChannelSetDefaults(&tcd);
  tcd.fd = 0;

  // Add channel to map
  size_t channel_key_length;
  const char *addr_ch = trudpUdpGetAddr((__CONST_SOCKADDR_ARG)&tcd.remaddr, tcd.addrlen, NULL);
  const char *channel_key = trudpMakeKey(addr_ch, remote_port_i, channel, &channel_key_length);
  free((char*)addr_ch);

  tcd.channel_key = ccl_malloc(channel_key_length);
  memcpy(tcd.channel_key, channel_key, channel_key_length);
  tcd.channel_key_length = channel_key_length;

  trudpChannelData *tcd_return = _trudpChannelAddToMap(td, &tcd);
  return tcd_return;
}

/**
 * Reset trudp channel
 *
 * @param tcd Pointer to trudpChannelData
 */
static void _trudpChannelReset(trudpChannelData *tcd) {
  _trudpChannelFree(tcd);
}

/**
 * Destroy trudp channel
 *
 * @param tcd Pointer to trudpChannelData
 */
void trudpChannelDestroy(trudpChannelData *tcd) {
  trudpChannelSendEvent(tcd, DISCONNECTED, NULL, 0, NULL);
  _trudpChannelFree(tcd);
  tcd->fd = 0;
  trudpSendQueueDestroy(tcd->sendQueue);
  trudpWriteQueueDestroy(tcd->writeQueue);
  trudpReceiveQueueDestroy(tcd->receiveQueue);

  char *channel_key = tcd->channel_key;
  teoMapDelete(tcd->td->map, (uint8_t*)channel_key, tcd->channel_key_length);
  free(channel_key);
}

// ============================================================================

/**
 * Return next packet id in range 1..(PACKET_ID_LIMIT-1)
 * intentionally avoiding value 0
 *
 * @param packetId value to increment
 */
static  uint32_t _trudpGetNextSeqId(uint32_t packetId) {
    // due to packetId == 0 threaten in special way as initialization one
    // we avoid packetId == 0 in sequential increments
    // i.e. after PACKET_ID_LIMIT-1 will go 1
    packetId = modAddU(packetId, 1, PACKET_ID_LIMIT);
    if (packetId != 0) {
        return packetId;
    }
    return modAddU(packetId, 1, PACKET_ID_LIMIT);
}

/**
 * Return sequential distance between fromId and toId
 * Returns zero if fromId == toId otherwise it returns signed distance (toId - fromId)
 * with respect wrapping around at (PACKET_ID_LIMIT-1) and considering all
 * distances between 0..PACKET_ID_LIMIT/2 as positive and all distances beyond as negative
 * It behaves similar to how signed integer overflow works, but for arbitrary PACKET_ID_LIMIT
 *
 * @param fromId first sequential packet id
 * @param toId second sequential packet id
 */
static int32_t _trudpGetSeqIdDistance(uint32_t fromId, uint32_t toId) {
    uint32_t diff = modSubU(toId, fromId, PACKET_ID_LIMIT);
    if (diff < (PACKET_ID_LIMIT/2)) {
        return diff;
    }
    return diff - PACKET_ID_LIMIT;
}

/**
 * Get new channel send Id
 *
 * @param tcd Pointer to trudpChannelData
 * @return New send Id
 */
static  uint32_t _trudpChannelGetNewId(trudpChannelData *tcd) {
    uint32_t retval = tcd->sendId;
    tcd->sendId = _trudpGetNextSeqId(tcd->sendId);
    return retval;
}

/**
 * Get current channel send Id
 *
 * @param tcd Pointer to trudpChannelData
 * @return Send Id
 */
static uint32_t _trudpChannelGetId(trudpChannelData *tcd) {

  return tcd->sendId;
}

/**
 * Make key from channel data
 *
 * @param tcd Pointer to trudpChannelData
 *
 * @return Static buffer with key ip:port:channel
 */
const char *trudpChannelMakeKey(trudpChannelData *tcd) {

  int port;
  size_t key_length;
  const char *addr = trudpUdpGetAddr((__CONST_SOCKADDR_ARG)&tcd->remaddr, tcd->addrlen, &port);
  return trudpMakeKey(addr, port, tcd->channel, &key_length);
}

/**
 * Check that channel is disconnected and send DISCONNECTED event
 *
 * @param tcd Pointer to trudpChannelData
 * @param ts Current timestamp
 * @return -1 if disconnected event was sent, 0 otherwise
 */
int trudpChannelCheckDisconnected(trudpChannelData *tcd, uint64_t ts) {

  // Disconnect channel at long last receive
  if (tcd->lastReceived &&
      ts - tcd->lastReceived > trudpOpt_CORE_disconnectTimeoutDelay_us) {

    //        log_info("TrUdp", "trudpChannelSendEvent DISCONNECTED in
    //        trudpChannelCheckDisconnected");

    // Send disconnect event
    uint32_t lastReceived = ts - tcd->lastReceived;
    trudpChannelSendEvent(tcd, DISCONNECTED, &lastReceived, sizeof(lastReceived),
                   NULL);
    return -1;
  }
  return 0;
}

// Process received packet ====================================================

/**
 * Calculate Triptime
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet
 * @param send_data_length
 */
static void _trudpChannelCalculateTriptime(trudpChannelData *tcd, void *packet,
                                           size_t send_data_length) {

  tcd->triptime = trudpGetTimestamp() - trudpPacketGetTimestamp(packet);

  // Calculate and set Middle Triptime value
  tcd->triptimeMiddle = tcd->triptimeMiddle == START_MIDDLE_TIME
                            ? tcd->triptime * tcd->triptimeFactor
                            : // Set first middle time
                            tcd->triptime > tcd->triptimeMiddle
                                ? tcd->triptime * tcd->triptimeFactor
                                : // Set middle time to max triptime
                                (tcd->triptimeMiddle * 19 + tcd->triptime) /
                                    20.0; // Calculate middle value

  // Correct triptimeMiddle
  if (tcd->triptimeMiddle < tcd->triptime * tcd->triptimeFactor)
    tcd->triptimeMiddle = tcd->triptime * tcd->triptimeFactor;
  if (tcd->triptimeMiddle > tcd->triptime * 10)
    tcd->triptimeMiddle = tcd->triptime * 10;
  if (tcd->triptimeMiddle > MAX_TRIPTIME_MIDDLE)
    tcd->triptimeMiddle = MAX_TRIPTIME_MIDDLE;

  // tcd->triptimeMiddle *= 5;

  // Statistic
  tcd->stat.ack_receive++;
  tcd->stat.triptime_last = tcd->triptime;
  tcd->stat.wait = tcd->triptimeMiddle / 1000.0;
  trudpStatProcessLast10Send(tcd, packet, send_data_length);
}

/**
 * Set last received field to current timestamp
 *
 * @param tcd Pointer to trudpChannelData
 */
static void _trudpChannelSetLastReceived(trudpChannelData *tcd) {
  tcd->lastReceived = teoGetTimestampFull();
}

/**
 * Create ACK packet and send it back to sender
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet Pointer to received packet
 */
static void _trudpChannelSendACK(trudpChannelData *tcd, trudpPacket* packet) {
  trudpPacket* ack_packet = trudpPacketACKcreateNew(packet);
  trudpChannelSendEvent(tcd, PROCESS_SEND, ack_packet, trudpPacketACKlength(), NULL);
  trudpPacketCreatedFree(ack_packet);
  _trudpChannelSetLastReceived(tcd);
}

/**
 * Create ACK to RESET packet and send it back to sender
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet Pointer to received packet
 */
static void _trudpChannelSendACKtoRESET(trudpChannelData *tcd, trudpPacket* packet) {
  trudpPacket* ack_packet = trudpPacketACKtoRESETcreateNew(packet);
  trudpChannelSendEvent(tcd, PROCESS_SEND, ack_packet, trudpPacketACKlength(), NULL);
  trudpPacketCreatedFree(ack_packet);
  _trudpChannelSetLastReceived(tcd);
}

/**
 * Create ACK to PING packet and send it back to sender
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet Pointer to received packet
 */
static void _trudpChannelSendACKtoPING(trudpChannelData *tcd, trudpPacket* packet) {
  trudpPacket* ack_packet = trudpPacketACKtoPINGcreateNew(packet);
  trudpChannelSendEvent(tcd, PROCESS_SEND, ack_packet,
                 trudpPacketGetPacketLength(packet), NULL);
  trudpPacketCreatedFree(ack_packet);
  _trudpChannelSetLastReceived(tcd);
}

/**
 * Create RESET packet and send it to sender
 *
 * @param tcd Pointer to trudpChannelData
 * @param data NULL
 * @param data_length 0
 */
void trudpChannelSendRESET(trudpChannelData *tcd, void *data,
                           size_t data_length) {

  if (tcd) {
    void *packetRESET = trudpPacketRESETcreateNew(_trudpChannelGetNewId(tcd),
                  tcd->channel);
    trudpChannelSendEvent(tcd, PROCESS_SEND, packetRESET, trudpPacketRESETlength(),
                  NULL);
    trudpPacketCreatedFree(packetRESET);
    trudpChannelSendEvent(tcd, SEND_RESET, data, data_length, NULL);
  }
}

/**
 * Calculate ACK Expected Time
 *
 * @param tcd Pointer to trudpChannelData
 * @param current_time_usec Current time (uSec)
 * @param retransmit This is retransmitted
 *
 * @return Current time plus
 */
static uint64_t _trudpChannelCalculateExpectedTime(trudpChannelData *tcd,
                                                   uint64_t current_time_usec,
                                                   int retransmit) {

  // int rtt = tcd->triptimeMiddle + RTT * (retransmit);
  // int rtt = tcd->triptimeMiddle + (RTT/10);// * (retransmit?0.5:0);
  uint32_t rtt = tcd->triptimeMiddle + RTT;
  if (rtt > MAX_RTT)
    rtt = MAX_RTT;
  uint64_t expected_time = current_time_usec + rtt;

  return expected_time;
}

/**
 * Increment statistics send queue size value
 *
 * @param tcd Pointer to trudpChannelData
 */
static void _trudpChannelIncrementStatSendQueueSize(trudpChannelData *tcd) {
  tcd->td->stat.sendQueue.size_current++;
}

static void _trudpChannelIncrementStatWriteQueueSize(trudpChannelData *tcd) {
  tcd->td->stat.writeQueue.size_current++;
}

static void _updateMainExpectedTimeAndChannel(trudpChannelData *tcd, uint64_t new_expected_time) {
    tcd->td->expected_max_time = new_expected_time;
    tcd->td->channel_key = tcd->channel_key;
}

/**
 * Send packet
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet Pointer to send data
 * @param packetLength Data length
 * @param save_to_send_queue Save to send queue if true
 *
 * @return Zero on error
 */
static size_t _trudpChannelSendPacket(trudpChannelData *tcd,
                                      trudpPacket *packet, size_t packetLength,
                                      int save_to_send_queue) {
    size_t size_sq = trudpSendQueueSize(tcd->sendQueue);

    int sendNowFlag = size_sq < NORMAL_S_SIZE;
    if(size_sq == 1) {
      trudpSendQueueData *data = trudpSendQueueGetFirst(tcd->sendQueue);
      if(trudpPacketGetId((trudpPacket *)data->packet) == 0) sendNowFlag = 0;
    }

    // Save packet to send queue
    if (save_to_send_queue) {
        if (sendNowFlag) {
            uint64_t expected_time = _trudpChannelCalculateExpectedTime(tcd, teoGetTimestampFull(), 0);
            if (tcd->td->expected_max_time > expected_time) {
                _updateMainExpectedTimeAndChannel(tcd, expected_time);
            }
            trudpSendQueueAdd(tcd->sendQueue, packet, packetLength, expected_time);
            _trudpChannelIncrementStatSendQueueSize(tcd);
        } else {
            void *packetCopy = ccl_malloc(packetLength);
            memcpy(packetCopy, packet, packetLength);
            trudpWriteQueueAdd(tcd->writeQueue, NULL, packetCopy, packetLength);
            _trudpChannelIncrementStatWriteQueueSize(tcd);
        }
    }

    // Send data (add to write queue)
    if (!save_to_send_queue || sendNowFlag) {
        // Send packet to trudp event loop
        trudpChannelSendEvent(tcd, PROCESS_SEND, packet, packetLength, NULL);
        tcd->stat.packets_send++; // Send packets statistic
    }
    //    else if(save_to_send_queue)
    //    trudp_start_send_queue_cb(tcd->td->psq_data, 0);

    return packetLength;
}

/**
 * Send PING packet and send it back to sender
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet Pointer to received packet
 */
size_t trudpChannelSendPING(trudpChannelData *tcd, void *data,
                            size_t data_length) {

  // Create DATA package
  size_t packetLength;
  trudpPacket *packet = trudpPacketPINGcreateNew(
      _trudpChannelGetId(tcd), tcd->channel, data, data_length, &packetLength);

  // Send data
  size_t rv = _trudpChannelSendPacket(tcd, packet, packetLength, 0);

  // Free created packet
  trudpPacketCreatedFree(packet);

  tcd->lastSentPing = teoGetTimestampFull();
  return rv;
}

/**
 * Send data
 *
 * @param tcd Pointer to trudpChannelData
 * @param data Pointer to send data
 * @param data_length Data length
 *
 * @return Zero on error
 */
size_t trudpChannelSendData(trudpChannelData *tcd, void *data,
                            size_t data_length) {

  size_t rv = 0;

  //    if( trudpSendQueueSize(tcd->sendQueue) <= 50 ||
  //         ( tcd->sendId % 100 != 100 - trudpSendQueueSize(tcd->sendQueue) ) )
  //         {

  // Create DATA package
  size_t packetLength;
  trudpPacket *packet =
      trudpPacketDATAcreateNew(_trudpChannelGetNewId(tcd), tcd->channel, data,
                               data_length, &packetLength);

  // Send data
  rv = _trudpChannelSendPacket(tcd, packet, packetLength, 1);

  // Free created packet
  trudpPacketCreatedFree(packet);

  //    }

  return rv;
}

/**
 * Process received packet
 *
 * @param tcd Pointer to trudpChannelData
 * @param packet Pointer to received packet
 * @param packet_length Packet length
 *
 * @return 1 when Trudp packet is processed, 0 if packet is not a Trudp packet,
 * -1 on invalid Trudp packet
 */
int trudpChannelProcessReceivedPacket(trudpChannelData *tcd, uint8_t *data,
                                        size_t packet_length) {
  trudpPacket* packet = trudpPacketCheck(data, packet_length);

  int result = 1;

  // Check and process TR-UDP packet
  if (packet != NULL) {
    // Check packet type
    int type = trudpPacketGetType(packet);
    switch (type) {

    // ACK to DATA packet received
    case TRU_ACK: {
      // Find packet in send queue by id
      size_t send_data_length = 0;
      trudpSendQueueData *sqd =
          trudpSendQueueFindById(tcd->sendQueue, trudpPacketGetId(packet));
      if (sqd) {
        trudpPacket* sq_packet = trudpPacketQueueDataGetPacket(sqd);

        // Process ACK data callback
        trudpChannelSendEvent(tcd, GOT_ACK, sq_packet, sqd->packet_length, NULL);

        // Remove packet from send queue
        send_data_length = trudpPacketGetDataLength(sq_packet);
        trudpSendQueueDelete(tcd->sendQueue, sqd);
        tcd->td->stat.sendQueue.size_current--;

        if (tcd->td->channel_key == tcd->channel_key) {
            trudpRecalculateExpectedSendTime(tcd->td);
        } else {
            if (tcd->td->expected_max_time == UINT64_MAX) {
                LTRACK_E("TrudpChannel", "expected_max_time so BIG, while we got ack from channel %s", tcd->channel_key);
            }
            uint64_t expected_time = trudpSendQueueGetExpectedTime(tcd->sendQueue);
            if (tcd->td->expected_max_time > expected_time) {
                _updateMainExpectedTimeAndChannel(tcd, expected_time);
            }
        }

        if (trudpWriteQueueSize(tcd->writeQueue) > 0) {
          trudpWriteQueueData *wqd_first =
              trudpWriteQueueGetFirst(tcd->writeQueue);
          _trudpChannelSendPacket(tcd, wqd_first->packet_ptr,
                                  wqd_first->packet_length, 1);
          free(wqd_first->packet_ptr);
          trudpWriteQueueDeleteFirst(tcd->writeQueue);
          tcd->td->stat.writeQueue.size_current--;
        }
      }

      // Calculate triptime
      _trudpChannelCalculateTriptime(tcd, packet, send_data_length);
      _trudpChannelSetLastReceived(tcd);
    } break;

    // ACK to RESET packet received
    case TRU_ACK | TRU_RESET: {
      // Send event
      trudpChannelSendEvent(tcd, GOT_ACK_RESET, NULL, 0, NULL);

      // Reset TR-UDP
      _trudpChannelReset(tcd);

      // Statistic
      tcd->stat.ack_receive++;
      _trudpChannelSetLastReceived(tcd);

    } break;

    // ACK to PING packet received
    case TRU_ACK | TRU_PING: /*TRU_ACK_PING:*/ {

      // Calculate Triptime
      _trudpChannelCalculateTriptime(tcd, packet, packet_length);
      _trudpChannelSetLastReceived(tcd);

      // Send event
      trudpChannelSendEvent(tcd, GOT_ACK_PING, trudpPacketGetData(packet),
                     trudpPacketGetDataLength(packet), NULL);

    } break;

    // PING packet received
    case TRU_PING: {

      // Send event
      trudpChannelSendEvent(tcd, GOT_PING, trudpPacketGetData(packet),
                     trudpPacketGetDataLength(packet), NULL);

      // Calculate Triptime
      _trudpChannelCalculateTriptime(tcd, packet, packet_length);

      // Create ACK packet and send it back to sender
      _trudpChannelSendACKtoPING(tcd, packet);

      // Statistic
      tcd->stat.packets_receive++;
      trudpStatProcessLast10Receive(tcd, packet);

      tcd->outrunning_cnt = 0; // Reset outrunning flag

    } break;

    // DATA packet received
    case TRU_DATA: {

      // Create ACK packet and send it back to sender
      _trudpChannelSendACK(tcd, packet);

      if (trudpOpt_DBG_dumpDataPacketHeaders) {
        char buffer[8192];
        trudpPacketHeaderDump(buffer, sizeof(buffer), packet);
        LTRACK_I("TrudpChannel", "%s", buffer);
      }

      // Reset when wait packet with id 0 and receive packet with
      // another id
      if (!tcd->receiveExpectedId && trudpPacketGetId(packet)) {

        // Send event
        uint32_t id = trudpPacketGetId(packet);
        trudpChannelSendEvent(tcd, SEND_RESET, NULL, 0, NULL);

        trudpChannelSendRESET(tcd, &id, sizeof(id));
        break;
      }

      // Check expected Id and return data
      if (trudpPacketGetId(packet) == tcd->receiveExpectedId) {

        // Send Got Data event
        trudpChannelSendEventGotData(tcd, packet);

        // Proceed to next expected id
        tcd->receiveExpectedId = _trudpGetNextSeqId(tcd->receiveExpectedId);
        // Check received queue for saved packet with expected id
        trudpReceiveQueueData *rqd;
        while ((rqd = trudpReceiveQueueFindById(tcd->receiveQueue,
                                                tcd->receiveExpectedId))) {
          trudpPacket* rq_packet = trudpPacketQueueDataGetPacket(rqd);

          // Send Got Data event
          trudpChannelSendEventGotData(tcd, rq_packet);

          // Delete element from received queue
          trudpReceiveQueueDelete(tcd->receiveQueue, rqd);
          // Proceed to next expected id
          tcd->receiveExpectedId = _trudpGetNextSeqId(tcd->receiveExpectedId);
        }

        // Statistic
        tcd->stat.packets_receive++;
        trudpStatProcessLast10Receive(tcd, packet);

        tcd->outrunning_cnt = 0; // Reset outrunning flag
        break;
      }

      // Save outrunning packet to receiveQueue
      if (_trudpGetSeqIdDistance(tcd->receiveExpectedId, trudpPacketGetId(packet)) > 0 &&
                !trudpReceiveQueueFindById(tcd->receiveQueue,
                                          trudpPacketGetId(packet))) {

        trudpReceiveQueueAdd(tcd->receiveQueue, packet, packet_length, 0);
        tcd->outrunning_cnt++; // Increment outrunning count

        // Statistic
        tcd->stat.packets_receive++;
        trudpStatProcessLast10Receive(tcd, packet);
        break;
      }

      // Reset channel if packet id = 0 and we are waiting for non-zero one
      // Don't reset in case we waiting for id=1, just skip it to mitigate case
      // the remote side did not receive our ACK for packet id=0 and sent it again
      if ((trudpPacketGetId(packet) == 0) && !tcd->zero_tolerance_f && (tcd->receiveExpectedId != 1)) {
        // Send Send Reset event
        trudpChannelSendRESET(tcd, NULL, 0);
        break;
      }

      // Skip already processed packet
      // Statistic
      tcd->stat.packets_receive_dropped++;
      trudpStatProcessLast10Receive(tcd, packet);
    } break;

    // RESET packet received
    case TRU_RESET: {

      //                log_info("TrUdp", "trudpChannelSendEvent GOT_RESET in
      //                trudpChannelProcessReceivedPacket");

      // Create ACK to RESET packet and send it back to sender
      _trudpChannelSendACKtoRESET(tcd, packet);

      // Reset TR-UDP
      _trudpChannelReset(tcd);

      // Send Got Reset event
      trudpChannelSendEvent(tcd, GOT_RESET, NULL, 0, NULL);

    } break;

    // An undefined type of packet (skip it)
    default: {
      // Return error code
        result = -1;

    } break;
    }
  }
  // Packet is not TR-UDP packet
  else {
    trudpChannelSendEvent(tcd, GOT_DATA_NO_TRUDP, data, packet_length, NULL);
      result = 0;
  }

  return result;
}

// Send queue functions ======================================================

/**
 * Check send Queue elements and resend elements with expired time
 *
 * @param tcd Pointer to trudpChannelData
 * @param ts Current timestamp
 * @param next_expected_time [out] Next expected time: return expected time
 *          of next queue record or zero if not found, it may be NULL than not
 *          returned
 *
 * @return Number of resend packets or -1 if the channel was disconnected
 */
int trudpChannelSendQueueProcess(trudpChannelData *tcd, uint64_t ts,
                                 uint64_t *next_expected_time) {

  int rv = 0;
  trudpSendQueueData *tqd = NULL;

  // Get first element from send queue and check it expected time
  if (trudpSendQueueSize(tcd->sendQueue) &&
      (tqd = trudpSendQueueGetFirst(tcd->sendQueue)) &&
      tqd->expected_time <= ts) {

    // Change records expected time
    tqd->expected_time =
        _trudpChannelCalculateExpectedTime(tcd, ts, tqd->retrieves);
    if (tcd->td->expected_max_time > tqd->expected_time) {
        _updateMainExpectedTimeAndChannel(tcd, tqd->expected_time);
    } else if (tcd->td->channel_key == tcd->channel_key) {
      trudpRecalculateExpectedSendTime(tcd->td);
    }
    // Move record to the end of Queue \todo or don't move record to the end of
    // queue because it should be send first
    // trudpPacketQueueMoveToEnd(tcd->sendQueue, tqd);
    tcd->stat.packets_attempt++; // Attempt statistic parameter increment
    if (!tqd->retrieves)
      tqd->retrieves_start = ts;

    tqd->retrieves++;
    rv++;

    trudpPacket* tq_packet = trudpPacketQueueDataGetPacket(tqd);

    // Resend data
    trudpPacketUpdateTimestamp(tq_packet);
    trudpChannelSendEvent(tcd, PROCESS_SEND, tq_packet, tqd->packet_length, NULL);
  }

  // Disconnect channel at long last receive
  if (trudpChannelCheckDisconnected(tcd, ts) == -1) {

    tqd = NULL;
    rv = -1;
  } else {

// \todo Reset this channel at long retransmit
#if RESET_AT_LONG_RETRANSMIT
    if (ts - tqd->retrieves_start > trudpOpt_CORE_disconnectTimeoutDelay_us) {
      trudpChannelSendRESET(tcd, NULL, 0);
    } else {
#endif
// Get next value \todo if don't move than not need to re-get it
// tqd = trudpPacketQueueGetFirst(tcd->sendQueue);
#if RESET_AT_LONG_RETRANSMIT
    }
#endif
  }

  // If record exists
  if (next_expected_time) {
    if (rv != -1)
      tqd = trudpSendQueueGetFirst(tcd->sendQueue);
    *next_expected_time = tqd ? tqd->expected_time : 0;
  }

  return rv;
}

// Write queue functions ======================================================

/**
 * Process write queue and send first ready packet
 *
 * @param tcd
 *
 * @return Number of send packet or 0 if write queue is empty
 */
size_t trudpChannelWriteQueueProcess(trudpChannelData *tcd) {

  size_t retval = 0;
  trudpWriteQueueData *wqd = trudpWriteQueueGetFirst(tcd->writeQueue);
  if (wqd) {
    void *packet = wqd->packet_ptr ? wqd->packet_ptr : wqd->packet;
    trudpChannelSendEvent(tcd, PROCESS_SEND, packet, wqd->packet_length, NULL);
    trudpWriteQueueDeleteFirst(tcd->writeQueue);
    retval = wqd->packet_length;
  }

  return retval;
}


// Forward declare trudpData.Need some code-reorganize
void trudpRecalculateExpectedSendTime(struct trudpData *td) {
    teoMapIterator it;
    teoMapElementData *el;
    uint64_t min_time = UINT64_MAX;
    char *min_channel_key = NULL;
    uint64_t current_time = teoGetTimestampFull();
    teoMapIteratorReset(&it, td->map);
    while((el = teoMapIteratorNext(&it))) {
        trudpChannelData *tcd = (trudpChannelData *)teoMapIteratorElementData(el, NULL);
        uint64_t expected_time = trudpSendQueueGetExpectedTime(tcd->sendQueue);

        if (expected_time <= current_time) {
            min_time = current_time;
            min_channel_key = tcd->channel_key;
            break;
        } else if (expected_time < min_time) {
            min_time = expected_time;
            min_channel_key = tcd->channel_key;
        }
    }
    td->expected_max_time = min_time;
    td->channel_key = min_channel_key;
}