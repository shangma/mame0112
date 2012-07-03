/*

Trivia By Greyhound Electronics

  driver by Pierpaolo Prazzoli and graphic fixes by Tomasz Slanina
  based on Find Out driver

ROM BOARD (2764 ROMs)

FILE            CS
TRIVB1.BIN      D900
TRIVB2.BIN      4600

ROM BOARD (27128 ROMs)

FILE            CS
SPRT1_9.BIN     8600
GENERAL5.BIN    A900
ENTR2_9.BIN     B300
COMC2_9.BIN     2800
HCKY5_9.BIN     3700

ROM board has a part # UVM10B  1984
Main board has a part # UV-1B Rev 5 1982-83-84

Processor: Z80
Support Chips:(2) 8255s
RAM: 6117on ROM board and (24) MCM4517s on main board

Trivia games "No Coins" mode: if DSW "No Coins" is on, coin inputs are
replaced by a 6th button to start games. This is a feature of the PCB for private use.

Selection/Poker payout button: if pressed, all coins/credits are gone and added to the
payout bookkeeping, shown in the service mode under the coin in total. Last Winner shows
the last payout. Payout hardware is unknown.

*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/ticket.h"
#include "sound/dac.h"

static UINT8 *drawctrl;

static WRITE8_HANDLER( getrivia_bitmap_w )
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

	/* 3 button lamps for deal, cancel, stand in poker games;
    lamp order verified in poker and selection self tests */
	set_led_status(7,data & 0x20);
	set_led_status(5,data & 0x40);
	set_led_status(6,data & 0x80);
}

static WRITE8_HANDLER( sound_w )
{
	/* bit 3 - coin lockout, lamp10 in poker / lamp6 in trivia test modes */
	coin_lockout_global_w(~data & 0x08);
	set_led_status(9,data & 0x08);

	/* bit 5 - ticket out in trivia games */
	ticket_dispenser_w(0, (data & 0x20)<< 2);

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);
}

static WRITE8_HANDLER( sound2_w )
{
	/* bit 3,6 - coin lockout, lamp10+11 in selection test mode */
	coin_lockout_w(0, ~data & 0x08);
	coin_lockout_w(1, ~data & 0x40);
	set_led_status(9,data & 0x08);
	set_led_status(10,data & 0x40);

	/* bit 4,5 - lamps 12, 13 in selection test mode;
    12 lights up if dsw maximum bet = 30 an bet > 15 or if dsw maximum bet = 10 an bet = 10 */
	set_led_status(11,data & 0x10);
	set_led_status(12,data & 0x20);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);
}

static WRITE8_HANDLER( lamps2_w )
{
	/* bit 4 - play/raise button lamp, lamp 9 in poker test mode  */
	set_led_status(8,data & 0x10);
}

static WRITE8_HANDLER( nmi_w )
{
	/* bit 4 - play/raise button lamp, lamp 9 in selection test mode  */
	set_led_status(8,data & 0x10);

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);
}

static WRITE8_HANDLER( banksel_1_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x10000);
}
static WRITE8_HANDLER( banksel_2_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x14000);
}
static WRITE8_HANDLER( banksel_3_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x18000);
}
static WRITE8_HANDLER( banksel_4_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x1c000);
}
static WRITE8_HANDLER( banksel_5_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x20000);
}
static WRITE8_HANDLER( banksel_1_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x12000);
}
static WRITE8_HANDLER( banksel_2_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x16000);
}
static WRITE8_HANDLER( banksel_3_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x1a000);
}
static WRITE8_HANDLER( banksel_4_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x1e000);
}
static WRITE8_HANDLER( banksel_5_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x22000);
}

