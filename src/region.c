/* NetHack 3.6	region.c	$NHDT-Date: 1579655029 2020/01/22 01:03:49 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.60 $ */
/* Copyright (c) 1996 by Jean-Christophe Collet  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

/*
 * This should really go into the level structure, but
 * I'll start here for ease. It *WILL* move into the level
 * structure eventually.
 */

NhRegion **regions;
int n_regions = 0;
int max_regions = 0;

#define NO_CALLBACK (-1)

boolean inside_gas_cloud(genericptr, genericptr);
boolean expire_gas_cloud(genericptr, genericptr);
boolean revive_cthulhu(genericptr, genericptr);
boolean inside_rect(NhRect *, coordxy, coordxy);
boolean inside_region(NhRegion *, coordxy, coordxy);
NhRegion *create_region(NhRect *, int);
void add_rect_to_reg(NhRegion *, NhRect *);
void add_mon_to_reg(NhRegion *, struct monst *);
void remove_mon_from_reg(NhRegion *, struct monst *);
boolean mon_in_region(NhRegion *, struct monst *);

#if 0
NhRegion *clone_region(NhRegion *);
#endif
void free_region(NhRegion *);
void add_region(NhRegion *);
void remove_region(NhRegion *);

#if 0
void replace_mon_regions(struct monst *, struct monst *);
void remove_mon_from_regions(struct monst *);
NhRegion *create_msg_region(coordxy, coordxy, coordxy, coordxy, const char *, const char *);
boolean enter_force_field(genericptr, genericptr);
NhRegion *create_force_field(coordxy, coordxy, int, int);
#endif

static void reset_region_mids(NhRegion *);

static callback_proc callbacks[] = {
#define INSIDE_GAS_CLOUD 0
    inside_gas_cloud,
#define EXPIRE_GAS_CLOUD 1
    expire_gas_cloud,
#define REVIVE_CTHULHU 2    /* Cthulhu comes back... */
    revive_cthulhu
};

/* Should be inlined. */
boolean
inside_rect(NhRect *r, coordxy x, coordxy y)
{
    return (x >= r->lx && x <= r->hx && y >= r->ly && y <= r->hy);
}

/*
 * Check if a point is inside a region.
 */
