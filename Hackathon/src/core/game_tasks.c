/*=============================================================================
 * Event Horizon v3 — Task & Bot System Implementation
 *============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/game_tasks.h"

static float frand(void)  { return (float)rand() / (float)RAND_MAX; }
static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Base accuracy per task per level (before 90% cap) */
float bot_base_accuracy(int task_id, int level) {
    /* Base accuracy at level 0 when first unlocked */
    float base[] = { 0.45f, 0.50f, 0.55f, 0.40f, 0.0f };
    /* Per-level improvement */
    float per_lvl[] = { 0.15f, 0.13f, 0.12f, 0.16f, 0.0f };
    if (task_id < 0 || task_id >= NUM_TASKS) return 0;
    float acc = base[task_id] + per_lvl[task_id] * level;
    if (acc > BOT_ACC_CAP) acc = BOT_ACC_CAP;
    return acc;
}

float tasks_get_bot_accuracy(task_system_t *ts, int id) {
    if (id < 0 || id >= NUM_TASKS) return 0;
    if (!ts->bots[id].active) return 0;
    return ts->bots[id].accuracy;
}

const char *task_name(int id) {
    switch (id) {
        case TASK_DEFENSE: return "DEFENSE";
        case TASK_NAV:     return "NAVIG";
        case TASK_REACTOR: return "REACTOR";
        case TASK_COMMS:   return "COMMS";
        case TASK_REPAIRS: return "REPAIRS";
    }
    return "???";
}

const char *task_icon(int id) {
    switch (id) {
        case TASK_DEFENSE: return "[GUN]";
        case TASK_NAV:     return "[NAV]";
        case TASK_REACTOR: return "[PWR]";
        case TASK_COMMS:   return "[SIG]";
        case TASK_REPAIRS: return "[FIX]";
    }
    return "[?]";
}

void tasks_init(task_system_t *ts) {
    memset(ts, 0, sizeof(task_system_t));
    ts->energy = 100.0f;
    ts->energy_regen = 10.0f;
    ts->hull = 100.0f;
    ts->shields = 60.0f;
    ts->wave = 1;
    ts->show_title = 1;

    /* Reactor starts at reasonable output */
    ts->reactor.frequency = 0.0f;
    ts->reactor.speed = 1.5f;
    ts->reactor.player_adj = 0.0f;
    ts->reactor.output_pct = 70.0f;

    /* Bots: all locked initially, no bot for repairs */
    for (int i = 0; i < NUM_TASKS; i++) {
        ts->bots[i].unlocked = 0;
        ts->bots[i].active = 0;
        ts->bots[i].level = BOT_LVL_1;
        ts->bots[i].accuracy = bot_base_accuracy(i, BOT_LVL_1);
        ts->bots[i].energy_rate = BOT_ENERGY_COST_BASE;
        ts->bots[i].upgrade_cost = 200;
    }

    /* Nav: generate first waypoint */
    ts->nav.active = 0;
    ts->nav.timer = NAV_INTERVAL;
    ts->nav_drift = 0;

    /* Comms: first signal after delay */
    ts->comms.active = 0;
    ts->comms_cooldown = COMMS_INTERVAL * 0.5f;
    ts->comms.player_freq = 0.5f;
}

void tasks_unlock_bot(task_system_t *ts, int id) {
    if (id < 0 || id >= NUM_TASKS || id == TASK_REPAIRS) return;
    ts->bots[id].unlocked = 1;
    ts->bots[id].accuracy = bot_base_accuracy(id, ts->bots[id].level);
    printf("[BOT] %s Bot unlocked!\n", task_name(id));
}

int tasks_upgrade_bot(task_system_t *ts, int id) {
    if (id < 0 || id >= NUM_TASKS || id == TASK_REPAIRS) return 0;
    bot_t *b = &ts->bots[id];
    if (!b->unlocked) return 0;
    if (b->level >= BOT_MAX_LVL) return 0;
    if (ts->score < b->upgrade_cost) return 0;
    ts->score -= b->upgrade_cost;
    b->level++;
    b->accuracy = bot_base_accuracy(id, b->level);
    b->energy_rate = BOT_ENERGY_COST_BASE * (1.0f + b->level * 0.3f);
    b->upgrade_cost = (int)(b->upgrade_cost * 1.8f);
    printf("[BOT] %s Bot upgraded to Lv%d (%.0f%%)\n",
           task_name(id), b->level, b->accuracy * 100);
    return 1;
}

