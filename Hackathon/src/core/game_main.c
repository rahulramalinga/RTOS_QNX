/*=============================================================================
 * EVENT HORIZON v3: REACTOR CRISIS
 * Full-screen task views | Mouse aim | Bot automation | QNX RTOS
 *
 * Tasks switch with keys 1-5 or clicking tabs. Each task = full screen.
 * Small alert icons on the tab bar show when other tasks need attention.
 *============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/dispatch.h>
#include "../include/event_horizon.h"
#include "../include/ui_engine.h"
#include "../include/controller.h"
#include "../include/game_tasks.h"
#include <sys/keycodes.h>

#define W 1024
#define H 768
#define FOV 300.0f
#define CX (W/2)
#define CY (H/2-60)
#define Z_NEAR 1.0f
#define Z_FAR 200.0f
#define TAB_H 32
#define MAX_STARS 350
#define MAX_ROCKS 30
#define MAX_LASERS 15
#define MAX_PARTS 100
#define ROCK_VERTS 9
#define FRAME_US (1000000/60)
#define TRANS_SPEED 6.0f

typedef struct { float x,y,z; } v3;
typedef struct { float x,y; } v2;
typedef struct { v3 p; float speed; int layer; uint32_t col; } star_t;
typedef struct { 
    int on; v3 p; float spd,rad,rot; int hp; uint32_t col;
    v2 verts[ROCK_VERTS]; /* Polygonal shape */
} rock_t;
typedef struct { int on; v3 p; v3 dir; float spd; } laser_t;
typedef struct { int on; v3 p; v3 v; float life; uint32_t col; } part_t;

static star_t  stars[MAX_STARS];
static rock_t  rocks[MAX_ROCKS];
static laser_t lasers[MAX_LASERS];
static part_t  parts[MAX_PARTS];
static task_system_t TS;
static uint32_t *nebula = NULL;

/* Current task view and transition */
static int cur_task = TASK_DEFENSE;
static int next_task = -1;
static float trans_alpha = 0; /* 0=no transition, 0->1 fade out, 1->0 fade in */
static int trans_phase = 0;   /* 0=none, 1=fading out, 2=fading in */

static float fr(void){return(float)rand()/(float)RAND_MAX;}
static float fr2(void){return fr()*2-1;}
static float clp(float v,float a,float b){return v<a?a:(v>b?b:v);}
static int proj(v3 p,int*sx,int*sy,float*sc){
    if(p.z<Z_NEAR) return 0;
    *sc=FOV/p.z;
    float ox = 0, oy = 0;
    if(TS.shake > 0) {
        ox = (fr2() * TS.shake);
        oy = (fr2() * TS.shake);
    }
    *sx=CX+(int)(p.x**sc + ox);*sy=CY+(int)(p.y**sc + oy);return 1;}

/* Switch to a different task screen */
static void switch_task(int t) {
    if (t == cur_task || t < 0 || t >= NUM_TASKS) return;
    if (trans_phase != 0) return; /* already transitioning */
    next_task = t;
    trans_phase = 1;
    trans_alpha = 0;
}

/*--- Nebula background ---*/
static void gen_nebula(void){
    nebula=(uint32_t*)calloc(W*H,4);if(!nebula)return;
    for(int y=0;y<H;y++){float t=(float)y/H;
        uint8_t r=(uint8_t)(2+t*4),g=(uint8_t)(2+t*2),b=(uint8_t)(10+t*12);
        for(int x=0;x<W;x++)nebula[y*W+x]=COLOR_RGB(r,g,b);}
    srand(42);
    for(int i=0;i<14;i++){
        int bx=rand()%W,by=rand()%H,br=80+rand()%160;
        uint8_t cr=0,cg=0,cb=0;
        switch(i%4){case 0:cr=40;cg=10;cb=60;break;case 1:cr=10;cg=30;cb=50;break;
                    case 2:cr=15;cg=15;cb=45;break;case 3:cr=35;cg=8;cb=40;break;}
        for(int dy=-br;dy<=br;dy++){int py=by+dy;if(py<0||py>=H)continue;
            for(int dx=-br;dx<=br;dx++){int px=bx+dx;if(px<0||px>=W)continue;
                float d=sqrtf((float)(dx*dx+dy*dy));if(d>br)continue;
                float in=1.0f-(d/br);in*=in*0.7f;
                uint32_t e=nebula[py*W+px];
                uint8_t er=clp(((e>>16)&0xFF)+cr*in,0,255);
                uint8_t eg=clp(((e>>8)&0xFF)+cg*in,0,255);
                uint8_t eb=clp((e&0xFF)+cb*in,0,255);
                nebula[py*W+px]=COLOR_RGB(er,eg,eb);}}}
    srand(time(NULL));
}
static void blit_nebula(ui_context_t*ui){
    if(!nebula) return;
    void*p=ui->buffer_ptr[ui->buffer_index];
    int s=ui->stride[ui->buffer_index];
    for(int y=TAB_H;y<H;y++) memcpy((uint8_t*)p+y*s,&nebula[y*W],W*4);
}

/*--- Parallax Starfield ---*/
static void init_star(int i){
    stars[i].p.x = fr2() * 150;
    stars[i].p.y = fr2() * 110;
    stars[i].p.z = 10 + fr() * (Z_FAR - 10);
    
    /* Assign star to a layer (0=far, 1=mid, 2=near) */
    stars[i].layer = rand() % 3;
    float speeds[] = { 15.0f, 45.0f, 90.0f };
    stars[i].speed = speeds[stars[i].layer];

    uint8_t brightness = (uint8_t)(100 + fr() * 155);
    if (stars[i].layer == 2) { /* Near stars are colorful */
        int tint = rand() % 3;
        if      (tint == 0) stars[i].col = COLOR_RGB(brightness, (uint8_t)(brightness * 0.8f), (uint8_t)(brightness * 0.7f));
        else if (tint == 1) stars[i].col = COLOR_RGB((uint8_t)(brightness * 0.7f), (uint8_t)(brightness * 0.8f), brightness);
        else                stars[i].col = COLOR_RGB(brightness, brightness, brightness);
    } else {
        stars[i].col = COLOR_RGB(brightness, brightness, brightness);
    }
}

