/***************************************************************************

Reel Fun    (c) 1986
Find Out    (c) 1987
Trivia      (c) 1984 / 1986
Quiz        (c) 1991

driver by Nicola Salmoria

Trivia games "No Coins" mode: if DSW "No Coins" is on, coin inputs are
replaced by a 6th button to start games. This is a feature of the PCB for private use.

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/ticket.h"
#include "sound/dac.h"


VIDEO_UPDATE( findout )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
	return 0;
}


static UINT8 drawctrl[3];

static WRITE8_HANDLER( findout_drawctrl_w )
{
	drawctrl[offset] = data;
}

static WRITE8_HANDLER( findout_bitmap_w )
{
	int sx,sy;
	int fg,bg,mask,bits;
	static int prevoffset, yadd;

	videoram[offset] = data;

	yadd = (offset==prevoffset) ? (yadd+1):0;
	prevoffset = offset;

	fg = drawctrl[0] & 7;
	bg = 2;
	mask = 0xff;//drawctrl[2];
	bits = drawctrl[1];

	sx = 8 * (offset % 64);
	sy = offset / 64;
	sy = (sy + yadd) & 0xff;

//if (mask != bits)
//  popmessage("color %02x bits %02x mask %02x\n",fg,bits,mask);

	if (mask & 0x80) plot_pixel(tmpbitmap,sx+0,sy,(bits & 0x80) ? fg : bg);
	if (mask & 0x40) plot_pixel(tmpbitmap,sx+1,sy,(bits & 0x40) ? fg : bg);
	if (mask & 0x20) plot_pixel(tmpbitmap,sx+2,sy,(bits & 0x20) ? fg : bg);
	if (mask & 0x10) plot_pixel(tmpbitmap,sx+3,sy,(bits & 0x10) ? fg : bg);
	if (mask & 0x08) plot_pixel(tmpbitmap,sx+4,sy,(bits & 0x08) ? fg : bg);
	if (mask & 0x04) plot_pixel(tmpbitmap,sx+5,sy,(bits & 0x04) ? fg : bg);
	if (mask & 0x02) plot_pixel(tmpbitmap,sx+6,sy,(bits & 0x02) ? fg : bg);
	if (mask & 0x01) plot_pixel(tmpbitmap,sx+7,sy,(bits & 0x01) ? fg : bg);
}


static READ8_HANDLER( portC_r )
{
	return 4;
//  return (rand()&2);
}

static READ8_HANDLER( port1_r )
{
	return input_port_1_r(0) | (ticket_dispenser_0_r(0) >> 5);
}

static WRITE8_HANDLER( lamps_w )
{
	/* 5 button lamps */
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
	set_led_status(4,data & 0x10);
	/* lamps 6, 7, 8 in gt507, may be hopper slide 1, 2, 3 ? */
	set_led_status(5,data & 0x20);
	set_led_status(6,data & 0x40);
	set_led_status(7,data & 0x80);
}

static WRITE8_HANDLER( sound_w )
{
	/* bit 3 - coin lockout (lamp6 in test modes, set to lamp 10 as in getrivia.c) */
	coin_lockout_global_w(~data & 0x08);
	set_led_status(9,data & 0x08);

	/* bit 5 - ticket out in trivia games */
	ticket_dispenser_w(0, (data & 0x20)<< 2);

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);

//  logerror("%04x: sound_w %02x\n",activecpu_get_pc(),data);
//  popmessage("%02x",data);
}

static ppi8255_interface ppi8255_intf =
{
	2, 									/* 2 chips */
	{ input_port_0_r, input_port_2_r },	/* Port A read */
	{ port1_r, 	NULL },			/* Port B read */
	{ NULL,		portC_r },		/* Port C read */
	{ NULL,		NULL },			/* Port A write */
	{ NULL,		lamps_w },		/* Port B write */
	{ sound_w,	NULL },			/* Port C write */
};

MACHINE_RESET( findout )
{
	ppi8255_init(&ppi8255_intf);

	ticket_dispenser_init(100, 1, 1);
}


static READ8_HANDLER( catchall )
{
	int pc = activecpu_get_pc();

	if (pc != 0x3c74 && pc != 0x0364 && pc != 0x036d)	/* weed out spurious blit reads */
		logerror("%04x: unmapped memory read from %04x\n",pc,offset);

	return 0xff;
}

static WRITE8_HANDLER( banksel_main_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x8000);
}
static WRITE8_HANDLER( banksel_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x10000);
}
static WRITE8_HANDLER( banksel_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x18000);
}
static WRITE8_HANDLER( banksel_3_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x20000);
}
static WRITE8_HANDLER( banksel_4_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x28000);
}
static WRITE8_HANDLER( banksel_5_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x30000);
}