boolean
inside_region(NhRegion *reg, coordxy x, coordxy y)
{
    int i;

    if (reg == NULL || !inside_rect(&(reg->bounding_box), x, y)) {
        return FALSE;
    }
    for (i = 0; i < reg->nrects; i++) {
        if (inside_rect(&(reg->rects[i]), x, y)) {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Create a region. It does not activate it.
 */
NhRegion *
create_region(NhRect *rects, int nrect)
{
    int i;
    NhRegion *reg;

    reg = (NhRegion *) alloc(sizeof (NhRegion));
    (void) memset(reg, 0, sizeof(NhRegion));
    /* Determines bounding box */
    if (nrect > 0) {
        reg->bounding_box = rects[0];
    } else {
        reg->bounding_box.lx = COLNO;
        reg->bounding_box.ly = ROWNO;
        reg->bounding_box.hx = 0;
        reg->bounding_box.hy = 0;
    }
    reg->nrects = nrect;
    reg->rects = nrect > 0 ? (NhRect *)alloc((sizeof (NhRect)) * nrect) : NULL;
    for (i = 0; i < nrect; i++) {
        if (rects[i].lx < reg->bounding_box.lx) {
            reg->bounding_box.lx = rects[i].lx;
        }
        if (rects[i].ly < reg->bounding_box.ly) {
            reg->bounding_box.ly = rects[i].ly;
        }
        if (rects[i].hx > reg->bounding_box.hx) {
            reg->bounding_box.hx = rects[i].hx;
        }
        if (rects[i].hy > reg->bounding_box.hy) {
            reg->bounding_box.hy = rects[i].hy;
        }
        reg->rects[i] = rects[i];
    }
    reg->ttl = -1;      /* Defaults */
    reg->attach_2_u = FALSE;
    reg->attach_2_m = 0;
    /* reg->attach_2_o = NULL; */
    reg->enter_msg = NULL;
    reg->leave_msg = NULL;
    reg->expire_f = NO_CALLBACK;
    reg->enter_f = NO_CALLBACK;
    reg->can_enter_f = NO_CALLBACK;
    reg->leave_f = NO_CALLBACK;
    reg->can_leave_f = NO_CALLBACK;
    reg->inside_f = NO_CALLBACK;
    clear_hero_inside(reg);
    clear_heros_fault(reg);
    reg->n_monst = 0;
    reg->max_monst = 0;
    reg->monsters = NULL;
    reg->arg = zeroany;
    return reg;
}

/*
 * Add rectangle to region.
 */
void
add_rect_to_reg(NhRegion *reg, NhRect *rect)
{
    NhRect *tmp_rect;

    tmp_rect = (NhRect *) alloc(sizeof (NhRect) * (reg->nrects + 1));
    if (reg->nrects > 0) {
        (void) memcpy((genericptr_t) tmp_rect, (genericptr_t) reg->rects,
                      (sizeof (NhRect) * reg->nrects));
        free((genericptr_t) reg->rects);
    }
    tmp_rect[reg->nrects] = *rect;
    reg->nrects++;
    reg->rects = tmp_rect;
    /* Update bounding box if needed */
    if (reg->bounding_box.lx > rect->lx) {
        reg->bounding_box.lx = rect->lx;
    }
    if (reg->bounding_box.ly > rect->ly) {
        reg->bounding_box.ly = rect->ly;
    }
    if (reg->bounding_box.hx < rect->hx) {
        reg->bounding_box.hx = rect->hx;
    }
    if (reg->bounding_box.hy < rect->hy) {
        reg->bounding_box.hy = rect->hy;
    }
}

/*
 * Add a monster to the region
 */
void
add_mon_to_reg(NhRegion *reg, struct monst *mon)
{
    int i;
    unsigned *tmp_m;

    if (reg->max_monst <= reg->n_monst) {
        tmp_m = (unsigned *)
                alloc(sizeof (unsigned) * (reg->max_monst + MONST_INC));
        if (reg->max_monst > 0) {
            for (i = 0; i < reg->max_monst; i++) {
                tmp_m[i] = reg->monsters[i];
            }
            free((genericptr_t) reg->monsters);
        }
        reg->monsters = tmp_m;
        reg->max_monst += MONST_INC;
    }
    reg->monsters[reg->n_monst++] = mon->m_id;
}

/*
 * Remove a monster from the region list (it left or died...)
 */
void
remove_mon_from_reg(NhRegion *reg, struct monst *mon)
{
    int i;

    for (i = 0; i < reg->n_monst; i++) {
        if (reg->monsters[i] == mon->m_id) {
            reg->n_monst--;
            reg->monsters[i] = reg->monsters[reg->n_monst];
            return;
        }
    }
}

/*
 * Check if a monster is inside the region.
 * It's probably quicker to check with the region internal list
 * than to check for coordinates.
 */
boolean
mon_in_region(NhRegion *reg, struct monst *mon)
{
    int i;

    for (i = 0; i < reg->n_monst; i++) {
        if (reg->monsters[i] == mon->m_id) {
            return TRUE;
        }
    }

    return FALSE;
}

#if 0
/* not yet used */

/*
 * Clone (make a standalone copy) the region.
 */
NhRegion *
clone_region(reg)
NhRegion *reg;
{
    NhRegion *ret_reg;

    ret_reg = create_region(reg->rects, reg->nrects);
    ret_reg->ttl = reg->ttl;
    ret_reg->attach_2_u = reg->attach_2_u;
    ret_reg->attach_2_m = reg->attach_2_m;
    /* ret_reg->attach_2_o = reg->attach_2_o; */
    ret_reg->expire_f = reg->expire_f;
    ret_reg->enter_f = reg->enter_f;
    ret_reg->can_enter_f = reg->can_enter_f;
    ret_reg->leave_f = reg->leave_f;
    ret_reg->can_leave_f = reg->can_leave_f;
    ret_reg->player_flags = reg->player_flags;  /* set/clear_hero_inside,&c*/
    ret_reg->n_monst = reg->n_monst;
    if (reg->n_monst > 0) {
        ret_reg->monsters = (unsigned *)
                            alloc((sizeof (unsigned)) * reg->n_monst);
        (void) memcpy((genericptr_t) ret_reg->monsters, (genericptr_t) reg->monsters,
                      sizeof (unsigned) * reg->n_monst);
    } else {
        ret_reg->monsters = NULL;
    }
    return ret_reg;
}

#endif  /*0*/

/*
 * Free mem from region.
 */
void
free_region(NhRegion *reg)
{
    if (reg) {
        if (reg->rects) {
            free((genericptr_t) reg->rects);
        }
        if (reg->monsters) {
            free((genericptr_t) reg->monsters);
        }
        if (reg->enter_msg) {
            free((void *)reg->enter_msg);
        }
        if (reg->leave_msg) {
            free((void *)reg->leave_msg);
        }
        free((genericptr_t) reg);
    }
}

/*
 * Add a region to the list.
 * This actually activates the region.
 */
void
add_region(NhRegion *reg)
{
    NhRegion **tmp_reg;
    int i, j;

    if (max_regions <= n_regions) {
        tmp_reg = regions;
        regions = (NhRegion **)alloc(sizeof (NhRegion *) * (max_regions + 10));
        if (max_regions > 0) {
            (void) memcpy((genericptr_t) regions, (genericptr_t) tmp_reg,
                          max_regions * sizeof (NhRegion *));
            free((genericptr_t) tmp_reg);
        }
        max_regions += 10;
    }
    regions[n_regions] = reg;
    n_regions++;
    /* Check for monsters inside the region */
    for (i = reg->bounding_box.lx; i <= reg->bounding_box.hx; i++) {
        for (j = reg->bounding_box.ly; j <= reg->bounding_box.hy; j++) {
            /* Some regions can cross the level boundaries */
            if (!isok(i, j)) {
                continue;
            }
            if (MON_AT(i, j) && inside_region(reg, i, j)) {
                add_mon_to_reg(reg, level.monsters[i][j]);
            }
            if (reg->visible && cansee(i, j)) {
                newsym(i, j);
            }
        }
    }
    /* Check for player now... */
    if (inside_region(reg, u.ux, u.uy)) {
        set_hero_inside(reg);
    } else {
        clear_hero_inside(reg);
    }
}

/*
 * Remove a region from the list & free it.
 */
void
remove_region(NhRegion *reg)
{
    int i, x, y;

    for (i = 0; i < n_regions; i++) {
        if (regions[i] == reg) {
            break;
        }
    }
    if (i == n_regions) {
        return;
    }

    /* remove region before potential newsym() calls, but don't free it yet */
    if (--n_regions != i) {
        regions[i] = regions[n_regions];
    }

    regions[n_regions] = (NhRegion *) 0;
    /* Update screen if necessary */
    reg->ttl = -2L; /* for visible_region_at */
    if (reg->visible) {
        for (x = reg->bounding_box.lx; x <= reg->bounding_box.hx; x++) {
            for (y = reg->bounding_box.ly; y <= reg->bounding_box.hy; y++) {
                if (isok(x, y) && inside_region(reg, x, y) && cansee(x, y)) {
                    newsym(x, y);
                }
            }
        }
    }

    free_region(reg);
}

/*
 * Remove all regions and clear all related data (This must be down
 * when changing level, for instance).
 */
void
clear_regions(void)
{
    int i;

    for (i = 0; i < n_regions; i++) {
        free_region(regions[i]);
    }
    n_regions = 0;
    if (max_regions > 0) {
        free((genericptr_t) regions);
    }
    max_regions = 0;
    regions = NULL;
}

/*
 * This function is called every turn.
 * It makes the regions age, if necessary and calls the appropriate
 * callbacks when needed.
 */
void
run_regions(void)
{
    int i, j, k;
    int f_indx;

    /* End of life ? */
    /* Do it backward because the array will be modified */
    for (i = n_regions - 1; i >= 0; i--) {
        if (regions[i] == NULL) {
            abort();
        }
        if (regions[i]->ttl == 0) {
            if ((f_indx = regions[i]->expire_f) == NO_CALLBACK ||
                (*callbacks[f_indx])(regions[i], (genericptr_t) 0))
                remove_region(regions[i]);
        }
    }

    /* Process remaining regions */
    for (i = 0; i < n_regions; i++) {
        /* Make the region age */
        if (regions[i]->ttl > 0) {
            regions[i]->ttl--;
        }
        /* Check if player is inside region */
        f_indx = regions[i]->inside_f;
        if (f_indx != NO_CALLBACK && hero_inside(regions[i])) {
            (void) (*callbacks[f_indx])(regions[i], (genericptr_t) 0);
        }
        /* Check if any monster is inside region */
        if (f_indx != NO_CALLBACK) {
            for (j = 0; j < regions[i]->n_monst; j++) {
                struct monst *mtmp = find_mid(regions[i]->monsters[j], FM_FMON);

                if (!mtmp || mtmp->mhp <= 0 ||
                    (*callbacks[f_indx])(regions[i], mtmp)) {
                    /* The monster died, remove it from list */
                    k = (regions[i]->n_monst -= 1);
                    regions[i]->monsters[j] = regions[i]->monsters[k];
                    regions[i]->monsters[k] = 0;
                    --j; /* current slot has been reused; recheck it next */
                }
            }
        }
    }
}

/*
 * check whether player enters/leaves one or more regions.
 */
boolean
in_out_region(coordxy x, coordxy y)
{
    int i, f_indx = 0;

    /* First check if we can do the move */
    for (i = 0; i < n_regions; i++) {
        if (inside_region(regions[i], x, y) &&
             !hero_inside(regions[i]) &&
             !regions[i]->attach_2_u) {
            if ((f_indx = regions[i]->can_enter_f) != NO_CALLBACK) {
                if (!(*callbacks[f_indx])(regions[i], (genericptr_t) 0)) {
                    return FALSE;
                }
            }
        } else if (hero_inside(regions[i]) &&
                   !inside_region(regions[i], x, y) &&
                   !regions[i]->attach_2_u) {
            if ((f_indx = regions[i]->can_leave_f) != NO_CALLBACK) {
                if (!(*callbacks[f_indx])(regions[i], (genericptr_t) 0)) {
                    return FALSE;
                }
            }
        }
    }

    /* Callbacks for the regions we do leave */
    for (i = 0; i < n_regions; i++) {
        if (hero_inside(regions[i]) &&
            !regions[i]->attach_2_u && !inside_region(regions[i], x, y)) {
            clear_hero_inside(regions[i]);
            if (regions[i]->leave_msg != NULL) {
                pline("%s", regions[i]->leave_msg);
            }
            if ((f_indx = regions[i]->leave_f) != NO_CALLBACK) {
                (void) (*callbacks[f_indx])(regions[i], (genericptr_t) 0);
            }
        }
    }

    /* Callbacks for the regions we do enter */
    for (i = 0; i < n_regions; i++) {
        if (!hero_inside(regions[i]) &&
            !regions[i]->attach_2_u && inside_region(regions[i], x, y)) {
            set_hero_inside(regions[i]);
            if (regions[i]->enter_msg != NULL) {
                pline("%s", regions[i]->enter_msg);
            }
            if ((f_indx = regions[i]->enter_f) != NO_CALLBACK) {
                (void) (*callbacks[f_indx])(regions[i], (genericptr_t) 0);
            }
        }
    }

    return TRUE;
}

/*
 * check whether a monster enters/leaves one or more regions.
 */
boolean
m_in_out_region(struct monst *mon, coordxy x, coordxy y)
{
    int i, f_indx;

    /* First check if we can do the move */
    for (i = 0; i < n_regions; i++) {
        if (inside_region(regions[i], x, y) &&
            !mon_in_region(regions[i], mon) &&
            regions[i]->attach_2_m != mon->m_id) {
            if ((f_indx = regions[i]->can_enter_f) != NO_CALLBACK) {
                if (!(*callbacks[f_indx])(regions[i], mon)) {
                    return FALSE;
                }
            }
        } else if (mon_in_region(regions[i], mon) &&
                   !inside_region(regions[i], x, y) &&
                   regions[i]->attach_2_m != mon->m_id) {
            if ((f_indx = regions[i]->can_leave_f) != NO_CALLBACK) {
                if (!(*callbacks[f_indx])(regions[i], mon)) {
                    return FALSE;
                }
            }
        }
    }

    /* Callbacks for the regions we do leave */
    for (i = 0; i < n_regions; i++) {
        if (mon_in_region(regions[i], mon) &&
            regions[i]->attach_2_m != mon->m_id &&
            !inside_region(regions[i], x, y)) {
            remove_mon_from_reg(regions[i], mon);
            if ((f_indx = regions[i]->leave_f) != NO_CALLBACK) {
                (void) (*callbacks[f_indx])(regions[i], mon);
            }
        }
    }

    /* Callbacks for the regions we do enter */
    for (i = 0; i < n_regions; i++) {
        if (!hero_inside(regions[i]) &&
            !regions[i]->attach_2_u && inside_region(regions[i], x, y)) {
            add_mon_to_reg(regions[i], mon);
            if ((f_indx = regions[i]->enter_f) != NO_CALLBACK) {
                (void) (*callbacks[f_indx])(regions[i], mon);
            }
        }
    }

    return TRUE;
}

/*
 * Checks player's regions after a teleport for instance.
 */
void
update_player_regions(void)
{
    int i;

    for (i = 0; i < n_regions; i++) {
        if (!regions[i]->attach_2_u && inside_region(regions[i], u.ux, u.uy)) {
            set_hero_inside(regions[i]);
        } else {
            clear_hero_inside(regions[i]);
        }
    }
}

/*
 * Ditto for a specified monster.
 */
void
update_monster_region(struct monst *mon)
{
    int i;

    for (i = 0; i < n_regions; i++) {
        if (inside_region(regions[i], mon->mx, mon->my)) {
            if (!mon_in_region(regions[i], mon)) {
                add_mon_to_reg(regions[i], mon);
            }
        } else {
            if (mon_in_region(regions[i], mon)) {
                remove_mon_from_reg(regions[i], mon);
            }
        }
    }
}

#if 0
/* not yet used */

/*
 * Change monster pointer in regions
 * This happens, for instance, when a monster grows and
 * need a new structure (internally that is).
 */
void
replace_mon_regions(monold, monnew)
struct monst *monold, *monnew;
{
    int i;

    for (i = 0; i < n_regions; i++) {
        if (mon_in_region(regions[i], monold)) {
            remove_mon_from_reg(regions[i], monold);
            add_mon_to_reg(regions[i], monnew);
        }
    }
}

/*
 * Remove monster from all regions it was in (ie monster just died)
 */
void
remove_mon_from_regions(mon)
struct monst *mon;
{
    int i;

    for (i = 0; i < n_regions; i++) {
        if (mon_in_region(regions[i], mon)) {
            remove_mon_from_reg(regions[i], mon);
        }
    }
}

#endif  /*0*/

/*
 * Check if a spot is under a visible region (eg: gas cloud).
 * Returns NULL if not, otherwise returns region.
 */
NhRegion *
visible_region_at(coordxy x, coordxy y)
{
    int i;

    for (i = 0; i < n_regions; i++) {
        if (!regions[i]->visible || regions[i]->ttl <= 0) {
            continue;
        }
        if (inside_region(regions[i], x, y)) {
            return regions[i];
        }
    }
    return (NhRegion *) 0;
}

void
show_region(NhRegion *reg, coordxy x, coordxy y)
{
    show_glyph(x, y, reg->glyph);
}

/**
 * save_regions :
 */
void
save_regions(NHFILE *nhfp)
{
    NhRegion *r;
    int i, j;
    unsigned n;

    if (!perform_bwrite(nhfp)) {
        goto skip_lots;
    }
    if (nhfp->structlevel) {
        /* timestamp */
        bwrite(nhfp->fd, (genericptr_t) &moves, sizeof (moves));
        bwrite(nhfp->fd, (genericptr_t) &n_regions, sizeof (n_regions));
    }
    for (i = 0; i < n_regions; i++) {
        r = regions[i];
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &r->bounding_box, sizeof (NhRect));
            bwrite(nhfp->fd, (genericptr_t) &r->nrects, sizeof (short));
        }
        for (j = 0; j < r->nrects; j++) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) &r->rects[j], sizeof (NhRect));
            }
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &r->attach_2_u, sizeof (boolean));
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &r->attach_2_m, sizeof (unsigned));
        }

        n = !r->enter_msg ? 0U : (unsigned) strlen(r->enter_msg);
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &n, sizeof n);
        }
        if (n > 0) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) r->enter_msg, n);
            }
        }
        n = !r->leave_msg ? 0U : (unsigned) strlen(r->leave_msg);
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &n, sizeof n);
        }
        if (n > 0) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) r->leave_msg, n);
            }
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &r->ttl, sizeof (long));
            bwrite(nhfp->fd, (genericptr_t) &r->expire_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &r->can_enter_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &r->enter_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &r->can_leave_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &r->leave_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &r->inside_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &r->player_flags, sizeof (unsigned));
            bwrite(nhfp->fd, (genericptr_t) &r->n_monst, sizeof (short));
        }
        for (j = 0; j < r->n_monst; j++) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) &r->monsters[j], sizeof (unsigned));
            }
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &r->visible, sizeof (boolean));
            bwrite(nhfp->fd, (genericptr_t) &r->glyph, sizeof (int));
            bwrite(nhfp->fd, (genericptr_t) &r->arg, sizeof (anything));
        }
    }

 skip_lots:
    if (release_data(nhfp)) {
        clear_regions();
    }
}