static void draw_stars(ui_context_t*ui, float dt){
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].p.z -= stars[i].speed * dt;
        if (stars[i].p.z < Z_NEAR) {
            init_star(i);
            stars[i].p.z = Z_FAR;
        }
        
        int sx, sy; float sc;
        if (!proj(stars[i].p, &sx, &sy, &sc)) continue;
        if (sx < 0 || sx >= W || sy < TAB_H || sy >= H) continue;

        int sz = (stars[i].layer + 1);
        if (sz == 1) ui_put_pixel(ui, sx, sy, stars[i].col);
        else ui_fill_rect(ui, sx, sy, sz, sz, stars[i].col);
    }
}

/*--- Polygonal Asteroids ---*/
static void spawn_rock(void){
    for(int i=0;i<MAX_ROCKS;i++){if(rocks[i].on)continue;
        rocks[i].on=1;rocks[i].p.x=fr2()*80;rocks[i].p.y=fr2()*50;
        rocks[i].p.z=Z_FAR*0.7f+fr()*Z_FAR*0.3f;
        rocks[i].spd=25+fr()*30+TS.wave*3;
        rocks[i].rad=3+fr()*5;rocks[i].hp=(int)(rocks[i].rad/2)+1;
        rocks[i].rot=fr()*6.28f;
        
        /* Generate jagged polygon */
        for(int v=0; v<ROCK_VERTS; v++){
            float ang = (float)v / ROCK_VERTS * 6.28318f;
            float r = 0.7f + fr()*0.6f;
            rocks[i].verts[v].x = cosf(ang) * r;
            rocks[i].verts[v].y = sinf(ang) * r;
        }

        uint8_t v_col=(uint8_t)(100+rand()%60);
        rocks[i].col=COLOR_RGB(v_col, (uint8_t)(v_col*0.8f), (uint8_t)(v_col*0.6f)); return; }
}
static void update_rocks(float dt){
    for(int i=0;i<MAX_ROCKS;i++){if(!rocks[i].on)continue;
        rocks[i].p.z-=rocks[i].spd*dt;
        rocks[i].rot += 0.5f * dt;
        if(rocks[i].p.z<Z_NEAR){rocks[i].on=0;
            float dmg=rocks[i].rad*3;
            if(TS.shields>15){TS.shields-=dmg*1.5f;if(TS.shields<0)TS.shields=0;}
            else{TS.hull-=dmg;TS.shake=10;tasks_spawn_breach(&TS,1+rocks[i].rad*0.5f);}}}
}
static void draw_rocks(ui_context_t*ui){
    for(int i=0;i<MAX_ROCKS;i++){if(!rocks[i].on)continue;
        int sx,sy;float sc;if(!proj(rocks[i].p,&sx,&sy,&sc))continue;
        if(sy<TAB_H)continue;
        
        float r_pix = rocks[i].rad * sc;
        if(r_pix < 2) continue;

        /* Draw polygonal faces with simple shading */
        for(int v=0; v<ROCK_VERTS; v++){
            int vn = (v+1)%ROCK_VERTS;
            float ang = rocks[i].rot;
            float s = sinf(ang), c = cosf(ang);
            
            /* Rotated & scaled vertices */
            int x1 = sx + (int)((rocks[i].verts[v].x*c - rocks[i].verts[v].y*s) * r_pix);
            int y1 = sy + (int)((rocks[i].verts[v].x*s + rocks[i].verts[v].y*c) * r_pix);
            int x2 = sx + (int)((rocks[i].verts[vn].x*c - rocks[i].verts[vn].y*s) * r_pix);
            int y2 = sy + (int)((rocks[i].verts[vn].x*s + rocks[i].verts[vn].y*c) * r_pix);

            /* Shading based on 'lighting' from top-left */
            float nx = (rocks[i].verts[v].x + rocks[i].verts[vn].x)*0.5f;
            float ny = (rocks[i].verts[v].y + rocks[i].verts[vn].y)*0.5f;
            float light = (nx*(-0.7f) + ny*(-0.7f) + 1.0f) * 0.5f;
            
            uint8_t cr = (uint8_t)(((rocks[i].col>>16)&0xFF) * light);
            uint8_t cg = (uint8_t)(((rocks[i].col>>8)&0xFF) * light);
            uint8_t cb = (uint8_t)((rocks[i].col&0xFF) * light);
            
            ui_fill_triangle(ui, sx, sy, x1, y1, x2, y2, COLOR_RGB(cr, cg, cb));
            ui_draw_line(ui, x1, y1, x2, y2, COLOR_RGB(cr+20, cg+20, cb+20));
        }

        if(rocks[i].p.z<40){ /* Proximity warning glow */
            uint8_t g=(uint8_t)(255*(1-rocks[i].p.z/40));
            ui_draw_circle(ui,sx,sy,(int)r_pix+2,COLOR_ARGB(0xFF,g,g/3,0));
        }
    }
}

