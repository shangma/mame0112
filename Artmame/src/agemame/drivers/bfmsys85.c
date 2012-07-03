/***************************************************************************

    bfmsys85.c

    Bellfruit system85 driver, (under heavy construction !!!)

    A.G.E Code Copyright (C) 2004-2007 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


******************************************************************************************

  19-08-2005: Re-Animator

Standard system85 memorymap
___________________________________________________________________________           
   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-----------------------------------------
0000-1FFF  |R/W| D D D D D D D D | RAM (8k) battery backed up
-----------+---+-----------------+-----------------------------------------
2000-21FF  | W | D D D D D D D D | Reel 3 + 4 stepper latch
-----------+---+-----------------+-----------------------------------------
2200-23FF  | W | D D D D D D D D | Reel 1 + 2 stepper latch
-----------+---+-----------------+-----------------------------------------
2400-25FF  | W | D D D D D D D D | vfd + coin inhibits 
-----------+---+-----------------+-----------------------------------------
2600-27FF  | W | D D D D D D D D | electro mechanical meters
-----------+---+-----------------+-----------------------------------------
2800-28FF  | W | D D D D D D D D | triacs used for payslides/hoppers
-----------+---+-----------------+-----------------------------------------
2A00       |R/W| D D D D D D D D | MUX data
-----------+---+-----------------+-----------------------------------------
2A01       | W | D D D D D D D D | MUX control
-----------+---+-----------------+-----------------------------------------
2E00       | R | ? ? ? ? ? ? D D | IRQ status
-----------+---+-----------------+-----------------------------------------
3000       | W | D D D D D D D D | AY8912 data
-----------+---+-----------------+-----------------------------------------
3200       | W | D D D D D D D D | AY8912 address reg
-----------+---+-----------------+-----------------------------------------
3402       |R/W| D D D D D D D D | MC6850 control reg
-----------+---+-----------------+-----------------------------------------
3403       |R/W| D D D D D D D D | MC6850 data
-----------+---+-----------------+-----------------------------------------
3600       | W | ? ? ? ? ? ? D D | MUX enable
-----------+---+-----------------+-----------------------------------------
4000-5FFF  | R | D D D D D D D D | ROM (8k)
-----------+---+-----------------+-----------------------------------------
6000-7FFF  | R | D D D D D D D D | ROM (8k)
-----------+---+-----------------+-----------------------------------------
8000-FFFF  | R | D D D D D D D D | ROM (32k)
-----------+---+-----------------+-----------------------------------------

  TODO: - change this 

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/awpvid.h"
#include "machine/lamps.h"
#include "machine/steppers.h" // stepper motor
#include "machine/vacfdisp.h"  // vfd
#include "machine/mmtr.h"
#include "sound/ay8910.h"

#define VFD_RESET  0x20
#define VFD_CLOCK1 0x80
#define VFD_DATA   0x40   

// local prototypes ///////////////////////////////////////////////////////


// local vars /////////////////////////////////////////////////////////////

static int mmtr_latch;		  // mechanical meter latch
static int triac_latch;		  // payslide triac latch
static int vfd_latch;		  // vfd latch
static int irq_status;		  // custom chip IRQ status
static int optic_pattern;     // reel optics
static int acia_status;		  // MC6850 status
static int locked;			  // hardware lock/unlock status (0=unlocked)
static int timer_enabled;
static int reel_changed;  
static int coin_inhibits;
static int mux_output_strobe;
static int mux_input_strobe;
static int mux_input;

// user interface stuff ///////////////////////////////////////////////////

static UINT8 Lamps[128];		  // 128 multiplexed lamps
static UINT8 Inputs[64];		  // ??  multiplexed inputs

static struct AY8910interface ay8912_interface = // sound chip
{
0
};

///////////////////////////////////////////////////////////////////////////
// called if board is reset ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_RESET( bfm_sys85 )
{
	logerror("sys85_reset\n");

	vfd_latch         = 0;
	mmtr_latch        = 0;
	triac_latch       = 0;
	irq_status        = 0;
	timer_enabled     = 1;
	coin_inhibits     = 0;
	mux_output_strobe = 0;
	mux_input_strobe  = 0;
	mux_input         = 0;

	vfd_reset(0);	// reset display1

  // reset stepper motors /////////////////////////////////////////////////
	{
		int pattern =0, i;

		for ( i = 0; i < 6; i++)
		{
			Stepper_reset_position(i);
			if ( Stepper_optic_state(i) ) pattern |= 1<<i;
		}
	optic_pattern = pattern;
	}

	acia_status   = 0x02; // MC6850 transmit buffer empty !!!
	locked		  = 0x00; // hardware is NOT locked
}

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( watchdog_w )
{
}

///////////////////////////////////////////////////////////////////////////

static INTERRUPT_GEN( timer_irq )
{
	if ( timer_enabled )
	{
		irq_status = 0x01 |0x02; //0xff;
		cpunum_set_input_line(0, M6809_IRQ_LINE, PULSE_LINE );
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( irqlatch_r )
{
	int result = irq_status | 0x02;

	irq_status = 0;

	return result;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel12_w )
{
	if ( Stepper_update(0, data>>4) ) reel_changed |= 0x01;
	if ( Stepper_update(1, data   ) ) reel_changed |= 0x02;

	if ( Stepper_optic_state(0) ) optic_pattern |=  0x01;
	else                          optic_pattern &= ~0x01;
	if ( Stepper_optic_state(1) ) optic_pattern |=  0x02;
	else                          optic_pattern &= ~0x02;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel34_w )
{
	if ( Stepper_update(2, data>>4) ) reel_changed |= 0x04;
	if ( Stepper_update(3, data   ) ) reel_changed |= 0x08;

	if ( Stepper_optic_state(2) ) optic_pattern |=  0x04;
	else                          optic_pattern &= ~0x04;
	if ( Stepper_optic_state(3) ) optic_pattern |=  0x08;
	else                          optic_pattern &= ~0x08;
}

///////////////////////////////////////////////////////////////////////////
// mechanical meters //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mmtr_w )
{
	int  changed = mmtr_latch ^ data;
	long cycles  = MAME_TIME_TO_CYCLES(0, mame_timer_get_time() );

	mmtr_latch = data;

	if ( changed & 0x01 )	Mechmtr_update(0, cycles, data & 0x01 );
	if ( changed & 0x02 )	Mechmtr_update(1, cycles, data & 0x02 );
	if ( changed & 0x04 )	Mechmtr_update(2, cycles, data & 0x04 );
	if ( changed & 0x08 )	Mechmtr_update(3, cycles, data & 0x08 );
	if ( changed & 0x10 )	Mechmtr_update(4, cycles, data & 0x10 );
	if ( changed & 0x20 )	Mechmtr_update(5, cycles, data & 0x20 );
	if ( changed & 0x40 )	Mechmtr_update(6, cycles, data & 0x40 );
	if ( changed & 0x80 )	Mechmtr_update(7, cycles, data & 0x80 );

	if ( data ) cpunum_set_input_line(0, M6809_FIRQ_LINE, PULSE_LINE );
}
///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mmtr_r )
{
	return mmtr_latch;
}
///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vfd_w )
{
	int changed = vfd_latch ^ data;

	vfd_latch = data;

	if ( changed )
	{
		if ( changed & VFD_RESET )
		{ // vfd reset line changed 
			if ( !(data & VFD_RESET) )
			{ // reset the vfd
				vfd_reset(0);
				vfd_reset(1);
				vfd_reset(2);

				logerror("vfd reset\n");
			}
		}


		if ( changed & VFD_CLOCK1 )
		{ // clock line changed
			if ( !(data & VFD_CLOCK1) && (data & VFD_RESET) )
			{ // new data clocked into vfd //////////////////////////////////////
			vfd_shift_data(0, data & VFD_DATA );
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////
// input / output multiplexers ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// conversion table BFM strobe data to internal lamp numbers
static const UINT8 BFM_strcnv85[] = 
{
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
  0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37, 0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47, 0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57, 0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
  0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67, 0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
  0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, 0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,

  0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87, 0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
  0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97, 0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
  0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7, 0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
  0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7, 0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
  0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7, 0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
  0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7, 0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
  0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7, 0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
  0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7, 0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux_ctrl_w )
{
	switch ( data & 0xF0 )
	{
	case 0x10:
	//logerror(" sys85 mux: left entry matrix scan %02X\n", data & 0x0F);
	break;

	case 0x20:
	//logerror(" sys85 mux: set scan rate %02X\n", data & 0x0F);
	break;

	case 0x40:
	//logerror(" sys85 mux: read strobe");
	mux_input_strobe = data & 0x07;

	if ( mux_input_strobe == 5 ) Inputs[5] = 0xFF ^ optic_pattern;

	mux_input = ~Inputs[mux_input_strobe];
	break;

	case 0x80:
	mux_output_strobe = data & 0x0F;
	break;

	case 0xC0:
	//logerror(" sys85 mux: clear all outputs\n");
	break;

	case 0xE0:	  // End of interrupt
	break;

  }
}

static READ8_HANDLER( mux_ctrl_r )
{
  // software waits for bit7 to become low

  return 0;
}

static WRITE8_HANDLER( mux_data_w )
{
	int pattern = 0x01, i,
	off = mux_output_strobe<<4;

	for ( i = 0; i < 8; i++ )
	{
		Lamps[ BFM_strcnv85[off]] = data & pattern ? 1 : 0;
		pattern <<= 1;
		off++;
	}

	if ( mux_output_strobe == 0 ) Lamps_SetBrightness(0, 128, (UINT8*)Lamps);
}

static READ8_HANDLER( mux_data_r )
{
	return mux_input;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux_enable_w )
{
}

///////////////////////////////////////////////////////////////////////////
// serial port ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( aciactrl_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( aciadata_w )
{
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( aciastat_r )
{
	return acia_status;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( aciadata_r )
{
	return 0; //Not actually, but keeps compiler happy for now
}

///////////////////////////////////////////////////////////////////////////
// payslide triacs ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( triac_w )
{
	triac_latch = data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( triac_r )
{
	return triac_latch;
}

// system85 board init ////////////////////////////////////////////////////

#if 0 //Disabled
static const unsigned short AddressDecode[]=
{
	0x0800,0x1000,0x0001,0x0004,0x0008,0x0020,0x0080,0x0200,
	0x0100,0x0040,0x0002,0x0010,0x0400,0x2000,0x4000,0x8000,

	0
};

static const unsigned char DataDecode[]=
{
	0x02,0x08,0x20,0x40,0x10,0x04,0x01,0x80,

	0
};


static unsigned char codec_data[256];




static void decode_sc1(int rom_region)
{
	UINT8 *tmp, *rom;

	rom = memory_region(rom_region);

	tmp = malloc(0x10000);

	if ( tmp )
	{
		int i;
		long address;

		memcpy(tmp, rom, 0x10000);

		for ( i = 0; i < 256; i++ )
		{ 
			unsigned char data, pattern, newdata, *tab;
			data    = i;

			tab     = (UINT8*)DataDecode;
			pattern = 0x01;
			newdata = 0;
			do
			{
				newdata |= data & pattern ? *tab : 0;
				pattern <<= 1;
			} while ( *(++tab) );

			codec_data[i] = newdata;
		}

	for ( address = 0; address < 0x10000; address++)
	{
		int			  newaddress, pattern;
		unsigned short *tab;

		tab      = (unsigned short*)AddressDecode;
		pattern  = 0x0001;
		newaddress = 0;
		do
		{
			newaddress |= address & pattern ? *tab : 0;
			pattern <<= 1;
		} while ( *(++tab) );

		rom[newaddress] = codec_data[ tmp[address] ];
	}
	free( tmp );
}

}

#endif
// machine start (called only once) ////////////////////////////////////////

static MACHINE_START( bfm_sys85 )
{

	Lamps_init(128);					

	Stepper_init(0, STEPPER_48STEP_REEL);
	Stepper_init(1, STEPPER_48STEP_REEL);
	Stepper_init(2, STEPPER_48STEP_REEL);
	Stepper_init(3, STEPPER_48STEP_REEL);
  
	vfd_init(0,VFDTYPE_MSC1937,1);

  //decode_sc1(REGION_CPU1);
	return 0;
}


// memory map for bellfruit system85 board ////////////////////////////////

static ADDRESS_MAP_START( memmap, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) //8k RAM
	AM_RANGE(0x2000, 0x21FF) AM_WRITE(reel34_w)			// reel 3+4 latch
	AM_RANGE(0x2200, 0x23FF) AM_WRITE(reel12_w)			// reel 1+2 latch
	AM_RANGE(0x2400, 0x25FF) AM_WRITE(vfd_w)			// vfd latch

	AM_RANGE(0x2600, 0x27FF) AM_READ(mmtr_r)			// mechanical meter latch
	AM_RANGE(0x2600, 0x27FF) AM_WRITE(mmtr_w)			// mech meters
	AM_RANGE(0x2800, 0x2800) AM_READ(triac_r)			// payslide triacs
	AM_RANGE(0x2800, 0x29FF) AM_WRITE(triac_w)			// triacs
	AM_RANGE(0x2A00, 0x2A00) AM_READ(mux_data_r)		// mux data read
	AM_RANGE(0x2A00, 0x2A00) AM_WRITE(mux_data_w)		// mux
	AM_RANGE(0x2A01, 0x2A01) AM_READ(mux_ctrl_r)		// mux status register
	AM_RANGE(0x2A01, 0x2A01) AM_WRITE(mux_ctrl_w)		// mux control register
	AM_RANGE(0x2E00, 0x2E00) AM_READ(irqlatch_r)		// irq latch ( MC6850 / timer )

	AM_RANGE(0x3000, 0x3000) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x3001, 0x3001) AM_READ(soundlatch_r)		// AY8912 
	AM_RANGE(0x3200, 0x3200) AM_WRITE(AY8910_control_port_0_w)

	AM_RANGE(0x3402, 0x3402) AM_WRITE(aciactrl_w)		// MC6850 control register
	AM_RANGE(0x3403, 0x3403) AM_WRITE(aciadata_w)		// MC6850 data register

	AM_RANGE(0x3406, 0x3406) AM_READ(aciastat_r)		// MC6850 status
	AM_RANGE(0x3407, 0x3407) AM_READ(aciadata_r)		// MC6850 data
	AM_RANGE(0x3600, 0x3600) AM_WRITE(mux_enable_w)		// mux enable

	AM_RANGE(0x4000, 0xffff) AM_READ(MRA8_ROM)			// 48K ROM
	AM_RANGE(0x8000, 0xFFFF) AM_WRITE(watchdog_w)		// kick watchdog

ADDRESS_MAP_END

// machine driver for system85 board //////////////////////////////////////

static MACHINE_DRIVER_START( bfmsystem85 )

	MDRV_MACHINE_START(bfm_sys85)						// main system85 board initialisation
	MDRV_MACHINE_RESET(bfm_sys85)
	MDRV_CPU_ADD_TAG("main", M6809, 1000000 )			// 6809 CPU at 1 Mhz
	MDRV_CPU_PROGRAM_MAP(memmap,0)						// setup read and write memorymap
	MDRV_CPU_PERIODIC_INT(timer_irq, TIME_IN_HZ(1000) )	// generate 1000 IRQ's per second

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 1000000)						// add AY8912 soundchip
	MDRV_SOUND_CONFIG(ay8912_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_NVRAM_HANDLER(generic_0fill)					// load/save nv RAM

	// fake video ////////////////////////
	MDRV_DEFAULT_LAYOUT(layout_awpvid)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 288, 29)
	MDRV_SCREEN_VISIBLE_AREA(  0, 288-1, 0, 29-1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_VIDEO_START( awpvideo)
	MDRV_VIDEO_UPDATE(awpvideo)

	MDRV_PALETTE_LENGTH(22)
	MDRV_COLORTABLE_LENGTH(22)
	MDRV_PALETTE_INIT(awpvideo)

MACHINE_DRIVER_END

// input ports for system85 board /////////////////////////////////////////

INPUT_PORTS_START( bfmsys85_inputs )

	PORT_START_TAG("IN0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.25")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN  )

	PORT_START_TAG("IN1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("10")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("11")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("12")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("13")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("14")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("15")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("16")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_BUTTON8) PORT_NAME("17")

	//PORT_BIT( 0x01, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	//PORT_BIT( 0x02, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Bookkeeping") PORT_CODE(KEYCODE_F1) PORT_TOGGLE

INPUT_PORTS_END


// ROM definition /////////////////////////////////////////////////////////

ROM_START( bfmsc271 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sc271.bin",  0x8000, 0x8000,  CRC(58e9c9df) SHA1(345c5aa279327d7142edc6823aad0cfd40cbeb73))
ROM_END

//    year,name,   parent, machine,  input,           init, monitor,     company,     fullname,            flags
GAME( 1985,bfmsc271, 0, bfmsystem85, bfmsys85_inputs,  0,	  0,       "BFM/ELAM",   "Supercards (Dutch, Game Card 39-340-271?)", GAME_NOT_WORKING|GAME_SUPPORTS_SAVE )