void
rest_regions(NHFILE *nhfp)
{
    NhRegion *r;
    int i, j;
    unsigned n = 0;
    long tmstamp = 0L;
    char *msg_buf;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);

    clear_regions(); /* Just for security */
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &tmstamp, sizeof (tmstamp));
    }
    if (ghostly) {
        tmstamp = 0;
    } else {
        tmstamp = (moves - tmstamp);
    }

    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &n_regions, sizeof (n_regions));
    }

    max_regions = n_regions;
    if (n_regions > 0) {
        regions = (NhRegion **) alloc(sizeof (NhRegion *) * n_regions);
    }
    for (i = 0; i < n_regions; i++) {
        r = regions[i] = (NhRegion *) alloc(sizeof (NhRegion));
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &r->bounding_box, sizeof (NhRect));
            mread(nhfp->fd, (genericptr_t) &r->nrects, sizeof (short));
        }
        if (r->nrects > 0) {
            r->rects = (NhRect *) alloc(r->nrects * sizeof (NhRect));
        } else {
            r->rects = (NhRect *) 0;
        }
        for (j = 0; j < r->nrects; j++) {
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) &r->rects[j], sizeof (NhRect));
            }
        }
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &r->attach_2_u, sizeof (boolean));
            mread(nhfp->fd, (genericptr_t) &r->attach_2_m, sizeof (unsigned));
        }

        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &n, sizeof n);
        }
        if (n > 0) {
            msg_buf = (char *) alloc(n + 1);
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) msg_buf, n);
            }
            msg_buf[n] = '\0';
        } else {
            msg_buf = (char *) 0;
        }
        r->enter_msg = (const char *) msg_buf;

        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &n, sizeof n);
        }
        if (n > 0) {
            msg_buf = (char *) alloc(n + 1);
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) msg_buf, n);
            }
            msg_buf[n] = '\0';
        } else {
            msg_buf = (char *) 0;
        }
         r->leave_msg = (const char *) msg_buf;

        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &r->ttl, sizeof (long));
        }
        /* check for expired region */
        if (r->ttl >= 0L) {
            r->ttl = (r->ttl > tmstamp) ? r->ttl - tmstamp : 0L;
        }
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &r->expire_f, sizeof (short));
            mread(nhfp->fd, (genericptr_t) &r->can_enter_f, sizeof (short));
            mread(nhfp->fd, (genericptr_t) &r->enter_f, sizeof (short));
            mread(nhfp->fd, (genericptr_t) &r->can_leave_f, sizeof (short));
            mread(nhfp->fd, (genericptr_t) &r->leave_f, sizeof (short));
            mread(nhfp->fd, (genericptr_t) &r->inside_f, sizeof (short));
            mread(nhfp->fd, (genericptr_t) &r->player_flags, sizeof (unsigned));
        }
        if (ghostly) { /* settings pertained to old player */
            clear_hero_inside(r);
            clear_heros_fault(r);
        }
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &r->n_monst, sizeof (short));
        }
        if (r->n_monst > 0) {
            r->monsters = (unsigned *) alloc(r->n_monst * sizeof (unsigned));
        } else {
            r->monsters = (unsigned *) 0;
        }
        r->max_monst = r->n_monst;
        for (j = 0; j < r->n_monst; j++) {
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) &r->monsters[j], sizeof (unsigned));
            }
        }
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &r->visible, sizeof (boolean));
            mread(nhfp->fd, (genericptr_t) &r->glyph, sizeof (int));
            mread(nhfp->fd, (genericptr_t) &r->arg, sizeof (anything));
        }
    }
    /* remove expired regions, do not trigger the expire_f callback (yet!);
       also update monster lists if this data is coming from a bones file */
    for (i = n_regions - 1; i >= 0; i--) {
        r = regions[i];
        if (r->ttl == 0L) {
            remove_region(r);
        } else if (ghostly && r->n_monst > 0) {
            reset_region_mids(r);
        }
    }
}

