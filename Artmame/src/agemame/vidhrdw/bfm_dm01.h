/************************************************************************************

    bfm_dm01.h

    Bellfruit dotmatrix driver, (under heavy construction !!!)

    19-08-2005: Re-Animator
    25-08-2005: Fixed feedback through sc2 input line

    A.G.E Code Copyright (c) 2004-2007 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org

*************************************************************************************/
#ifndef INC_BFMDM01
#define INC_BFMDM01

ADDRESS_MAP_EXTERN( bfm_dm01_memmap );

INTERRUPT_GEN( bfm_dm01_vbl );

VIDEO_START(  bfm_dm01 );
VIDEO_UPDATE( bfm_dm01 );

void BFM_dm01_reset(void);

void BFM_dm01_writedata(UINT8 data);

int BFM_dm01_busy(void);


struct mame_bitmap *BFM_dm01_getBitmap(void);

#endif
