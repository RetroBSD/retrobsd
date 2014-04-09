 /*
  * Copyright (C) yajin 2008<yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __VP_TIMER_H__
#define __VP_TIMER_H__

#include "vp_clock.h"
#include "utils.h"

#define VP_TIMER_BASE 1000000000LL

typedef void vp_timer_cb (void *opaque);

struct vp_timer {
    vp_clock_t *clock;
    m_int64_t expire_time;
    m_int64_t set_time;
    vp_timer_cb *cb;
    void *opaque;
    struct vp_timer *next;
};
typedef struct vp_timer vp_timer_t;
extern vp_timer_t *active_timers[2];

vp_timer_t *vp_new_timer (vp_clock_t * clock, vp_timer_cb * cb, void *opaque);
void vp_free_timer (vp_timer_t * ts);
void vp_mod_timer (vp_timer_t * ts, m_int64_t expire_time);
void vp_del_timer (vp_timer_t * ts);
int vp_timer_pending (vp_timer_t * ts);
int vp_timer_expired (vp_timer_t * timer_head, m_int64_t current_time);
void vp_run_timers (vp_timer_t ** ptimer_head, m_int64_t current_time);
void init_timers (void);

#endif
