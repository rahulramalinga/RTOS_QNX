#ifndef UI_ENGINE_H
#define UI_ENGINE_H

#include <screen/screen.h>
#include <stdint.h>

/*=============================================================================
 * Event Horizon - QNX Screen Software Rendering Engine
 *
 * High-performance CPU-based 2D renderer using QNX Screen API.
 * Double-buffered, direct framebuffer access for maximum throughput.
 * Targets 30-60 FPS on QNX VMs without GPU acceleration.
 *============================================================================*/

/* --- Configuration --- */
#define MAX_ENTITIES     200
#define FONT_CHAR_W      5
#define FONT_CHAR_H      7
#define FONT_SCALE_MAX   4

/* --- Color Helpers (ARGB8888) --- */
#define COLOR_ARGB(a, r, g, b) \
    ((uint32_t)((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define COLOR_RGB(r, g, b)     COLOR_ARGB(0xFF, r, g, b)

/* Predefined color palette */
#define CLR_BLACK        0xFF000000
#define CLR_WHITE        0xFFFFFFFF
#define CLR_RED          0xFFFF0000
#define CLR_GREEN        0xFF00FF00
#define CLR_BLUE         0xFF0000FF
#define CLR_CYAN         0xFF00FFFF
#define CLR_MAGENTA      0xFFFF00FF
#define CLR_YELLOW       0xFFFFFF00
#define CLR_ORANGE       0xFFFF8800
#define CLR_SKYBLUE      0xFF4488FF
#define CLR_LIME         0xFF88FF00
#define CLR_DARK_GRAY    0xFF222222
#define CLR_MED_GRAY     0xFF666666
#define CLR_LIGHT_GRAY   0xFFAAAAAA
#define CLR_DARK_RED     0xFF880000
#define CLR_DARK_GREEN   0xFF008800
#define CLR_DARK_BLUE    0xFF000088
#define CLR_DARK_CYAN    0xFF006688
#define CLR_PURPLE       0xFF6600AA
#define CLR_TEAL         0xFF008888
#define CLR_GOLD         0xFFFFAA00
#define CLR_TRANSPARENT  0x00000000

/* --- Entity System (retained-mode rectangles for HUD) --- */
typedef struct {
    int x, y;
    int width, height;
    uint32_t color;
    int active;
} game_entity_t;

/* --- Input State --- */
typedef struct {
    int mouse_x, mouse_y;
    int mouse_buttons;
    int mouse_buttons_prev;  /* previous frame for click edge detection */
    int mouse_clicked;       /* buttons that were JUST pressed this frame */
    int key_sym;       /* last key pressed (screen key sym) */
    int key_flags;     /* key flags */
    int key_pressed;   /* 1 if a key event occurred this frame */
} input_state_t;

/* Mouse button masks (QNX Screen API) */
#define MOUSE_LEFT   (1 << 0)
#define MOUSE_RIGHT  (1 << 1)
#define MOUSE_MIDDLE (1 << 2)

/* --- UI Context (the renderer) --- */
typedef struct {
    /* QNX Screen handles */
    screen_context_t   screen_ctx;
    screen_window_t    screen_win;
    screen_buffer_t    screen_buf[2];
    void              *buffer_ptr[2];
    int                stride[2];
    screen_event_t     screen_ev;

    /* Display info */
    int width, height;
    int buffer_index;
    int rect[4];

    /* Input */
    input_state_t      input;

    /* Retained entities */
    game_entity_t      entities[MAX_ENTITIES];

    /* Frame timing */
    uint64_t           frame_start_ns;
    uint64_t           last_frame_ns;
    float              delta_time;       /* seconds since last frame */
    float              fps;
    float              fps_accum;
    int                fps_frame_count;
    uint64_t           fps_last_update;
} ui_context_t;

/*=============================================================================
 * Lifecycle
 *============================================================================*/
int  ui_init(ui_context_t *ctx, int width, int height);
void ui_shutdown(ui_context_t *ctx);

/*=============================================================================
 * Input Handling
 *============================================================================*/
void ui_poll_events(ui_context_t *ctx);

/*=============================================================================
 * Entity Management (retained-mode rectangles)
 *============================================================================*/
int  ui_add_entity(ui_context_t *ctx, int x, int y, int w, int h, uint32_t color);
void ui_remove_entity(ui_context_t *ctx, int id);
void ui_update_entity(ui_context_t *ctx, int id, int x, int y);

/*=============================================================================
 * Immediate-Mode Drawing Primitives
 * These draw directly into the current back buffer.
 *============================================================================*/

/* Frame management */
void ui_begin_frame(ui_context_t *ctx);   /* clear buffer, record timing */
void ui_end_frame(ui_context_t *ctx);     /* post buffer, swap, update FPS */

/* Pixel */
void ui_put_pixel(ui_context_t *ctx, int x, int y, uint32_t color);

/* Rectangles */
void ui_fill_rect(ui_context_t *ctx, int x, int y, int w, int h, uint32_t color);
void ui_draw_rect(ui_context_t *ctx, int x, int y, int w, int h, uint32_t color);

/* Lines */
void ui_draw_line(ui_context_t *ctx, int x0, int y0, int x1, int y1, uint32_t color);
void ui_draw_hline(ui_context_t *ctx, int x, int y, int len, uint32_t color);
void ui_draw_vline(ui_context_t *ctx, int x, int y, int len, uint32_t color);

/* Circles */
void ui_draw_circle(ui_context_t *ctx, int cx, int cy, int radius, uint32_t color);
void ui_fill_circle(ui_context_t *ctx, int cx, int cy, int radius, uint32_t color);

/* Triangles */
void ui_fill_triangle(ui_context_t *ctx, int x0, int y0, int x1, int y1,
                       int x2, int y2, uint32_t color);

/* Text (built-in 5x7 bitmap font) */
void ui_draw_char(ui_context_t *ctx, int x, int y, char c, uint32_t color, int scale);
void ui_draw_text(ui_context_t *ctx, int x, int y, const char *text, uint32_t color, int scale);
int  ui_text_width(const char *text, int scale);

/* Gradient fill (vertical) */
void ui_fill_rect_gradient_v(ui_context_t *ctx, int x, int y, int w, int h,
                              uint32_t color_top, uint32_t color_bottom);

/* Alpha-blended pixel (simple 50% blend) */
void ui_put_pixel_blend(ui_context_t *ctx, int x, int y, uint32_t color);

/* Utility: render all retained entities (called internally by ui_end_frame
   if you want, or call manually after drawing background) */
void ui_render_entities(ui_context_t *ctx);

/* Visual Filter: Draw 25% scanlines every 4px (v5.0) */
void ui_draw_scanlines(ui_context_t *ctx);

/* Utility: get current FPS as float */
float ui_get_fps(ui_context_t *ctx);

/* Utility: get delta time in seconds */
float ui_get_dt(ui_context_t *ctx);

#endif /* UI_ENGINE_H */
