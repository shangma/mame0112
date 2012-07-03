/*************************************************************************************

    awpvid.c

    AWP Hardware video simulation system

    A.G.E Code Copyright (C) 2004-2007 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


*************************************************************************************/
#ifndef INC_AWP_VIDEO
#define INC_AWP_VIDEO
#include "machine/lamps.h"
#include "machine/steppers.h"

extern const char layout_awpvid[];	/* main layout positioning 6 reels, a lamp matrix and a VFD */
extern void draw_reel(int rno, int rsteps, int symbols);

extern void generic_awp_setup(int screen);
extern void awp_vfd_setup(int vfd,mame_bitmap *bitmap);
extern void awp_reel_setup(int totalreels);

extern void awp_lamp_draw(void);

extern UINT8 fade[MAX_LAMPS];
extern UINT8 steps[MAX_STEPPERS];
extern UINT8 symbols[MAX_STEPPERS];
extern UINT8 reelpos[MAX_STEPPERS];
extern UINT8 fadeenable, vfdcol, totalreels;

VIDEO_START( awpvideo );
VIDEO_UPDATE( awpvideo );
PALETTE_INIT( awpvideo );

#endif
