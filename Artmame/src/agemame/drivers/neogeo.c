
/***************************************************************************
    M.A.M.E. Neo Geo driver presented to you by the Shin Emu Keikaku team.

    The following people have all spent probably far too much time on this:

    AVDB
    Bryan McPhail
    Fuzz
    Ernesto Corvi
    Andrew Prime

    Neogeo Motherboard (info - courtesy of Guru)
    --------------------------------------------

    PCB Layout (single slot, older version)

    NEO-MVH MV1
    |---------------------------------------------------------------------|
    |       4558                                                          |
    |                                          HC04  HC32                 |
    |                      SP-S2.SP1  NEO-E0   000-L0.L0   LS244  AS04    |
    |             YM2610                                                  |
    | 4558                                                                |
    |       4558                        5814  HC259   SFIX.SFIX           |
    |                                                             NEO-I0  |
    | HA13001 YM3016                    5814                              |
    --|                                                                   |
      |     4558                                                          |
    --|                                                 SM1.SM1   LS32    |
    |                                                                     |
    |                           LSPC-A0         PRO-C0            LS244   |
    |                                                                     |
    |J              68000                                                 |
    |A                                                                    |
    |M                                                                    |
    |M                                                      NEO-ZMC2      |
    |A                                                                    |
    |   LS273  NEO-G0                          58256  58256     Z80A      |
    |                           58256  58256   58256  58256     6116      |
    |   LS273 5864                                                        |
    --| LS05  5864  PRO-B0                                                |
      |                                                                   |
    --|             LS06   HC32           D4990A    NEO-F0   24.000MHz    |
    |                      DSW1    BATT3.6V 32.768kHz       NEO-D0        |
    |                                           2003  2003                |
    |---------------------------------------------------------------------|

    Notes:
          68k clock: 12.000MHz
          Z80 clock: 4.000MHz
       YM2610 clock: 8.000MHz
              VSync: 60Hz
              HSync: 15.21kHz

             Custom SNK chips
             ----------------
             NEO-G0: QFP64
             NEO-E0: QFP64
             PRO-B0: QFP136
            LSPC-A0: QFP160
             PRO-C0: QFP136
             NEO-F0: QFP64
             NEO-D0: QFP64
           NEO-ZMC2: QFP80
             NEO-I0: QFP64

             ROMs        Type
             ----------------------------
             SP-S2.SP1   TC531024 (DIP40)
             000-L0.L0   TC531000 (DIP28)
             SFIX.SFIX   D27C1000 (DIP32)
             SM1.SM1     MB832001 (DIP32)

    ------------------------------------------------------

=============================================================================

Points to note, known and proven information deleted from this map:

    0x3000001       Dipswitches
                bit 0 : Selftest
                bit 1 : Unknown (Unused ?) \ something to do with
                bit 2 : Unknown (Unused ?) / auto repeating keys ?
                bit 3 : \
                bit 4 :  | communication setting ?
                bit 5 : /
                bit 6 : free play
                bit 7 : stop mode ?

    0x320001        bit 0 : COIN 1
                bit 1 : COIN 2
                bit 2 : SERVICE
                bit 3 : UNKNOWN
                bit 4 : UNKNOWN
                bit 5 : UNKNOWN
                bit 6 : 4990 test pulse bit.
                bit 7 : 4990 data bit.

    0x380051        4990 control write register
                bit 0: C0
                bit 1: C1
                bit 2: C2
                bit 3-7: unused.

                0x02 = shift.
                0x00 = register hold.
                0x04 = ????.
                0x03 = time read (reset register).

    0x3c000c        IRQ acknowledge

    0x380011        Backup bank select

    0x3a0001        Enable display.
    0x3a0011        Disable display

    0x3a0003        Swap in Bios (0x80 bytes vector table of BIOS)
    0x3a0013        Swap in Rom  (0x80 bytes vector table of ROM bank)

    0x3a000d        lock backup ram
    0x3a001d        unlock backup ram

    0x3a000b        set game vector table (?)  mirror ?
    0x3a001b        set bios vector table (?)  mirror ?

    0x3a000c        Unknown (ghost pilots)
    0x31001c        Unknown (ghost pilots)

    IO word read

    0x3c0002        return vidram word (pointed to by 0x3c0000)
    0x3c0006        ?????.
    0x3c0008        shadow adress for 0x3c0000 (not confirmed).
    0x3c000a        shadow adress for 0x3c0002 (confirmed, see
                               Puzzle de Pon).
    IO word write

    0x3c0006        Unknown, set vblank counter (?)

    0x3c0008        shadow address for 0x3c0000 (not confirmed)
    0x3c000a        shadow address for 0x3c0002 (not confirmed)

    The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
    accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
    A schematic for this device can be found on the NEC webpages.

******************************************************************************/

/*

--------------------------------------------------------------------------------

   Driver Notes / Known Problems

   Fatal Fury 3 crashes during the ending
    -- this doesn't occur if the language is set to Japanese, maybe the english endings
       are incomplete / buggy?

   Graphical Glitches caused by incorrect timing?
    -- Blazing Star glitches during the tunnel sequence
    -- Ninja Combat sometimes glitches
    -- Some raster effects are imperfect (off by a couple of lines)

--------------------------------------------------------------------------------

    *NON* bugs

    Bad zooming in the Kof2003 bootlegs
     -- this is what happens if you try and use the normal bios with a pcb set, it
        looks like the bootleggers didn't care.

    Glitches at the edges of the screen
     -- the real hardware can display 320x240 but most of the games seem designed
        to work with a width of 304, some less.

    Distorted jumping sound in Nightmare in the Dark
     -- Confirmed on real hardware

--------------------------------------------------------------------------------

    Not Implemented

    Multi Cart support
     -- the MVS can take up to 6 carts depending on the board being used.

 --------------------------------------------------------------------------------

*/



