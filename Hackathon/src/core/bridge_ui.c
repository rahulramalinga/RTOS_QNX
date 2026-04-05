#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/dispatch.h>
#include "../include/ui_engine.h"
#include "../include/event_horizon.h"

// Game Config
#define MAX_ASTEROIDS 20
#define MAX_LASERS 10

#define MAX_STARS 50

typedef struct {
    float x, y, z;
    int size;
    uint32_t color;
    int id;
} star_t;

typedef struct {
    int id;
    int active;
    float x, y, z;
    float speed;
} game_obj_t;

int main() {
    ui_context_t ui;
    int width = 1024, height = 768;
    int coid;
    srand(time(NULL));

    /* Connect to engine_service */
    while ((coid = name_open(ENGINE_ATTACH_NAME, 0)) == -1) {
        printf("[UI] Waiting for Engine Service...\n");
        sleep(1);
    }

    if (ui_init(&ui, width, height) != 0) {
        printf("Failed to initialize UI\n");
        return 1;
    }

    /* HUD elements */
    ui_add_entity(&ui, 0, height - 120, width, 120, 0xFF222222);
    ui_add_entity(&ui, 300, height - 50, 400, 15, 0xFF111111);
    int energy_fg = ui_add_entity(&ui, 300, height - 50, 400, 15, 0xFF00AAFF);

    /* Starfield initialization (Parallax Skybox) */
    star_t stars[MAX_STARS];
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = (rand() % width);
        stars[i].y = (rand() % (height - 120));
        stars[i].z = (float)(rand() % 100) / 100.0f;
        stars[i].size = (int)(stars[i].z * 3) + 1;
        stars[i].color = 0xFFFFFFFF;
        stars[i].id = ui_add_entity(&ui, (int)stars[i].x, (int)stars[i].y, stars[i].size, stars[i].size, stars[i].color);
    }

    game_obj_t asteroids[MAX_ASTEROIDS] = {0};
    int crosshair_id = ui_add_entity(&ui, width/2, height/2, 12, 12, 0xFF00FF00);

    status_req_t req = {MSG_TYPE_GET_STATUS};
    status_reply_t status;
    int loop_counter = 0;

    printf("Starting Ultra-Fast game loop...\n");

    while (1) {
        ui_poll_events(&ui);
        
        /* 1. Reduce IPC overhead - poll engine every 4th frame */
        if (loop_counter % 4 == 0) {
            MsgSend(coid, &req, sizeof(req), &status, sizeof(status));
        }

        /* 2. Direct Draw Stars (Fastest possible background) */
        void *ptr = ui.buffer_ptr[ui.buffer_index];
        if (ptr) {
            for (int i = 0; i < MAX_STARS; i++) {
                stars[i].y += stars[i].z * 4.0f;
                if (stars[i].y > height - 120) stars[i].y = 0;
                
                // Direct pixel write: color is 0xFFFFFFFF
                uint32_t *pixel = (uint32_t*)((uint8_t*)ptr + ((int)stars[i].y * ui.stride[ui.buffer_index]) + ((int)stars[i].x * 4));
                *pixel = 0xFFFFFFFF;
            }
        }

        /* 3. 3D Asteroid Scaling */
        if (rand() % 20 == 0) {
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (!asteroids[i].active) {
                    asteroids[i].active = 1;
                    asteroids[i].z = 0.05f; 
                    asteroids[i].x = (float)(width / 2 + (rand() % 100 - 50));
                    asteroids[i].y = (float)(height / 2 - 100 + (rand() % 100 - 50));
                    asteroids[i].speed = 0.015f;
                    asteroids[i].id = ui_add_entity(&ui, (int)asteroids[i].x, (int)asteroids[i].y, 2, 2, 0xFFAAAAAA);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (asteroids[i].active) {
                asteroids[i].z += asteroids[i].speed;
                int size = (int)(asteroids[i].z * 80);
                
                float dx = (asteroids[i].x - (width/2));
                float dy = (asteroids[i].y - (height/2));
                float cur_x = asteroids[i].x + (dx * asteroids[i].z * 5);
                float cur_y = asteroids[i].y + (dy * asteroids[i].z * 5);

                if (asteroids[i].z > 1.2f || cur_y > height - 120 || cur_x < 0 || cur_x > width) {
                    asteroids[i].active = 0;
                    ui_remove_entity(&ui, asteroids[i].id);
                } else {
                    ui.entities[asteroids[i].id].width = size;
                    ui.entities[asteroids[i].id].height = size;
                    ui_update_entity(&ui, asteroids[i].id, (int)cur_x - size/2, (int)cur_y - size/2);
                }
            }
        }

        /* 3. Crosshair & HUD */
        int tx = ui.mouse_pos[0] - 6 + (int)(status.imu_x * 12);
        int ty = ui.mouse_pos[1] - 6 + (int)(status.imu_y * 12);
        ui_update_entity(&ui, crosshair_id, tx, ty);
        ui.entities[energy_fg].width = (int)((400.0 * status.energy_level) / 100.0);

        ui_render_frame(&ui);
        usleep(16000); 
    }

    ui_shutdown(&ui);
    name_close(coid);
    return 0;
}
