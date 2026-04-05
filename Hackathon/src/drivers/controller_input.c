#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/controller.h"

#define UDP_PORT 5000

int controller_init(controller_state_t *state) {
    memset(state, 0, sizeof(controller_state_t));
    
    /* Open UDP Socket */
    state->udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (state->udp_sock < 0) {
        perror("socket");
        return -1;
    }

    /* Set socket to non-blocking */
    int flags = fcntl(state->udp_sock, F_GETFL, 0);
    if (flags == -1 || fcntl(state->udp_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl non-block");
        close(state->udp_sock);
        return -1;
    }

    /* Bind to all local addresses */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(state->udp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(state->udp_sock);
        return -1;
    }

    strncpy(state->name, "UDP Remote Controller", sizeof(state->name)-1);
    printf("[CONTROLLER] Listening for UDP packets on port %d...\n", UDP_PORT);
    return 0;
}

void controller_poll(controller_state_t *state) {
    if (state->udp_sock < 0) return;

    udp_input_t packet;
    struct sockaddr_in rx_addr;
    socklen_t rx_len = sizeof(rx_addr);
    int received = 0;

    /* Drain the UDP buffer - only keep the LATEST packet per frame */
    while (recvfrom(state->udp_sock, &packet, sizeof(packet), 0, 
                    (struct sockaddr *)&rx_addr, &rx_len) == sizeof(packet)) {
        received = 1;
        state->connected = 1;

        state->left_stick_x  = packet.axes[0];
        state->left_stick_y  = -packet.axes[1]; /* Invert Y for cockpit aim */
        state->right_stick_x = packet.axes[2];
        state->right_stick_y = packet.axes[3];
        state->left_trigger  = packet.axes[4];
        state->right_trigger = packet.axes[5];

        state->buttons_prev = state->buttons;
        state->buttons      = packet.buttons;
    }

    if (!received) {
        /* No new data this frame, just update edges */
        state->buttons_prev = state->buttons;
    }
}

void controller_shutdown(controller_state_t *state) {
    if (state->udp_sock >= 0) {
        close(state->udp_sock);
        state->udp_sock = -1;
    }
}

int controller_pressed(controller_state_t *state, uint32_t button) {
    return (state->buttons & button) && !(state->buttons_prev & button);
}

const char* controller_name(controller_state_t *state) {
    return state->name;
}