/*--- Lasers (FIXED: fire from ship center toward crosshair 3D position) ---*/
static void fire_laser(void){
    if(TS.fire_cooldown>0||TS.weapon_heat>92||TS.energy<4)return;
    for(int i=0;i<MAX_LASERS;i++){if(lasers[i].on)continue;
        lasers[i].on=1;
        /* Start at ship nose (center, near camera) */
        lasers[i].p.x=0; lasers[i].p.y=0; lasers[i].p.z=3.0f;
        /* Direction: toward where the crosshair points in 3D space */
        float tx = TS.aim_x * 60.0f;  /* target X at Z=100 */
        float ty = TS.aim_y * 45.0f;  /* target Y at Z=100 */
        float tz = 100.0f;
        float len = sqrtf(tx*tx + ty*ty + tz*tz);
        lasers[i].dir.x = tx/len;
        lasers[i].dir.y = ty/len;
        lasers[i].dir.z = tz/len;
        lasers[i].spd = 220.0f;
        TS.energy-=4;TS.weapon_heat+=12;TS.fire_cooldown=0.15f;
        TS.shake += 1.8f; /* Recoil */
        return;}
}
static void update_lasers(float dt){
    for(int i=0;i<MAX_LASERS;i++){if(!lasers[i].on)continue;
        lasers[i].p.x+=lasers[i].dir.x*lasers[i].spd*dt;
        lasers[i].p.y+=lasers[i].dir.y*lasers[i].spd*dt;
        lasers[i].p.z+=lasers[i].dir.z*lasers[i].spd*dt;
        if(lasers[i].p.z>Z_FAR){lasers[i].on=0;continue;}
        for(int j=0;j<MAX_ROCKS;j++){if(!rocks[j].on)continue;
            float dx=lasers[i].p.x-rocks[j].p.x,dy=lasers[i].p.y-rocks[j].p.y,
                  dz=lasers[i].p.z-rocks[j].p.z;
            if(sqrtf(dx*dx+dy*dy+dz*dz)<rocks[j].rad+2.5f){
                lasers[i].on=0;rocks[j].hp--;
                TS.shake += 2.5f; /* Impact shake */
                if(rocks[j].hp<=0){
                    rocks[j].on=0;TS.score+=15*(int)rocks[j].rad;TS.kills++;
                    TS.hit_flash = 0.08f; /* Visual feedback */
                    for(int k=0;k<MAX_PARTS;k++){if(parts[k].on)continue;
                        parts[k].on=1;parts[k].p=rocks[j].p;
                        parts[k].v.x=fr2()*35;parts[k].v.y=fr2()*35;parts[k].v.z=fr2()*20;
                        parts[k].life=0.3f+fr()*0.3f;
                        parts[k].col=fr()>0.5f?CLR_ORANGE:CLR_YELLOW;break;}}break;}}
    }
}
static void draw_lasers(ui_context_t*ui){
    for(int i=0;i<MAX_LASERS;i++){if(!lasers[i].on)continue;
        int sx,sy;float sc;if(!proj(lasers[i].p,&sx,&sy,&sc))continue;
        if(sy<TAB_H)continue;
        int l=(int)(4*sc);if(l<3)l=3;if(l>25)l=25;
        
        /* Bloom Trail */
        ui_draw_line(ui,CX,H/2,sx,sy,COLOR_ARGB(0x30,0,180,255));
        
        /* Multi-pass Glow core */
        ui_fill_circle(ui, sx, sy, l/2+2, COLOR_ARGB(0x40, 0, 100, 255));
        ui_fill_circle(ui, sx, sy, l/3+1, COLOR_ARGB(0x80, 100, 200, 255));
        
        ui_draw_line(ui, sx, sy-l, sx, sy+l, CLR_CYAN);
        ui_draw_line(ui, sx-1, sy-l+1, sx-1, sy+l-1, CLR_WHITE);
    }
}

/*--- Particles ---*/
static void update_parts(float dt){
    for(int i=0;i<MAX_PARTS;i++){if(!parts[i].on)continue;
        parts[i].p.x+=parts[i].v.x*dt;parts[i].p.y+=parts[i].v.y*dt;
        parts[i].p.z+=parts[i].v.z*dt;parts[i].life-=dt;
        if(parts[i].life<=0)parts[i].on=0;}
}
static void draw_parts(ui_context_t*ui){
    for(int i=0;i<MAX_PARTS;i++){if(!parts[i].on)continue;
        int sx,sy;float sc;if(!proj(parts[i].p,&sx,&sy,&sc))continue;
        if(sy<TAB_H)continue;
        float f=clp(parts[i].life,0,1);
        uint8_t r=(uint8_t)(((parts[i].col>>16)&0xFF)*f);
        uint8_t g=(uint8_t)(((parts[i].col>>8)&0xFF)*f);
        uint8_t b=(uint8_t)((parts[i].col&0xFF)*f);
        int sz=(int)(sc*1.5f);if(sz<1)sz=1;if(sz>6)sz=6;
        ui_fill_rect(ui,sx,sy,sz,sz,COLOR_RGB(r,g,b));}
}

/*--- Defense bot ---*/
static void bot_defense(float dt){
    if(!TS.bots[TASK_DEFENSE].active)return;
    TS.bots[TASK_DEFENSE].timer+=dt;
    if(TS.bots[TASK_DEFENSE].timer<1.2f)return;
    TS.bots[TASK_DEFENSE].timer=0;
    int best=-1;float bz=Z_FAR;
    for(int i=0;i<MAX_ROCKS;i++){if(!rocks[i].on)continue;
        if(rocks[i].p.z<bz){bz=rocks[i].p.z;best=i;}}
    if(best<0||TS.weapon_heat>90||TS.energy<3)return;
    TS.weapon_heat+=12;TS.energy-=3;
    if(fr()<TS.bots[TASK_DEFENSE].accuracy){
        rocks[best].hp--;
        if(rocks[best].hp<=0){rocks[best].on=0;TS.score+=5*(int)rocks[best].rad;TS.kills++;}}
}

/*--- Crosshair ---*/
static void draw_xhair(ui_context_t*ui){
    int cx=CX+(int)(TS.aim_x*(W/3)),cy=CY+(int)(TS.aim_y*(H/3));
    if(cy < TAB_H+10) cy = TAB_H+10;
    if(cy > H-10)     cy = H-10;
    ui_draw_circle(ui,cx,cy,14,COLOR_RGB(0,200,100));
    ui_draw_line(ui,cx-18,cy,cx-8,cy,CLR_GREEN);
    ui_draw_line(ui,cx+8,cy,cx+18,cy,CLR_GREEN);
    ui_draw_line(ui,cx,cy-18,cx,cy-8,CLR_GREEN);
    ui_draw_line(ui,cx,cy+8,cx,cy+18,CLR_GREEN);
    ui_fill_rect(ui,cx-1,cy-1,3,3,CLR_WHITE);
    /* Beam origin indicator at center */
    ui_draw_circle(ui,CX,H/2,4,CLR_DARK_CYAN);
}

/*=== FULL-SCREEN TASK VIEWS ===*/

