#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dispatch.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include "../include/event_horizon.h"

#define SERIAL_PORT "/dev/ser1"

int setup_serial(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("[REACTOR] Error opening serial port");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

int main(int argc, char *argv[]) {
    int coid;
    sensor_msg_t msg;
    reply_msg_t reply;
    int serial_fd;
    char buffer[256];
    int bytes_read;

    /* Connect to 'engine_service' using name_open */
    printf("[REACTOR] Waiting for Engine Service '%s'...\n", ENGINE_ATTACH_NAME);
    
    while ((coid = name_open(ENGINE_ATTACH_NAME, 0)) == -1) {
        sleep(1);
    }
    
    printf("[REACTOR] Connected to Engine Service.\n");

    /* Setup Serial */
    serial_fd = setup_serial(SERIAL_PORT);
    if (serial_fd != -1) {
        printf("[REACTOR] Serial port %s opened successfully.\n", SERIAL_PORT);
    } else {
        printf("[REACTOR] Falling back to DUMMY data mode.\n");
    }

    srand(time(NULL));

    while (1) {
        msg.msg_type = MSG_TYPE_SENSOR_DATA;

        if (serial_fd != -1) {
            /* Basic line reading from serial */
            bytes_read = read(serial_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                /* Expecting CSV: energy,imu_x,imu_y,imu_z */
                if (sscanf(buffer, "%f,%f,%f,%f", &msg.energy_level, &msg.imu_x, &msg.imu_y, &msg.imu_z) != 4) {
                    /* If parse fails, just skip or use dummy? Let's use dummy if parse fails */
                }
            }
        } else {
            /* Dummy data mode */
            msg.energy_level = (float)(rand() % 100);
            msg.imu_x = (float)(rand() % 100) / 10.0;
            msg.imu_y = (float)(rand() % 100) / 10.0;
            msg.imu_z = (float)(rand() % 100) / 10.0;
        }

        printf("[REACTOR] Sending Data: Energy=%.2f%%, IMU(%.2f, %.2f, %.2f)\n",
               msg.energy_level, msg.imu_x, msg.imu_y, msg.imu_z);

        /* Send sensor data to the Engine process */
        if (MsgSend(coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("[REACTOR] MsgSend failed");
            break;
        }

        /* Throttle if in dummy mode, otherwise serial speed controls loop */
        if (serial_fd == -1) {
            usleep(500000);
        }
    }

    /* Clean up */
    if (serial_fd != -1) close(serial_fd);
    name_close(coid);
    return EXIT_SUCCESS;
}