static ADDRESS_MAP_START( getrivia_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x4000, 0x47ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4800, 0x4803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0x600f, 0x600f) AM_WRITE(banksel_5_1_w)
	AM_RANGE(0x6017, 0x6017) AM_WRITE(banksel_4_1_w)
	AM_RANGE(0x601b, 0x601b) AM_WRITE(banksel_3_1_w)
	AM_RANGE(0x601d, 0x601d) AM_WRITE(banksel_2_1_w)
	AM_RANGE(0x601e, 0x601e) AM_WRITE(banksel_1_1_w)
	AM_RANGE(0x608f, 0x608f) AM_WRITE(banksel_5_2_w)
	AM_RANGE(0x6097, 0x6097) AM_WRITE(banksel_4_2_w)
	AM_RANGE(0x609b, 0x609b) AM_WRITE(banksel_3_2_w)
	AM_RANGE(0x609d, 0x609d) AM_WRITE(banksel_2_2_w)
	AM_RANGE(0x609e, 0x609e) AM_WRITE(banksel_1_2_w)
	AM_RANGE(0x8000, 0x8002) AM_WRITE(MWA8_RAM) AM_BASE(&drawctrl)
	AM_RANGE(0x8000, 0x9fff) AM_ROM /* space for diagnostic ROM? */
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_RAM, getrivia_bitmap_w) AM_BASE(&videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gselect_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x4000, 0x40ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4400, 0x4400) AM_WRITE(banksel_1_1_w)
	AM_RANGE(0x4401, 0x4401) AM_WRITE(banksel_1_2_w)
	AM_RANGE(0x4402, 0x4402) AM_WRITE(banksel_2_1_w)
	AM_RANGE(0x4403, 0x4403) AM_WRITE(banksel_2_2_w)
	AM_RANGE(0x4800, 0x4803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0x8000, 0x8002) AM_WRITE(MWA8_RAM) AM_BASE(&drawctrl)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_RAM, getrivia_bitmap_w) AM_BASE(&videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gepoker_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x4000, 0x47ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4800, 0x4803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0x60ef, 0x60ef) AM_WRITE(banksel_3_1_w)
	AM_RANGE(0x60f7, 0x60f7) AM_WRITE(banksel_2_2_w)
	AM_RANGE(0x60fb, 0x60fb) AM_WRITE(banksel_2_1_w)
	AM_RANGE(0x60fd, 0x60fd) AM_WRITE(banksel_1_2_w)
	AM_RANGE(0x60fe, 0x60fe) AM_WRITE(banksel_1_1_w)
	AM_RANGE(0x8000, 0x8002) AM_WRITE(MWA8_RAM) AM_BASE(&drawctrl)
	AM_RANGE(0x8000, 0xbfff) AM_ROM /* space for diagnostic ROM? */
	AM_RANGE(0xe000, 0xffff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_RAM, getrivia_bitmap_w) AM_BASE(&videoram)
ADDRESS_MAP_END