static void draw_defense_view(ui_context_t*ui,float dt){
    blit_nebula(ui);draw_stars(ui,dt);draw_parts(ui);
    draw_rocks(ui);draw_lasers(ui);
    if(!TS.game_over)draw_xhair(ui);
    
    /* Cockpit HUD Framework (v5.0 Enhanced) */
    ui_draw_line(ui, 0, H-120, 150, H, CLR_DARK_CYAN);
    ui_draw_line(ui, W, H-120, W-150, H, CLR_DARK_CYAN);
    ui_draw_line(ui, 150, H, W-150, H, CLR_DARK_CYAN);
    ui_draw_rect(ui, 20, H-100, 100, 80, COLOR_ARGB(0x40, 0, 40, 80));
    ui_draw_text(ui, 30, H-90, "AUX", CLR_SKYBLUE, 1);
    
    /* Hit Flash Overlay */
    if(TS.hit_flash > 0) {
        uint8_t a = (uint8_t)(TS.hit_flash * 1500); /* Flash intensity */
        if(a > 180) a = 180;
        for(int y=TAB_H; y<H; y+=4) {
            ui_draw_hline(ui, 0, y, W, COLOR_ARGB(a, 255, 255, 255));
        }
    }
    
    /* Status overlay */
    char b[64];
    snprintf(b,sizeof(b),"HEAT: %d%%",(int)TS.weapon_heat);
    ui_draw_text(ui,20,H-50,b,TS.weapon_heat>80?CLR_RED:CLR_ORANGE,2);
    int ac=0;for(int i=0;i<MAX_ROCKS;i++)if(rocks[i].on)ac++;
    snprintf(b,sizeof(b),"THREATS: %d",ac);
    ui_draw_text(ui,20,H-28,b,ac>5?CLR_RED:CLR_WHITE,1);
    if(TS.bots[TASK_DEFENSE].active){
        snprintf(b,sizeof(b),"BOT Lv%d [%d%%]",TS.bots[TASK_DEFENSE].level,
            (int)(TS.bots[TASK_DEFENSE].accuracy*100));
        ui_draw_text(ui,W-160,H-28,b,CLR_LIME,1);}
}

static void draw_nav_view(ui_context_t*ui){
    blit_nebula(ui);
    int cx=W/2,cy=H/2;
    ui_draw_text(ui,cx-100,TAB_H+20,"NAVIGATION DECODE",CLR_CYAN,3);
    ui_draw_text(ui,cx-140,TAB_H+60,
        "Match the arrows: UP DOWN LEFT RIGHT",CLR_MED_GRAY,1);
    if(TS.nav.active&&!TS.nav.decoded){
        const char*arrows[]={"UP","RT","DN","LT"};
        uint32_t acols[]={CLR_GREEN,CLR_YELLOW,CLR_WHITE,CLR_MED_GRAY};
        /* Draw large sequence */
        for(int i=0;i<NAV_SEQ_LEN;i++){
            int x=cx-NAV_SEQ_LEN*40/2+i*40;
            int st=0;
            if(i<TS.nav.current_idx)st=0;      /* done */
            else if(i==TS.nav.current_idx)st=1; /* current */
            else st=3;                           /* pending */
            /* Draw box */
            uint32_t bc=acols[st];
            ui_draw_rect(ui,x,cy-20,35,35,bc);
            if(i==TS.nav.current_idx)
                ui_fill_rect(ui,x+1,cy-19,33,33,COLOR_RGB(20,30,10));
            ui_draw_text(ui,x+5,cy-10,arrows[TS.nav.sequence[i]],bc,2);
        }
        ui_draw_text(ui,cx-60,cy+40,"Press matching arrow key!",CLR_YELLOW,1);
    }else{
        ui_draw_text(ui,cx-80,cy,"Awaiting waypoint data...",CLR_MED_GRAY,2);
        char b[32];snprintf(b,sizeof(b),"Next in: %.0fs",TS.nav.timer);
        ui_draw_text(ui,cx-50,cy+30,b,CLR_DARK_CYAN,1);
    }
    /* Drift meter */
    char b[32];snprintf(b,sizeof(b),"DRIFT: %d%%",(int)TS.nav_drift);
    ui_draw_text(ui,50,H-60,b,TS.nav_drift>30?CLR_RED:CLR_WHITE,2);
    int bw=300;
    ui_fill_rect(ui,50,H-35,bw,12,COLOR_RGB(20,20,20));
    ui_fill_rect(ui,50,H-35,(int)(bw*clp(TS.nav_drift,0,100)/100),12,
        TS.nav_drift>50?CLR_RED:(TS.nav_drift>25?CLR_YELLOW:CLR_GREEN));
    ui_draw_rect(ui,50,H-35,bw,12,CLR_MED_GRAY);
    if(TS.bots[TASK_NAV].active){
        snprintf(b,sizeof(b),"NAV BOT Lv%d [%d%%]",TS.bots[TASK_NAV].level,
            (int)(TS.bots[TASK_NAV].accuracy*100));
        ui_draw_text(ui,W-200,H-28,b,CLR_LIME,1);}
}

static void draw_reactor_view(ui_context_t*ui){
    blit_nebula(ui);
    int cx=W/2,cy=H/2-30;
    ui_draw_text(ui,cx-120,TAB_H+20,"REACTOR CALIBRATION",CLR_CYAN,3);
    ui_draw_text(ui,cx-150,TAB_H+60,
        "Keep the indicator in the GREEN zone (UP/DOWN)",CLR_MED_GRAY,1);
    /* Large reactor bar */
    int bw=600,bh=40,bx=cx-bw/2,by=cy;
    ui_fill_rect(ui,bx,by,bw,bh,COLOR_RGB(15,15,15));
    /* Zones: red-yellow-GREEN-yellow-red */
    int zw=bw/5;
    ui_fill_rect(ui,bx,by,zw,bh,COLOR_RGB(60,15,15));
    ui_fill_rect(ui,bx+zw,by,zw,bh,COLOR_RGB(60,60,15));
    ui_fill_rect(ui,bx+zw*2,by,zw,bh,COLOR_RGB(15,60,15));
    ui_fill_rect(ui,bx+zw*3,by,zw,bh,COLOR_RGB(60,60,15));
    ui_fill_rect(ui,bx+zw*4,by,bw-zw*4,bh,COLOR_RGB(60,15,15));
    /* Indicator */
    float rv=sinf(TS.reactor.frequency)+TS.reactor.player_adj;
    rv=clp(rv,-1,1);
    int ix=bx+(int)((rv+1)*0.5f*bw);
    ui_fill_rect(ui,ix-4,by-6,9,bh+12,CLR_WHITE);
    ui_draw_rect(ui,bx,by,bw,bh,CLR_MED_GRAY);
    /* Zone labels */
    ui_draw_text(ui,bx+zw*2+zw/2-20,by+bh+8,"GREEN",CLR_GREEN,1);
    /* Output */
    char b[64];
    snprintf(b,sizeof(b),"POWER OUTPUT: %d%%",(int)TS.reactor.output_pct);
    uint32_t oc=TS.reactor.output_pct>90?CLR_GREEN:
        (TS.reactor.output_pct>60?CLR_YELLOW:CLR_RED);
    ui_draw_text(ui,cx-80,by+bh+30,b,oc,2);
    snprintf(b,sizeof(b),"Ship Energy: %d%%",(int)TS.energy);
    ui_draw_text(ui,cx-60,by+bh+60,b,CLR_GOLD,2);
    if(TS.bots[TASK_REACTOR].active){
        snprintf(b,sizeof(b),"REACTOR BOT Lv%d [%d%%]",TS.bots[TASK_REACTOR].level,
            (int)(TS.bots[TASK_REACTOR].accuracy*100));
        ui_draw_text(ui,W-220,H-28,b,CLR_LIME,1);}
}

