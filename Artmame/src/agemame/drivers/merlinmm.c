/*
26th July 2005

Checked inputs through service mode
*/


#include "driver.h"
#include "sound/sn76496.h"

extern WRITE8_HANDLER( pingpong_videoram_w );
extern WRITE8_HANDLER( pingpong_colorram_w );

extern PALETTE_INIT( pingpong );
extern VIDEO_START( pingpong );
extern VIDEO_UPDATE( pingpong );

static int intenable;

static WRITE8_HANDLER( coin_w )
{
	/* bit 2 = irq enable, bit 3 = nmi enable */
	intenable = data & 0x0c;

	/* bit 0/1 = coin counters */
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);

	/* other bits unknown */
}

static INTERRUPT_GEN( pingpong_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if (intenable & 0x04) cpunum_set_input_line(0, 0, HOLD_LINE);
	}
	else if (cpu_getiloops() % 2)
	{
		if (intenable & 0x08) cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static ADDRESS_MAP_START( merlinmm_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x5000, 0x53ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x5400, 0x57ff) AM_RAM
	AM_RANGE(0x6000, 0x6007) AM_WRITENOP /*solenoid writes*/
	AM_RANGE(0x7000, 0x7000) AM_READ(input_port_4_r)
	AM_RANGE(0x8000, 0x83ff) AM_RAM AM_WRITE(pingpong_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8400, 0x87ff) AM_RAM AM_WRITE(pingpong_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9000, 0x9002) AM_RAM
	AM_RANGE(0x9003, 0x9052) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x9053, 0x97ff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_WRITE(coin_w)	/* irq enables */
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa080, 0xa080) AM_READ(input_port_1_r)
	AM_RANGE(0xa100, 0xa100) AM_READ(input_port_2_r)
	AM_RANGE(0xa180, 0xa180) AM_READ(input_port_3_r)
	AM_RANGE(0xa200, 0xa200) AM_WRITE(MWA8_NOP)		/* SN76496 data latch */
	AM_RANGE(0xa400, 0xa400) AM_WRITE(SN76496_0_w)	/* trigger read */
	AM_RANGE(0xa600, 0xa600) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END

