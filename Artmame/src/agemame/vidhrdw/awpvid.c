/*************************************************************************************

    awpvid.c

    AWP Hardware video simulation system

    A.G.E Code Copyright (C) 2004-2007 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


**************************************************************************************
    NOTE: Fading lamp system currently only recognises three lamp states:

    0=off, 1, 2= fully on

    Based on evidence, newer techs may need more states, but we can give them their own
    handlers at some stage in the distant future.


   Instructions:
   In order to set up displays (VFDs, dot matrices, etc) we normally set up the unique
   displays first, and then add the remainder in order.

   Everything starts with screen 0, which will be a VFD, unless another display type is
   present, in which case that will be first. The priorities (in descending order) are:

   Full video screens
   Dot matrix displays
   VFDs

   Currently, only Scorpion 1 uses more than one display type.
**************************************************************************************/
#include "driver.h"
#include "ui.h"
#include "rendlay.h"
#include "machine/lamps.h"
#include "machine/steppers.h"
#include "machine/vacfdisp.h"
#include "awpvid.lh"

UINT8 fade[MAX_LAMPS];
UINT8 steps[MAX_STEPPERS];
UINT8 symbols[MAX_STEPPERS];
UINT8 reelpos[MAX_STEPPERS];
UINT8 fadeenable, vfdcol, totalreels;

void awp_vfd_setup(int vfd, mame_bitmap *bitmap)
{
	draw_14seg(bitmap,0,16+vfd,17+vfd);
}

void awp_reel_setup(int totalreels)
{
	int x;
	char rstep[10],rsym[10];
		for ( x = 0; x < totalreels; x++ )
		{
			sprintf(rstep, "ReelSteps%d",x+1);
			sprintf(rsym, "ReelSymbols%d",x+1);

			if (!output_get_value(rstep))
			{
				steps[x] = 6 ;
			}
			else
			{
				steps[x] = output_get_value(rstep);
			}

			if (!output_get_value(rsym))
			{
				symbols[x] = 1 ;
			}
			else
			{
				symbols[x] = output_get_value(rsym);
			}
		}
}
static void fade_control(int lamp)
{
	UINT8 state,power;
	power=Lamps_GetBrightness(lamp);
	state=output_get_lamp_value(lamp);

	if (power)
	{
		switch (state)
		{
			case 0:
			case 1:
			state = state + 1;
			break;
		}
	}	

	if (!power)
	{
		switch (state)
		{
			case 2:
			case 1:
			state = state - 1;
			break;
		}
	}
	output_set_lamp_value(lamp,(state));
}

void awp_lamp_draw(void)
{
	int i,nrlamps;

	nrlamps = Lamps_GetNumberLamps();
	for ( i = 0; i < (nrlamps+1); i++ )
	{
		if(!fadeenable)
		{
			output_set_lamp_value(i,Lamps_GetBrightness(i));
		}
		else
		{
			fade_control(i);
		}
	}
}

//            Reel No. Steps between    No. of 
//                   displayed symbols Symbols
void draw_reel(int rno, int rsteps, int symbols)
{
	int m;
	int x = rno + 1;
	char rg[10], rga[10], rgb[10];

	sprintf(rg,"reel%d", x);
	reelpos[rno] = Stepper_get_position(rno);
	if (reelpos[rno] == output_get_value(rg))
	{
		// Not moved, no need to update.
	}
	else
	{
		for ( m = 0; m < (symbols-1); m++ )
		{
			if ((Stepper_get_position(rno) + rsteps * m) > Stepper_get_max(rno))
			{
				sprintf(rga,"reel%da%d", x, m);
				output_set_value(rga,(Stepper_get_position(rno) + rsteps * m - Stepper_get_max(rno)));
			}
		else
			{
				sprintf(rga,"reel%da%d", x, m);
				output_set_value(rga,(Stepper_get_position(rno) + rsteps * m));
			}
			if ((Stepper_get_position(rno) - rsteps * m) < 0)
			{
				sprintf(rgb,"reel%db%d", x, m);
				output_set_value(rgb,(Stepper_get_position(rno) - (rsteps * m - Stepper_get_max(rno))));
			}
		else
			{
				sprintf(rgb,"reel%db%d", x, m);
				output_set_value(rgb,(Stepper_get_position(rno) - rsteps * m));
			}
		}
		sprintf(rg,"reel%d", x);
		output_set_value(rg,(Stepper_get_position(rno)));
	}
}