#include "driver.h"
#include "machine/pd4990a.h"
#include "cpu/z80/z80.h"
#include "neogeo.h"
#include "sound/2610intf.h"


/* values probed by Razoola from the real board */
#define RASTER_LINES 264
/* VBLANK should fire on line 248 */
#define RASTER_COUNTER_START 0x1f0	/* value assumed right after vblank */
#define RASTER_COUNTER_RELOAD 0x0f8	/* value assumed after 0x1ff */
#define RASTER_LINE_RELOAD (0x200 - RASTER_COUNTER_START)

#define SCANLINE_ADJUST 3	/* in theory should be 0, give or take an off-by-one mistake */


/******************************************************************************/

unsigned int neogeo_frame_counter;
unsigned int neogeo_frame_counter_speed=4;

/******************************************************************************/

static INT32 irq2start=1000,irq2control;
static INT32 current_rastercounter,current_rasterline,scanline_read;
static UINT32 irq2pos_value;
static INT32 vblank_int,scanline_int;

/*  flags for irq2control:

    0x07 unused? kof94 sets some random combination of these at the character
         selection screen but only because it does andi.w #$ff2f, $3c0006. It
         clears them immediately after.

    0x08 shocktro2, stops autoanim counter

    Maybe 0x07 writes to the autoanim counter, meaning that in conjunction with
    0x08 one could fine control it. However, if that was the case, writing the
    the IRQ2 control bits would interfere with autoanimation, so I'm probably
    wrong.

    0x10 irq2 enable, tile engine scanline irq that is triggered
    when a certain scanline is reached.

    0x20 when set, the next values written in the irq position register
    sets irq2 to happen N lines after the current one

    0x40 when set, irq position register is automatically loaded at vblank to
    set the irq2 line.

    0x80 when set, every time irq2 is triggered the irq position register is
    automatically loaded to set the next irq2 line.

    0x80 and 0x40 may be set at the same time (Viewpoint does this).
*/

#define IRQ2CTRL_AUTOANIM_STOP		0x08
#define IRQ2CTRL_ENABLE				0x10
#define IRQ2CTRL_LOAD_RELATIVE		0x20
#define IRQ2CTRL_AUTOLOAD_VBLANK	0x40
#define IRQ2CTRL_AUTOLOAD_REPEAT	0x80


static void update_interrupts(void)
{
	int level = 0;

	/* determine which interrupt is active */
	if (vblank_int) level = 1;
	if (scanline_int) level = 2;

	/* either set or clear the appropriate lines */
	if (level)
		cpunum_set_input_line(0, level, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 7, CLEAR_LINE);
}

static WRITE16_HANDLER( neo_irqack_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 4) vblank_int = 0;
		if (data & 2) scanline_int = 0;
		update_interrupts();
	}
}


static INT32 fc = 0;

static INT32 neogeo_raster_enable = 1;