INPUT_PORTS_START( merlinmm )
	PORT_START_TAG("IN0")
	PORT_DIPNAME( 0x01, 0x01, "Bank 3-3")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bank 3-2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bank 3-1")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Door Close")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Door Open")		//Seems strange, one input to register an open door
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )	//And a different one for closing it!
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START_TAG("IN2")
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, "Stake" )
	PORT_DIPSETTING(    0x02, "10p" )
	PORT_DIPSETTING(    0x00, "20p" )
	PORT_DIPNAME( 0x04, 0x04, "Bank 1-6")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bank 1-5")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "10p Enabled" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "20p Enabled" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "50p Enabled" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "GBP 1.00 Enabled" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START_TAG("IN3")
	PORT_DIPNAME( 0x01, 0x01, "Bank 2-8")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bank 2-7")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bank 2-6")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bank 2-5")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Bank 2-4")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bank 2-3")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Bank 2-2")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bank 2-1")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN4")
	PORT_DIPNAME( 0x01, 0x01, "10p Level" )//Most likely to be optos, rather than DIPs.
	PORT_DIPSETTING(    0x01, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x00, DEF_STR( High ) )
	PORT_DIPNAME( 0x02, 0x02, "20p Level" )
	PORT_DIPSETTING(    0x02, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x00, DEF_STR( High ) )
	PORT_DIPNAME( 0x04, 0x04, "50p Level" )
	PORT_DIPSETTING(    0x04, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x00, DEF_STR( High ) )
	PORT_DIPNAME( 0x08, 0x08, "GBP 1.00 Level" )
	PORT_DIPSETTING(    0x08, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x00, DEF_STR( High ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1) PORT_NAME("10p")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2) PORT_NAME("20p")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3) PORT_NAME("50p")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4) PORT_NAME("GBP 1.00")
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	2,		/* 2 bits per pixel */
	{ 4, 0 },	/* the bitplanes are packed in one nibble */
	{ 3, 2, 1, 0, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },	/* x bit */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8   },     /* y bit */
	16*8	/* every char takes 16 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,		/* 16*16 sprites */
	128,		/* 128 sprites */
	2,		/* 2 bits per pixel */
	{ 4, 0 },	/* the bitplanes are packed in one nibble */
	{ 12*16+3,12*16+2,12*16+1,12*16+0,
	   8*16+3, 8*16+2, 8*16+1, 8*16+0,
	   4*16+3, 4*16+2, 4*16+1, 4*16+0,
	        3,      2,      1,      0 },			/* x bit */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8  },    /* y bit */
	64*8	/* every char takes 64 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,         0, 64 },
	{ REGION_GFX2, 0, &spritelayout,    64*4, 64 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( merlinmm )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("cpu",Z80,18432000/6)		/* 3.072 MHz (probably) Too fast for merlinmm*/
	MDRV_CPU_PROGRAM_MAP(merlinmm_map,0)
	MDRV_CPU_VBLANK_INT(pingpong_interrupt,2)
	
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4+64*4)

	MDRV_PALETTE_INIT(pingpong)
	MDRV_VIDEO_START(pingpong)
	MDRV_VIDEO_UPDATE(pingpong)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 18432000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( merlinmm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "merlinmm.ic2", 0x0000, 0x4000, CRC(ea5b6590) SHA1(fdd5873c67761955e33260743cc45075dea34fb4) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "merlinmm.7h",  0x0000, 0x2000, CRC(f7d535aa) SHA1(65f100c15b07ec3aa21f5ed132e2fbf6e9120dbe) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "merl_sp.12c",  0x0000, 0x2000, CRC(517ecd57) SHA1(b0d4e2d106cddd6d19acd0e10f2d32544c84a900) )	

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "merlinmm.3j",  0x0000, 0x0020, CRC(d56e91f4) SHA1(152d88e4d168f697030d96c02ab9aeb220cc765d) ) /* palette */
	ROM_LOAD( "pingpong.11j", 0x0020, 0x0100, CRC(09d96b08) SHA1(81405e33eacc47f91ea4c7221d122f7e6f5b1e5d) ) /* sprites */
	ROM_LOAD( "pingpong.5h",  0x0120, 0x0100, CRC(8456046a) SHA1(8226f1325c14eb8aed5cd3c3d6bad9f9fd88c5fa) ) /* characters */
ROM_END