void generic_awp_setup(int screen)
{
	vfdcol = (output_get_value("VFDR11")|output_get_value("VFDG11")|output_get_value("VFDB11"))? 1:0;

	fadeenable = (output_get_value("FadeLamps"))? 1:0;

	//This is a fairly nasty hack (wouldn't say it was ghastly, though).
	//We need to set up the pen colours to match the VFD output, since we can't directly set it up in the layouts.
	//In theory, we could draw a white (255,255,255) VFD, and use overlays, but that's even less accurate.
	palette_set_color(Machine,16,output_get_value("VFDR11"),output_get_value("VFDG11"),output_get_value("VFDB11"));
	palette_set_color(Machine,17,output_get_value("VFDR01"),output_get_value("VFDG01"),output_get_value("VFDB01"));
	palette_set_color(Machine,18,output_get_value("VFDR12"),output_get_value("VFDG12"),output_get_value("VFDB12"));
	palette_set_color(Machine,19,output_get_value("VFDR02"),output_get_value("VFDG02"),output_get_value("VFDB02"));
	palette_set_color(Machine,20,output_get_value("VFDR13"),output_get_value("VFDG13"),output_get_value("VFDB13"));
	palette_set_color(Machine,21,output_get_value("VFDR03"),output_get_value("VFDG03"),output_get_value("VFDB03"));

	if (!output_get_value("TotalReels"))
	{
		totalreels = 6 ;
	}
		else
	{
		totalreels = output_get_value("TotalReels");
	}

	awp_reel_setup(totalreels);
}


// video initialisation ///////////////////////////////////////////////////

VIDEO_START( awpvideo )
{
	generic_awp_setup(0);
	return 0;
}
// video update ///////////////////////////////////////////////////////////

VIDEO_UPDATE( awpvideo )
{
	int x;
	if (screen == 0)
	{
		awp_lamp_draw();
		for ( x = 0; x < totalreels; x++ )
		{
			draw_reel(x,steps[x],symbols[x]);
		}
		awp_lamp_draw();
		if (!vfdcol)
		{
			draw_14seg(bitmap,0,3,9);
		}
		else
		{
			awp_vfd_setup(0,bitmap);
		}
	}
	return 0;
}

PALETTE_INIT( awpvideo )
{
	palette_set_color(Machine, 0,0x00,0x00,0x00);
	palette_set_color(Machine, 1,0x00,0x00,0xFF);
	palette_set_color(Machine, 2,0x00,0xFF,0x00);
	palette_set_color(Machine, 3,0x00,0xFF,0xFF);
	palette_set_color(Machine, 4,0xFF,0x00,0x00);
	palette_set_color(Machine, 5,0xFF,0x00,0xFF);
	palette_set_color(Machine, 6,0xFF,0xFF,0x00);
	palette_set_color(Machine, 7,0xFF,0xFF,0xFF);
	palette_set_color(Machine, 8,0x80,0x80,0x80);
	palette_set_color(Machine, 9,0x00,0x00,0x80);
	palette_set_color(Machine,10,0x00,0x80,0x00);
	palette_set_color(Machine,11,0x00,0x80,0x80);
	palette_set_color(Machine,12,0x80,0x00,0x00);
	palette_set_color(Machine,13,0x80,0x00,0x80);
	palette_set_color(Machine,14,0x80,0x80,0x00);
	palette_set_color(Machine,15,0x80,0x80,0x80);
	palette_set_color(Machine,16,0x00,0x00,0x00);
	palette_set_color(Machine,17,0x00,0x00,0x00);
	palette_set_color(Machine,18,0x00,0x00,0x00);
	palette_set_color(Machine,19,0x00,0x00,0x00);
	palette_set_color(Machine,20,0x00,0x00,0x00);
	palette_set_color(Machine,21,0x00,0x00,0x00);
}
