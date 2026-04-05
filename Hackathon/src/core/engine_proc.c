#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/dispatch.h>
#include "../include/event_horizon.h"

int main(int argc, char *argv[]) {
    name_attach_t *attach;
    sensor_msg_t msg;
    status_reply_t status_reply;
    reply_msg_t reply;
    int rcvid;

    /* Global state: The Ship's 'Black Box' */
    float last_energy = 100.0;
    float last_imu[3] = {0.0, 0.0, 0.0};

    /* Register the name 'engine_service' for QNX IPC */
    if ((attach = name_attach(NULL, ENGINE_ATTACH_NAME, 0)) == NULL) {
        perror("name_attach failed");
        return EXIT_FAILURE;
    }

    printf("[ENGINE] Core Logic Service Started. Listening on '%s'...\n", ENGINE_ATTACH_NAME);

    while (1) {
        /* Receive message from Reactor OR UI */
        rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

        if (rcvid == -1) {
            perror("MsgReceive failed");
            continue;
        }

        if (rcvid == 0) continue; // System pulse

        /* Handle standard messages */
        if (msg.msg_type == MSG_TYPE_SENSOR_DATA) {
            // Reactor pushing new data
            last_energy = msg.energy_level;
            last_imu[0] = msg.imu_x;
            last_imu[1] = msg.imu_y;
            last_imu[2] = msg.imu_z;

            reply.status = 0;
            MsgReply(rcvid, 0, &reply, sizeof(reply));
        } else if (msg.msg_type == MSG_TYPE_GET_STATUS) {
            // UI pulling data for rendering
            status_reply.energy_level = last_energy;
            status_reply.imu_x = last_imu[0];
            status_reply.imu_y = last_imu[1];
            status_reply.imu_z = last_imu[2];
            MsgReply(rcvid, 0, &status_reply, sizeof(status_reply));
        } else {
            MsgError(rcvid, ENOSYS);
        }
    }

    name_detach(attach, 0);
    return EXIT_SUCCESS;
}