static void draw_comms_view(ui_context_t*ui){
    blit_nebula(ui);
    int cx=W/2,cy=H/2-20;
    ui_draw_text(ui,cx-120,TAB_H+20,"SIGNAL FREQUENCY LOCK",CLR_CYAN,3);
    ui_draw_text(ui,cx-130,TAB_H+60,
        "Tune the dial to match the signal (A/D keys)",CLR_MED_GRAY,1);
    if(TS.comms.active){
        int bw=700,bh=30,bx=cx-bw/2,by=cy;
        ui_fill_rect(ui,bx,by,bw,bh,COLOR_RGB(15,15,25));
        /* Draw sine wave hint */
        for(int x=0;x<bw;x+=2){
            float f=(float)x/bw;
            float diff=fabsf(f-TS.comms.target_freq);
            int amp=(int)(15.0f/(diff*10+0.5f));if(amp>14)amp=14;
            ui_draw_vline(ui,bx+x,by+bh/2-amp,amp*2,COLOR_RGB(40,40,80));}
        /* Target marker */
        int tx=bx+(int)(TS.comms.target_freq*bw);
        ui_fill_rect(ui,tx-1,by-8,3,bh+16,CLR_RED);
        ui_draw_text(ui,tx-10,by-20,"SIG",CLR_RED,1);
        /* Player dial */
        int px=bx+(int)(TS.comms.player_freq*bw);
        ui_fill_rect(ui,px-3,by-5,7,bh+10,CLR_GREEN);
        ui_draw_text(ui,px-10,by+bh+8,"YOU",CLR_GREEN,1);
        ui_draw_rect(ui,bx,by,bw,bh,CLR_MED_GRAY);
        /* Quality & timer */
        char b[64];
        snprintf(b,sizeof(b),"LOCK: %d%%",(int)(TS.comms.quality*100));
        ui_draw_text(ui,cx-40,by+bh+35,b,
            TS.comms.quality>0.8f?CLR_GREEN:CLR_YELLOW,2);
        snprintf(b,sizeof(b),"TIME: %.1fs",TS.comms.timer);
        ui_draw_text(ui,cx-40,by+bh+60,b,
            TS.comms.timer<3?CLR_RED:CLR_WHITE,2);
        const char*rewards[]={"SCORE BONUS","SHIELD BOOST","ENERGY BOOST","THREAT WARNING"};
        snprintf(b,sizeof(b),"Reward: %s",rewards[TS.comms.reward_type]);
        ui_draw_text(ui,cx-60,by+bh+85,b,CLR_GOLD,1);
    }else{
        ui_draw_text(ui,cx-70,cy,"Scanning for signals...",CLR_MED_GRAY,2);
        char b[32];snprintf(b,sizeof(b),"Next in: %.0fs",TS.comms_cooldown);
        ui_draw_text(ui,cx-40,cy+30,b,CLR_DARK_CYAN,1);
    }
    if(TS.bots[TASK_COMMS].active){
        char b[48];snprintf(b,sizeof(b),"COMMS BOT Lv%d [%d%%]",TS.bots[TASK_COMMS].level,
            (int)(TS.bots[TASK_COMMS].accuracy*100));
        ui_draw_text(ui,W-200,H-28,b,CLR_LIME,1);}
}

static void draw_repairs_view(ui_context_t*ui){
    blit_nebula(ui);
    int cx=W/2,cy=H/2-40;
    ui_draw_text(ui,cx-100,TAB_H+20,"HULL REPAIR BAY",CLR_CYAN,3);
    ui_draw_text(ui,cx-140,TAB_H+60,
        "Click breaches to dispatch repair crews (MANUAL ONLY)",CLR_ORANGE,1);
    /* Ship cross-section diagram */
    int sx=cx-120,sy=cy-30,sw=240,sh=180;
    ui_draw_rect(ui,sx,sy,sw,sh,CLR_MED_GRAY);
    ui_draw_line(ui,sx,sy,sx+sw/2,sy-20,CLR_MED_GRAY); /* nose */
    ui_draw_line(ui,sx+sw,sy,sx+sw/2,sy-20,CLR_MED_GRAY);
    /* Section labels */
    const char*sn[]={"FWD","AFT","PORT","STAR"};
    int spos[][2]={{cx-10,sy+10},{cx-10,sy+sh-25},{sx+10,cy+30},{sx+sw-40,cy+30}};
    /* Section boxes */
    int sbox[][4]={{sx+sw/4,sy,sw/2,sh/3},
                   {sx+sw/4,sy+sh*2/3,sw/2,sh/3},
                   {sx,sy+sh/3,sw/4,sh/3},
                   {sx+sw*3/4,sy+sh/3,sw/4,sh/3}};
    for(int i=0;i<4;i++){
        uint32_t bc=COLOR_RGB(20,30,20);
        /* Check for breaches in this section */
        for(int j=0;j<MAX_BREACHES;j++){
            if(TS.breaches[j].active&&TS.breaches[j].section==i){
                bc=TS.breaches[j].being_repaired?COLOR_RGB(50,50,10):COLOR_RGB(50,10,10);}}
        ui_fill_rect(ui,sbox[i][0],sbox[i][1],sbox[i][2],sbox[i][3],bc);
        ui_draw_rect(ui,sbox[i][0],sbox[i][1],sbox[i][2],sbox[i][3],CLR_MED_GRAY);
        ui_draw_text(ui,spos[i][0],spos[i][1],sn[i],CLR_WHITE,1);
    }
    /* Breach details */
    int by=cy+sh+20;
    int bc=0;
    for(int i=0;i<MAX_BREACHES;i++){
        if(!TS.breaches[i].active) continue;
        bc++;
        int bx=160+i*180,by=H/2-100;
        ui_draw_rect(ui,bx-20,by-20,140,140,CLR_DARK_CYAN);
        char b[64];float pct=TS.breaches[i].repair_progress/REPAIR_TIME*100;
        snprintf(b,sizeof(b),"%s - %s - %d%%",sn[TS.breaches[i].section],
            TS.breaches[i].being_repaired?"REPAIRING":"LEAKING",(int)pct);
        ui_draw_text(ui,cx-100,by,b,
            TS.breaches[i].being_repaired?CLR_YELLOW:CLR_RED,2);
        /* Progress bar */
        ui_fill_rect(ui,cx+100,by+2,120,12,COLOR_RGB(20,20,20));
        ui_fill_rect(ui,cx+100,by+2,(int)(120*pct/100),12,
            TS.breaches[i].being_repaired?CLR_YELLOW:CLR_RED);
        ui_draw_rect(ui,cx+100,by+2,120,12,CLR_MED_GRAY);
        by+=28;
    }
    if(bc==0)ui_draw_text(ui,cx-50,by,"Hull intact",CLR_GREEN,2);
    /* Click to repair instruction */
    if(bc>0)ui_draw_text(ui,cx-100,H-40,"CLICK to start repairing!",CLR_YELLOW,2);
    /* Hull bar */
    char b[32];snprintf(b,sizeof(b),"HULL: %d%%",(int)TS.hull);
    ui_draw_text(ui,50,H-60,b,TS.hull>30?CLR_GREEN:CLR_RED,2);
    ui_fill_rect(ui,50,H-35,300,12,COLOR_RGB(20,20,20));
    ui_fill_rect(ui,50,H-35,(int)(300*TS.hull/100),12,TS.hull>30?CLR_GREEN:CLR_RED);
    ui_draw_rect(ui,50,H-35,300,12,CLR_MED_GRAY);
    ui_draw_text(ui,W-180,H-28,"NO BOT - MANUAL ONLY",CLR_ORANGE,1);
}

