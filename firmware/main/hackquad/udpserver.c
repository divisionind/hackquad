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

#include <string.h>

#include "hackquad/udpserver.h"
#include "hackquad/hackquad_msg.h"
#include "esp_log.h"
#include "freertos/task.h"

#if UDPSERVER_LOG_RECVB
static void print_data(u8 *data, size_t len) {
    printf("received (%i) = ", len);

    for (size_t i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
    }

    printf("\n");
}
#endif

int udp_yield(struct udp_context *ctx) {
    const struct udp_packet_header *head;
    ssize_t read = recvfrom(ctx->sock, ctx->recv_buffer, UDPSERVER_RECV_BUFFER_SIZE, 0, &ctx->from,
                            &ctx->fromlen);

    if (read < (ssize_t) sizeof(struct udp_packet_header)) {
        ESP_LOGE(TAG, "error in recvfrom(), errno = %i", errno);
        return ESP_FAIL;
    }

#if UDPSERVER_LOG_RECVB
    print_data(ctx->recv_buffer, read);
#endif

    // no fragmentation supported, must arrive as entire/confined packet
    head = (struct udp_packet_header *) ctx->recv_buffer;

    // ensure data order, discard old packets
    if (head->nonce == 0) {
        // prime nonce, takes priority
        ctx->latest_recv_nonce = 0;
    } else if (head->nonce < ctx->latest_recv_nonce) {
        return ESP_FAIL;
    }

    // call recv handler
    ctx->recv_handler(head->id, ctx->recv_buffer + sizeof(struct udp_packet_header),
                      read - sizeof(struct udp_packet_header));
    return ESP_OK;
}

int udp_create(struct udp_context *ctx, u32 ipaddr /* IPADDR_ANY */, u16 port) {
    // init udp server
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ipaddr);
    addr.sin_port = htons(port);

    ctx->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (ctx->sock < 0) {
        ESP_LOGE(TAG, "failed to create socket, errno: %i", errno);
        return ESP_FAIL;
    }

    if (bind(ctx->sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "failed to bind socket, errno: %i", errno);
        return ESP_FAIL;
    }

    ctx->fromlen = sizeof(ctx->from);

    ESP_LOGI(TAG, "created udp socket on ::%i", port);
    return ESP_OK;
}

ssize_t udp_sendp(struct udp_context *ctx, u8 *data, size_t len) {
    if (*ctx->from.sa_data)
        return sendto(ctx->sock, data, len, 0, &ctx->from, ctx->fromlen);
    else
        return 0;
}