/* to support '#stats' wizard-mode command */
void
region_stats(const char *hdrfmt, char *hdrbuf, long int *count, long int *size)
{
    NhRegion *rg;
    int i;

    /* other stats formats take one parameter; this takes two */
    Sprintf(hdrbuf, hdrfmt, (long) sizeof (NhRegion), (long) sizeof (NhRect));
    *count = (long) n_regions; /* might be 0 even though max_regions isn't */
    *size = (long) max_regions * (long) sizeof (NhRegion);
    for (i = 0; i < n_regions; ++i) {
        rg = regions[i];
        *size += (long) rg->nrects * (long) sizeof (NhRect);
        if (rg->enter_msg) {
            *size += (long) (strlen(rg->enter_msg) + 1);
        }
        if (rg->leave_msg) {
            *size += (long) (strlen(rg->leave_msg) + 1);
        }
        *size += (long) rg->max_monst * (long) sizeof *rg->monsters;
    }
    /* ? */
}

/* update monster IDs for region being loaded from bones; `ghostly' implied */
static void
reset_region_mids(NhRegion *reg)
{
    int i = 0, n = reg->n_monst;
    unsigned *mid_list = reg->monsters;

    while (i < n)
        if (!lookup_id_mapping(mid_list[i], &mid_list[i])) {
            /* shrink list to remove missing monster; order doesn't matter */
            mid_list[i] = mid_list[--n];
        } else {
            /* move on to next monster */
            ++i;
        }
    reg->n_monst = n;
    return;
}

