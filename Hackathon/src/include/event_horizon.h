#ifndef EVENT_HORIZON_H
#define EVENT_HORIZON_H

#include <stdint.h>
#include <sys/iomsg.h>

/* Channel and Connection names for QNX Name Service */
#define ENGINE_ATTACH_NAME "engine_service"

/* Message types */
typedef enum {
    MSG_TYPE_SENSOR_DATA = 0x100,
    MSG_TYPE_HEARTBEAT    = 0x200,
    MSG_TYPE_GET_STATUS   = 0x300
} msg_type_t;

/* Sensor Data Structure from ESP32 */
typedef struct {
    uint16_t msg_type;
    float energy_level;    /* 0.0 to 100.0 */
    float imu_x;
    float imu_y;
    float imu_z;
} sensor_msg_t;

/* Status Request Structure */
typedef struct {
    uint16_t msg_type;
} status_req_t;

/* Status Reply Structure */
typedef struct {
    float energy_level;
    float imu_x;
    float imu_y;
    float imu_z;
} status_reply_t;

/* Simple Reply Structure */
typedef struct {
    uint16_t status;
} reply_msg_t;

#endif /* EVENT_HORIZON_H */
