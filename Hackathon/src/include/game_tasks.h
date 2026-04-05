#ifndef GAME_TASKS_H
#define GAME_TASKS_H

#include <stdint.h>

/*=============================================================================
 * Event Horizon v3 — Task & Bot System
 *
 * 5 subsystems compete for player attention and ship energy.
 * Bots can automate tasks but consume energy and are capped at 90% max.
 * Repairs are ALWAYS manual.
 *============================================================================*/

/* Task IDs */
#define TASK_DEFENSE    0
#define TASK_NAV        1
#define TASK_REACTOR    2
#define TASK_COMMS      3
#define TASK_REPAIRS    4
#define NUM_TASKS       5

/* Bot upgrade levels */
#define BOT_LVL_NONE   0
#define BOT_LVL_1      1
#define BOT_LVL_2      2
#define BOT_LVL_3      3
#define BOT_MAX_LVL    3

/* Max accuracy even at max upgrade (90% cap) */
#define BOT_ACC_CAP    0.90f

/* Bot energy cost per second when active */
#define BOT_ENERGY_COST_BASE  3.0f

/* Repair system */
#define MAX_BREACHES   4
#define REPAIR_TIME    3.0f

/* Nav decode */
#define NAV_SEQ_LEN    6
#define NAV_INTERVAL   8.0f

/* Comms */
#define COMMS_INTERVAL 12.0f

/* --- Bot state --- */
typedef struct {
    int   unlocked;       /* 1 if available to assign */
    int   active;         /* 1 if currently running */
    int   level;          /* upgrade level 0-3 */
    float accuracy;       /* current accuracy (computed from level, capped 90%) */
    float energy_rate;    /* energy/sec consumed */
    float timer;          /* internal bot timer */
    int   upgrade_cost;   /* score cost to upgrade */
} bot_t;

/* --- Breach (for repairs) --- */
typedef struct {
    int   active;
    int   section;        /* 0-3: which hull section */
    float severity;       /* damage per second if unrepaired */
    float repair_progress;/* 0 to REPAIR_TIME */
    int   being_repaired; /* 1 if player is currently repairing */
} breach_t;

/* --- Nav waypoint --- */
typedef struct {
    int   sequence[NAV_SEQ_LEN]; /* 0=UP,1=RIGHT,2=DOWN,3=LEFT */
    int   current_idx;
    float scroll_pos;     /* scrolling position for display */
    int   active;
    int   decoded;
    float timer;
} nav_waypoint_t;

/* --- Comms signal --- */
typedef struct {
    float target_freq;    /* 0.0-1.0 target frequency */
    float player_freq;    /* 0.0-1.0 player's dial position */
    int   active;
    int   locked;         /* 1 if successfully tuned */
    float timer;          /* time left to tune */
    float quality;        /* 0-1 how close the match is */
    int   reward_type;    /* 0=score, 1=shield, 2=energy, 3=warning */
} comms_signal_t;

/* --- Reactor state --- */
typedef struct {
    float frequency;      /* oscillating value -1 to 1 */
    float speed;          /* oscillation speed */
    float player_adj;     /* player's adjustment offset */
    float output_pct;     /* 0-100% actual output */
} reactor_state_t;

/* --- Full task system --- */
typedef struct {
    /* Bots (index by TASK_*) - no bot for TASK_REPAIRS */
    bot_t bots[NUM_TASKS];

    /* Shared energy pool */
    float energy;         /* 0-100, shared by ship + all bots */
    float energy_regen;   /* base regen rate */

    /* Defense */
    float defense_auto_timer;
    int   defense_auto_target; /* asteroid index bot is targeting */

    /* Navigation */
    nav_waypoint_t nav;
    float nav_drift;      /* 0-100, drift off course */
    float nav_drift_dmg;  /* damage per second from drift */

    /* Reactor */
    reactor_state_t reactor;

    /* Comms */
    comms_signal_t comms;
    float comms_cooldown;

    /* Repairs (manual only) */
    breach_t breaches[MAX_BREACHES];
    float hull;           /* 0-100 */
    float hull_leak_rate; /* atmosphere leak from unrepaired breaches */

    /* Scoring */
    int   score;
    int   kills;
    int   wave;
    float wave_timer;
    float time;

    /* Game state */
    int   game_over;
    int   show_title;
    float shields;
    float weapon_heat;

    /* Aim (mouse + controller) */
    float aim_x, aim_y;
    float fire_cooldown;

    /* Effects & Feedback */
    float shake;
    float hit_flash;     /* Scren flash duration (0 to 0.1s) */
} task_system_t;

/* --- API --- */
void tasks_init(task_system_t *ts);
void tasks_update(task_system_t *ts, float dt);
void tasks_spawn_breach(task_system_t *ts, float severity);
void tasks_unlock_bot(task_system_t *ts, int task_id);
int  tasks_upgrade_bot(task_system_t *ts, int task_id);
float tasks_get_bot_accuracy(task_system_t *ts, int task_id);

/* Bot base accuracy per level (before cap) */
float bot_base_accuracy(int task_id, int level);

/* Task names for UI */
const char *task_name(int task_id);
const char *task_icon(int task_id);

#endif /* GAME_TASKS_H */