/*=== TAB BAR ===*/
static void draw_tabs(ui_context_t*ui){
    ui_fill_rect(ui,0,0,W,TAB_H,COLOR_RGB(8,10,18));
    ui_draw_hline(ui,0,TAB_H-1,W,CLR_DARK_CYAN);
    int tw=W/NUM_TASKS;
    const char*names[]={"1:DEFENSE","2:NAVIG","3:REACTOR","4:COMMS","5:REPAIRS"};
    for(int i=0;i<NUM_TASKS;i++){
        int tx=i*tw;
        /* Active tab highlight */
        if(i==cur_task) ui_fill_rect(ui,tx,0,tw,TAB_H,COLOR_RGB(15,25,40));
        ui_draw_vline(ui,tx,0,TAB_H,CLR_DARK_CYAN);
        /* Name */
        ui_draw_text(ui,tx+8,8,names[i],i==cur_task?CLR_CYAN:CLR_MED_GRAY,1);
        /* Alert indicators for inactive tasks */
        if(i!=cur_task){
            int alert=0;
            if(i==TASK_DEFENSE){int ac=0;for(int j=0;j<MAX_ROCKS;j++)if(rocks[j].on)ac++;
                if(ac>5)alert=2;else if(ac>2)alert=1;}
            if(i==TASK_NAV&&TS.nav.active&&!TS.nav.decoded)alert=1;
            if(i==TASK_NAV&&TS.nav_drift>40)alert=2;
            if(i==TASK_REACTOR&&TS.reactor.output_pct<60)alert=2;
            else if(i==TASK_REACTOR&&TS.reactor.output_pct<90)alert=1;
            if(i==TASK_COMMS&&TS.comms.active)alert=1;
            if(i==TASK_REPAIRS){for(int j=0;j<MAX_BREACHES;j++)
                if(TS.breaches[j].active&&!TS.breaches[j].being_repaired)alert=2;}
            if(alert==2)ui_fill_circle(ui,tx+tw-12,TAB_H/2,5,CLR_RED);
            else if(alert==1)ui_fill_circle(ui,tx+tw-12,TAB_H/2,4,CLR_YELLOW);
        }
        /* Bot status dot & Processing Bar (v5.0) */
        if(i!=TASK_REPAIRS&&TS.bots[i].active) {
            ui_fill_circle(ui,tx+tw-24,TAB_H/2,3,CLR_LIME);
            /* Pulse/Progress bar at top of tab */
            float p_cycle = sinf(TS.time * 8.0f + i) * 0.5f + 0.5f;
            ui_fill_rect(ui, tx+5, 2, (int)(p_cycle * (tw-10)), 3, COLOR_ARGB(0x80, 0, 255, 100));
        }
    }
    /* Resource bars in tab area */
    char b[48];
    snprintf(b,sizeof(b),"E:%d%% S:%d%% H:%d%%",
        (int)TS.energy,(int)TS.shields,(int)TS.hull);
    ui_draw_text(ui,W-200,3,b,CLR_GOLD,1);
    snprintf(b,sizeof(b),"W%d SCR:%d %.0fFPS",TS.wave,TS.score,ui_get_fps(ui));
    ui_draw_text(ui,W-200,16,b,CLR_WHITE,1);
}

/*=== Transition overlay ===*/
static void draw_transition(ui_context_t*ui,float dt){
    if(trans_phase==0)return;
    if(trans_phase==1){
        trans_alpha+=TRANS_SPEED*dt;
        if(trans_alpha>=1.0f){trans_alpha=1.0f;trans_phase=2;cur_task=next_task;}
    }else if(trans_phase==2){
        trans_alpha-=TRANS_SPEED*dt;
        if(trans_alpha<=0){trans_alpha=0;trans_phase=0;}
    }
    uint8_t a=(uint8_t)(trans_alpha*200);
    for(int y=TAB_H;y<H;y+=3)
        for(int x=0;x<W;x+=3)
            ui_put_pixel(ui,x,y,COLOR_ARGB(a,0,0,0));
}