#if 0
/* not yet used */

/*--------------------------------------------------------------*
*                               *
*           Create Region with just a message   *
*                               *
*--------------------------------------------------------------*/

NhRegion *
create_msg_region(x, y, w, h, msg_enter, msg_leave)
coordxy x, y;
coordxy w, h;
const char *msg_enter;
const char *msg_leave;
{
    NhRect tmprect;
    NhRegion *reg = create_region((NhRect *) 0, 0);

    if (msg_enter) {
        reg->enter_msg = dupstr(msg_enter);
    }
    if (msg_leave) {
        reg->leave_msg = dupstr(msg_leave);
    }
    tmprect.lx = x;
    tmprect.ly = y;
    tmprect.hx = x + w;
    tmprect.hy = y + h;
    add_rect_to_reg(reg, &tmprect);
    reg->ttl = -1;
    return reg;
}


/*--------------------------------------------------------------*
*                               *
*           Force Field Related Code        *
*           (unused yet)                *
*--------------------------------------------------------------*/

boolean
enter_force_field(p1, p2)
genericptr_t p1;
genericptr_t p2;
{
    struct monst *mtmp;

    if (p2 == NULL) {       /* That means the player */
        if (!Blind) {
            You("bump into %s. Ouch!",
                Hallucination ? "an invisible tree" :
                "some kind of invisible wall");
        } else {
            pline("Ouch!");
        }
    } else {
        mtmp = (struct monst *) p2;
        if (canseemon(mtmp)) {
            pline("%s bumps into %s!", Monnam(mtmp), something);
        }
    }
    return FALSE;
}