static INTERRUPT_GEN( neogeo_raster_interrupt )
{
	int line = RASTER_LINES - cpu_getiloops();

	current_rasterline = line;

	{
		int l = line;

		if (l == RASTER_LINES) l = 0;	/* vblank */
		if (l < RASTER_LINE_RELOAD)
			current_rastercounter = RASTER_COUNTER_START + l;
		else
			current_rastercounter = RASTER_COUNTER_RELOAD + l - RASTER_LINE_RELOAD;
	}

	if (irq2control & IRQ2CTRL_ENABLE)
	{
		if (line == irq2start)
		{
//logerror("trigger IRQ2 at raster line %d (raster counter %d)\n",line,current_rastercounter);
			if (irq2control & IRQ2CTRL_AUTOLOAD_REPEAT)
				irq2start += (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */

			scanline_int = 1;
		}
	}

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		if (irq2control & IRQ2CTRL_AUTOLOAD_VBLANK)
			irq2start = (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */
		else
			irq2start = 1000;


		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter */
		if (!(irq2control & IRQ2CTRL_AUTOANIM_STOP))
		{
			if (fc++>neogeo_frame_counter_speed)	/* fixed animation speed */
			{
				fc=0;
				neogeo_frame_counter++;
			}
		}

		/* return a standard vblank interrupt */
//logerror("trigger IRQ1\n");
		vblank_int = 1;	   /* vertical blank */
	}

	update_interrupts();
}

static INT32 pending_command;
static INT32 result_code;

/* Calendar, coins + Z80 communication */
static READ16_HANDLER( timer16_r )
{
	UINT16 res;
	int coinflip = pd4990a_testbit_r(0);
	int databit = pd4990a_databit_r(0);

//  logerror("CPU %04x - Read timer\n",activecpu_get_pc());

	res = (readinputport(4) & ~(readinputport(5) & 0x20)) ^ (coinflip << 6) ^ (databit << 7);

	res |= result_code << 8;
	if (pending_command) res &= 0x7fff;

	return res;
}

static WRITE16_HANDLER( neo_z80_w )
{
	/* tpgold uses 16-bit writes, this can't be correct */
//  if (ACCESSING_LSB)
//      return;

	soundlatch_w(0,(data>>8)&0xff);
	pending_command = 1;
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	/* spin for a while to let the Z80 read the command (fixes hanging sound in pspikes2) */
//  cpu_spinuntil_time(TIME_IN_USEC(20));
	cpu_boost_interleave(0, TIME_IN_USEC(20));
}

static int mjneogo_select;

static WRITE16_HANDLER ( mjneogeo_w )
{
	mjneogo_select = data;
}

static READ16_HANDLER ( mjneogeo_r )
{
	UINT16 res;

/*
cpu #0 (PC=00C18B9A): unmapped memory word write to 00380000 = 0012 & 00FF
cpu #0 (PC=00C18BB6): unmapped memory word write to 00380000 = 001B & 00FF
cpu #0 (PC=00C18D54): unmapped memory word write to 00380000 = 0024 & 00FF
cpu #0 (PC=00C18D6C): unmapped memory word write to 00380000 = 0009 & 00FF
cpu #0 (PC=00C18C40): unmapped memory word write to 00380000 = 0000 & 00FF
*/
	res = 0;

	switch (mjneogo_select)
	{
		case 0x00:
		res = 0; // nothing?
		break;
		case 0x09:
		res = (readinputport(7) << 8); // a,b,c,d,e,g ....
		break;
		case 0x12:
		res = (readinputport(8) << 8); // h,i,j,k,l ...
		break;
		case 0x1b:
		res = (readinputport(0) << 8); // player 1 normal inputs?
		break;
		case 0x24:
		res = (readinputport(9) << 8); // call etc.
		break;
		default:
		break;
	}

	return res + readinputport(3);
}



int neogeo_has_trackball;
static int ts;

static WRITE16_HANDLER( trackball_select_16_w )
{
	ts = data & 1;
}

static READ16_HANDLER( controller1_16_r )
{
	UINT16 res;

	if (neogeo_has_trackball)
		res = (readinputport(ts?7:0) << 8) + readinputport(3);
	else
		res = (readinputport(0) << 8) + readinputport(3);

	return res;
}
static READ16_HANDLER( controller2_16_r )
{
	UINT16 res;

	res = (readinputport(1) << 8);

	return res;
}
static READ16_HANDLER( controller3_16_r )
{
	if (memcard_present() == -1)
		return (readinputport(2) << 8);
	else
		return ((readinputport(2) << 8)&0x8FFF);
}
static READ16_HANDLER( controller4_16_r )
{
	return (readinputport(6) & ~(readinputport(5) & 0x40));
}

static READ16_HANDLER( popbounc1_16_r )
{
	UINT16 res;

	res = (readinputport(ts?0:7) << 8) + readinputport(3);

	return res;
}

static READ16_HANDLER( popbounc2_16_r )
{
	UINT16 res;

	res = (readinputport(ts?1:8) << 8);

	return res;
}

static WRITE16_HANDLER( neo_bankswitch_w )
{
	int bankaddress;

	if (memory_region_length(REGION_CPU1) <= 0x100000)
	{
logerror("warning: bankswitch to %02x but no banks available\n",data);
		return;
	}

	data = data&0x7;
	bankaddress = (data+1)*0x100000;
	if (bankaddress >= memory_region_length(REGION_CPU1))
	{
logerror("PC %06x: warning: bankswitch to empty bank %02x\n",activecpu_get_pc(),data);
		bankaddress = 0x100000;
	}

	neogeo_set_cpu1_second_bank(bankaddress);
}



/* TODO: Figure out how this really works! */
static READ16_HANDLER( neo_control_16_r )
{
	int res;

	/*
        The format of this very important location is:  AAAA AAAA A??? BCCC

        A is the raster line counter. mosyougi relies solely on this to do the
          raster effects on the title screen; sdodgeb loops waiting for the top
          bit to be 1; zedblade heavily depends on it to work correctly (it
          checks the top bit in the IRQ2 handler).
        B is definitely a PAL/NTSC flag. Evidence:
          1) trally changes the position of the speed indicator depending on
             it (0 = lower 1 = higher).
          2) samsho3 sets a variable to 60 when the bit is 0 and 50 when it's 1.
             This is obviously the video refresh rate in Hz.
          3) samsho3 sets another variable to 256 or 307. This could be the total
             screen height (including vblank), or close to that.
          Some games (e.g. lstbld2, samsho3) do this (or similar):
          bclr    #$0, $3c000e.l
          when the bit is set, so 3c000e (whose function is unknown) has to be
          related
        C is a variable speed counter. In blazstar, it controls the background
          speed in level 2.
    */

	scanline_read = 1;	/* needed for raster_busy optimization */

	res =	((current_rastercounter << 7) & 0xff80) |	/* raster counter */
			(neogeo_frame_counter & 0x0007);			/* frame counter */

	logerror("PC %06x: neo_control_16_r (%04x)\n",activecpu_get_pc(),res);

	return res;
}


/* this does much more than this, but I'm not sure exactly what */
WRITE16_HANDLER( neo_control_16_w )
{
	logerror("%06x: neo_control_16_w %04x\n",activecpu_get_pc(),data);

	/* Auto-Anim Speed Control */
	neogeo_frame_counter_speed = (data >> 8) & 0xff;

	irq2control = data & 0xff;
}

static WRITE16_HANDLER( neo_irq2pos_16_w )
{
	logerror("%06x: neo_irq2pos_16_w offset %d %04x\n",activecpu_get_pc(),offset,data);

	if (offset)
		irq2pos_value = (irq2pos_value & 0xffff0000) | (UINT32)data;
	else
		irq2pos_value = (irq2pos_value & 0x0000ffff) | ((UINT32)data << 16);

	if (irq2control & IRQ2CTRL_LOAD_RELATIVE)
	{
//      int line = (irq2pos_value + 3) / 0x180; /* ridhero gives 0x17d */
		int line = (irq2pos_value + 0x3b) / 0x180;	/* turfmast goes as low as 0x145 */

		irq2start = current_rasterline + line;

		logerror("irq2start = %d, current_rasterline = %d, current_rastercounter = %d\n",irq2start,current_rasterline,current_rastercounter);
	}
}



static READ16_HANDLER ( neogeo_video_r )
{

	/* 8-bit reads of the low byte do NOT return the correct value on real hardware */
	/* they actually seem to return 0xcf in tests, but kof2002 requires 0xff for the
       'how to play' screen to work correctly */
	UINT16 retdata=0xffff;

	if (!ACCESSING_MSB)
	{
		return 0xff;
	}

	offset &=0x3;

	switch (offset<<1)
	{
		case 0: retdata=neogeo_vidram16_data_r(0,mem_mask);break;
		case 2: retdata=neogeo_vidram16_data_r(0,mem_mask);break;
		case 4:	retdata=neogeo_vidram16_modulo_r(0,mem_mask);break;
		case 6:	retdata=neo_control_16_r(0,mem_mask);break;
	}

	return retdata;
}

static WRITE16_HANDLER( neogeo_video_w )
{
	int line = RASTER_LINES - cpu_getiloops();

	/* If Video RAM changes force a partial update to the previous line */
	video_screen_update_partial(0, line-24); // tuned by ssideki4 / msyogui


	offset &=0x7;

	switch (offset<<1)
	{
		case 0x0:neogeo_vidram16_offset_w(0,data,mem_mask); break;
		case 0x2:neogeo_vidram16_data_w(0,data,mem_mask); break;
		case 0x4:neogeo_vidram16_modulo_w(0,data,mem_mask); break;
		case 0x6:neo_control_16_w(0,data,mem_mask); break;
		case 0x8:neo_irq2pos_16_w(0,data,mem_mask); break;
		case 0xa:neo_irq2pos_16_w(1,data,mem_mask); break;
		case 0xc:neo_irqack_w(0,data,mem_mask); break;
		case 0xe:break; /* Unknown, see control_r */
	}
}

static WRITE16_HANDLER(neogeo_syscontrol_w)
{
	offset &=0x7f;

	switch (offset<<1)
	{
		case 0x00: trackball_select_16_w(0,data,mem_mask);break;

		case 0x30: break; // LEDs (latch)
		case 0x40: break; // LEDs (send)


		case 0x50: pd4990a_control_16_w(0,data,mem_mask);break;
		case 0x60: break; // coin counters
		case 0x62: break; // coin counters
		case 0x64: break; // coin lockout
		case 0x66: break;// coun lockout

		case 0xd0: pd4990a_control_16_w(0,data,mem_mask);break;

		case 0xe0: break;// coin counters
		case 0xe2: break;// coin counters
		case 0xe4: break;// coin lockout
		case 0xe6: break;// coun lockout

		default: /* put warning message here */ break;
	}
}

static WRITE16_HANDLER( neogeo_syscontrol2_w )
{
	offset &=0xf;

	switch (offset<<1)
	{
		/* BIOS Select */
		case 0x00: break;
		case 0x02: neogeo_select_bios_vectors(0,data,mem_mask); break;
		case 0x04: break;
		case 0x06: break;
		case 0x08: break;
		case 0x0a: neo_board_fix_16_w(0,data,mem_mask);break;
		case 0x0c: neogeo_sram16_lock_w(0,data,mem_mask);break;
		case 0x0e:neogeo_setpalbank1_16_w(0,data,mem_mask);break;
		/*GAME Select */
		case 0x10: break;
		case 0x12: neogeo_select_game_vectors(0,data,mem_mask);break;
		case 0x14: break;
		case 0x16: break;
		case 0x18: break;
		case 0x1a: neo_game_fix_16_w(0,data,mem_mask);break;
		case 0x1c: neogeo_sram16_unlock_w(0,data,mem_mask);break;
		case 0x1e: neogeo_setpalbank0_16_w(0,data,mem_mask);break;
	}
}

static READ16_HANDLER(controller1and4_16_r)
{
	UINT16 retvalue=0;

	switch ((offset<<1)&0x80)
	{
		case 0x00: retvalue = controller1_16_r(0,mem_mask);break;
		case 0x80: retvalue = controller4_16_r(0,mem_mask);break;
	}

	return retvalue;
}

/******************************************************************************/

/*
Games check for the text '-SNK STG SYSTEM-' at 0xc20010
if they find it they jump to a subroutine at 0xc200b8.
For and example of this see routine at 0xb013c in kof2002
*/


/* NeoGeo Memory Map (not finished) *

0x000000   0x0fffff   r/o        Rom Bank 1

0x100000   0x1fffff   r/w        Work Ram (0xffff in size, mirrored 16 times)

0x200000   0x2fffff   r/o        Rom Bank 2
0x200000   0x2fffef   w/o        Protection etc. on some games
0x2ffff0   0x2fffff   w/o        Banking registers

0x3c0000   0x3dffff   r/w        Video Access

0x400000   0x7fffff   r/w        Palette (0x1fff in size, mirrored)

0x800000   0x800fff   r/w        Mem Card (mirrored?)

0xc00000   0xcfffff   r/o        BIOS rom (mirrored 8 times)

0xd00000   0xdfffff   r/w        SRAM

*/


/* Mirroring information thanks to Razoola */

static ADDRESS_MAP_START( neogeo_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)							/* Rom bank 1 */
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_BANK1) AM_MIRROR(0x0f0000)	/* Ram bank 1 (mirrored to 0x1fffff) */
	AM_RANGE(0x200000, 0x2fffff) AM_READ(MRA16_BANK4)						/* Rom bank 2 */
	AM_RANGE(0x300000, 0x31ffff) AM_READ(controller1and4_16_r)				/* Inputs */
	AM_RANGE(0x320000, 0x33ffff) AM_READ(timer16_r)							/* Coins, Calendar, Z80 communication */
	AM_RANGE(0x340000, 0x35ffff) AM_READ(controller2_16_r)					/* Inputs */
	AM_RANGE(0x380000, 0x39ffff) AM_READ(controller3_16_r)					/* Inputs */
	AM_RANGE(0x3c0000, 0x3dffff) AM_READ(neogeo_video_r)					/* Video Hardware */
	AM_RANGE(0x400000, 0x7fffff) AM_READ(neogeo_paletteram16_r)				/* Palette */
	AM_RANGE(0x800000, 0x800fff) AM_READ(neogeo_memcard16_r)				/* Memory Card */
	AM_RANGE(0xc00000, 0xc1ffff) AM_READ(MRA16_BANK3) AM_MIRROR(0x0e0000)	/* Bios rom (mirrored every 128k for standard bios) */
	AM_RANGE(0xd00000, 0xd0ffff) AM_READ(neogeo_sram16_r) AM_MIRROR(0x0f0000)	/* 64k battery backed SRAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( neogeo_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)	  /* ghost pilots writes to ROM */
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_BANK1) AM_MIRROR(0x0f0000)	/* WORK RAM (mirrored to 0x1fffff) */
	/* Some games have protection devices in the 0x200000 region, it appears to map to cart space, not surprising, the rom is read here too */
	AM_RANGE(0x2ffff0, 0x2fffff) AM_WRITE(neo_bankswitch_w)			/* Bankswitch for standard games */
	AM_RANGE(0x300000, 0x31ffff) AM_WRITE(watchdog_reset16_w)		/* Watchdog, NOTE, odd addresses only! */
	AM_RANGE(0x320000, 0x33ffff) AM_WRITE(neo_z80_w)				/* Sound CPU  EVEN BYTES only!*/
	AM_RANGE(0x380000, 0x39ffff) AM_WRITE(neogeo_syscontrol_w)		/* Coin Counters, LEDs, Clock etc. */
	AM_RANGE(0x3a0000, 0x3affff) AM_WRITE(neogeo_syscontrol2_w)		/* BIOS / Game select etc. */
	AM_RANGE(0x3c0000, 0x3dffff) AM_WRITE(neogeo_video_w)			/* Video Hardware */
	AM_RANGE(0x400000, 0x4fffff) AM_WRITE(neogeo_paletteram16_w)	/* Palettes */
	AM_RANGE(0x800000, 0x800fff) AM_WRITE(neogeo_memcard16_w)		/* Memory card */
	AM_RANGE(0xd00000, 0xd0ffff) AM_WRITE(neogeo_sram16_w) AM_BASE(&neogeo_sram16) AM_MIRROR(0x0f0000)	/* 64k battery backed SRAM */