INPUT_PORTS_START( gselect )
	PORT_START      /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Poker: Discard Cards" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x06, 0x06, "Poker: Pay on" )
	PORT_DIPSETTING(    0x06, "any Pair" )
	PORT_DIPSETTING(    0x04, "Pair of Eights or better" )
	PORT_DIPSETTING(    0x02, "Pair of Jacks or better" )
	PORT_DIPSETTING(    0x00, "Pair of Aces only" )
	PORT_DIPNAME( 0x08, 0x00, "Maximum Bet" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x10, 0x10, "Poker: Credits needed for 2 Jokers" )
	PORT_DIPSETTING(    0x10, "8" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xe0, 0x80, "Payout Percentage" )
	PORT_DIPSETTING(    0xe0, "35" )
	PORT_DIPSETTING(    0xc0, "40" )
	PORT_DIPSETTING(    0xa0, "45" )
	PORT_DIPSETTING(    0x80, "50" )
	PORT_DIPSETTING(    0x60, "55" )
	PORT_DIPSETTING(    0x40, "60" )
	PORT_DIPSETTING(    0x20, "65" )
	PORT_DIPSETTING(    0x00, "70" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_IMPULSE(2) PORT_NAME("Button 12 ?")
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_IMPULSE(2) PORT_NAME ("Payout")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_IMPULSE(2) PORT_NAME ("Play / Raise")

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_IMPULSE(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_IMPULSE(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_IMPULSE(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_IMPULSE(2) PORT_NAME ("Deal")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_IMPULSE(2) PORT_NAME ("Cancel")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_IMPULSE(2) PORT_NAME ("Stand")
/*  Button 8, 6, 7 order verified in test mode switch test */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gepoker )
	PORT_INCLUDE( gselect )

	PORT_MODIFY("IN0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* no coin 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* no button 12 */
INPUT_PORTS_END

static ppi8255_interface getrivia_ppi8255_intf =
{
	2, 						/* 2 chips */
	{ input_port_0_r, input_port_2_r },  /* Port A read */
	{ port1_r,        NULL },            /* Port B read */
	{ NULL,           NULL },            /* Port C read */
	{ NULL,           NULL },            /* Port A write */
	{ NULL,           lamps_w },         /* Port B write */
	{ sound_w,        lamps2_w },        /* Port C write */
};

static ppi8255_interface gselect_ppi8255_intf =
{
	2, 						/* 2 chips */
	{ input_port_0_r, input_port_2_r },  /* Port A read */
	{ input_port_1_r, NULL },            /* Port B read */
	{ NULL,           input_port_3_r },  /* Port C read */
	{ NULL,           NULL },            /* Port A write */
	{ NULL,           lamps_w },         /* Port B write */
	{ sound2_w,       nmi_w },           /* Port C write */
};

static MACHINE_RESET( getrivia )
{
	ppi8255_init(&getrivia_ppi8255_intf);

	ticket_dispenser_init(100, 1, 1);
}

static MACHINE_RESET( gselect )
{
	ppi8255_init(&gselect_ppi8255_intf);
}

static MACHINE_DRIVER_START( getrivia )
	MDRV_CPU_ADD_TAG("cpu",Z80,4000000) /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(getrivia_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(48, 511-48, 16, 255-16)
	MDRV_PALETTE_LENGTH(256)

	MDRV_MACHINE_RESET(getrivia)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gselect )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(getrivia)

	MDRV_CPU_MODIFY("cpu")
	MDRV_CPU_PROGRAM_MAP(gselect_map,0)

	MDRV_MACHINE_RESET(gselect)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gepoker )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(getrivia)

	MDRV_CPU_MODIFY("cpu")
	MDRV_CPU_PROGRAM_MAP(gepoker_map,0)
MACHINE_DRIVER_END

/***************************************************
Rom board is UVM-1A

Contains:
 3 2732  eproms (Program Code, 1 empty socket)
 2 X2212P (Ram chips, no battery backup)
 DM7408N

****************************************************/

ROM_START( m075 )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "m075.1", 0x00000, 0x1000, CRC(ad42465b) SHA1(3f06847a9aecb0592f99419dba9be5f18005d57b) ) /* rom board UMV-1A */
	ROM_LOAD( "m075.2", 0x01000, 0x1000, CRC(bd129fc2) SHA1(2e05ba34922c16d127be32941447013efea05bcd) )
	ROM_LOAD( "m075.3", 0x02000, 0x1000, CRC(45725bc9) SHA1(9e6dcbec955ef8190f2307ddb367b24b7f34338d) ) /* Rom loads _ALL_WRONG_ */
ROM_END

/***************************************************
Rom board is UVM-1B

Contains:
 4 2732  eproms (Program Code)
 Battery (3.5V litium battery) backed up NEC 8444XF301
 DM7408N

****************************************************/

ROM_START( superbwl )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "super_bowl.1", 0x00000, 0x1000, CRC(82edf064) SHA1(8a26377590282f51fb39d013452ba11252e7dd05) ) /* rom board UMV-1B */
	ROM_LOAD( "super_bowl.2", 0x01000, 0x1000, CRC(2438dd1f) SHA1(26bbd1cb3d0d5b93f61b92ff95948ac9de060715) )
	ROM_LOAD( "super_bowl.3", 0x02000, 0x1000, CRC(9b111430) SHA1(9aaa755f3e4b369477c1a0525c994a19fe0f6107) )
	ROM_LOAD( "super_bowl.4", 0x03000, 0x1000, CRC(037cad42) SHA1(d4037a28bb49b31358b5d560e5e028d958ae2bc9) ) /* Rom loads _ALL_WRONG_ */
ROM_END

/***************************************************
Rom board is UVM-4B

Contains 5 2764 eproms, MMI PAL16R4CN

Battery (3V litium battery) backed up HM6117P-4

Roms labeled as:

4/1  at spot 1
BLJK at spot 2
POKR at spot 3
SPRD at spot 4
SLOT at spot 3

Alt set included BONE in place of SPRD & a newer SLOT

These board sets may also be known as the V116 (or V16)
sets as the alt set also included that name on the labels

****************************************************/

ROM_START( gs4002 )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "4-1.1",          0x00000, 0x2000, CRC(a456e456) SHA1(f36b96ac31ce0f128ecb94f94d1dbdd88ee03c77) ) /* V16M 5-4-84 */
	/* Banked roms */
	ROM_LOAD( "bljk_3-16-84.2", 0x10000, 0x2000, CRC(c3785523) SHA1(090f324fc7adb0a36b189cf04086f0e050895ee4) )
	ROM_LOAD( "pokr_5-16-84.3", 0x12000, 0x2000, CRC(f0e99cc5) SHA1(02fdc95974e503b6627930918fcc3c029a7a4612) )
	ROM_LOAD( "sprd_1-24-84.4", 0x14000, 0x2000, CRC(5fe90ed4) SHA1(38db69567d9c38f78127e581fdf924aca4926378) )
	ROM_LOAD( "slot_1-24-84.5", 0x16000, 0x2000, CRC(cd7cfa4c) SHA1(aa3de086e5a1018b9e5a18403a6144a6b0ed1036) )