ROM_START( cashquiz )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cashqcv5.ic3", 0x0000, 0x4000, CRC(8e9e2bed) SHA1(1894d40f89226a810c703ce5e49fdfd64d70287f) )
	/* 0x4000 - 0x7fff = extra hardware for question board */

	ROM_REGION( 0x80000, REGION_USER1, ROMREGION_ERASEFF) /* Question roms */
	ROM_LOAD( "q30_soaps.ic1",		0x00000, 0x8000, CRC(b35a30ac) SHA1(5daf52a6d973f5a1b1ec3395962bcab690c54e43) )
	ROM_LOAD( "q10.ic2",			0x08000, 0x8000, CRC(54962e11) SHA1(3c89ac26ebc002b2bc723f1424a7ba3db7a98e5f) )
	ROM_LOAD( "q29_newsoccrick.ic3",0x10000, 0x8000, CRC(03d47262) SHA1(8a849cb4d4440042042cbdc0f34feebe71d6cb37) )
	ROM_LOAD( "q28_sportstime.ic4", 0x18000, 0x8000, CRC(2bd00476) SHA1(88ed9d26909873c52273290686b4783563edfb61) )
	ROM_LOAD( "q20_mot.ic5",		0x20000, 0x8000, CRC(17a38baf) SHA1(5560932e4747a242df7c8b7bbaf8679c9a8be6ac) )
	ROM_LOAD( "q14_popmusic2.ic6",	0x28000, 0x8000, CRC(e486d6ee) SHA1(421723fa7604c0509092891e53723191bd62e294) )
	ROM_LOAD( "q26_screenent.ic7",	0x30000, 0x8000, CRC(9d130515) SHA1(bfc32219d4d4eaca4efa02c3c46125144c8cd286) )
	ROM_LOAD( "q19.ic8",			0x38000, 0x8000, CRC(9f3f77e6) SHA1(aa1600215e774b090f379a0aae520027cd1795c1) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cashq.7h",  0x0000, 0x2000, CRC(44b72a4f) SHA1(a993f1570cf9d8f86d4229198e9b1a0d6a92e51f) )
	ROM_CONTINUE(0x0000, 0x2000)	/* rom is a 27128 in a 2764 socket */

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cashq.12c",  0x0000, 0x2000, NO_DUMP ) // missing :-(

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "cashquiz.3j",  0x0000, 0x0020, CRC(dc70e23b) SHA1(90948f76d5c61eb57838e013aa93d733913a2d92) ) /* palette */
	ROM_LOAD( "pingpong.11j", 0x0020, 0x0100, CRC(09d96b08) SHA1(81405e33eacc47f91ea4c7221d122f7e6f5b1e5d) ) /* sprites */
	ROM_LOAD( "pingpong.5h",  0x0120, 0x0100, CRC(8456046a) SHA1(8226f1325c14eb8aed5cd3c3d6bad9f9fd88c5fa) ) /* characters */
ROM_END

DRIVER_INIT( merlinmm )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	int i;
	
	/* decrypt program code */
	for( i = 0; i < 0x4000; i++ )
		ROM[i] = BITSWAP8(ROM[i],0,1,2,3,4,5,6,7);
}

WRITE8_HANDLER( cashquiz_question_bank_w )
{
	logerror("%04x cashquiz_question_bank_w address %04x data %02x\n", activecpu_get_pc(), offset+0x4000, data);
	/* how do we interpret the bank values?  I can't make sense of them */
}

READ8_HANDLER( cashquiz_unk_extra_r )
{
	logerror("%04x cashquiz_unk_extra_r address %04x\n", activecpu_get_pc(), offset+0x4000);
	return rand();
}

WRITE8_HANDLER( cashquiz_unk_extra_w )
{
	logerror("%04x cashquiz_unk_extra_w address %04x data %02x\n", activecpu_get_pc(), offset+0x4000, data);
}

DRIVER_INIT( cashquiz )
{
	UINT8 *ROM;
	int i;

	/* decrypt program code */
	ROM = memory_region(REGION_CPU1);
	for( i = 0; i < 0x4000; i++ )
		ROM[i] = BITSWAP8(ROM[i],0,1,2,3,4,5,6,7);

	/* decrypt questions - like wizz quiz? (some of the question roms match..) */
	ROM = memory_region(REGION_USER1);
	for( i = 0; i < 0x80000; i++ )
		ROM[i] = BITSWAP8(ROM[i],0,1,2,3,4,5,6,7);

	/* extra hardware ends up being mapped in the banked area */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, cashquiz_unk_extra_w ); // catch anything
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4001, 0, 0, cashquiz_question_bank_w); // seems to be banking..

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, cashquiz_unk_extra_r ); // catch anything
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x57ff, 0, 0, MRA8_BANK1); // reads bank data here?
	memory_set_bankptr( 1, memory_region(REGION_USER1) );
}


GAME( 1986, merlinmm, 0, merlinmm, merlinmm, merlinmm, ROT90,"Zilec - Zenitone", "Merlins Money Maze",0 )
GAME( 1986, cashquiz, 0, merlinmm, merlinmm, cashquiz, ROT0, "Zilec - Zenitone", "Cash Quiz (Type B, Version 5)", GAME_NOT_WORKING )