ADDRESS_MAP_END

/******************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK5)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_BANK6)
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_BANK7)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_BANK8)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xf7ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


static UINT32 bank[4] = {
	0x08000,
	0x0c000,
	0x0e000,
	0x0f000
};


static READ8_HANDLER( z80_port_r )
{
#if 0
{
	char buf[80];
	sprintf(buf,"%05x %05x %05x %05x",bank[0],bank[1],bank[2],bank[3]);
	popmessage(buf);
}
#endif

	switch (offset & 0xff)
	{
	case 0x00:
		pending_command = 0;
		return soundlatch_r(0);
		break;

	case 0x04:
		return YM2610_status_port_0_A_r(0);
		break;

	case 0x05:
		return YM2610_read_port_0_r(0);
		break;

	case 0x06:
		return YM2610_status_port_0_B_r(0);
		break;

	case 0x08:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[3] = 0x0800 * ((offset >> 8) & 0x7f) + 0x10000;
			memory_set_bankptr(8,&mem08[bank[3]]);
			return 0;
			break;
		}

	case 0x09:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[2] = 0x1000 * ((offset >> 8) & 0x3f) + 0x10000;
			memory_set_bankptr(7,&mem08[bank[2]]);
			return 0;
			break;
		}

	case 0x0a:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[1] = 0x2000 * ((offset >> 8) & 0x1f) + 0x10000;
			memory_set_bankptr(6,&mem08[bank[1]]);
			return 0;
			break;
		}

	case 0x0b:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[0] = 0x4000 * ((offset >> 8) & 0x0f) + 0x10000;
			memory_set_bankptr(5,&mem08[bank[0]]);
			return 0;
			break;
		}

	default:
logerror("CPU #1 PC %04x: read unmapped port %02x\n",activecpu_get_pc(),offset&0xff);
		return 0;
		break;
	}
}

static WRITE8_HANDLER( z80_port_w )
{
	switch (offset & 0xff)
	{
	case 0x04:
		YM2610_control_port_0_A_w(0,data);
		break;

	case 0x05:
		YM2610_data_port_0_A_w(0,data);
		break;

	case 0x06:
		YM2610_control_port_0_B_w(0,data);
		break;

	case 0x07:
		YM2610_data_port_0_B_w(0,data);
		break;

	case 0x08:
		/* NMI enable / acknowledge? (the data written doesn't matter) */
		break;

	case 0x0c:
		result_code = data;
		break;

	case 0x18:
		/* NMI disable? (the data written doesn't matter) */
		break;

	default:
