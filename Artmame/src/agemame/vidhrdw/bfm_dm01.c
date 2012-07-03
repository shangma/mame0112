/***************************************************************************

    bfm_dm01.c

    Bellfruit dotmatrix driver, (under heavy construction !!!)

    19-08-2005: Re-Animator
    25-08-2005: Fixed feedback through sc2 input line

    A.G.E Code Copyright (c) 2004-2007 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


Standard dm01 memorymap
           
    hex    |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-----------------------------------------
0000-1FFF  |R/W| D D D D D D D D | RAM (8k)
-----------+---+-----------------+-----------------------------------------
2000       |R/W| D D D D D D D D | control register
-----------+---+-----------------+-----------------------------------------
2800       |R/W| D D D D D D D D | mux
-----------+---+-----------------+-----------------------------------------
3000       |R/W| D D D D D D D D | communication with mainboard
-----------+---+-----------------+-----------------------------------------
3800       |R/W| D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
4000-FFFF  | W | D D D D D D D D | ROM (48k)
-----------+---+-----------------+-----------------------------------------

  TODO: - find out clockspeed of CPU

  Layout notes: the dot matrix is set to screen 0.
  Multiple matrices and matrix/VFD combos are not yet supported.
***************************************************************************/

#include "driver.h"
#include "ui.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/awpvid.h"
#include "bfm_sc2.h"
// local prototypes ///////////////////////////////////////////////////////

extern void Scorpion2_SetSwitchState(int strobe, int data, int state);
extern int  Scorpion2_GetSwitchState(int strobe, int data);

// local vars /////////////////////////////////////////////////////////////

#define DM_BYTESPERROW 9
#define DM_MAXLINES    21

#define FEEDBACK_STROBE 4
#define FEEDBACK_DATA   4

static int   control;
static int   xcounter;
static int   busy;
static int   data_avail;

static UINT8 scanline[DM_BYTESPERROW];

//static UINT8 dm_data[DM_BYTESPERROW*DM_MAXLINES];


static UINT8 comdata;

//static int   read_index;
//static int   write_index;

static mame_bitmap *dm_bitmap;

///////////////////////////////////////////////////////////////////////////

static int read_data(void)
{
	int data = comdata;

	data_avail = 0;

	return data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( control_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( control_w )
{
	int changed = control ^ data;

	control = data;

	if ( changed & 2 )
	{	// reset horizontal counter
		if ( !(data & 2) )
		{
			//int offset = 0;

			xcounter = 0;
		}
	}

	if ( changed & 8 )
	{ // bit 3 changed = BUSY line
		if ( data & 8 )	  busy = 0;
		else			  busy = 1;

		Scorpion2_SetSwitchState(FEEDBACK_STROBE,FEEDBACK_DATA, busy?0:1);
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux_r )
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux_w )
{
//  static UINT8 line[DM_BYTESPERROW*8];

	int x = xcounter;

	if ( x < DM_BYTESPERROW )
	{
		scanline[xcounter++] = data;
	}

	if ( x == 8 )
	{
    	int off = ((0xFF^data) & 0x7C) >> 2;	// 7C = 000001111100

		scanline[x] &= 0x80;
		if ( (off >= 0)  && (off < DM_MAXLINES) )
		{
			int p;

			x = 0;
			p = 0;

			while ( p < DM_BYTESPERROW )
			{
				UINT8 d = scanline[p++];
//				logerror("%d",d);
				plot_pixel( dm_bitmap, x++, off, (d & 0x80) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x40) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x20) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x10) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x08) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x04) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x02) ? 4 : 5 );
				plot_pixel( dm_bitmap, x++, off, (d & 0x01) ? 4 : 5 );
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( comm_r )
{
	int result = 0;

	if ( data_avail )
	{
		result = read_data();
		#if 0
		if ( data_avail() )
		{
			cpu_set_irq_line(1, M6809_IRQ_LINE, HOLD_LINE );	// trigger IRQ
		}
		#endif
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( comm_w )
{

}
///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( unknown_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( unknown_w )
{
}

///////////////////////////////////////////////////////////////////////////

ADDRESS_MAP_START( bfm_dm01_memmap, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_RAM)	// 8k RAM
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_RAM)	// 8k RAM
	AM_RANGE(0x2000, 0x2000) AM_READ(control_r)	// control reg
	AM_RANGE(0x2000, 0x2000) AM_WRITE(control_w)// control reg
	AM_RANGE(0x2800, 0x2800) AM_READ(mux_r)		// mux
	AM_RANGE(0x2800, 0x2800) AM_WRITE(mux_w)	// mux
	AM_RANGE(0x3000, 0x3000) AM_READ(comm_r)	//
	AM_RANGE(0x3000, 0x3000) AM_WRITE(comm_w)	//
	AM_RANGE(0x3800, 0x3800) AM_READ(unknown_r)	// ???
	AM_RANGE(0x3800, 0x3800) AM_WRITE(unknown_w)// ???
	AM_RANGE(0x4000, 0xFfff) AM_READ(MRA8_ROM)	// 48k  ROM

ADDRESS_MAP_END

///////////////////////////////////////////////////////////////////////////

mame_bitmap *BFM_dm01_getBitmap(void)
{
	return dm_bitmap;
}

///////////////////////////////////////////////////////////////////////////

void BFM_dm01_writedata(UINT8 data)
{
	comdata = data;
	data_avail = 1;


  //pulse IRQ line
	cpunum_set_input_line(1, M6809_IRQ_LINE, HOLD_LINE ); // trigger IRQ
}

///////////////////////////////////////////////////////////////////////////

INTERRUPT_GEN( bfm_dm01_vbl )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE );
}

///////////////////////////////////////////////////////////////////////////

VIDEO_START( bfm_dm01 )
{
	dm_bitmap = auto_bitmap_alloc(DM_BYTESPERROW*8, DM_MAXLINES);
	palette_set_color(machine,5,0x40,0x00,0x00);
	generic_awp_setup(0);
	return 0;
}

///////////////////////////////////////////////////////////////////////////
//rectangle clip = { 0, 400,  0,  300 };  //minx,maxx, miny,maxy
///////////////////////////////////////////////////////////////////////////

// We use the AWPVid helpers here, but not the main functions, as we need to replace
// the VFD with a dot matrix display.
// Note that we assume square pixels.

VIDEO_UPDATE( bfm_dm01 )
{
	int x;
	{
		if (screen == 0)
		{
			copybitmap(bitmap, dm_bitmap, 0,0,0,0, &Machine->screen[screen].visarea, TRANSPARENCY_NONE, 0);
			logerror("Busy=%X",data_avail);
			logerror("%X",Scorpion2_GetSwitchState(FEEDBACK_STROBE,FEEDBACK_DATA));

			for ( x = 0; x < totalreels; x++ )
			{
				draw_reel(x,steps[x],symbols[x]);
			}
			awp_lamp_draw();
		}

		if (screen == 1)
		{

			#if 0
			if ( sc2_show_door )
			{
				output_set_value("door",( Scorpion2_GetSwitchState(sc2_door_state>>4, sc2_door_state & 0x0F) ) );
			}
			#endif
		}
	}
	return 0;
}

int BFM_dm01_busy(void)
{
	return data_avail;
}

void BFM_dm01_reset(void)
{
	busy     = 0;
	control  = 0;
	xcounter = 0;
	data_avail = 0;

	//Scorpion2_SetSwitchState(FEEDBACK_STROBE,FEEDBACK_DATA, busy?0:1);
}