/* This signature is used to validate the question ROMs. Simple protection check? */
static int signature_answer,signature_pos;

static READ8_HANDLER( signature_r )
{
	return signature_answer;
}

static WRITE8_HANDLER( signature_w )
{
	if (data == 0) signature_pos = 0;
	else
	{
		static UINT8 signature[8] = { 0xff, 0x01, 0xfd, 0x05, 0xf5, 0x15, 0xd5, 0x55 };

		signature_answer = signature[signature_pos++];

		signature_pos &= 7;	/* safety; shouldn't happen */
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4803) AM_READ(ppi8255_0_r)
	AM_RANGE(0x5000, 0x5003) AM_READ(ppi8255_1_r)
	AM_RANGE(0x6400, 0x6400) AM_READ(signature_r)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x0000, 0xffff) AM_READ(catchall)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_WRITE(ppi8255_1_w)
	/* banked ROMs are enabled by low 6 bits of the address */
	AM_RANGE(0x603e, 0x603e) AM_WRITE(banksel_1_w)
	AM_RANGE(0x603d, 0x603d) AM_WRITE(banksel_2_w)
	AM_RANGE(0x603b, 0x603b) AM_WRITE(banksel_3_w)
	AM_RANGE(0x6037, 0x6037) AM_WRITE(banksel_4_w)
	AM_RANGE(0x602f, 0x602f) AM_WRITE(banksel_5_w)
	AM_RANGE(0x601f, 0x601f) AM_WRITE(banksel_main_w)
	AM_RANGE(0x6200, 0x6200) AM_WRITE(signature_w)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_ROM)	/* space for diagnostic ROM? */
	AM_RANGE(0x8000, 0x8002) AM_WRITE(findout_drawctrl_w)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(findout_bitmap_w)  AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)	/* overlapped by the above */
ADDRESS_MAP_END



#define TRIVIA_STANDARD_INPUT \
	PORT_START_TAG("IN0") \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )		/* ticket status */ \
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
\
	PORT_START	/* IN1 */\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( gt507uk )
	PORT_START      /* DSW A */
	PORT_DIPNAME( 0x01, 0x00, "If Ram Error" )
	PORT_DIPSETTING( 0x01, "Freeze" )
	PORT_DIPSETTING( 0x00, "Play" )
	PORT_DIPNAME( 0x0a, 0x08, "Payout" )
	PORT_DIPSETTING( 0x0a, "Bank" )
	PORT_DIPSETTING( 0x08, "N/A" )
	PORT_DIPSETTING( 0x02, "Credit" )
	PORT_DIPSETTING( 0x00, "Direct" )
	PORT_DIPNAME( 0x04, 0x04, "Payout Hardware" )
	PORT_DIPSETTING( 0x04, "Hopper" )
	PORT_DIPSETTING( 0x00, "Solenoid" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	TRIVIA_STANDARD_INPUT
	PORT_MODIFY("IN0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)	/* coin 3, 2, 4 order verified in test mode */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 ) PORT_IMPULSE(2)
INPUT_PORTS_END



static MACHINE_DRIVER_START( findout )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(findout)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(48, 511-48, 16, 255-16)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(findout)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gt507uk )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "triv_3_2.bin",    0x00000, 0x4000, CRC(2d72a081) SHA1(8aa32acf335d027466799b097e0de66bcf13247f) )
	ROM_LOAD( "rom_ad.bin",      0x08000, 0x2000, CRC(c81cc847) SHA1(057b7b75a2fe1abf88b23e7b2de230d9f96139f5) )
	ROM_LOAD( "aerospace",       0x10000, 0x8000, CRC(cb555d46) SHA1(559ae05160d7893ff96311a2177eba039a4cf186) )
	ROM_LOAD( "english_sport_4", 0x18000, 0x8000, CRC(6ae8a63d) SHA1(c6018141d8bbe0ed7619980bf7da89dd91d7fcc2) )
	ROM_LOAD( "general_facts",   0x20000, 0x8000, CRC(f921f108) SHA1(fd72282df5cee0e6ab55268b40785b3dc8e3d65b) )
	ROM_LOAD( "horrors",         0x28000, 0x8000, CRC(5f7b262a) SHA1(047480d6bf5c6d0603d538b84c996bd226f07f77) )
	ROM_LOAD( "pop_music",       0x30000, 0x8000, CRC(884fec7c) SHA1(b389216c17f516df4e15eee46246719dd4acb587) )
ROM_END

/*
When games are first run a RAM error will occur because the nvram needs initialising.
When ERROR appears, press F2, then F3 to reset, then F2 again and the game will start
*/

GAME( 1986, gt507uk,  0,      findout, gt507uk, 0, ROT0, "Grayhound Electronics", "Trivia (UK Version 5.07)",               GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