logerror("CPU #1 PC %04x: write %02x to unmapped port %02x\n",activecpu_get_pc(),data,offset&0xff);
		break;
	}
}

static ADDRESS_MAP_START( neo_readio, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READ(z80_port_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( neo_writeio, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(z80_port_w)
ADDRESS_MAP_END

/******************************************************************************/
#define NGIN0\
	PORT_START_TAG("IN0")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )\

#define NGIN1\
	PORT_START_TAG("IN1")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

#define NGIN2\
	PORT_START_TAG("IN2")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */\
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Next Game") PORT_CODE(KEYCODE_7)\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */\
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Previous Game") PORT_CODE(KEYCODE_8)\
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define NGIN3\
	PORT_START_TAG("DSW")\
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )\
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )\
	PORT_DIPSETTING(	0x00, "1?" )\
	PORT_DIPSETTING(	0x02, "2?" )\
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )\
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )\
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x30, "1" )\
	PORT_DIPSETTING(	0x20, "2" )\
	PORT_DIPSETTING(	0x10, "3" )\
	PORT_DIPSETTING(	0x00, "4" )\
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )\
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )\
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )\
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

#define NGIN4\
	PORT_START_TAG("IN4")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms */\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms */\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )


#define NGIN6\
	PORT_START_TAG("IN6")		/* Test switch */\
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)

