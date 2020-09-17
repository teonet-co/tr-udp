/*
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
 */

/*
 * File:   trudpcat.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on June 2, 2016, 1:11 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// C11 present
#if __STDC_VERSION__ >= 201112L
extern int usleep (__useconds_t __useconds);
#endif

#include "libtrudp/src/tr-udp.h"
#include "libtrudp/src/utils_r.h"
#include "libtrudp/src/tr-udp_stat.h"

// Integer options
static int o_debug = 0,
    o_statistic = 0,
    o_listen = 0,
    o_numeric = 0,
    o_buf_size = 4096;

// String options
static char *o_local_address = NULL,
     *o_local_port = NULL,
     *o_remote_address = NULL,
     *o_remote_port = NULL;

static int o_remote_port_i;

// Application exit code and flags
static int exit_code = EXIT_SUCCESS,
    connected_flag = 0,
    quit_flag = 0;

// Read buffer
static char *buffer;

/**
 * Show error and exit
 *
 * @param fmt
 * @param ...
 */
static void die(char *fmt, ...)
{
	va_list ap;
	fflush(stdout);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

/**
 * Show debug message
 *
 * @param fmt
 * @param ...
 */
static void debug(char *fmt, ...)
{
    static unsigned long idx = 0;
    va_list ap;
    if (o_debug) {
        fflush(stdout);
        fprintf(stderr, "%lu %.3f debug: ", ++idx, trudpGetTimestamp() / 1000.0);
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fflush(stderr);
    }
}

/**
 * Show statistic window
 * @param td Pointer to trudpData
 */
static void showStatistic(trudpData *td, int *show) {

    if(*show) {
//        gotoxy(0,0);
        cls();
//        //hidecursor();
        char *stat_str = ksnTRUDPstatShowStr(td);
        if(stat_str) {
            puts(stat_str);
            free(stat_str);
        }
    }
    // Check key !!!
    int ch = nb_getch();
    if(ch) {
        // ...
        printf("key %c pressed\n", ch);
        if(ch == 'S') { 
            *show = !*show;
            //if(!*show) showcursor();
        }
    }
}

/**
 * TR-UDP process data callback
 *
 * @param data
 * @param data_length
 * @param user_data
 */
static void processDataCb(void *td_ptr, void *data, size_t data_length,
        void *user_data) {

    trudpChannelData *tcd = (trudpChannelData *)td_ptr;


    debug("got %d byte data, id=%u: ", (int)data_length,
                trudpPacketGetId(trudpPacketGetPacket(data)));

    if(!o_statistic) {
        if(!o_debug)
            printf("#%u at %.3f [%.3f(%.3f) ms] ",
                   tcd->receiveExpectedId,
                   (double)trudpGetTimestamp() / 1000.0,
                   (double)tcd->triptime / 1000.0,
                   (double)tcd->triptimeMiddle / 1000.0);

        printf("%s\n",(char*)data);
    }
    else {
        // Show statistic window
        //showStatistic(tcd->td);
    }
    debug("\n");
}

/**
 * TR-UDP ACK processed callback
 *
 * @param td
 * @param data
 * @param data_length
 * @param user_data
 */
static void processAckCb(void *td_ptr, void *data, size_t data_length,
        void *user_data) {

    trudpChannelData *tcd = (trudpChannelData *)td_ptr;

    debug("got ACK id=%u processed %.3f(%.3f) ms\n",
           trudpPacketGetId(trudpPacketGetPacket(data)),
           (tcd->triptime)/1000.0, (tcd->triptimeMiddle)/1000.0  );
}

/**
 * TR-UDP send callback
 *
 * @param packet
 * @param packet_length
 * @param user_data
 */
static void sendPacketCb(void *tcd_ptr, void *packet, size_t packet_length,
        void *user_data) {

    trudpChannelData *tcd = (trudpChannelData *)tcd_ptr;

    //if(isWritable(tcd->td->fd, timeout) > 0) {   
    // Send to UDP
    trudpUdpSendto(tcd->td->fd, packet, packet_length,
            (__CONST_SOCKADDR_ARG) &tcd->remaddr, sizeof(tcd->remaddr));
    //}

    int port,type;
    uint32_t id = trudpPacketGetId(packet);
    const char *addr = trudpUdpGetAddr((__CONST_SOCKADDR_ARG)&tcd->remaddr, &port);
    if(!(type = trudpPacketGetType(packet))) {
        debug("send %d bytes, id=%u, to %s:%d, %.3f(%.3f) ms\n",
            (int)packet_length, id, addr, port,
            tcd->triptime / 1000.0, tcd->triptimeMiddle / 1000.0);
    }
    else {
        debug("send %d bytes %s id=%u, to %s:%d\n",
            (int)packet_length, type == 1 ? "ACK":"RESET", id, addr, port);
    }
}

/**
 * TR-UDP event callback
 *
 * @param tcd_ptr
 * @param event
 * @param data
 * @param data_size
 * @param user_data
 */
static void eventCb(void *tcd_ptr, int event, void *data, size_t data_size,
        void *user_data) {

    switch(event) {

        case DISCONNECTED: printf("Disconnected\n"); connected_flag = 0; break;
        default: break;
    }
}

/**
 * Connect to peer
 *
 * @param td
 * @return
 */
static trudpChannelData *connectToPeer(trudpData *td) {

    trudpChannelData *tcd = NULL;

    // Create remote address and Send "connect" packet
    if(!o_listen) {
        char *connect = "Connect with TR-UDP!";
        size_t connect_length = strlen(connect) + 1;
        tcd = trudpNewChannel(td, o_remote_address, o_remote_port_i, 0);
        trudpSendData(tcd, connect, connect_length);
        fprintf(stderr, "Connecting to %s:%u:%u\n", o_remote_address, o_remote_port_i, 0);
        connected_flag = 1;
    }

    return tcd;
}

/**
 * The TR-UDP cat network loop
 *
 * @param td Pointer to trudpData
 */
static void network_loop(trudpData *td) {

    // Read from UDP
    struct sockaddr_in remaddr; // remote address
    socklen_t addr_len = sizeof(remaddr);
    ssize_t recvlen = trudpUdpRecvfrom(td->fd, buffer, o_buf_size,
            (__SOCKADDR_ARG)&remaddr, &addr_len);

    // Process received packet
    if(recvlen > 0) {
        size_t data_length;
        trudpChannelData *tcd = trudpCheckRemoteAddr(td, &remaddr, addr_len, 0);
        trudpProcessChannelReceivedPacket(tcd, buffer, recvlen, &data_length);
    }

    // Process send queue
    trudpProcessSendQueue(td, 0);

    // Process write queue
    while(trudpProcessWriteQueue(td));
}

/**
 * The TR-UDP cat network loop with select function
 *
 * @param td Pointer to trudpData
 * @param delay Default read data timeout
 */
static void network_select_loop(trudpData *td, int timeout) {

    int rv = 1;
    fd_set rfds, wfds;
    struct timeval tv;

//    while(rv > 0) {
    // Watch server_socket to see when it has input.
    FD_ZERO(&wfds);
    FD_ZERO(&rfds);
    FD_SET(td->fd, &rfds);

    // Process write queue
    if(trudpWriteQueueSizeAll(td)) {
        FD_SET(td->fd, &wfds);
    }

    uint32_t timeout_sq = trudpGetSendQueueTimeout(td);
//    debug("set timeout: %.3f ms; default: %.3f ms, send_queue: %.3f ms%s\n",
//            (timeout_sq < timeout ? timeout_sq : timeout) / 1000.0,
//            timeout / 1000.0,
//            timeout_sq / 1000.0,
//            timeout_sq == UINT32_MAX ? "(queue is empty)" : ""
//    );

    // Wait up to ~50 ms. */
    uint32_t t = timeout_sq < timeout ? timeout_sq : timeout;
    usecToTv(&tv, t);

    rv = select((int)td->fd + 1, &rfds, &wfds, NULL, &tv);

    // Error
    if (rv == -1) {
        fprintf(stderr, "select() handle error\n");
        return;
    }

    // Timeout
    else if(!rv) { // Idle or Timeout event

        // Process send queue
        if(timeout_sq != UINT32_MAX) {
            int rv = trudpProcessSendQueue(td, 0);
            debug("process send queue ... %d\n", rv);
        }
    }

    // There is a data in fd
    else {

        // Process read fd
        if(FD_ISSET(td->fd, &rfds)) {

            struct sockaddr_in remaddr; // remote address
            socklen_t addr_len = sizeof(remaddr);
            ssize_t recvlen = trudpUdpRecvfrom(td->fd, buffer, o_buf_size,
                    (__SOCKADDR_ARG)&remaddr, &addr_len);

            // Process received packet
            if(recvlen > 0) {
                size_t data_length;
                trudpChannelData *tcd = trudpCheckRemoteAddr(td, &remaddr, addr_len, 0);
                trudpProcessChannelReceivedPacket(tcd, buffer, recvlen, &data_length);
            }
        }

        // Process write fd
        if(FD_ISSET(td->fd, &wfds)) {
            // Process write queue
            while(trudpProcessWriteQueue(td));
            //trudpProcessWriteQueue(td);
        }
    }
//    }
}

/**
 * Show usage screen
 *
 * @param name Start command to show in usage screen
 */
static void usage(char *name) {

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "    %s [options] <destination-IP> <destination-port>\n", name);
	fprintf(stderr, "    %s [options] -l -p <listening-port>\n", name);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -h          Help\n");
	fprintf(stderr, "    -d          Debug mode; use multiple times to increase verbosity.\n");
	fprintf(stderr, "    -l          Listen mode\n");
	fprintf(stderr, "    -p <port>   Local port\n");
	fprintf(stderr, "    -s <IP>     Source IP\n");
	fprintf(stderr, "    -B <size>   Buffer size\n");
        fprintf(stderr, "    -S          Show statistic\n");
//	fprintf(stderr, "    -n          Don't resolve hostnames\n");
	fprintf(stderr, "\n");
	exit(1);
}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main_select(int argc, char** argv) {

    #define APP_VERSION "0.0.14"

    // Show logo
    fprintf(stderr,
            "TR-UDP two node connect sample application ver " APP_VERSION "\n"
    );

    int i/*, connected_f = 0*/;
    o_local_port = "8000"; // Default local port
    o_local_address = "0.0.0.0"; // Default local address

    // Read parameters
    while(1) {
            int c = getopt (argc, argv, "hdlp:B:s:nS");
            if (c == -1) break;
            switch(c) {
                case 'h': usage(argv[0]);               break;
                case 'd': o_debug++;			break;
                case 'l': o_listen++;			break;
                case 'p': o_local_port = optarg;	break;
                case 'B': o_buf_size = atoi(optarg);	break;
                case 's': o_local_address = optarg;	break;
                //case 'n': o_numeric++;		  break;
                //case 'w': break;	// timeout for connects and final net reads
                case 'S': o_statistic++;                break;
                default:  die("Unhandled argument: %c\n", c); break;
            }
    }

    // Read arguments
    for(i = optind; i < argc; i++) {
        switch(i - optind) {
            case 0:	o_remote_address = argv[i]; 	break;
            case 1:	o_remote_port = argv[i];	break;
        }
    }

    // Check necessary arguments
    if(o_listen && (o_remote_port || o_remote_address)) usage(argv[0]);
    if(!o_listen && (!o_remote_port || !o_remote_address)) usage(argv[0]);

    // Show execution mode
    if(o_listen)
        fprintf(stderr, "Server started at %s:%s\n", o_local_address,
                o_local_port);
    else {
        o_remote_port_i = atoi(o_remote_port);
        fprintf(stderr, "Client start connection to %s:%d\n", o_remote_address,
                o_remote_port_i);
    }

    // Create read buffer
    buffer = malloc(o_buf_size);

    // Startup windows socket library
    #if defined(HAVE_MINGW) || defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    WSAStartup(0x0202, &wsaData);
    #endif

    // Bind UDP port and get FD (start listening at port)
    int port = atoi(o_local_port);
    int fd;
    if(o.listen) {
        fd = trudpUdpBindRaw(NULL, &port, 1);
    } else {
        fd = trudpUdpBindRaw(o.remote_address, &port, 1);
    }
    if(fd <= 0) die("Can't bind UDP port ...\n");
    else fprintf(stderr, "Start listening at port %d\n", port);

    // Initialize TR-UDP
    trudpData *td = trudpInit(fd, port, 0, NULL);

    // Set callback functions
//    trudpSetCallback(td, PROCESS_DATA, (trudpCb)processDataCb);
    trudpSetCallback(td, SEND, (trudpCb)sendPacketCb);
//    trudpSetCallback(td, PROCESS_ACK, (trudpCb)processAckCb);
    trudpSetCallback(td, EVENT, (trudpCb)eventCb);

    // Create messages
    char *hello_c = "Hello TR-UDP from client!";
    size_t hello_c_length = strlen(hello_c) + 1;
    //
    char hello_s[512]; 
    strcpy(hello_s, "Hello TR-UDP from server!");
    size_t hello_s_length = sizeof(hello_s); // strlen(hello_s) + 1;

    // Process networking
    i = 0;
    char *message;
    size_t message_length;
    const int DELAY = 500000; // uSec
    const int SEND_MESSAGE_AFTER_MIN = 500000; // uSec (mSec * 1000)
    int send_message_after = SEND_MESSAGE_AFTER_MIN;
    const int RECONNECT_AFTER = 6000000; // uSec (mSec * 1000)
    const int SHOW_STATISTIC_AFTER = 250000; // uSec (mSec * 1000)
    if(!o_listen) { message = hello_c; message_length = hello_c_length; }
    else { message = hello_s; message_length = hello_s_length; }
    uint32_t tt, tt_s = 0, tt_c = 0, tt_ss = 0;
    while (!quit_flag) {

        #define USE_SELECT 1

        #if !USE_SELECT
        network_loop(td);
        #else
        network_select_loop(td, send_message_after < DELAY ? send_message_after : DELAY);
        #endif

        // Current timestamp
        tt = trudpGetTimestamp();

        // Connect
        if(!o_listen && !connected_flag && (tt - tt_c) > RECONNECT_AFTER) {
            connectToPeer(td);
            tt_c = tt;
        }

        // Send message
        // random int between 0 and 500000 
        
        if((tt - tt_s) > send_message_after) {

            if(td->stat.sendQueue.size_current < 1000)
            trudpSendDataToAll(td, message, message_length);
            //send_message_after = (rand() % (500000 - 1)) + SEND_MESSAGE_AFTER_MIN;
            tt_s = tt;
        }

        // Show statistic
        if(/*o_statistic && */(tt - tt_ss) > SHOW_STATISTIC_AFTER) {

            showStatistic(td, &o_statistic);
            tt_ss = tt;
        }

        #if !USE_SELECT
        usleep(DELAY);
        #endif
        i++;
    }

    // Destroy TR-UDP
    trudpDestroy(td);
    free(buffer);

    printf("Executed!\n");

    // Cleanup socket library
    #if defined(HAVE_MINGW) || defined(_WIN32) || defined(_WIN64)
    WSACleanup();
    #endif

    return (EXIT_SUCCESS);
}
