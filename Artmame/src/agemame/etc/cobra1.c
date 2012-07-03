/*****************************************************************************************

    cobra1.c

    Bellfruit Cobra1 (aka Slipstream, aka Konix MultiSystem) (under heavy construction !!!)

    A.G.E Code Copyright (c) 2004-2006 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


******************************************************************************************
    This is just a guide to point out where everything is memory-wise.
    Since the Jaguar/Cojag project came out of this, presumably we'll need to create a subset
    of that GPU and DSP to make this work, which is far beyond my abilities.
    - El Condor

*****************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
//#include "cpu/i86/h83002.h"
//#include "sound/dac.h"

// crappy sample map

static ADDRESS_MAP_START( cobra_map_prog, ADDRESS_SPACE_PROGRAM, 16 )

	AM_RANGE(0x00000, 0x3ffff) AM_READWRITE(MRA16_RAM, cobra1_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x40000, 0x401ff) AM_WRITE(paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram)
	AM_RANGE(0x40200, 0x40fff) AM_NOP //reserved
	AM_RANGE(0x41000, 0x411ff) AM_ROM
	AM_RANGE(0x41200, 0x4127f) //DSP data constants
	AM_RANGE(0x41280, 0x412ff) //DSP registers
	AM_RANGE(0x41300, 0x413ff) AM_RAM	// DSP data RAM
	AM_RANGE(0x41400, 0x415ff) AM_RAM	// DSP program RAM
	AM_RANGE(0x41600, 0x7ffff) AM_NOP	// reserved
	AM_RANGE(0x80000, 0xbffff) AM_RAM	// expansion 0
	AM_RANGE(0xc0000, 0xfffff) AM_RAM	// expansion 1

ADDRESS_MAP_END

static ADDRESS_MAP_START( cobra_map_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
    AM_RANGE(0x00, 0x01) AM_READWRITE(horiz_lp_r, cobra_int_w)
    AM_RANGE(0x02, 0x03) AM_READWRITE(vert_lp_r, screen_start_w)
    AM_RANGE(0x04, 0x05) AM_READWRITE(cobra_status_r, horiz_count_w)
    AM_RANGE(0x06, 0x07) AM_WRITE(vert_count_w)
    AM_RANGE(0x08, 0x0a) AM_WRITE(screen_register_w)
    AM_RANGE(0x0b, 0x0b) AM_WRITE(int_ack_w)
    AM_RANGE(0x0c, 0x0c) AM_WRITE(screen_mode_w)
    AM_RANGE(0x0d, 0x0e) AM_WRITE(border_colour_w)// same layout as palette ram
    AM_RANGE(0x0f, 0x0f) AM_WRITE(colour_mask_w)
    AM_RANGE(0x10, 0x10) AM_WRITE(palette_index_w)
    AM_RANGE(0x11, 0x12) AM_WRITE(screen_end_w)
    AM_RANGE(0x13, 0x13) AM_WRITE(mem_config_w)//0=ROM, 1=DRAM, 2=SRAM, 3=PSRAM
    AM_RANGE(0x14, 0x14) AM_WRITE(general_w)//bit 0=1, second input bit 1=1, DSP output
    AM_RANGE(0x15, 0x15) AM_WRITE(diag_w)
    AM_RANGE(0x16, 0x16) AM_WRITE(int_disable)

    AM_RANGE(0x20, 0x22) AM_READ(blitter_dest)
    AM_RANGE(0x23, 0x25) AM_READ(blitter_source)
    AM_RANGE(0x26, 0x26) AM_READ(blitter_status)
    AM_RANGE(0x27, 0x27) AM_READ(blitter_inner)
    AM_RANGE(0x28, 0x28) AM_READ(blitter_outer)
    AM_RANGE(0x30, 0x32) AM_READ(blitter_program)
    AM_RANGE(0x33, 0x33) AM_READ(blitter_command)
    AM_RANGE(0x34, 0x34) AM_READ(blitter_control)

    AM_RANGE(0x40, 0x40) AM_READWRITE(input_port_1_r,input_port_3_w)
    AM_RANGE(0x50, 0x50) AM_READ(input_port_2_r)
    AM_RANGE(0x60, 0x6f) AM_READWRITE(io_1_r, io_1_w)
    AM_RANGE(0x70, 0x7f) AM_READWRITE(io_2_r, io_2_w)
    AM_RANGE(0x80, 0xff) AM_READWRITE(io_3_r, io_3_w)
ADDRESS_MAP_END

INPUT_PORTS_START( cobra )
INPUT_PORTS_END
static MACHINE_DRIVER_START( cobra )

	/* basic machine hardware */
	MDRV_CPU_ADD(i86, 17.734475/3 )
	MDRV_CPU_PROGRAM_MAP( cobra_map_prog, 0 )
	MDRV_CPU_IO_MAP( cobra_map_io, 0 )
//	MDRV_CPU_VBLANK_INT( irq1_line_pulse, 1 );

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)


	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE( 400, 300)
	MDRV_VISIBLE_AREA(  0, 400-1, 0, 300-1)
	MDRV_FRAMES_PER_SECOND(60)

	MDRV_DEFAULT_LAYOUT(layout_awpvid)
	MDRV_VIDEO_START( awpvideo)
	MDRV_VIDEO_UPDATE(awpvideo)

	MDRV_PALETTE_LENGTH(22)
	MDRV_COLORTABLE_LENGTH(22)
	MDRV_PALETTE_INIT(awpvideo)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************


                                ROMs Loading


***************************************************************************/



/***************************************************************************


                                Game Drivers


***************************************************************************/


