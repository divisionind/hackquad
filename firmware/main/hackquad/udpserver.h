/*
 * HackQuad - an open-source firmware+hardware quadcopter
 * Copyright (C) 2020, Andrew Howard, <divisionind.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef HACKQUAD_UDPSERVER_H
#define HACKQUAD_UDPSERVER_H

#include <stdbool.h>
#include <stddef.h>

#include "hackquad/lint_defs.h"
#include "lwip/sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UDPSERVER_RECV_BUFFER_SIZE 128
#define UDPSERVER_PORT             25565
#define UDPSERVER_LOG_RECVB        0

typedef void (*udp_recv_handler_t)(int id, u8 *data, size_t len);

struct udp_context {
    /* SET BY CALLER */
    udp_recv_handler_t recv_handler;

    /* PRIVATE */
    int sock;
    struct sockaddr from;
    socklen_t fromlen;
    u8 recv_buffer[UDPSERVER_RECV_BUFFER_SIZE];

    /* current sending nonce */
    u32 send_nonce:24;

    /* highest nonce recv-ed */
    u32 latest_recv_nonce:24;
};

struct __attribute__((packed)) udp_packet_header {
    u8 id;
    u32 nonce:24;
};

/**
 * Creates a UDP server.
 *
 * @param ctx    - udp server instance handlers and persistent data
 * @param ipaddr - address to bind to (e.g. IPADDR_ANY/0.0.0.0)
 * @param port
 * @return ESP_OK/FAIL
 */
int udp_create(struct udp_context *ctx, u32 ipaddr /* IPADDR_ANY */, u16 port);

/**
 * Receives packets from the udp socket. Blocking until a full packet is recieved and processed
 * by calling its handler.
 *
 * @param ctx
 * @return ESP_OK/FAIL
 */
int udp_yield(struct udp_context *ctx);

/**
 * Sends data to the last person data was received from.
 *
 * @param ctx
 * @param data - buffer to send
 * @param len  - amount of data from buffer to send
 * @return amount of data sent
 */
ssize_t udp_sendp(struct udp_context *ctx, u8 *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_UDPSERVER_H */
