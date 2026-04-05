#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/dispatch.h>
#include "../include/event_horizon.h"

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768

/* Function to draw a circle (for the radar) */
void draw_circle(SDL_Renderer *renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    int coid;
    status_reply_t status;
    sensor_msg_t msg;

    /* 1. Connect to the Engine Service (QNX IPC) */
    if ((coid = name_open(ENGINE_ATTACH_NAME, 0)) == -1) {
        perror("[UI] Failed to connect to engine_service. Is engine_proc running?");
        return EXIT_FAILURE;
    }

    /* 2. Initialize SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[UI] SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    /* 3. Create Window and Renderer (QNX Screen backend) */
    window = SDL_CreateWindow("Event Horizon - Command Deck", 
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("[UI] Window Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("[UI] Renderer Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    printf("[UI] Command Deck UI Started. Native SDL2 on QNX Screen.\n");

    int running = 1;
    SDL_Event e;
    msg.msg_type = MSG_TYPE_GET_STATUS;

    while (running) {
        /* Handle Input Events */
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }

        /* 4. Request Latest Status from Engine (IPC) */
        if (MsgSend(coid, &msg, sizeof(msg), &status, sizeof(status)) == -1) {
            perror("[UI] MsgSend failed");
            break;
        }

        /* 5. Rendering Pipeline */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Draw Radar (Green)
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        draw_circle(renderer, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 200);
        
        // Draw Crosshair (Red)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        int cx = (SCREEN_WIDTH/2) + (int)(status.imu_x * 50.0);
        int cy = (SCREEN_HEIGHT/2) + (int)(status.imu_y * 50.0);
        SDL_RenderDrawLine(renderer, cx - 20, cy, cx + 20, cy);
        SDL_RenderDrawLine(renderer, cx, cy - 20, cx, cy + 20);

        // Draw Energy Bar (Cyan)
        SDL_Rect energy_rect = { 50, 700, (int)(status.energy_level * 5.0), 30 };
        SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
        SDL_RenderFillRect(renderer, &energy_rect);
        
        // Outline for Energy Bar
        SDL_Rect outline_rect = { 50, 700, 500, 30 };
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &outline_rect);

        SDL_RenderPresent(renderer);
    }

    /* Cleanup */
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    name_close(coid);
    return EXIT_SUCCESS;
}