NhRegion *
create_force_field(x, y, radius, ttl)
coordxy x, y;
int radius, ttl;
{
    int i;
    NhRegion *ff;
    int nrect;
    NhRect tmprect;

    ff = create_region((NhRect *) 0, 0);
    nrect = radius;
    tmprect.lx = x;
    tmprect.hx = x;
    tmprect.ly = y - (radius - 1);
    tmprect.hy = y + (radius - 1);
    for (i = 0; i < nrect; i++) {
        add_rect_to_reg(ff, &tmprect);
        tmprect.lx--;
        tmprect.hx++;
        tmprect.ly++;
        tmprect.hy--;
    }
    ff->ttl = ttl;
    if (!in_mklev && !flags.mon_moving) {
        set_heros_fault(ff);    /* assume player has created it */
    }
    /* ff->can_enter_f = enter_force_field; */
    /* ff->can_leave_f = enter_force_field; */
    add_region(ff);
    return ff;
}

#endif  /*0*/

/*--------------------------------------------------------------*
*                               *
*           Gas cloud related code          *
*                               *
*--------------------------------------------------------------*/

/*
 * Here is an example of an expire function that may prolong
 * region life after some mods...
 */
boolean
expire_gas_cloud(genericptr_t p1, void *p2 UNUSED)
{
    NhRegion *reg;
    size_t damage;

    reg = (NhRegion *) p1;
    damage = reg->arg.a_int;

    /* If it was a thick cloud, it dissipates a little first */
    if (damage >= 5) {
        damage /= 2;    /* It dissipates, let's do less damage */
        reg->arg = zeroany;
        reg->arg.a_int = damage;
        reg->ttl = 2;   /* Here's the trick : reset ttl */
        return FALSE;   /* THEN return FALSE, means "still there" */
    }
    return TRUE;        /* OK, it's gone, you can free it! */
}

boolean
revive_cthulhu(genericptr_t p1, genericptr_t p2)
{
    boolean ret = expire_gas_cloud(p1, p2);
    if (ret) {
        /* Bring back Cthulhu! */
        int cx, cy;
        NhRegion *reg = (NhRegion *) p1;
        struct monst *cthulhu = NULL;
        coord cc;

        cx = (reg->bounding_box.lx + reg->bounding_box.hx) / 2;
        cy = (reg->bounding_box.ly + reg->bounding_box.hy) / 2;

        if (enexto(&cc, cx, cy, &mons[PM_CTHULHU])) {
            cx = cc.x;
            cy = cc.y;
        } else {
            cx = cy = 0; /* Place Cthulhu randomly */
        }

        /* Make sure Cthulhu doesn't get the Amulet again! :-) */
        cthulhu = makemon(&mons[PM_CTHULHU], cx, cy,
                          MM_NOCOUNTBIRTH | NO_MINVENT);
        if (cthulhu) {
            if (canseemon(cthulhu)) {
                pline("%s reforms!", Monnam(cthulhu));
            }
            /* don't let Cthulhu meditate after being killed once
             * by the player */
            wakeup(cthulhu, TRUE);
        }

    }
    return ret;
}

