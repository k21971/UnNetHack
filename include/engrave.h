/*  SCCS Id: @(#)engrave.h  3.4 1991/07/31  */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ENGRAVE_H
#define ENGRAVE_H

struct engr {
    struct engr *nxt_engr;
    char *engr_txt;
    coordxy engr_x, engr_y;
    unsigned engr_lth;  /* for save & restore; not length of text */
    long engr_time;     /* moment engraving was (will be) finished */
    xint8 engr_type;
#define DUST       1
#define ENGRAVE    2
#define BURN       3
#define MARK       4
#define ENGR_BLOOD 5
#define HEADSTONE  6
#define N_ENGRAVE  6
    Bitfield(guardobjects, 1); /* if engr_txt is "Elbereth", it is effective
                                * against monsters when an object is present
                                * even when hero isn't (so behaves similarly
                                * to how Elbereth did in 3.4.3) */
    Bitfield(nowipeout, 1);    /* this engraving will not degrade */
    Bitfield(eread, 1);        /* the engraving text has been read or felt */
    /* 5 free bits */
};

#define newengr(lth) (struct engr *)alloc((unsigned)(lth) + sizeof(struct engr))
#define dealloc_engr(engr) free((genericptr_t) (engr))

#endif /* ENGRAVE_H */
