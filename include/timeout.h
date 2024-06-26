/*  SCCS Id: @(#)timeout.h  3.4 1999/02/13  */
/* Copyright 1994, Dean Luick                     */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TIMEOUT_H
#define TIMEOUT_H

/* generic timeout function */
typedef void (*timeout_proc)(ANY_P *, long);

/* kind of timer */
enum timer_type {
    TIMER_NONE = 0,
    TIMER_LEVEL = 1,   /* event specific to level [melting ice] */
    TIMER_GLOBAL = 2,  /* event follows current play [not used] */
    TIMER_OBJECT = 3,  /* event follows an object [various] */
    TIMER_MONSTER = 4, /* event follows a monster [not used] */
    NUM_TIMER_KINDS    /* 5 */
};

/* save/restore timer ranges */
#define RANGE_LEVEL  0      /* save/restore timers staying on level */
#define RANGE_GLOBAL 1      /* save/restore timers following global play */

/*
 * Timeout functions.  Add a define here, then put it in the table
 * in timeout.c.  "One more level of indirection will fix everything."
 */
enum timeout_types {
    ROT_ORGANIC = 0, /* for buried organics */
    ROT_CORPSE,
    REVIVE_MON,
    ZOMBIFY_MON,
    BURN_OBJECT,
    HATCH_EGG,
    FIG_TRANSFORM,
    MELT_ICE_AWAY,

    NUM_TIME_FUNCS
};

#define timer_is_pos(ttype) ((ttype) == MELT_ICE_AWAY)
#define timer_is_obj(ttype) ((ttype) == ROT_ORGANIC   \
                          || (ttype) == ROT_CORPSE    \
                          || (ttype) == REVIVE_MON    \
                          || (ttype) == ZOMBIFY_MON   \
                          || (ttype) == BURN_OBJECT   \
                          || (ttype) == HATCH_EGG     \
                          || (ttype) == FIG_TRANSFORM)

/* used in timeout.c */
typedef struct fe {
    struct fe *next;           /* next item in chain */
    long timeout;              /* when we time out */
    unsigned long tid;         /* timer ID */
    short kind;                /* kind of use */
    short func_index;          /* what to call when we time out */
    anything arg;          /* pointer to timeout argument */
    Bitfield (needs_fixup, 1); /* does arg need to be patched? */
} timer_element;

#endif /* TIMEOUT_H */