/*--- Title/GameOver (same as before, abbreviated) ---*/
static void draw_title(ui_context_t*ui,controller_state_t*ctrl,float dt){
    blit_nebula(ui);draw_stars(ui,dt);
    int cx=W/2,y=100;
    ui_draw_text(ui,cx-175,y,"EVENT HORIZON",CLR_CYAN,4);y+=50;
    ui_draw_text(ui,cx-110,y,"REACTOR CRISIS",CLR_GOLD,2);y+=30;
    ui_draw_text(ui,cx-150,y,"5 Subsystems | Bots | Energy Management",CLR_MED_GRAY,1);y+=30;
    ui_draw_text(ui,cx-80,y,"Keys 1-5: Switch tasks",CLR_LIGHT_GRAY,1);y+=14;
    ui_draw_text(ui,cx-80,y,"Mouse: Aim + Fire + Repair",CLR_LIGHT_GRAY,1);y+=14;
    ui_draw_text(ui,cx-80,y,"Right-click tab: Toggle bot",CLR_LIGHT_GRAY,1);y+=14;
    ui_draw_text(ui,cx-80,y,"U: Upgrade bot | R: Restart",CLR_LIGHT_GRAY,1);y+=14;
    ui_draw_text(ui,cx-80,y,"Arrows: Reactor/Nav | A/D: Comms",CLR_LIGHT_GRAY,1);y+=25;
    if(ctrl->connected){
        ui_draw_text(ui,cx-80,y,"CONTROLLER DETECTED!",CLR_LIME,1);y+=14;}
    y+=10;float pulse=0.5f+0.5f*sinf(TS.time*3);uint8_t pv=(uint8_t)(128+pulse*127);
    ui_draw_text(ui,cx-100,y,"PRESS SPACE TO LAUNCH",COLOR_RGB(pv,pv,pv),2);
}
static void draw_gameover(ui_context_t*ui){
    for(int y=150;y<420;y++)for(int x=200;x<W-200;x++)
        ui_put_pixel_blend(ui,x,y,COLOR_ARGB(0xBB,0,0,0));
    int cx=W/2;char b[64];
    ui_draw_text(ui,cx-100,170,"HULL BREACH",CLR_RED,3);
    ui_draw_text(ui,cx-70,220,"GAME OVER",CLR_WHITE,2);
    snprintf(b,sizeof(b),"SCORE: %d",TS.score);
    ui_draw_text(ui,cx-50,270,b,CLR_GOLD,2);
    snprintf(b,sizeof(b),"WAVE %d | %d KILLS",TS.wave,TS.kills);
    ui_draw_text(ui,cx-65,310,b,CLR_SKYBLUE,1);
    float pulse=0.5f+0.5f*sinf(TS.time*3);uint8_t pv=(uint8_t)(150+pulse*105);
    ui_draw_text(ui,cx-80,370,"PRESS R TO RETRY",COLOR_RGB(pv,pv,pv),2);
}

/* Notification */
static char notif[64]={0};static float notif_t=0;
static void notify(const char*m){strncpy(notif,m,63);notif_t=3;}
static void draw_notif(ui_context_t*ui,float dt){
    if(notif_t <= 0) return;
    notif_t -= dt;
    float a = clp(notif_t, 0, 1);
    uint8_t v=(uint8_t)(255*a);int tw=ui_text_width(notif,2);
    ui_fill_rect(ui,CX-tw/2-10,TAB_H+10,tw+20,28,COLOR_RGB(0,v/5,v/4));
    ui_draw_text(ui,CX-tw/2,TAB_H+16,notif,COLOR_ARGB(0xFF,v,v,v),2);
}

