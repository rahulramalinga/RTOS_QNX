#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

/* --- UDP Protocol (matches host_controller.py) --- */
typedef struct __attribute__((packed)) {
    float axes[6];       /* LSX, LSY, RSX, RSY, LT, RT */
    uint32_t buttons;    /* Bitmask */
} udp_input_t;

/* --- Local state --- */
typedef struct {
    int connected;
    char name[64];

    /* Analog Axes (-1.0 to 1.0) */
    float left_stick_x, left_stick_y;
    float right_stick_x, right_stick_y;
    float left_trigger, right_trigger;

    /* Digitals (bitmask) */
    uint32_t buttons;
    uint32_t buttons_prev;

    /* UDP Listener */
    int udp_sock;
} controller_state_t;

/* Button definitions (XInput style) */
#define CTRL_BTN_A        (1 << 0)
#define CTRL_BTN_B        (1 << 1)
#define CTRL_BTN_X        (1 << 2)
#define CTRL_BTN_Y        (1 << 3)
#define CTRL_BTN_LB       (1 << 4)
#define CTRL_BTN_RB       (1 << 5)
#define CTRL_BTN_BACK     (1 << 6)
#define CTRL_BTN_START    (1 << 7)
#define CTRL_BTN_L_THUMB  (1 << 8)
#define CTRL_BTN_R_THUMB  (1 << 9)

/* Lifecycle */
int  controller_init(controller_state_t *state);
void controller_poll(controller_state_t *state);
void controller_shutdown(controller_state_t *state);

/* Helper */
int  controller_pressed(controller_state_t *state, uint32_t button);
const char* controller_name(controller_state_t *state);

#endif /* CONTROLLER_H */
