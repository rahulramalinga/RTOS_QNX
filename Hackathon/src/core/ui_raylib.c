#include "raylib.h"
#include "SDL.h" // Include for error reporting
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/dispatch.h>
#include "../include/event_horizon.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MAX_STARS 200

typedef struct {
    Vector3 position;
    float speed;
} Star;

int main(void)
{
    // Force QNX environment for SDL
    setenv("SDL_VIDEODRIVER", "qnx", 1);
    setenv("SDL_RENDER_DRIVER", "opengles2", 1);
    setenv("SCREEN_FORMAT", "RGBX8888", 1);

    printf("[UI] --- DIAGNOSTIC START ---\n");
    
    // Check if Screen is reachable
    FILE *f = fopen("/dev/screen", "r");
    if (f) {
        printf("[UI] /dev/screen is accessible.\n");
        fclose(f);
    } else {
        printf("[UI] ERROR: /dev/screen is NOT accessible! (Check if 'screen' is running)\n");
    }

    // Manually test SDL Video Init
    printf("[UI] Testing SDL_Init(SDL_INIT_VIDEO)...\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[UI] SDL_Init(VIDEO) FAILED: %s\n", SDL_GetError());
        fflush(stdout);
        return 1;
    }
    printf("[UI] SDL_Init(VIDEO) SUCCESS!\n");
    fflush(stdout);

    // 1. Connect to QNX Engine Service
    int coid;
    while ((coid = name_open(ENGINE_ATTACH_NAME, 0)) == -1) {
        printf("[UI] Waiting for engine_service...\n");
        fflush(stdout);
        sleep(1);
    }

    // 2. Initialize Raylib
    printf("[UI] Initializing Raylib Window...\n");
    fflush(stdout);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Event Horizon - Command Deck 2.0");
    
    if (!IsWindowReady()) {
        printf("[UI] CRITICAL: SDL Initialization Failed: %s\n", SDL_GetError());
        fflush(stdout);
        return 1;
    }

    printf("[UI] SDL Window Ready!\n");
    fflush(stdout);
    DisableCursor(); // Capture the mouse
    SetTargetFPS(60);

    // 3. Setup 3D Camera
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, -1.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // 4. Initialize Deep Space (Starfield)
    Star stars[MAX_STARS];
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].position = (Vector3){ (float)GetRandomValue(-100, 100), (float)GetRandomValue(-100, 100), (float)GetRandomValue(-200, 0) };
        stars[i].speed = (float)GetRandomValue(5, 20) / 10.0f;
    }

    status_req_t req = { MSG_TYPE_GET_STATUS };
    status_reply_t status = { 0 };

    printf("[UI] Event Horizon Cockpit Online.\n");

    while (!WindowShouldClose())
    {
        // 5. IPC: Fetch Reactor Data
        if (MsgSend(coid, &req, sizeof(req), &status, sizeof(status)) == -1) {
            // Re-attempt logic could go here
        }

        // 6. Logic: Update Stars and View
        for (int i = 0; i < MAX_STARS; i++) {
            stars[i].position.z += stars[i].speed;
            if (stars[i].position.z > 5) stars[i].position.z = -200;
        }

        // Use IMU data to tilt the cockpit
        float tilt_x = status.imu_x * 0.1f;
        float tilt_y = status.imu_y * 0.1f;

        // 7. Draw the Cockpit
        BeginDrawing();
            ClearBackground(BLACK);

            BeginMode3D(camera);
                // Draw Stars
                for (int i = 0; i < MAX_STARS; i++) {
                    DrawSphere(stars[i].position, 0.1f, WHITE);
                }
                
                // Draw some distant nebula-like spheres
                DrawSphere((Vector3){-20, 10, -50}, 5.0f, (Color){100, 0, 100, 100});
                DrawSphere((Vector3){30, -5, -80}, 8.0f, (Color){0, 100, 100, 100});
            EndMode3D();

            // --- 2D HUD OVERLAY (The Cockpit Glass) ---
            
            // Cockpit Frame (Left/Right bars)
            DrawRectangle(0, 0, 40, SCREEN_HEIGHT, DARKGRAY);
            DrawRectangle(SCREEN_WIDTH - 40, 0, 40, SCREEN_HEIGHT, DARKGRAY);
            
            // Dashboard (Bottom console)
            DrawRectangle(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, 100, (Color){30, 30, 30, 255});
            DrawRectangleLines(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, 100, GRAY);

            // Energy Level HUD
            DrawText("REACTOR CORE STABILITY", 60, SCREEN_HEIGHT - 80, 20, SKYBLUE);
            DrawRectangle(60, SCREEN_HEIGHT - 50, 400, 25, BLACK);
            DrawRectangle(60, SCREEN_HEIGHT - 50, (int)(status.energy_level * 4.0f), 25, SKYBLUE);
            DrawRectangleLines(60, SCREEN_HEIGHT - 50, 400, 25, RAYWHITE);

            // Crosshair (Follows Mouse relative to Center)
            Vector2 mouse = GetMousePosition();
            DrawCircleLines(mouse.x, mouse.y, 20, GREEN);
            DrawLine(mouse.x - 10, mouse.y, mouse.x + 10, mouse.y, GREEN);
            DrawLine(mouse.x, mouse.y - 10, mouse.x, mouse.y + 10, GREEN);

            // Radar Scan
            DrawCircleLines(SCREEN_WIDTH - 150, SCREEN_HEIGHT - 150, 80, GREEN);
            DrawLine(SCREEN_WIDTH - 150, SCREEN_HEIGHT - 150, SCREEN_WIDTH - 150 + (int)(sin(GetTime()*2)*80), SCREEN_HEIGHT - 150 + (int)(cos(GetTime()*2)*80), GREEN);
            
            // Telemetry
            DrawText(TextFormat("IMU TILT: [%.2f, %.2f]", status.imu_x, status.imu_y), 60, 20, 20, LIME);
            DrawFPS(SCREEN_WIDTH - 100, 20);

        EndDrawing();
    }

    CloseWindow();
    name_close(coid);
    return 0;
}
