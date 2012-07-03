/***************************************************************************

 Galaxian/Moon Cresta hardware
 Lucky Today runs on a Galaxian board

Main clock: XTAL = 18.432 MHz
Z80 Clock: XTAL/6 = 3.072 MHz
Horizontal video frequency: HSYNC = XTAL/3/192/2 = 16 kHz
Video frequency: VSYNC = HSYNC/132/2 = 60.606060 Hz
VBlank duration: 1/VSYNC * (20/132) = 2500 us

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "galaxian.h"
#include "machine/luctoday.c"

static ADDRESS_MAP_START( map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x5000, 0x53ff) AM_READWRITE(MRA8_RAM,galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x5400, 0x57ff) AM_READ(galaxian_videoram_r)
	AM_RANGE(0x5800, 0x583f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5840, 0x585f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5860, 0x587f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5880, 0x58ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5800, 0x58ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6000, 0x6000) AM_READ(input_port_0_r)
	AM_RANGE(0x6800, 0x6800) AM_READ(input_port_1_r)
	AM_RANGE(0x7000, 0x7000) AM_READ(input_port_2_r)
	AM_RANGE(0x7800, 0x7fff) AM_READ(watchdog_reset_r)
	AM_RANGE(0x6000, 0x6001) AM_WRITE(galaxian_leds_w)
	AM_RANGE(0x6002, 0x6002) AM_WRITE(galaxian_coin_lockout_w)
	AM_RANGE(0x6003, 0x6003) AM_WRITE(galaxian_coin_counter_w)
	AM_RANGE(0x6004, 0x6007) AM_WRITE(galaxian_lfo_freq_w)
	AM_RANGE(0x6800, 0x6802) AM_WRITE(galaxian_background_enable_w)
	AM_RANGE(0x6803, 0x6803) AM_WRITE(galaxian_noise_enable_w)
	AM_RANGE(0x6805, 0x6805) AM_WRITE(galaxian_shoot_enable_w)
	AM_RANGE(0x6806, 0x6807) AM_WRITE(galaxian_vol_w)
	AM_RANGE(0x7001, 0x7001) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x7004, 0x7004) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x7006, 0x7006) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x7007, 0x7007) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x7800, 0x7800) AM_WRITE(galaxian_pitch_w)
	AM_RANGE(0xfffc, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

INPUT_PORTS_START( luctoday )
   PORT_START_TAG("IN0") //These inputs are clearly wrong, they need a full test
   PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
   PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )PORT_2WAY PORT_NAME("Remove Credit from Bet")
   PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )PORT_2WAY PORT_NAME("Add Credit to Bet")
   PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
   PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BILL1 )

   PORT_START_TAG("IN1")
   PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
   PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
   PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
   PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
   PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
   PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
   PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
   PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

   PORT_START_TAG("DSW0")
   PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
   PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static const gfx_layout galaxian_charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};
static const gfx_layout galaxian_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static gfx_decode galaxian_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &galaxian_charlayout,   0, 8 },
	{ REGION_GFX1, 0x0000, &galaxian_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( galaxian )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_PROGRAM_MAP(map,0)

	MDRV_SCREEN_REFRESH_RATE(16000.0/132/2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(galaxian_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+2+64)		/* 32 for the characters, 2 for the bullets, 64 for the stars */
	MDRV_COLORTABLE_LENGTH(8*4)
	MDRV_MACHINE_RESET(galaxian)
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(galaxian)
	MDRV_VIDEO_UPDATE(galaxian)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(galaxian_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( luctoday )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "ltprog1.bin", 0x0000, 0x0800, CRC(59c389b9) SHA1(1e158ced3b56db2c51e422fb4c0b8893565f1956))
	ROM_LOAD( "ltprog2.bin", 0x2000, 0x0800, CRC(ac3893b1) SHA1(f6b9cd8111b367ff7030cba52fe965959d92568f))

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ltchar2.bin", 0x0000, 0x0800, CRC(8cd73bdc) SHA1(6174f7347d2c96f9c5074bc0da5a370c9b07461b))
	ROM_LOAD( "ltchar1.bin", 0x0800, 0x0800, CRC(b5ba9946) SHA1(7222cbe8c41ca74b214f4dd5439bf69d90f4644e))

	ROM_REGION( 0x0020, REGION_PROMS, 0 )//This may not be the correct prom
	ROM_LOAD( "74s288.ch", 0x0000, 0x0020, BAD_DUMP CRC(24652bc4) SHA1(d89575f3749c75dc963317fe451ffeffd9856e4d))
ROM_END

ROM_START( chewing )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "1.bin", 0x0000, 0x1000, CRC(7470b347) SHA1(315d2631b50a6e469b9538318d95452e8d2e1f69) )
	ROM_LOAD( "7l.bin", 0x2000, 0x0800, CRC(78ebed36) SHA1(e80185737c8ac448901cf0e60ca50d967c323b34) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bin", 0x0000, 0x0800, CRC(88c605f3) SHA1(938a9fadfa0994a1d2fc9b3266ec4ccdb5ec6d3a) )
	ROM_LOAD( "3.bin", 0x0800, 0x0800, CRC(77ac016a) SHA1(fa5b1e79603ca8d2ee7b3d0a78f12d9ffeec3fd4) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "74s288.ch", 0x0000, 0x0020, CRC(24652bc4) SHA1(d89575f3749c75dc963317fe451ffeffd9856e4d) )
ROM_END

GAME( 1980, luctoday, 0,        galaxian, luctoday, 0,        ROT270, "Sigma", "Lucky Today",GAME_WRONG_COLORS | GAME_SUPPORTS_SAVE )
GAME( 19??, chewing,  0,        galaxian, luctoday, 0,        ROT90,  "unknown", "Chewing Gum", GAME_SUPPORTS_SAVE )