ROM_END

ROM_START( gs4002a )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "4-1.1",          0x00000, 0x2000, CRC(a456e456) SHA1(f36b96ac31ce0f128ecb94f94d1dbdd88ee03c77) ) /* V16M 5-4-84 */
	/* Banked roms */
	ROM_LOAD( "bljk_3-16-84.2", 0x10000, 0x2000, CRC(c3785523) SHA1(090f324fc7adb0a36b189cf04086f0e050895ee4) )
	ROM_LOAD( "pokr_5-16-84.3", 0x12000, 0x2000, CRC(f0e99cc5) SHA1(02fdc95974e503b6627930918fcc3c029a7a4612) )
	ROM_LOAD( "bone_5-16-84.4", 0x14000, 0x2000, CRC(eccd2fb0) SHA1(2683e432ffcca4280c31f57b2596e4389bc59b7b) )
	ROM_LOAD( "slot_9-24-84.5", 0x16000, 0x2000, CRC(25d8c504) SHA1(2d52b66e8a1f06f486015440668bd924a123dad0) )
ROM_END

/*
Greyhound Poker board...

Standard Greyhound Electronics Inc UV-1B mainboard.

Rom board UVM 10 B or UMV 10 C

Battery backed up NEC D449C ram
PAL16R4
74L374

Roms in this order on the UMV 10 C board:

Label        Normaly in the slot
--------------------------------
High
Control
rom1         Joker Poker
rom2         Black jack
rom3         Rolling Bones
rom4         Casino Slots
rom5         Horse Race

For UMV 10 B: 1C, 2C, 1, 2, 3, 4, & 5

There looks to be several types and combos for these, some are "ICB" or "IAM"
It also looks like operators mixed & matched to upgrade (some times incorrectly)
their boards.  Sets will be filled in as found and dumped.

There are some versions, like, the ICB sets that use 2764 roms for all roms. While the IAM set uses
27128 roms for all roms.  These roms are the correct size, but it's currently not known if the rom
board (UVM 10 B/C) "sees" them as 27128 or the standard size of 2764.

Dumped, but not known to be supported by any High/Control combo:
ROM_LOAD( "rollingbones_am_3-16-84",  0x16000, 0x4000, CRC(41879e9b) SHA1(5106d5772bf43b28817e27efd16c785359cd929e) ) // Might work with IAM control, once it gets figured out

The ICB set may also be known as the M105 set as some label sets included that name.

*/

ROM_START( gepoker ) /* v50.02 with most roms for ICB dated 8-16-84 */
	ROM_REGION( 0x1b000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "control_icb_8-16",  0x00000, 0x2000, CRC(0103963d) SHA1(9bc646e721048b84111e0686eaca23bc24eee3e2) )
	ROM_LOAD( "high_icb_6-25",     0x0e000, 0x2000, CRC(dfb6592e) SHA1(d68de9f537d3c14279dc576424d195bb266e3897) )
	/* Banked roms */
	ROM_LOAD( "jokerpoker_icb_8-16-84",    0x10000, 0x2000, CRC(0834a1e6) SHA1(663e6f4e0586eb9b84d3098aef8c596585c27304) )
	ROM_LOAD( "blackjack_icb_8-16-84",     0x12000, 0x2000, CRC(cff27ffd) SHA1(fd85b54400b2f22ae92042b01a2c162e64d2d066) )
	ROM_LOAD( "rollingbones_icb_8-16-84",  0x14000, 0x2000, CRC(52d66cb6) SHA1(57db34906fcafd37f3a361df209dafe080aeac16) )
	ROM_LOAD( "casinoslots_icb_8-16-84",   0x16000, 0x2000, CRC(3db002a3) SHA1(7dff4efceee37b25328303cf0606bf4baa4df5f3) )
	ROM_LOAD( "horserace_icb_3-19-85",     0x18000, 0x2000, CRC(f1e6e61e) SHA1(944b1ab4af911e5ed136f1fca3c44219726eeebb) )
ROM_END

