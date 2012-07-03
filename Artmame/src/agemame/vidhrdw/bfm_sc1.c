/*************************************************************************************

    bfm_sc1.c


    A.G.E Code Copyright (c) 2004-2007 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org

    Layout notes: In accordance with the AWPVid guidelines, the Adder screen takes
    priority at screen 0, with the VFDs following afterwards.
*************************************************************************************/
#include "vidhrdw/awpvid.h"
#include "vidhrdw/bfm_adr2.h"
#include "machine/vacfdisp.h"
#include "render.h"

VIDEO_RESET( adder2awp )
{
	video_reset_adder2(machine);
}

VIDEO_START( adder2awp )
{
	video_start_adder2(machine);
	generic_awp_setup(0);
	return 0;
}

VIDEO_UPDATE( adder2awp )
{
	int x;
	if (screen == 0)
	{
		adder2_update(bitmap);
	}
	if (screen == 1)
	{
		for ( x = 0; x < totalreels; x++ )
		{
			draw_reel(x,steps[x],symbols[x]);
		}
		awp_lamp_draw();
		if (!vfdcol)
		{
			draw_16seg(bitmap,0,1,9);
		}
		else
		{			
			awp_vfd_setup(0,bitmap);
		}
	}
	return 0;
}