INPUT_PORTS_START( neogeo )
	NGIN0
	NGIN1
	NGIN2
	NGIN3
	NGIN4
	/* Fake*/
	PORT_START_TAG("IN5")

NGIN6
INPUT_PORTS_END

INPUT_PORTS_START( vliner )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Big")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Small")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Double Up")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("P1 Start/Collect")

	NGIN1 /* there is no player 2 in this game */

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Payout") /* to enable selection in Test Switch */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* This bit is used.. */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )  /* there is no player 2 in this game */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* This bit is used.. */
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	NGIN3

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Operator Menu") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Clear Credit")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Hopper Out")

	PORT_START_TAG("IN5")

	NGIN6
INPUT_PORTS_END

/******************************************************************************/

/* character layout (same for all games) */
static const gfx_layout charlayout =	/* All games */
{
	8,8,			/* 8 x 8 chars */
	RGN_FRAC(1,1),
	4,				/* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* 32 bytes per char */
};

/* Placeholder and also reminder of how this graphic format is put together */
static const gfx_layout dummy_mvs_tilelayout =
{
	16,16,	 /* 16*16 sprites */
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	128*8		/* char modulo */
};

static const gfx_decode neogeo_mvs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ REGION_GFX2, 0, &charlayout, 0, 16 },
	{ REGION_GFX3, 0, &dummy_mvs_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void neogeo_sound_irq( int irq )
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

struct YM2610interface neogeo_ym2610_interface =
{
	neogeo_sound_irq,
	REGION_SOUND2,
	REGION_SOUND1
};

/******************************************************************************/

/*

     - The watchdog timer will reset the system after ~0.13 seconds
       On an MV-1F MVS system, the following code was used to test:
          000100  203C 0001 4F51             MOVE.L   #0x14F51,D0
          000106  13C0 0030 0001             MOVE.B   D0,0x300001
          00010C  5380                       SUBQ.L   #1,D0
          00010E  64FC                       BCC.S    *-0x2 [0x10C]
          000110  13C0 0030 0001             MOVE.B   D0,0x300001
          000116  60F8                       BRA.S    *-0x6 [0x110]
       This code loops long enough to sometimes cause a reset, sometimes not.
       The move takes 16 cycles, subq 8, bcc 10 if taken and 8 if not taken, so:
       (0x14F51 * 18 + 14) cycles / 12000000 cycles per second = 0.128762 seconds
       Newer games force a reset using the following code (this from kof99):
          009CDA  203C 0003 0D40             MOVE.L   #0x30D40,D0
          009CE0  5380                       SUBQ.L   #1,D0
          009CE2  64FC                       BCC.S    *-0x2 [0x9CE0]
       Note however that there is a valid code path after this loop.

   -----

    The watchdog is used as a form of protecetion on a number of games,
    previously this was implemented as a specific hack which locked a single
    address of SRAM.

    What actually happens is if the game doesn't find valid data in the
    backup ram it will initialize it, then sit in a loop.  The watchdog
    should then reset the system while it is in this loop.  If the watchdog
    fails to reset the system the code will continue and set a value in
    backup ram to indiate that the protection check has failed.

*/


static MACHINE_DRIVER_START( neogeo )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 12000000) /* verified */
	MDRV_CPU_PROGRAM_MAP(neogeo_readmem,neogeo_writemem)
	MDRV_CPU_VBLANK_INT(neogeo_raster_interrupt,RASTER_LINES)

	MDRV_CPU_ADD(Z80, 4000000) /* verified */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(neo_readio,neo_writeio)

	MDRV_SCREEN_REFRESH_RATE(15625.0 / 264) /* verified with real PCB */
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_WATCHDOG_TIME_INIT(TIME_IN_SEC(0.128762))

	MDRV_MACHINE_START(neogeo)
	MDRV_MACHINE_RESET(neogeo)
	MDRV_NVRAM_HANDLER(neogeo)
	MDRV_MEMCARD_HANDLER(neogeo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(40*8, 32*8)
    MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(neogeo_mvs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(neogeo_mvs)
	MDRV_VIDEO_UPDATE(neogeo)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(neogeo_ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.60)
	MDRV_SOUND_ROUTE(0, "right", 0.60)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

/******************************************************************************/

/****
 These are the known Bios Roms, Set options.bios to the one you want
 ****/

SYSTEM_BIOS_START( neogeo )
	SYSTEM_BIOS_ADD( 0, "euro",       "Europe MVS (Ver. 2)" )
	SYSTEM_BIOS_ADD( 1, "euro-s1",    "Europe MVS (Ver. 1)" )
	SYSTEM_BIOS_ADD( 2, "us",         "US MVS (Ver. 2?)" )
	SYSTEM_BIOS_ADD( 3, "us-e",       "US MVS (Ver. 1)" )
	SYSTEM_BIOS_ADD( 4, "asia",       "Asia MVS (Ver. 3)" )
	SYSTEM_BIOS_ADD( 5, "japan",      "Japan MVS (Ver. 3)" )
	SYSTEM_BIOS_ADD( 6, "japan-s2",   "Japan MVS (Ver. 2)" )
	SYSTEM_BIOS_ADD( 7, "japan-s1",   "Japan MVS (Ver. 1)" )

//  SYSTEM_BIOS_ADD( 8, "uni-bios.10","Unibios MVS (Hack, Ver. 1.0)" )
//  SYSTEM_BIOS_ADD( 9, "uni-bios.11","Unibios MVS (Hack, Ver. 1.1)" )
//  SYSTEM_BIOS_ADD(10, "debug",      "Debug MVS (Hack?)" )
//  SYSTEM_BIOS_ADD(11, "asia-aes",   "Asia AES" )
SYSTEM_BIOS_END

#define ROM_LOAD16_WORD_SWAP_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(bios+1)) /* Note '+1' */

#define NEOGEO_BIOS \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "sp-s2.sp1",    0x00000, 0x020000, CRC(9036d879) SHA1(4f5ed7105b7128794654ce82b51723e16e389543) ) /* Europe, 1 Slot, has also been found on a 4 Slot (the old hacks were designed for this one) */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "sp-s.sp1",     0x00000, 0x020000, CRC(c7f2fa45) SHA1(09576ff20b4d6b365e78e6a5698ea450262697cd) ) /* Europe, 4 Slot */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 2, "usa_2slt.bin", 0x00000, 0x020000, CRC(e72943de) SHA1(5c6bba07d2ec8ac95776aa3511109f5e1e2e92eb) ) /* US, 2 Slot */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 3, "sp-e.sp1",     0x00000, 0x020000, CRC(2723a5b5) SHA1(5dbff7531cf04886cde3ef022fb5ca687573dcb8) ) /* US, 6 Slot (V5?) */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 4, "asia-s3.rom",  0x00000, 0x020000, CRC(91b64be3) SHA1(720a3e20d26818632aedf2c2fd16c54f213543e1) ) /* Asia */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 5, "vs-bios.rom",  0x00000, 0x020000, CRC(f0e8f27d) SHA1(ecf01eda815909f1facec62abf3594eaa8d11075) ) /* Japan, Ver 6 VS Bios */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 6, "sp-j2.rom",    0x00000, 0x020000, CRC(acede59c) SHA1(b6f97acd282fd7e94d9426078a90f059b5e9dd91) ) /* Japan, Older */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 7, "sp1.jipan.1024",0x00000, 0x020000,  CRC(9fb0abe4) SHA1(18a987ce2229df79a8cf6a84f968f0e42ce4e59d) ) /* Japan, Older */ \

//  ROM_LOAD16_WORD_SWAP_BIOS( 8, "uni-bios.10",  0x00000, 0x020000, CRC(0ce453a0) SHA1(3b4c0cd26c176fc6b26c3a2f95143dd478f6abf9) ) /* Universe Bios v1.0 (hack) */
//  ROM_LOAD16_WORD_SWAP_BIOS( 9, "uni-bios.11",  0x00000, 0x020000, CRC(5dda0d84) SHA1(4153d533c02926a2577e49c32657214781ff29b7) ) /* Universe Bios v1.1 (hack) */
//  ROM_LOAD16_WORD_SWAP_BIOS(10, "neodebug.rom", 0x00000, 0x020000, CRC(698ebb7d) SHA1(081c49aa8cc7dad5939833dc1b18338321ea0a07) ) /* Debug (Development) Bios */
//  ROM_LOAD16_WORD_SWAP_BIOS(11, "aes-bios.bin", 0x00000, 0x020000, CRC(d27a71f1) SHA1(1b3b22092f30c4d1b2c15f04d1670eb1e9fbea07) ) /* AES Console (Asia?) Bios */

/* note you'll have to modify the last for lines of each block to use the extra bios roms,
   they're hacks / homebrew / console bios roms so Mame doesn't list them by default */



#define NEO_BIOS_SOUND_512K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x90000, REGION_CPU2, 0 ) \
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x80000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x80000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_SOUND_256K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x40000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x40000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_SOUND_128K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x20000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x20000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_SOUND_64K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x10000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x10000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NO_DELTAT_REGION

#define NEO_SFIX_128K(name, hash) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x20000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

#define NEO_SFIX_64K(name, hash) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x10000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

#define NEO_SFIX_32K(name, hash) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x08000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )


/******************************************************************************/

ROM_START( vliner )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "vl_p1.rom", 0x000000, 0x080000, CRC(72a2c043) SHA1(b34bcc10ff33e4465126a6865fe8bf6b6a3d6cee) )

	NEO_SFIX_128K( "vl_s1.rom", CRC(972d8c31) SHA1(41f09ef28a3791668ea304c74b8b06c117a50e9a) )

	NEO_BIOS_SOUND_64K( "vl_m1.rom", CRC(9b92b7d1) SHA1(2c9b777feb9a8e43fa1bd942aba5afe3b5427d94) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_ERASE00 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "vl_c1.rom", 0x000000, 0x80000, CRC(5118f7c0) SHA1(b6fb6e9cbb660580d98e00780ebf248c0995145a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "vl_c2.rom", 0x000001, 0x80000, CRC(efe9b33e) SHA1(910c651aadce9bf59e51c338ceef62287756d2e8) ) /* Plane 2,3 */
ROM_END

ROM_START( vlinero )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "vl_p1_54.rom", 0x000000, 0x080000, CRC(172efc18) SHA1(8ca739f8780a9e6fa19ac2c3e931d75871603f58) )

	NEO_SFIX_128K( "vl_s1.rom", CRC(972d8c31) SHA1(41f09ef28a3791668ea304c74b8b06c117a50e9a) )

	NEO_BIOS_SOUND_64K( "vl_m1.rom", CRC(9b92b7d1) SHA1(2c9b777feb9a8e43fa1bd942aba5afe3b5427d94) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_ERASE00 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "vl_c1.rom", 0x000000, 0x80000, CRC(5118f7c0) SHA1(b6fb6e9cbb660580d98e00780ebf248c0995145a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "vl_c2.rom", 0x000001, 0x80000, CRC(efe9b33e) SHA1(910c651aadce9bf59e51c338ceef62287756d2e8) ) /* Plane 2,3 */
ROM_END

/******************************************************************************/

/* dummy entry for the dummy bios driver */
ROM_START( neogeo )
	ROM_REGION16_BE( 0x020000, REGION_USER1, 0 )
	NEOGEO_BIOS

	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )
ROM_END




static UINT16 *brza_sram;

READ16_HANDLER( vliner_2c0000_r )
{
	return 0x0003;
}

READ16_HANDLER( vliner_coins_r )
{
	UINT16 res;
	res = readinputport(4);

	return res;
}

READ16_HANDLER( vliner_timer16_r )
{
	UINT16 res;
	int coinflip = pd4990a_testbit_r(0);
	int databit = pd4990a_databit_r(0);

	res = 0x3f ^ (coinflip << 6) ^ (databit << 7);

	res |= result_code << 8;
	if (pending_command) res &= 0x7fff;

	return res;
}


READ16_HANDLER( brza_sram16_2_r )
{
	return brza_sram[offset];
}

WRITE16_HANDLER( brza_sram16_2_w )
{
	COMBINE_DATA(&brza_sram[offset]);
}


DRIVER_INIT( vliner )
{
	brza_sram = auto_malloc(0x2000);

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x201FFF, 0, 0, brza_sram16_2_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x201FFF, 0, 0, brza_sram16_2_w);
	memory_install_read16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x320000, 0x320001, 0, 0, vliner_timer16_r );
	memory_install_read16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x280000, 0x280001, 0, 0, vliner_coins_r );
	memory_install_read16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2c0000, 0x2c0001, 0, 0, vliner_2c0000_r );

	init_neogeo(machine);
}