ROM_START( gepoker1 ) /* v50.08 with most roms for IAM dated 8-16-84 */
	ROM_REGION( 0x24000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "control_iam_8-16",  0x00000, 0x4000, CRC(345434b9) SHA1(ec880f6f5e90aa971937e0270701e323f6a83671) ) /* all roms were 27128, twice the size of other sets */
	ROM_LOAD( "high_iam_8-16",     0x0c000, 0x4000, CRC(57000fb7) SHA1(144723eb528c4816b9aa4b0ba77d2723c6442546) ) /* Is only the 1st half used by the board / program? */
	/* Banked roms */
	ROM_LOAD( "jokerpoker_iam_8-16-84",    0x10000, 0x4000, CRC(33794a87) SHA1(2b46809623713582746d9f817db33077f15a3684) ) /* This set is verified correct by 3 different sets checked */
	ROM_LOAD( "blackjack_iam_8-16-84",     0x14000, 0x4000, CRC(6e10b5b8) SHA1(5dc294b4a562193a99b0d307323fcc084a053426) )
	ROM_LOAD( "rollingbones_iam_8-16-84",  0x18000, 0x4000, CRC(26949774) SHA1(20571b955521ec3929430249aa651cee8a97043d) )
	ROM_LOAD( "casinoslots_iam_8-16-84",   0x1c000, 0x4000, CRC(c5a1eec6) SHA1(43d31bfe4cbbb6b86f52f675f513050866443176) )
	ROM_LOAD( "horserace_iam_3-19-84",     0x20000, 0x4000, CRC(7b9e75cb) SHA1(0db8da6f5f59f57886766bec96102d43796567ef) )
ROM_END

ROM_START( gepoker2 ) /* v50.02 with roms for ICB dated 9-30-86 */
	ROM_REGION( 0x1b000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "control_icb_9-30",  0x00000, 0x2000, CRC(08b996f2) SHA1(5f5efb5015ec9571cc94734c18debfadaa28f585) )
	ROM_LOAD( "high_icb_6-25a",    0x0e000, 0x2000, CRC(6ddc1750) SHA1(ee19206b7f4a98e3e7647414127f4e09b3e9134f) )
	/* Banked roms */
	ROM_LOAD( "jokerpoker_icb_9-30-86",    0x10000, 0x2000, CRC(a1473367) SHA1(9b37ccafc02704e8f1d61150326494e86148d84e) )
	ROM_LOAD( "casinoslots_icb_9-30-86",   0x12000, 0x2000, CRC(713c3963) SHA1(a9297c04fc44522ca6891516a2c744712132896a) )
ROM_END

ROM_START( gepoker3 ) /* v50.02 with control dated 9-30-84 */
	ROM_REGION( 0x1b000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "control_icb_9-30",  0x00000, 0x2000, CRC(08b996f2) SHA1(5f5efb5015ec9571cc94734c18debfadaa28f585) )
	ROM_LOAD( "high_icb_6-25a",    0x0e000, 0x2000, CRC(6ddc1750) SHA1(ee19206b7f4a98e3e7647414127f4e09b3e9134f) )
	/* Banked roms */
	ROM_LOAD( "jokerpoker_cb_10-19-88",    0x10000, 0x2000, CRC(a590af75) SHA1(63bc64fbc9ac0c489b1f4894d77a4be13d7251e7) )
ROM_END

GAME( 1982, m075,     0,        gselect,  gselect,  0, ROT0, "Greyhound Electronics", "M075 Poker (Version 16.03B)",            GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAME( 1982, superbwl, 0,        gselect,  gselect,  0, ROT0, "Greyhound Electronics", "Super Bowl (Version 16.03B)",            GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

GAME( 1982, gs4002,   0,        gselect,  gselect,  0, ROT0, "Greyhound Electronics", "Selection (Version 40.02TMB) set 1",     GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1982, gs4002a,  gs4002,   gselect,  gselect,  0, ROT0, "Greyhound Electronics", "Selection (Version 40.02TMB) set 2",     GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

GAME( 1984, gepoker,  0,        gepoker,  gepoker,  0, ROT0, "Greyhound Electronics", "Poker (Version 50.02 ICB)",              GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gepoker1, 0,        gepoker,  gepoker,  0, ROT0, "Greyhound Electronics", "Poker (Version 50.08 IAM)",              GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAME( 1984, gepoker2, gepoker,  gepoker,  gepoker,  0, ROT0, "Greyhound Electronics", "Poker (Version 50.02 ICB set 2)",        GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gepoker3, gepoker,  gepoker,  gepoker,  0, ROT0, "Greyhound Electronics", "Poker (Version 50.02 ICB set 3)",        GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