/*=== MAIN ===*/
int main(int argc,char*argv[]){
    ui_context_t ui;controller_state_t ctrl;int coid;
    (void)argc;(void)argv;
    printf("=== EVENT HORIZON v3 ===\n");
    controller_init(&ctrl);
    coid=-1;for(int r=0;r<5;r++){coid=name_open(ENGINE_ATTACH_NAME,0);if(coid>=0)break;sleep(1);}
    if(ui_init(&ui,W,H)!=0){printf("Screen init failed!\n");return 1;}
    gen_nebula();tasks_init(&TS);
    for(int i=0;i<MAX_STARS;i++)init_star(i);
    memset(rocks,0,sizeof(rocks));memset(lasers,0,sizeof(lasers));memset(parts,0,sizeof(parts));

    status_req_t req={MSG_TYPE_GET_STATUS};status_reply_t st={0};
    int frame=0,pw=1;

    while(1){
        ui_poll_events(&ui);controller_poll(&ctrl);
        float dt=ui_get_dt(&ui);
        if(ui.input.key_pressed&&ui.input.key_sym==KEYCODE_ESCAPE)break;

        /* Title */
        if(TS.show_title){
            TS.time+=dt;ui_begin_frame(&ui);draw_title(&ui,&ctrl,dt);ui_end_frame(&ui);
            if((ui.input.key_pressed&&ui.input.key_sym==KEYCODE_SPACE)||
               (ui.input.mouse_clicked&MOUSE_LEFT)||
               controller_pressed(&ctrl,CTRL_BTN_A)){TS.show_title=0;TS.time=0;}
            usleep(FRAME_US);frame++;continue;
        }

        /* Task switching: keys 1-5 */
        if(ui.input.key_pressed){
            int k=ui.input.key_sym;
            if(k=='1')switch_task(0);
            if(k=='2')switch_task(1);
            if(k=='3')switch_task(2);
            if(k=='4')switch_task(3);
            if(k=='5')switch_task(4);
        }
        /* Tab click switching */
        if((ui.input.mouse_clicked&MOUSE_LEFT)&&ui.input.mouse_y<TAB_H){
            int tab=ui.input.mouse_x/(W/NUM_TASKS);
            if(tab>=0&&tab<NUM_TASKS)switch_task(tab);
        }
        /* Right-click tab = toggle bot */
        if((ui.input.mouse_clicked&MOUSE_RIGHT)&&ui.input.mouse_y<TAB_H){
            int tab=ui.input.mouse_x/(W/NUM_TASKS);
            if(tab>=0&&tab<NUM_TASKS&&tab!=TASK_REPAIRS&&TS.bots[tab].unlocked)
                TS.bots[tab].active=!TS.bots[tab].active;
        }

        /* Mouse aim (defense view) */
        if(cur_task==TASK_DEFENSE&&ui.input.mouse_y>TAB_H){
            TS.aim_x=((float)ui.input.mouse_x-CX)/(W/3.0f);
            TS.aim_y=((float)ui.input.mouse_y-CY)/(H/3.0f);
            TS.aim_x=clp(TS.aim_x,-1,1);TS.aim_y=clp(TS.aim_y,-1,1);
        }
        /* Left click = fire (defense view) */
        if((ui.input.mouse_clicked&MOUSE_LEFT)&&cur_task==TASK_DEFENSE&&ui.input.mouse_y>TAB_H)
            fire_laser();
        /* Left click on repairs = start repairing */
        if((ui.input.mouse_clicked&MOUSE_LEFT)&&cur_task==TASK_REPAIRS&&ui.input.mouse_y>TAB_H){
            for(int i=0;i<MAX_BREACHES;i++){
                if(TS.breaches[i].active&&!TS.breaches[i].being_repaired){
                    TS.breaches[i].being_repaired=1;break;}}
        }
        /* Comms tuning with mouse drag */
        if(cur_task==TASK_COMMS&&(ui.input.mouse_buttons&MOUSE_LEFT)&&ui.input.mouse_y>TAB_H+100){
            TS.comms.player_freq=(float)(ui.input.mouse_x-162)/700.0f;
            TS.comms.player_freq=clp(TS.comms.player_freq,0,1);
        }

        /* Keyboard input */
        if(ui.input.key_pressed){int k=ui.input.key_sym;
            if(k==KEYCODE_SPACE&&cur_task==TASK_DEFENSE)fire_laser();
            if(k==KEYCODE_UP)TS.reactor.player_adj-=0.15f;
            if(k==KEYCODE_DOWN)TS.reactor.player_adj+=0.15f;
            TS.reactor.player_adj=clp(TS.reactor.player_adj,-1,1);
            /* Nav decode */
            if(TS.nav.active&&!TS.nav.decoded&&cur_task==TASK_NAV){
                int dir=-1;
                if(k==KEYCODE_UP)    dir=0;
                if(k==KEYCODE_RIGHT) dir=1;
                if(k==KEYCODE_DOWN)  dir=2;
                if(k==KEYCODE_LEFT)  dir=3;
                if(dir>=0&&dir==TS.nav.sequence[TS.nav.current_idx]){
                    TS.nav.current_idx++;
                    if(TS.nav.current_idx>=NAV_SEQ_LEN){
                        TS.nav.decoded=1;TS.nav.active=0;TS.nav_drift-=20;TS.score+=50;}}
                else if(dir>=0){TS.nav_drift+=8;}}
            /* Comms tuning */
            if(k==KEYCODE_A||k==KEYCODE_CAPITAL_A)TS.comms.player_freq-=0.05f;
            if(k==KEYCODE_D||k==KEYCODE_CAPITAL_D)TS.comms.player_freq+=0.05f;
            TS.comms.player_freq=clp(TS.comms.player_freq,0,1);
            /* Upgrade */
            if(k==KEYCODE_U||k==KEYCODE_CAPITAL_U){
                if(cur_task!=TASK_REPAIRS&&tasks_upgrade_bot(&TS,cur_task))
                    notify("BOT UPGRADED!");}
            /* Restart */
            if((k==KEYCODE_R||k==KEYCODE_CAPITAL_R)&&TS.game_over){
                tasks_init(&TS);memset(rocks,0,sizeof(rocks));
                memset(lasers,0,sizeof(lasers));memset(parts,0,sizeof(parts));
                TS.show_title=0;cur_task=TASK_DEFENSE;}
        }
        /* Controller (UDP Stream) */
        if(ctrl.connected){
            /* Use absolute stick positions or relative? Let's use relative for precision aiming */
            float sens = 2.5f * dt;
            TS.aim_x += ctrl.left_stick_x * sens;
            TS.aim_y += ctrl.left_stick_y * sens;
            
            TS.aim_x=clp(TS.aim_x,-1,1);
            TS.aim_y=clp(TS.aim_y,-1,1);

            if(ctrl.right_trigger > 0.5f && TS.fire_cooldown <= 0) fire_laser();
            if(controller_pressed(&ctrl, CTRL_BTN_A)) fire_laser();
            
            /* Tab switching via Bumpers */
            if(controller_pressed(&ctrl, CTRL_BTN_LB)){ int p=cur_task-1; if(p<0)p=4; switch_task(p); }
            if(controller_pressed(&ctrl, CTRL_BTN_RB)){ int p=cur_task+1; if(p>4)p=0; switch_task(p); }
            
            /* Restart */
            if(controller_pressed(&ctrl, CTRL_BTN_START) && TS.game_over){
                tasks_init(&TS); TS.show_title=0; cur_task=TASK_DEFENSE; }
        }

        /* IPC */
        if(coid>=0&&frame%4==0)
            if(MsgSend(coid,&req,sizeof(req),&st,sizeof(st))==0)
                TS.energy_regen=8+st.energy_level*0.1f;

        /* Update everything (all tasks run simultaneously!) */
        float sr=0.012f+TS.wave*0.006f;if(fr()<sr)spawn_rock();
        update_rocks(dt);update_lasers(dt);update_parts(dt);
        bot_defense(dt);tasks_update(&TS,dt);

        /* Wave notifications */
        if(TS.wave!=pw){pw=TS.wave;
            if(TS.wave==2)notify("GUNNER BOT UNLOCKED! [Tab 1]");
            if(TS.wave==3)notify("REACTOR BOT UNLOCKED! [Tab 3]");
            if(TS.wave==4)notify("NAV BOT UNLOCKED! [Tab 2]");
            if(TS.wave==5)notify("COMMS BOT UNLOCKED! [Tab 4]");}

        /* Render */
        ui_begin_frame(&ui);
        /* Draw current task's full-screen view */
        switch(cur_task){
            case TASK_DEFENSE:draw_defense_view(&ui,dt);break;
            case TASK_NAV:draw_nav_view(&ui);break;
            case TASK_REACTOR:draw_reactor_view(&ui);break;
            case TASK_COMMS:draw_comms_view(&ui);break;
            case TASK_REPAIRS:draw_repairs_view(&ui);break;
        }
        draw_tabs(&ui);
        draw_transition(&ui,dt);
        draw_notif(&ui,dt);
        if(TS.game_over)draw_gameover(&ui);
        
        /* Final Visual Filter (v5.0) */
        ui_draw_scanlines(&ui);
        
        ui_end_frame(&ui);

        usleep(FRAME_US);frame++;
    }
    if(nebula) free(nebula);
    controller_shutdown(&ctrl);
    ui_shutdown(&ui);
    if(coid >= 0) name_close(coid);
    return 0;
}