/******************************************************************************/

static UINT32 cpu1_second_bankaddress;

void neogeo_set_cpu1_second_bank(UINT32 bankaddress)
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	cpu1_second_bankaddress = bankaddress;
	memory_set_bankptr(4,&RAM[bankaddress]);
}

void neogeo_init_cpu2_setbank(void)
{
	UINT8 *mem08 = memory_region(REGION_CPU2);

	memory_set_bankptr(5,&mem08[bank[0]]);
	memory_set_bankptr(6,&mem08[bank[1]]);
	memory_set_bankptr(7,&mem08[bank[2]]);
	memory_set_bankptr(8,&mem08[bank[3]]);
}

static void neogeo_init_cpu_banks(void)
{
	neogeo_set_cpu1_second_bank(cpu1_second_bankaddress);
	neogeo_init_cpu2_setbank();
}

void neogeo_register_main_savestate(void)
{
	state_save_register_global(neogeo_frame_counter);
	state_save_register_global(neogeo_frame_counter_speed);
	state_save_register_global(current_rastercounter);
	state_save_register_global(current_rasterline);
	state_save_register_global(scanline_read);
	state_save_register_global(irq2start);
	state_save_register_global(irq2control);
	state_save_register_global(irq2pos_value);
	state_save_register_global(vblank_int);
	state_save_register_global(scanline_int);
	state_save_register_global(fc);
	state_save_register_global(neogeo_raster_enable);
	state_save_register_global(pending_command);
	state_save_register_global(result_code);
	state_save_register_global(ts);
	state_save_register_global_array(bank);
	state_save_register_global(neogeo_rng);
	state_save_register_global(cpu1_second_bankaddress);

	state_save_register_func_postload(neogeo_init_cpu_banks);
}

/******************************************************************************/

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the NEOGEO_BIOS files, so that we do not have to include */
/* them in every zip file */
GAMEB( 1990, neogeo, 0, neogeo, neogeo, neogeo, neogeo, ROT0, "SNK", "Neo-Geo", NOT_A_DRIVER )

/******************************************************************************/

/*     YEAR  NAME      PARENT    BIOS    MACHINE INPUT    INIT      MONITOR  */

/* SNK */
/* Brezzasoft */
GAMEB( 2001, vliner,   neogeo,   neogeo, neogeo, vliner,  vliner,   ROT0, "Dyna / BrezzaSoft", "V-Liner (v0.6e)",0)
GAMEB( 2001, vlinero,  vliner,   neogeo, neogeo, vliner,  vliner,   ROT0, "Dyna / BrezzaSoft", "V-Liner (v0.54)",0)
