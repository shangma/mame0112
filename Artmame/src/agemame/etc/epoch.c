/*****************************************************************************************

    epoch.c

    Maygay EPOCH driver, (under heavy construction !!!)

    A.G.E Code Copyright (c) 2004-2006 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


******************************************************************************************

    This will not work until h83002 emulation is considerably improved
    It is included solely in rememberance of the 'masterclass', long since abandoned by
    the members of Fruit Forums (http://www.fruitforums.com)

*****************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
//#include "cpu/h83002/h83002.h"
#include "sound/ymz280b.h"
#include "vidhrdw/awpvid.h"
#include "machine/lamps.h"
#include "machine/steppers.h" // stepper motor
#include "machine/vacfdisp.h"  // vfd
#include "machine/mmtr.h"

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0	// irq
};
// crappy sample map

static ADDRESS_MAP_START( s12h8iomap, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(H8_PORT7, H8_PORT7)
//	AM_RANGE(H8_PORT8, H8_PORT8)
//	AM_RANGE(H8_PORTA, H8_PORTA)
//	AM_RANGE(H8_PORTB, H8_PORTB)
//	AM_RANGE(H8_SERIAL_B, H8_SERIAL_B)
//	AM_RANGE(H8_ADC_0_H, H8_ADC_0_L)
//	AM_RANGE(H8_ADC_1_H, H8_ADC_1_L)
//	AM_RANGE(H8_ADC_2_H, H8_ADC_2_L)
//	AM_RANGE(H8_ADC_3_H, H8_ADC_3_L)
ADDRESS_MAP_END

static ADDRESS_MAP_START( s12h8rwmap, ADDRESS_SPACE_PROGRAM, 16 )
//	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
//	AM_RANGE(0x080000, 0x08ffff) 
//	AM_RANGE(0x280000, 0x287fff) 
//	AM_RANGE(0x300000, 0x300001) 
//	AM_RANGE(0x300002, 0x300003) 
//	AM_RANGE(0xfe0800, 0xfe09ff) AM_NOP	// lamps, 0x80 flash 0x40 dim
//	AM_RANGE(0xfe0a00, 0xfe0bff) AM_NOP	// led, 0x80 flash 0x40 dim
//	AM_RANGE(0xfe0c00, 0xfe0fff) AM_NOP	// INPUT
//	AM_RANGE(0xfe1000, 0xfe11ff) AM_NOP	// OUTPUT
//	AM_RANGE(0xfe1200, 0xfe120b) AM_NOP	// lamp timer
//	AM_RANGE(0xfe120c, 0xfe1210) AM_NOP	// led timer
//	AM_RANGE(0xfe1218, 0xfe1218) AM_NOP	// which lamp dim
//	AM_RANGE(0xfe1219, 0xfe1219) AM_NOP	// which led dim
//	AM_RANGE(0xffff14, 0xffff14) AM_NOP	// enable
//	AM_RANGE(0xffff15, 0xffff15) AM_NOP	// status

ADDRESS_MAP_END

INPUT_PORTS_START( epoch )
INPUT_PORTS_END
static MACHINE_DRIVER_START( epoch )

	/* basic machine hardware */
	MDRV_CPU_ADD(H83002, 14745600 )
	MDRV_CPU_PROGRAM_MAP( s12h8rwmap, 0 )
//	MDRV_CPU_IO_MAP( s12h8iomap, 0 )
//	MDRV_CPU_VBLANK_INT( irq1_line_pulse, 1 );

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)


	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE( 400, 300)
	MDRV_VISIBLE_AREA(  0, 400-1, 0, 300-1)
	MDRV_FRAMES_PER_SECOND(50)

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



ROM_START( mgsimpsn )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "eg1vera.11s", 0x0000000, 0x080000, CRC(9e44645e) SHA1(374eb4a4c09d6b5b7af5ff0efec16b4d2aacbe2b) )
	ROM_LOAD16_BYTE( "tet2-4v2.2", 0x000000, 0x080000, CRC(5bfa32c8) SHA1(55fb2872695fcfbad13f5c0723302e72da69e44a) )	// v2.2
	ROM_LOAD16_BYTE( "tet2-1v2.2", 0x000001, 0x080000, CRC(919116d0) SHA1(3e1c0fd4c9175b2900a4717fbb9e8b591c5f534d) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, CRC(a8a61954) SHA1(86c3db10b348ba1f44ff696877b8b20845fa53de) )

ROM_END


/***************************************************************************


                                Game Drivers


***************************************************************************/

GAME( 1997, mgsimpsn, 0,        epoch, epoch, 0,       ROT0,   "Maygay", "The Simpsons (Maygay AWP)",GAME_NOT_WORKING )