boolean
inside_gas_cloud(genericptr_t p1, genericptr_t p2)
{
    NhRegion *reg;
    struct monst *mtmp;
    int dam;

    reg = (NhRegion *) p1;
    dam = reg->arg.a_int;
    if (p2 == NULL) {       /* This means *YOU* Bozo! */
        u.incloud = TRUE;
        if (nonliving(youmonst.data) || Breathless) {
            return FALSE;
        }
        if (!Blind) {
            make_blinded(1L, FALSE);
        }
        if (u.uinvulnerable) {
            return FALSE;
        }
        if (!Poison_resistance) {
            pline("%s is burning your %s!", Something, makeplural(body_part(LUNG)));
            You("cough and spit blood!");
            dam = Maybe_Half_Phys(rnd(dam) + 5);
            if (Half_gas_damage) {
                /* worn towel */
                dam = (dam + 1) / 2;
            }
            losehp(dam, "gas cloud", KILLED_BY_AN);
            return FALSE;
        } else {
            /* You("cough!"); */
            return FALSE;
        }
    } else {            /* A monster is inside the cloud */
        mtmp = (struct monst *) p2;

        /* Non living, non breathing, aquatic, and
           poison-resistant monsters are not concerned */
        if (!nonliving(mtmp->data) && !breathless(mtmp->data) &&
            !amphibious(mtmp->data) && !resists_poison(mtmp)) {
            if (cansee(mtmp->mx, mtmp->my)) {
                pline("%s coughs!", Monnam(mtmp));
            }
            if (heros_fault(reg)) {
                setmangry(mtmp, TRUE);
            }
            if (haseyes(mtmp->data) && mtmp->mcansee) {
                mtmp->mblinded = 1;
                mtmp->mcansee = 0;
            }
            if (resists_poison(mtmp)) {
                return FALSE;
            }
            mtmp->mhp -= rnd(dam) + 5;
            if (mtmp->mhp <= 0) {
                if (heros_fault(reg)) {
                    killed(mtmp);
                } else {
                    monkilled(mtmp, "gas cloud", AD_DRST);
                }
                if (mtmp->mhp <= 0) { /* not lifesaved */
                    return TRUE;
                }
            }
        }
    }
    return FALSE;       /* Monster is still alive */
}

NhRegion *
create_cthulhu_death_cloud(coordxy x, coordxy y, int radius, size_t damage, int duration)
{
    NhRegion *cloud;

    cloud = create_gas_cloud(x, y, radius, damage, duration);
    if (cloud) {
        cloud->expire_f = REVIVE_CTHULHU;
    }

    return cloud;
}