void tasks_spawn_breach(task_system_t *ts, float severity) {
    for (int i = 0; i < MAX_BREACHES; i++) {
        if (!ts->breaches[i].active) {
            ts->breaches[i].active = 1;
            ts->breaches[i].section = rand() % 4;
            ts->breaches[i].severity = severity;
            ts->breaches[i].repair_progress = 0;
            ts->breaches[i].being_repaired = 0;
            return;
        }
    }
}

/* --- Generate nav waypoint --- */
static void gen_nav_waypoint(task_system_t *ts) {
    ts->nav.active = 1;
    ts->nav.decoded = 0;
    ts->nav.current_idx = 0;
    ts->nav.scroll_pos = 0;
    for (int i = 0; i < NAV_SEQ_LEN; i++)
        ts->nav.sequence[i] = rand() % 4;
}

/* --- Generate comms signal --- */
static void gen_comms_signal(task_system_t *ts) {
    ts->comms.active = 1;
    ts->comms.locked = 0;
    ts->comms.target_freq = 0.15f + frand() * 0.7f;
    ts->comms.timer = 10.0f;
    ts->comms.quality = 0;
    ts->comms.reward_type = rand() % 4;
}

void tasks_update(task_system_t *ts, float dt) {
    if (ts->game_over) return;
    ts->time += dt;
    ts->wave_timer += dt;

    /* Wave progression (every 40 seconds) */
    if (ts->wave_timer > 40.0f) {
        ts->wave++;
        ts->wave_timer = 0;
        /* Unlock bots progressively */
        if (ts->wave == 2) tasks_unlock_bot(ts, TASK_DEFENSE);
        if (ts->wave == 3) tasks_unlock_bot(ts, TASK_REACTOR);
        if (ts->wave == 4) tasks_unlock_bot(ts, TASK_NAV);
        if (ts->wave == 5) tasks_unlock_bot(ts, TASK_COMMS);
    }

    /* --- Bot energy drain (shared resource!) --- */
    for (int i = 0; i < NUM_TASKS; i++) {
        if (ts->bots[i].active && ts->bots[i].unlocked) {
            ts->energy -= ts->bots[i].energy_rate * dt;
            if (ts->energy <= 0) {
                /* Out of energy: force-disable ALL bots */
                ts->energy = 0;
                for (int j = 0; j < NUM_TASKS; j++)
                    ts->bots[j].active = 0;
                break;
            }
        }
    }

    /* --- Reactor system (oscillating bar) --- */
    ts->reactor.frequency += ts->reactor.speed * dt;
    if (ts->reactor.frequency > 3.14159f)
        ts->reactor.frequency -= 6.28318f;
    ts->reactor.speed = 1.5f + ts->wave * 0.2f; /* gets faster */

    float reactor_val = sinf(ts->reactor.frequency) + ts->reactor.player_adj;
    reactor_val = clampf(reactor_val, -1.0f, 1.0f);
    float abs_val = fabsf(reactor_val);
    if (abs_val < 0.25f) ts->reactor.output_pct = 100.0f;
    else if (abs_val < 0.55f) ts->reactor.output_pct = 70.0f;
    else ts->reactor.output_pct = 40.0f;

    /* Reactor bot: tries to counteract */
    if (ts->bots[TASK_REACTOR].active) {
        float target = -sinf(ts->reactor.frequency);
        float bot_err = (1.0f - ts->bots[TASK_REACTOR].accuracy) * frand() * 0.4f;
        ts->reactor.player_adj += (target + bot_err - ts->reactor.player_adj) * 1.5f * dt;
    }

    /* Energy regen from reactor */
    ts->energy += ts->energy_regen * (ts->reactor.output_pct / 100.0f) * dt;
    ts->energy = clampf(ts->energy, 0, 100);

    /* --- Navigation system --- */
    if (!ts->nav.active) {
        ts->nav.timer -= dt;
        if (ts->nav.timer <= 0) {
            gen_nav_waypoint(ts);
            ts->nav.timer = NAV_INTERVAL;
        }
    } else {
        ts->nav.scroll_pos += 1.2f * dt;
        /* Nav bot: auto-decode */
        if (ts->bots[TASK_NAV].active && !ts->nav.decoded) {
            ts->bots[TASK_NAV].timer += dt;
            if (ts->bots[TASK_NAV].timer > 0.8f) {
                ts->bots[TASK_NAV].timer = 0;
                if (frand() < ts->bots[TASK_NAV].accuracy) {
                    ts->nav.current_idx++;
                    if (ts->nav.current_idx >= NAV_SEQ_LEN) {
                        ts->nav.decoded = 1;
                        ts->nav.active = 0;
                        ts->nav_drift -= 15.0f;
                        ts->score += 20;
                    }
                } else {
                    /* Wrong decode = more drift */
                    ts->nav_drift += 5.0f;
                }
            }
        }
        /* Timeout: missed waypoint */
        if (ts->nav.scroll_pos > NAV_SEQ_LEN + 2.0f && !ts->nav.decoded) {
            ts->nav.active = 0;
            ts->nav_drift += 20.0f;
        }
    }
    /* Drift causes damage */
    ts->nav_drift = clampf(ts->nav_drift, 0, 100);
    ts->nav_drift_dmg = ts->nav_drift * 0.03f;
    ts->hull -= ts->nav_drift_dmg * dt;
    ts->nav_drift -= 2.0f * dt; /* natural drift correction */

    /* --- Comms system --- */
    if (!ts->comms.active) {
        ts->comms_cooldown -= dt;
        if (ts->comms_cooldown <= 0) {
            gen_comms_signal(ts);
            ts->comms_cooldown = COMMS_INTERVAL;
        }
    } else {
        ts->comms.timer -= dt;
        float diff = fabsf(ts->comms.player_freq - ts->comms.target_freq);
        ts->comms.quality = 1.0f - clampf(diff * 5.0f, 0, 1);

        /* Comms bot: auto-tune */
        if (ts->bots[TASK_COMMS].active) {
            float dir = (ts->comms.target_freq > ts->comms.player_freq) ? 1.0f : -1.0f;
            float speed = 0.3f * ts->bots[TASK_COMMS].accuracy;
            float noise = (1.0f - ts->bots[TASK_COMMS].accuracy) * frand() * 0.1f;
            ts->comms.player_freq += (dir * speed + noise) * dt;
            ts->comms.player_freq = clampf(ts->comms.player_freq, 0, 1);
        }

        /* Signal locked? */
        if (ts->comms.quality > 0.85f) {
            ts->comms.locked = 1;
            ts->comms.active = 0;
            /* Rewards */
            switch (ts->comms.reward_type) {
                case 0: ts->score += 30 + (int)(ts->comms.quality * 20); break;
                case 1: ts->shields = clampf(ts->shields + 20, 0, 100); break;
                case 2: ts->energy = clampf(ts->energy + 15, 0, 100); break;
                case 3: /* Warning: slow asteroids for 5 sec - handled in main */ break;
            }
        }
        if (ts->comms.timer <= 0) {
            ts->comms.active = 0; /* missed signal */
        }
    }

    /* --- Repairs (manual only, no bot) --- */
    ts->hull_leak_rate = 0;
    for (int i = 0; i < MAX_BREACHES; i++) {
        if (!ts->breaches[i].active) continue;
        if (ts->breaches[i].being_repaired) {
            ts->breaches[i].repair_progress += dt;
            if (ts->breaches[i].repair_progress >= REPAIR_TIME) {
                ts->breaches[i].active = 0;
                ts->score += 25;
            }
        } else {
            ts->hull_leak_rate += ts->breaches[i].severity;
        }
    }
    ts->hull -= ts->hull_leak_rate * dt;

    /* Clamp resources */
    ts->shields = clampf(ts->shields, 0, 100);
    ts->hull = clampf(ts->hull, 0, 100);
    ts->weapon_heat -= 18.0f * dt;
    ts->weapon_heat = clampf(ts->weapon_heat, 0, 100);
    ts->fire_cooldown -= dt;
    if (ts->fire_cooldown < 0) ts->fire_cooldown = 0;
    /* v5.0 Visual Effect Decays */
    if (ts->shake > 0.1f) ts->shake *= 0.90f; /* Exponential decay */
    else ts->shake = 0;
    
    if (ts->hit_flash > 0) ts->hit_flash -= dt * 0.8f;
    if (ts->hit_flash < 0) ts->hit_flash = 0;

    /* Game over */
    if (ts->hull <= 0) {
        ts->hull = 0;
        ts->game_over = 1;
    }
}