static boolean
is_hero_inside_gas_cloud(void)
{
    int i;

    for (i = 0; i < n_regions; i++) {
        if (hero_inside(regions[i]) &&
             regions[i]->inside_f == INSIDE_GAS_CLOUD) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Create a gas cloud which starts at (x,y) and grows outward from it via
 * breadth-first search.
 * cloudsize is the number of squares the cloud will attempt to fill.
 * damage is how much it deals to afflicted creatures. */
#define MAX_CLOUD_SIZE 150
NhRegion *
create_gas_cloud(coordxy x, coordxy y, int cloudsize, int damage, int duration)
{
    NhRegion *cloud;
    int i, j;
    NhRect tmprect;

    /* store visited coords */
    coordxy xcoords[MAX_CLOUD_SIZE];
    coordxy ycoords[MAX_CLOUD_SIZE];
    xcoords[0] = x;
    ycoords[0] = y;
    int curridx;
    int newidx = 1; /* initial spot is already taken */
    boolean inside_cloud = is_hero_inside_gas_cloud();

    /* a single-point cloud on hero and it deals no damage.
       probably a natural cause of being polyed. don't message about it */
    if (!flags.mon_moving && u_at(x, y) && cloudsize == 1 &&
         (!damage || (damage && m_poisongas_ok(&youmonst) == M_POISONGAS_OK))) {
        inside_cloud = TRUE;
    }

    if (cloudsize > MAX_CLOUD_SIZE) {
        impossible("create_gas_cloud: cloud too large (%d)!", cloudsize);
        cloudsize = MAX_CLOUD_SIZE;
    }

    for (curridx = 0; curridx < newidx; curridx++) {
        if (newidx >= cloudsize) {
            break;
        }
        int xx = xcoords[curridx];
        int yy = ycoords[curridx];
        /* Do NOT check for if there is already a gas cloud created at some
         * other time at this position. They can overlap. */

        /* Primitive Fisher-Yates-Knuth shuffle to randomize the order of
         * directions chosen. */
        coord dirs[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
        for (i = 4; i > 0; --i) {
            coordxy swapidx = rn2(i);
            coord tmp = dirs[swapidx];
            dirs[swapidx] = dirs[i-1];
            dirs[i-1] = tmp;
        }
        int nvalid = 0; /* # of valid adjacent spots */
        for (i = 0; i < 4; ++i) {
            /* try all 4 directions */

            int dx = dirs[i].x, dy = dirs[i].y;
            boolean isunpicked = TRUE;

            if (valid_cloud_pos(xx + dx, yy + dy)) {
                nvalid++;
                /* don't pick a location we've already picked */
                for (j = 0; j < newidx; ++j) {
                    if (xcoords[j] == xx + dx && ycoords[j] == yy + dy) {
                        isunpicked = FALSE;
                        break;
                    }
                }
                /* randomly disrupt the natural breadth-first search, so that
                 * clouds released in open spaces don't always tend towards a
                 * rhombus shape */
                if (nvalid == 4 && !rn2(2)) {
                    continue;
                }

                if (isunpicked) {
                    xcoords[newidx] = xx + dx;
                    ycoords[newidx] = yy + dy;
                    newidx++;
                }
            }
            if (newidx >= cloudsize) {
                /* don't try further directions */
                break;
            }
        }
    }
    /* We have now either filled up xcoord and ycoord entirely or run out
       of space.  In either case, newidx is the correct total number of
       coordinates inserted. */
    cloud = create_region((NhRect *) 0, 0);
    for (i = 0; i < newidx; ++i) {
        tmprect.lx = tmprect.hx = xcoords[i];
        tmprect.ly = tmprect.hy = ycoords[i];
        add_rect_to_reg(cloud, &tmprect);
    }
    cloud->ttl = duration;
    /* If cloud was constrained in small space, give it more time to live. */
    cloud->ttl = (cloud->ttl * cloudsize) / newidx;

    if (!in_mklev && !flags.mon_moving) {
        set_heros_fault(cloud); /* assume player has created it */
    }
    cloud->inside_f = INSIDE_GAS_CLOUD;
    cloud->expire_f = EXPIRE_GAS_CLOUD;
    cloud->arg = cg.zeroany;
    cloud->arg.a_int = damage;
    cloud->visible = TRUE;
    cloud->glyph = cmap_to_glyph(damage ? S_poisoncloud : S_cloud);
    add_region(cloud);

    if (!in_mklev && !inside_cloud && is_hero_inside_gas_cloud()) {
        You("are enveloped in a cloud of %s!",
            damage ? "noxious gas" : "steam");
    }

    return cloud;
}

/* create a single gas cloud from selection */
NhRegion *
create_gas_cloud_selection(struct selectionvar *sel, int damage)
{
    NhRegion *cloud;
    NhRect tmprect;
    coordxy x, y;
    NhRect r = cg.zeroNhRect;
    boolean inside_cloud = is_hero_inside_gas_cloud();

    selection_getbounds(sel, &r);

    cloud = create_region((NhRect *) 0, 0);
    for (x = r.lx; x <= r.hx; x++) {
        for (y = r.ly; y <= r.hy; y++) {
            if (selection_getpoint(x, y, sel)) {
                tmprect.lx = tmprect.hx = x;
                tmprect.ly = tmprect.hy = y;
                add_rect_to_reg(cloud, &tmprect);
            }
        }
    }

    if (!in_mklev && !flags.mon_moving) {
        set_heros_fault(cloud); /* assume player has created it */
    }
    cloud->inside_f = INSIDE_GAS_CLOUD;
    cloud->expire_f = EXPIRE_GAS_CLOUD;
    cloud->arg = cg.zeroany;
    cloud->arg.a_int = damage;
    cloud->visible = TRUE;
    cloud->glyph = cmap_to_glyph(damage ? S_poisoncloud : S_cloud);
    add_region(cloud);

    if (!in_mklev && !inside_cloud && is_hero_inside_gas_cloud()) {
        You("are enveloped in a cloud of %s!",
            damage ? "noxious gas" : "steam");
    }
    return cloud;
}

/* for checking troubles during prayer; is hero at risk? */
boolean
region_danger(void)
{
    int i, f_indx, n = 0;

    for (i = 0; i < n_regions; i++) {
        /* only care about regions that hero is in */
        if (!hero_inside(regions[i])) {
            continue;
        }
        f_indx = regions[i]->inside_f;
        /* the only type of region we understand is gas_cloud */
        if (f_indx == INSIDE_GAS_CLOUD) {
            /* completely harmless if you don't need to breathe */
            if (nonliving(youmonst.data) || Breathless) {
                continue;
            }
            /* minor inconvenience if you're poison resistant;
               not harmful enough to be a prayer-level trouble */
            if (Poison_resistance) {
                continue;
            }
            ++n;
        }
    }
    return n ? TRUE : FALSE;
}

/* for fixing trouble at end of prayer;
   danger detected at start of prayer might have expired by now */
void
region_safety(void)
{
    NhRegion *r = 0;
    int i, f_indx, n = 0;

    for (i = 0; i < n_regions; i++) {
        /* only care about regions that hero is in */
        if (!hero_inside(regions[i])) {
            continue;
        }
        f_indx = regions[i]->inside_f;
        /* the only type of region we understand is gas_cloud */
        if (f_indx == INSIDE_GAS_CLOUD) {
            if (!n++ && regions[i]->ttl >= 0) {
                r = regions[i];
            }
        }
    }

    if (n > 1 || (n == 1 && !r)) {
        /* multiple overlapping cloud regions or non-expiring one */
        safe_teleds(FALSE);
    } else if (r) {
        remove_region(r);
        pline_The("gas cloud enveloping you dissipates.");
    } else {
        /* cloud dissipated on its own, so nothing needs to be done */
        pline_The("gas cloud has dissipated.");
    }
    /* maybe cure blindness too */
    if ((Blinded & TIMEOUT) == 1L) {
        make_blinded(0L, TRUE);
    }
}

/*region.c*/
