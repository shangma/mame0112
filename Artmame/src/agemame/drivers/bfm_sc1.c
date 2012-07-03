/*****************************************************************************************

    bfm_sc1.c

    Bellfruit scorpion1 driver, (under heavy construction !!!)

    A.G.E Code Copyright (C) 2004-2006 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org


******************************************************************************************

  20-01-2007: J Wallace: Tidy up of coding
  30-12-2006: J Wallace: Fixed init routines.
  27-09-2006: El Condor: Started looking into A Question of Sport (not enabled).
  07-03-2006: El Condor: Recoded to more accurately represent the hardware setup.
  19-08-2005: Re-Animator
  16-08-2005: Converted to MAME protocol for when COBRA board is completed.
  25-08-2005: Added support for adder2 (Toppoker), added support for NEC upd7759 soundcard

Standard scorpion1 memorymap
___________________________________________________________________________________
   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-------------------------------------------------
0000-1FFF  |R/W| D D D D D D D D | RAM (8k) battery backed up
-----------+---+-----------------+-------------------------------------------------
2000-21FF  | W | D D D D D D D D | Reel 3 + 4 stepper latch
-----------+---+-----------------+-------------------------------------------------
2200-23FF  | W | D D D D D D D D | Reel 1 + 2 stepper latch
-----------+---+-----------------+-------------------------------------------------
2400-25FF  | W | D D D D D D D D | vfd + coin inhibits 
-----------+---+-----------------+-------------------------------------------------
2600-27FF  | W | D D D D D D D D | electro mechanical meters
-----------+---+-----------------+-------------------------------------------------
2800-28FF  | W | D D D D D D D D | triacs used for payslides/hoppers
-----------+---+-----------------+-------------------------------------------------
2A00       |R/W| D D D D D D D D | MUX1 control reg (IN: mux inputs, OUT:lamps)
-----------+---+-----------------+-------------------------------------------------
2A01       | W | D D D D D D D D | MUX1 low data bits
-----------+---+-----------------+-------------------------------------------------
2A02       | W | D D D D D D D D | MUX1 hi  data bits
-----------+---+-----------------+-------------------------------------------------
2E00       | R | ? ? ? ? ? ? D D | IRQ status
-----------+---+-----------------+-------------------------------------------------
3001       |   | D D D D D D D D | AY-8912 data
-----------+---+-----------------+-------------------------------------------------
3101       | W | D D D D D D D D | AY-8912 address
-----------+---+-----------------+-------------------------------------------------
3406       |R/W| D D D D D D D D | MC6850
-----------+---+-----------------+-------------------------------------------------
3407       |R/W| D D D D D D D D | MC6850
-----------+---+-----------------+-------------------------------------------------
3408       |R/W| D D D D D D D D | MUX2 control reg (IN: reel optos, OUT: lamps)
-----------+---+-----------------+-------------------------------------------------
3409       | W | D D D D D D D D | MUX2 low data bits
-----------+---+-----------------+-------------------------------------------------
340A       | W | D D D D D D D D | MUX2 hi  data bits
-----------+---+-----------------+-------------------------------------------------
3600       | W | ? ? ? ? ? ? D D | ROM page select (select page 3 after reset)
-----------+---+-----------------+-------------------------------------------------
4000-5FFF  | R | D D D D D D D D | ROM (8k)
-----------+---+-----------------+-------------------------------------------------
6000-7FFF  | R | D D D D D D D D | Paged ROM (8k)
           |   |                 |   page 0 : rom area 0x0000 - 0x1FFF
           |   |                 |   page 1 : rom area 0x2000 - 0x3FFF
           |   |                 |   page 2 : rom area 0x4000 - 0x5FFF
           |   |                 |   page 3 : rom area 0x6000 - 0x7FFF
-----------+---+-----------------+-------------------------------------------------
8000-FFFF  | R | D D D D D D D D | ROM (32k)
-----------+---+-----------------+-------------------------------------------------

Optional (on expansion card) (Cobra)
-----------+---+-----------------+-------------------------------------------------
3404       | R | D D D D D D D D | Coin acceptor, direct input
-----------+---+-----------------+-------------------------------------------------
3800-38FF  |R/W| D D D D D D D D | NEC uPD7759 soundchip W:control R:status
           |   |                 | normaly 0x3801 is used can conflict with reel5+6
-----------+---+-----------------+-------------------------------------------------
3800-38FF  | W | D D D D D D D D | Reel 5 + 6 stepper latch (normally 0x3800 used)
-----------+---+-----------------+-------------------------------------------------

  TODO: - add stuff, such as Question of Sport dump, to help driver, as Cobra 1
          board of particular interest.

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/awpvid.h"
#include "vidhrdw/bfm_adr2.h"
#include "vidhrdw/bfm_sc1.h"
#include "machine/lamps.h"
#include "machine/steppers.h" // stepper motor
#include "machine/vacfdisp.h"  // vfd
#include "machine/mmtr.h"
#include "sound/ay8910.h"
#include "sound/upd7759.h"
#include "bfm_sc2.h"

#include "bfm_sc1.lh"
#define VFD_RESET  0x20
#define VFD_CLOCK1 0x80
#define VFD_DATA   0x40   

#define MASTER_CLOCK	(4000000)
#define ADDER_CLOCK		(8000000)

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
static int mux1_outputlatch;
static int mux1_datalo;
static int mux1_datahi;
static int mux1_input;

static int mux2_outputlatch;
static int mux2_datalo;
static int mux2_datahi;
static int mux2_input;

static int watchdog_cnt;
static int watchdog_kicked;

// user interface stuff ///////////////////////////////////////////////////

static UINT8 Lamps[256];		  // 256 multiplexed lamps
UINT8 Inputs[64];				  // 64? multiplexed inputs

///////////////////////////////////////////////////////////////////////////

void Scorpion1_SetSwitchState(int strobe, int data, int state)
{
	if ( state ) Inputs[strobe] |=  (1<<data);
	else		   Inputs[strobe] &= ~(1<<data);
	//writeinputport(strobe, 1<<data, state)
}

///////////////////////////////////////////////////////////////////////////

int Scorpion1_GetSwitchState(int strobe, int data)
{
	int state = 0;

 	if ( strobe < 7 && data < 8 ) state = (Inputs[strobe] & (1<<data))?1:0;

	return state;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1,data & 0x03);
}

///////////////////////////////////////////////////////////////////////////

static INTERRUPT_GEN( timer_irq )
{
	if ( watchdog_kicked )
	{
		watchdog_cnt    = 0;
		watchdog_kicked = 0;
	}
	else
	{
		watchdog_cnt++;
		if ( watchdog_cnt > 2 )	// this is a hack, i don't know what the watchdog timeout is, 3 IRQ's works fine
		{  // reset board
			mame_schedule_soft_reset(Machine);		// reset entire machine. CPU 0 should be enough, but that doesn't seem to work !!
			return;
		}
	}

	if ( timer_enabled )
	{
		irq_status = 0x01 |0x02; //0xff;

	    Inputs[2] = input_port_1_r(0);

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
	if ( locked & 0x01 )
	{	// hardware is still locked,
		if ( data == 0x46 ) locked &= ~0x01;
	}
	else
	{
		if ( Stepper_update(0, data>>4) ) reel_changed |= 0x01;
		if ( Stepper_update(1, data   ) ) reel_changed |= 0x02;

		if ( Stepper_optic_state(0) ) optic_pattern |=  0x01;
		else                          optic_pattern &= ~0x01;
		if ( Stepper_optic_state(1) ) optic_pattern |=  0x02;
		else                          optic_pattern &= ~0x02;
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel34_w )
{
	if ( locked & 0x02 )
	{	// hardware is still locked,
	if ( data == 0x42 ) locked &= ~0x02;
	}
	else
	{
	if ( Stepper_update(2, data>>4) ) reel_changed |= 0x04;
	if ( Stepper_update(3, data   ) ) reel_changed |= 0x08;

	if ( Stepper_optic_state(2) ) optic_pattern |=  0x04;
	else                          optic_pattern &= ~0x04;
	if ( Stepper_optic_state(3) ) optic_pattern |=  0x08;
	else                          optic_pattern &= ~0x08;
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel56_w )
{
	if ( Stepper_update(4, data>>4) ) reel_changed |= 0x10;
	if ( Stepper_update(5, data   ) ) reel_changed |= 0x20;

	if ( Stepper_optic_state(4) ) optic_pattern |=  0x10;
	else                          optic_pattern &= ~0x10;
	if ( Stepper_optic_state(5) ) optic_pattern |=  0x20;
	else                          optic_pattern &= ~0x20;
}

///////////////////////////////////////////////////////////////////////////
// mechanical meters //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mmtr_w )
{
	if ( locked & 0x04 )
	{	// hardware is still locked,
		locked &= ~0x04;
	}
	else
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
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mmtr_r )
{
	return mmtr_latch;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( dipcoin_r )
{
	return input_port_0_r(0) & 0x1F;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( nec_r )
{
	return 1;
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
			{ // new data clocked into vfd
				vfd_shift_data(0, data & VFD_DATA );
			}
		}
  	}
}

///////////////////////////////////////////////////////////////////////////
// input / output multiplexers ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// conversion table BFM strobe data to internal lamp numbers

static const UINT8 BFM_strcnv[] = 
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37, 0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47, 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57, 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67, 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,

	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, 0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F, 0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F, 0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F, 0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F, 0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F, 0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F, 0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F, 0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux1latch_r )
{
	return mux1_input;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux1datlo_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux1dathi_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux1latch_w )
{
	int changed = mux1_outputlatch ^ data;

	mux1_outputlatch = data;

	if ( changed & 0x08 )
	{ // clock changed

		int input_strobe = data & 0x07;

		if ( !(data & 0x08) )
		{ // clock changed to low
			int strobe,	offset,	pattern, i;

			strobe  = data & 0x07;
			offset  = strobe<<4;
			pattern = 0x01;

			for ( i = 0; i < 8; i++ )
			{
				Lamps[ BFM_strcnv[offset  ] ] = mux1_datalo & pattern?1:0;
				Lamps[ BFM_strcnv[offset+8] ] = mux1_datahi & pattern?1:0;
				pattern<<=1;
				offset++;
			}

			if ( strobe == 0 ) Lamps_SetBrightness(0, 255, (UINT8*)Lamps);
		}

		if ( !(data & 0x08) )
		{
			Inputs[ input_strobe ] = readinputport(input_strobe);

			mux1_input = Inputs[ input_strobe ];
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux1datlo_w )
{
	mux1_datalo = data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux1dathi_w )
{
	mux1_datahi = data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux2latch_r )
{
	return mux2_input;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux2datlo_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux2dathi_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux2latch_w )
{
	int changed = mux2_outputlatch ^ data;

	mux2_outputlatch = data;

	if ( changed & 0x08 )
	{ // clock changed

		if ( (!data & 0x08) )
		{ // clock changed to low
			int strobe, offset, pattern, i;

			strobe  = data & 0x07;
			offset  = 128+(strobe<<4);
			pattern = 0x01;

			for ( i = 0; i < 8; i++ )
			{
				Lamps[ BFM_strcnv[offset  ] ] = mux2_datalo & pattern?1:0;
				Lamps[ BFM_strcnv[offset+8] ] = mux2_datahi & pattern?1:0;
				pattern<<=1;
				offset++;
			}
		}

		if ( !(data & 0x08) )
		{
			mux2_input = 0x3F ^ optic_pattern;
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux2datlo_w )
{
	mux2_datalo = data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux2dathi_w )
{
	mux2_datahi = data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( watchdog_w )
{
	watchdog_kicked = 1;
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
return 0;
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


static WRITE8_HANDLER( ay8910_porta_w )
{
	logerror(" AY8912 write port A %02X\n", data );
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( nec_reset_w )
{
	upd7759_start_w(0, 0);
	upd7759_reset_w(0, data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( nec_latch_w )
{
	upd7759_port_w (0, data&0x3F);	// setup sample
	upd7759_start_w(0, 0);
	upd7759_start_w(0, 1);			// start
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( esc_expansion_r )
{
	logerror("r 3404\n");

	return 0xFF;
}

static WRITE8_HANDLER( esc_expansion_w )
{
	logerror("w %02X->3404\n",data);
}

static READ8_HANDLER( esc_expansion2_r )
{
	logerror("r 3405\n");

	return 0xFF;
}

static WRITE8_HANDLER( esc_expansion2_w )
{
	logerror("w %02X->3405\n",data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vid_uart_tx_w )
{
	send_to_adder(data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vid_uart_ctrl_w )
{
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( vid_uart_rx_r )
{
	int data = receive_from_adder();

	return data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( vid_uart_ctrl_r )
{
	int status = 0;

	if ( sc2_data_from_adder  ) status |= 0x01; // receive  buffer full
	if ( !adder2_data_from_sc2) status |= 0x02; // transmit buffer empty

	return status;
}

// scorpion1 board init ///////////////////////////////////////////////////

static const unsigned short AddressDecode[]=
{
	0x0800,0x1000,0x0001,0x0004,0x0008,0x0020,0x0080,0x0200,
	0x0100,0x0040,0x0002,0x0010,0x0400,0x2000,0x4000,0x8000,

	0
};

static const UINT8 DataDecode[]=
{
	0x02,0x08,0x20,0x40,0x10,0x04,0x01,0x80,

	0
};

static UINT8 codec_data[256];


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
			UINT8 data, pattern, newdata, *tab;
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
			int newaddress,pattern;
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
// machine start (called only once) ////////////////////////////////////////

static MACHINE_RESET( bfm_sc1 )
{
///////////////////////////////////////////////////////////////////////////
// called if board is reset ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
	vfd_init(0,VFDTYPE_BFMBD1,0);
	vfd_latch         = 0;
	mmtr_latch        = 0;
	triac_latch       = 0;
	irq_status        = 0;
	timer_enabled     = 1;
	coin_inhibits     = 0;
	mux1_outputlatch  = 0x08;	// clock HIGH
	mux1_datalo       = 0;
	mux1_datahi		  = 0;
	mux1_input        = 0;
	mux2_outputlatch  = 0x08;	// clock HIGH
	mux2_datalo       = 0;
	mux2_datahi		  = 0;
	mux2_input        = 0;

	vfd_reset(0);	// reset display1
	vfd_reset(1);	// reset display2
	vfd_reset(2);	// reset display3

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
	locked		= 0x07;	// hardware is locked

// init rom bank ////////////////////////////////////////////////////////
	{
		UINT8 *rom = memory_region(REGION_CPU1);

		memory_configure_bank(1, 0, 1, &rom[0x10000], 0);
		memory_configure_bank(1, 1, 3, &rom[0x02000], 0x02000);

		memory_set_bank(1,3);
	}
}

///////////////////////////////////////////////////////////////////////////
// scorpion1 board memory map /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static ADDRESS_MAP_START( memmap, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) //8k RAM
	AM_RANGE(0x2000, 0x21FF) AM_WRITE(reel34_w)	  // reel 2+3 latch
	AM_RANGE(0x2200, 0x23FF) AM_WRITE(reel12_w)	  // reel 1+2 latch
	AM_RANGE(0x2400, 0x25FF) AM_WRITE(vfd_w)	  // vfd latch

	AM_RANGE(0x2600, 0x27FF) AM_READ(mmtr_r)      // mechanical meters
	AM_RANGE(0x2600, 0x27FF) AM_WRITE(mmtr_w)

	AM_RANGE(0x2800, 0x2800) AM_READ(triac_r)     // payslide triacs
	AM_RANGE(0x2800, 0x29FF) AM_WRITE(triac_w)

	AM_RANGE(0x2A00, 0x2A00) AM_READ(mux1latch_r) // mux1
	AM_RANGE(0x2A01, 0x2A01) AM_READ(mux1datlo_r)
	AM_RANGE(0x2A02, 0x2A02) AM_READ(mux1dathi_r)
	AM_RANGE(0x2A00, 0x2A00) AM_WRITE(mux1latch_w)
	AM_RANGE(0x2A01, 0x2A01) AM_WRITE(mux1datlo_w)
	AM_RANGE(0x2A02, 0x2A02) AM_WRITE(mux1dathi_w)

	AM_RANGE(0x2E00, 0x2E00) AM_READ(irqlatch_r)  // irq latch

	AM_RANGE(0x3001, 0x3001) AM_READ(soundlatch_r)// AY8912
	AM_RANGE(0x3101, 0x3201) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(AY8910_write_port_0_w)

	AM_RANGE(0x3406, 0x3406) AM_READ(aciastat_r)  // MC6850 status register
	AM_RANGE(0x3407, 0x3407) AM_READ(aciadata_r)  // MC6850 data register
	AM_RANGE(0x3406, 0x3406) AM_WRITE(aciactrl_w) // MC6850 control register
	AM_RANGE(0x3407, 0x3407) AM_WRITE(aciadata_w) // MC6850 data    register

	AM_RANGE(0x3408, 0x3408) AM_READ(mux2latch_r) // mux2
	AM_RANGE(0x3409, 0x3409) AM_READ(mux2datlo_r)
	AM_RANGE(0x340A, 0x340A) AM_READ(mux2dathi_r)
	AM_RANGE(0x3408, 0x3408) AM_WRITE(mux2latch_w)
	AM_RANGE(0x3409, 0x3409) AM_WRITE(mux2datlo_w)
	AM_RANGE(0x340A, 0x340A) AM_WRITE(mux2dathi_w)

	AM_RANGE(0x3404, 0x3404) AM_READ(dipcoin_r ) // coin input on gamecard
	AM_RANGE(0x3801, 0x3801) AM_READ(nec_r)		 // uPD5579 status on soundcard

	AM_RANGE(0x3600, 0x3600) AM_WRITE(bankswitch_w) // write bank
	AM_RANGE(0x3800, 0x39FF) AM_WRITE(reel56_w)	 // reel 5+6 latch

	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)	 // 8k  ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1) // 8k  paged ROM (4 pages)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)	 // 32k ROM

	AM_RANGE(0x8000, 0xFFFF) AM_WRITE(watchdog_w)// kick watchdog

ADDRESS_MAP_END

///////////////////////////////////////////////////////////////////////////
// scorpion1 board + adder2 expansion memory map //////////////////////////
///////////////////////////////////////////////////////////////////////////

static ADDRESS_MAP_START( memmap_adder2, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) //8k RAM
	AM_RANGE(0x2000, 0x21FF) AM_WRITE(reel34_w)	  // reel 2+3 latch
	AM_RANGE(0x2200, 0x23FF) AM_WRITE(reel12_w)	  // reel 1+2 latch
	AM_RANGE(0x2400, 0x25FF) AM_WRITE(vfd_w)	  // vfd latch

	AM_RANGE(0x2600, 0x27FF) AM_READ(mmtr_r)      // mechanical meters
	AM_RANGE(0x2600, 0x27FF) AM_WRITE(mmtr_w)
  
	AM_RANGE(0x2800, 0x2800) AM_READ(triac_r)     // payslide triacs
	AM_RANGE(0x2800, 0x29FF) AM_WRITE(triac_w)

	AM_RANGE(0x2A00, 0x2A00) AM_READ(mux1latch_r) // mux1
	AM_RANGE(0x2A01, 0x2A01) AM_READ(mux1datlo_r)
	AM_RANGE(0x2A02, 0x2A02) AM_READ(mux1dathi_r)
	AM_RANGE(0x2A00, 0x2A00) AM_WRITE(mux1latch_w)
	AM_RANGE(0x2A01, 0x2A01) AM_WRITE(mux1datlo_w)
	AM_RANGE(0x2A02, 0x2A02) AM_WRITE(mux1dathi_w)

	AM_RANGE(0x2E00, 0x2E00) AM_READ(irqlatch_r)  // irq latch

	AM_RANGE(0x3001, 0x3001) AM_READ(soundlatch_r)// AY8912
	AM_RANGE(0x3001, 0x3001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x3101, 0x3201) AM_WRITE(AY8910_control_port_0_w)

	AM_RANGE(0x3406, 0x3406) AM_READ(aciastat_r)  // MC6850 status register
	AM_RANGE(0x3407, 0x3407) AM_READ(aciadata_r)  // MC6850 data register
	AM_RANGE(0x3406, 0x3406) AM_WRITE(aciactrl_w) // MC6850 control register
	AM_RANGE(0x3407, 0x3407) AM_WRITE(aciadata_w) // MC6850 data    register

	AM_RANGE(0x3408, 0x3408) AM_READ(mux2latch_r) // mux2
	AM_RANGE(0x3409, 0x3409) AM_READ(mux2datlo_r)
	AM_RANGE(0x340A, 0x340A) AM_READ(mux2dathi_r)
	AM_RANGE(0x3408, 0x3408) AM_WRITE(mux2latch_w)
	AM_RANGE(0x3409, 0x3409) AM_WRITE(mux2datlo_w)
	AM_RANGE(0x340A, 0x340A) AM_WRITE(mux2dathi_w)

	AM_RANGE(0x3404, 0x3404) AM_READ(dipcoin_r ) // coin input on gamecard
	AM_RANGE(0x3801, 0x3801) AM_READ(nec_r)		 // uPD5579 status on soundcard

	AM_RANGE(0x3600, 0x3600) AM_WRITE(bankswitch_w) // write bank
	AM_RANGE(0x3800, 0x39FF) AM_WRITE(reel56_w)	 // reel 5+6 latch

	AM_RANGE(0x3e00, 0x3e00) AM_READ( vid_uart_ctrl_r)	// video uart control reg read
	AM_RANGE(0x3e00, 0x3e00) AM_WRITE(vid_uart_ctrl_w)	// video uart control reg write
	AM_RANGE(0x3e01, 0x3e01) AM_READ( vid_uart_rx_r)	// video uart receive  reg
	AM_RANGE(0x3e01, 0x3e01) AM_WRITE(vid_uart_tx_w)	// video uart transmit reg

	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)	 // 8k  ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1) // 8k  paged ROM (4 pages)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)	 // 32k ROM

	AM_RANGE(0x8000, 0xFFFF) AM_WRITE(watchdog_w)// kick watchdog

ADDRESS_MAP_END


///////////////////////////////////////////////////////////////////////////
// scorpion1 board + upd7759 soundcard memory map /////////////////////////
///////////////////////////////////////////////////////////////////////////

static ADDRESS_MAP_START( sc1_nec_uk, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) //8k RAM
	AM_RANGE(0x2000, 0x21FF) AM_WRITE(reel34_w)	  // reel 2+3 latch
	AM_RANGE(0x2200, 0x23FF) AM_WRITE(reel12_w)	  // reel 1+2 latch
	AM_RANGE(0x2400, 0x25FF) AM_WRITE(vfd_w)	  // vfd latch

	AM_RANGE(0x2600, 0x27FF) AM_READ(mmtr_r)      // mechanical meters
	AM_RANGE(0x2600, 0x27FF) AM_WRITE(mmtr_w)
  
	AM_RANGE(0x2800, 0x2800) AM_READ(triac_r)     // payslide triacs
	AM_RANGE(0x2800, 0x29FF) AM_WRITE(triac_w)

	AM_RANGE(0x2A00, 0x2A00) AM_READ(mux1latch_r) // mux1
	AM_RANGE(0x2A01, 0x2A01) AM_READ(mux1datlo_r)
	AM_RANGE(0x2A02, 0x2A02) AM_READ(mux1dathi_r)
	AM_RANGE(0x2A00, 0x2A00) AM_WRITE(mux1latch_w)
	AM_RANGE(0x2A01, 0x2A01) AM_WRITE(mux1datlo_w)
	AM_RANGE(0x2A02, 0x2A02) AM_WRITE(mux1dathi_w)

	AM_RANGE(0x2E00, 0x2E00) AM_READ(irqlatch_r)	  // irq latch

	AM_RANGE(0x3001, 0x3001) AM_READ(soundlatch_r)// AY8912
	AM_RANGE(0x3001, 0x3001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x3101, 0x3201) AM_WRITE(AY8910_control_port_0_w)

	AM_RANGE(0x3406, 0x3406) AM_READ(aciastat_r)  // MC6850 status register
	AM_RANGE(0x3407, 0x3407) AM_READ(aciadata_r)  // MC6850 data register
	AM_RANGE(0x3406, 0x3406) AM_WRITE(aciactrl_w) // MC6850 control register
	AM_RANGE(0x3407, 0x3407) AM_WRITE(aciadata_w) // MC6850 data    register

	AM_RANGE(0x3408, 0x3408) AM_READ(mux2latch_r) // mux2
	AM_RANGE(0x3409, 0x3409) AM_READ(mux2datlo_r)
	AM_RANGE(0x340A, 0x340A) AM_READ(mux2dathi_r)
	AM_RANGE(0x3408, 0x3408) AM_WRITE(mux2latch_w)
	AM_RANGE(0x3409, 0x3409) AM_WRITE(mux2datlo_w)
	AM_RANGE(0x340A, 0x340A) AM_WRITE(mux2dathi_w)

	AM_RANGE(0x3600, 0x3600) AM_WRITE(bankswitch_w) // write bank

	AM_RANGE(0x3801, 0x3801) AM_READ(nec_r)      // uPD5579 status on soundcard
	AM_RANGE(0x3800, 0x39FF) AM_WRITE(nec_latch_w)


	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)	 // 8k  ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1) // 8k  paged ROM (4 pages)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)	 // 32k ROM

	AM_RANGE(0x8000, 0xFFFF) AM_WRITE(watchdog_w)// kick watchdog

ADDRESS_MAP_END

///////////////////////////////////////////////////////////////////////////
// scorpion1 board + cobra expansion //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static ADDRESS_MAP_START( memmap_esc, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) //8k RAM
	AM_RANGE(0x2000, 0x21FF) AM_WRITE(reel34_w)	  // reel 2+3 latch
	AM_RANGE(0x2200, 0x23FF) AM_WRITE(reel12_w)	  // reel 1+2 latch
	AM_RANGE(0x2400, 0x25FF) AM_WRITE(vfd_w)	  // vfd latch

	AM_RANGE(0x2600, 0x27FF) AM_READ(mmtr_r)      // mechanical meters
	AM_RANGE(0x2600, 0x27FF) AM_WRITE(mmtr_w)
  
	AM_RANGE(0x2800, 0x2800) AM_READ(triac_r)     // payslide triacs
	AM_RANGE(0x2800, 0x29FF) AM_WRITE(triac_w)

	AM_RANGE(0x2A00, 0x2A00) AM_READ(mux1latch_r) // mux1
	AM_RANGE(0x2A01, 0x2A01) AM_READ(mux1datlo_r)
	AM_RANGE(0x2A02, 0x2A02) AM_READ(mux1dathi_r)
	AM_RANGE(0x2A00, 0x2A00) AM_WRITE(mux1latch_w)
	AM_RANGE(0x2A01, 0x2A01) AM_WRITE(mux1datlo_w)
	AM_RANGE(0x2A02, 0x2A02) AM_WRITE(mux1dathi_w)

	AM_RANGE(0x2E00, 0x2E00) AM_READ(irqlatch_r)  // irq latch

	AM_RANGE(0x3001, 0x3001) AM_READ(soundlatch_r)// AY8912
	AM_RANGE(0x3001, 0x3001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x3101, 0x3201) AM_WRITE(AY8910_control_port_0_w)

	AM_RANGE(0x3404, 0x3404) AM_READ( esc_expansion_r )
	AM_RANGE(0x3404, 0x3404) AM_WRITE(esc_expansion_w )

	AM_RANGE(0x3405, 0x3405) AM_READ( esc_expansion2_r)
	AM_RANGE(0x3405, 0x3405) AM_WRITE(esc_expansion2_w)

	AM_RANGE(0x3406, 0x3406) AM_READ(aciastat_r)  // MC6850 status register
	AM_RANGE(0x3407, 0x3407) AM_READ(aciadata_r)  // MC6850 data register
	AM_RANGE(0x3406, 0x3406) AM_WRITE(aciactrl_w) // MC6850 control register
	AM_RANGE(0x3407, 0x3407) AM_WRITE(aciadata_w) // MC6850 data    register

	AM_RANGE(0x3408, 0x3408) AM_READ(mux2latch_r) // mux2
	AM_RANGE(0x3409, 0x3409) AM_READ(mux2datlo_r)
	AM_RANGE(0x340A, 0x340A) AM_READ(mux2dathi_r)
	AM_RANGE(0x3408, 0x3408) AM_WRITE(mux2latch_w)
	AM_RANGE(0x3409, 0x3409) AM_WRITE(mux2datlo_w)
	AM_RANGE(0x340A, 0x340A) AM_WRITE(mux2dathi_w)

	AM_RANGE(0x3404, 0x3404) AM_READ(dipcoin_r ) // coin input on gamecard
	AM_RANGE(0x3801, 0x3801) AM_READ(nec_r)		 // uPD5579 status on soundcard

	AM_RANGE(0x3600, 0x3600) AM_WRITE(bankswitch_w) // write bank
	AM_RANGE(0x3800, 0x39FF) AM_WRITE(reel56_w)	 // reel 5+6 latch

	AM_RANGE(0x3e00, 0x3e00) AM_READ( vid_uart_ctrl_r)	// video uart control reg read
	AM_RANGE(0x3e00, 0x3e00) AM_WRITE(vid_uart_ctrl_w)	// video uart control reg write
	AM_RANGE(0x3e01, 0x3e01) AM_READ( vid_uart_rx_r)	// video uart receive  reg
	AM_RANGE(0x3e01, 0x3e01) AM_WRITE(vid_uart_tx_w)	// video uart transmit reg

	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)	 // 8k  ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1) // 8k  paged ROM (4 pages)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)	 // 32k ROM

	AM_RANGE(0x8000, 0xFFFF) AM_WRITE(watchdog_w)// kick watchdog

ADDRESS_MAP_END


///////////////////////////////////////////////////////////////////////////

static ADDRESS_MAP_START( unknown_vid_memmap, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0xffff) AM_READ(MRA8_ROM)

ADDRESS_MAP_END

// input ports for scorpion1 board ////////////////////////////////////////

INPUT_PORTS_START( scorpion1 )

	PORT_START_TAG("STROBE0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN5 ) PORT_IMPULSE(3)
	PORT_SERVICE_NO_TOGGLE(0x20,IP_ACTIVE_HIGH)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Red  Test")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_DIPNAME( 0x01, 0x00, "DIL10" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL11" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL12" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL13" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL14" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL15" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL16" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_DIPNAME( 0x01, 0x00, "DIL02" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL03" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL04" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL05" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL06" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL07" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL08" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_END

// input ports for bfm every second counts ////////////////////////////////

INPUT_PORTS_START( bfmesc_inputs )

INPUT_PORTS_END

// input ports for scorpion1 board ////////////////////////////////////////

INPUT_PORTS_START( bfmclatt )
	PORT_START_TAG("STROBE0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("10p")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("20p")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("50p")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("1 Pound")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE(0x20,IP_ACTIVE_HIGH)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Red Test")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Cancel")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Hold 1")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Hold 2")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Hold 3")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Hold 4")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Exchange")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON7 )	PORT_NAME("Stop/Gamble")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1  )	PORT_NAME("Start")
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_INTERLOCK )	PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_Q) PORT_TOGGLE
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE )	PORT_NAME("Refill Key")   PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_INTERLOCK )	PORT_NAME("Front Door")   PORT_CODE(KEYCODE_W) PORT_TOGGLE
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_DIPNAME( 0x01, 0x00, "DIL10" ) PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL11" ) PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL12" ) PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL13" ) PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL14" ) PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL15" ) PORT_DIPLOCATION("DIL:15")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL16" ) PORT_DIPLOCATION("DIL:16")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_DIPNAME( 0x01, 0x00, "DIL02" ) PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL03" ) PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL04" ) PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL05" ) PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL06" ) PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL07" ) PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL08" ) PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_END

INPUT_PORTS_START( toppoker )
	PORT_START_TAG("STROBE0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(100) PORT_NAME("Fl 5.00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(100) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(100) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(100) PORT_NAME("Fl 0.50")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN5 ) PORT_IMPULSE(100)
	PORT_SERVICE_NO_TOGGLE(0x20,IP_ACTIVE_HIGH)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Red  Test")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_DIPNAME( 0x01, 0x00, "DIL10" ) PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL11" ) PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL12" ) PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL13" ) PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL14" ) PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL15" ) PORT_DIPLOCATION("DIL:15")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL16" ) PORT_DIPLOCATION("DIL:16")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_DIPNAME( 0x01, 0x00, "DIL02" ) PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL03" ) PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL04" ) PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL05" ) PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL06" ) PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL07" ) PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL08" ) PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_END

static const gfx_layout charlayout =
{
  8,8,		  // 8 * 8 characters
  64,		  // 8192  characters
  4,		  // 4     bits per pixel
  { 0,0,0,0 },
  { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
  { 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
  4
};

static const gfx_layout charlayout2 =
{
  8,8,		  // 8 * 8 characters
  64,		  // 8192  characters
  4,		  // 4     bits per pixel
  { 0,0,0,0 },
  { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
  { 0*64*8, 1*64*8, 2*64*8, 3*64*8, 4*64*8, 5*64*8, 6*64*8, 7*64*8 },
  4
};

// this is a strange beast !!!!
//
// characters are grouped by 64 (512 pixels)
// there are max 128 of these groups

static const gfx_decode esc_gfxdecodeinfo[] = 
{
  { REGION_CPU2,  0x8000, &charlayout, 0, 16 },
  { REGION_CPU2,  0x8000, &charlayout2,0, 16 },
  { -1 } /* end of array */
};

static struct AY8910interface ay8912_interface =
{
	0,
	0,
	ay8910_porta_w,
	0
};

static struct upd7759_interface upd7759_interface =
 {
	REGION_SOUND1,		/* memory region */
	0
 };

///////////////////////////////////////////////////////////////////////////
// machine driver for scorpion1 board /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_DRIVER_START( scorpion1 )
	MDRV_MACHINE_RESET(bfm_sc1)							// main scorpion1 board initialisation
	MDRV_CPU_ADD_TAG("main", M6809, MASTER_CLOCK/4 )		// 6809 CPU at 1 Mhz
	MDRV_CPU_PROGRAM_MAP(memmap,0)						// setup read and write memorymap
	MDRV_CPU_PERIODIC_INT(timer_irq, TIME_IN_HZ(1000) )	// generate 1000 IRQ's per second

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK/4)
	MDRV_SOUND_CONFIG(ay8912_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_DEFAULT_LAYOUT(layout_awpvid)
	/* video hardware */
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


///////////////////////////////////////////////////////////////////////////
// machine driver for scorpion1 board + adder2 extension //////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_DRIVER_START( scorpion1_adder2 )
	MDRV_IMPORT_FROM( scorpion1 )
  
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(memmap_adder2,0)		// setup read and write memorymap

	MDRV_DEFAULT_LAYOUT(layout_bfm_sc1)
	MDRV_SCREEN_ADD("ADDER", 0x000)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_SIZE( 400, 300)
	MDRV_SCREEN_VISIBLE_AREA(  0, 400-1, 0, 300-1)

	MDRV_SCREEN_ADD("AWPVID", 0x000)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_SIZE( 288, 29)
	MDRV_SCREEN_VISIBLE_AREA(  0, 288-1, 0, 29-1)

	MDRV_VIDEO_START( adder2awp)
	MDRV_VIDEO_RESET( adder2awp)
	MDRV_VIDEO_UPDATE(adder2awp)

	MDRV_PALETTE_LENGTH(22)
	MDRV_COLORTABLE_LENGTH(22)
	MDRV_PALETTE_INIT(awpvideo)
	MDRV_GFXDECODE(adder2_gfxdecodeinfo)

	MDRV_CPU_ADD_TAG("adder2", M6809, ADDER_CLOCK/4 )	// adder2 board 6809 CPU at 2 Mhz
	MDRV_CPU_PROGRAM_MAP(adder2_memmap,0)				// setup adder2 board memorymap
	MDRV_CPU_VBLANK_INT(adder2_vbl, 1);					// board has a VBL IRQ

MACHINE_DRIVER_END

///////////////////////////////////////////////////////////////////////////
// machine driver for scorpion1 board /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_DRIVER_START( scorpion1_nec_uk )

	MDRV_IMPORT_FROM( scorpion1 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sc1_nec_uk,0)			  // setup read and write memorymap

	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd7759_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

///////////////////////////////////////////////////////////////////////////
// machine driver for scorpion1 + cobra board /////////////////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_DRIVER_START( scorpion1_esc )

	MDRV_IMPORT_FROM( scorpion1 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(memmap_esc,0)		  // setup read and write memorymap

	/* video hardware */
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
	MDRV_GFXDECODE(esc_gfxdecodeinfo)

//  MDRV_CPU_ADD_TAG("cobra", Z80, 2000000 )  // Cobra??
//  MDRV_CPU_PROGRAM_MAP(unknown_vid_memmap,0)              // setup adder2 board memorymap

//  MDRV_CPU_VBLANK_INT(adder2_vbl, 1);                     // board has a VBL IRQ

MACHINE_DRIVER_END

// ROM definition /////////////////////////////////////////////////////////

ROM_START( bfmlotse )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "lotusse.bin",  0x0000, 0x10000,  CRC(636dadc4) SHA1(85bad5d76dac028fe9f3303dd09e8266aba7db4d))
ROM_END

///////////////////////////////////////////////////////////////////////////

ROM_START( bfmroul )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "rou029.bin",   0x8000, 0x08000,  CRC(31723f0A) SHA1(e220976116a0aaf24dc0c4af78a9311a360e8104))
ROM_END

///////////////////////////////////////////////////////////////////////////

ROM_START( bfmclatt )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "39370196.1",   0x8000, 0x08000,  CRC(4c2e465f) SHA1(101939d37d9c033f6d1dfb83b4beb54e4061aec2))
	ROM_LOAD( "39370196.2",   0x0000, 0x08000,  CRC(c809c22d) SHA1(fca7515bc84d432150ffe5e32fccc6aed458b8b0))
ROM_END

///////////////////////////////////////////////////////////////////////////

ROM_START( bfmesc )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "esc12int",   0x8000, 0x08000,  CRC(741a1fe6) SHA1(e741d0ae0d2f11036a358120381e4b0df4a560a1))

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64 for code + data */
	ROM_LOAD( "esccobpa",   0x0000, 0x10000,  CRC(d8eadeb7) SHA1(9b94f1454e6a17bf8321b0ef4ddd0ed1a56150f7))
	
ROM_END

#if 0
ROM_START( quessprt )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "39360107.p1", 0x0000, 0x8000, CRC(7e802634))
	ROM_LOAD( "qospa.bin",   0x8000, 0x8000, CRC(ad505138))

	ROM_REGION( 0x200000, REGION_CPU2, 0 )
	ROM_LOAD( "qosrom0.bin",   0x000000, 0x80000, CRC(4f150634))
	ROM_LOAD( "qosrom1.bin",   0x080000, 0x80000, CRC(94611c03))
	ROM_LOAD( "qosrom2.bin",   0x100000, 0x80000, CRC(c39737db))
	ROM_LOAD( "qosrom3.bin",   0x180000, 0x80000, CRC(785b8ff9))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "qossnd1.bin", 0x00000, 0x10000, CRC(888a29f8))
	ROM_LOAD( "qossnd2.bin", 0x10000, 0x10000, CRC(d7874a47))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )


	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )

ROM_END
#endif
///////////////////////////////////////////////////////////////////////////

ROM_START( toppoker )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750899.bin", 0x0000, 0x10000,  CRC(639d1d62) SHA1(80620c14bf9f953588555510fc2e6e930140923f))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "tpk010.vid", 0x0000, 0x20000,  CRC(ea4eddca) SHA1(5fb805d35376ec7ee8d58684e584621dbb2b2a9c))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "tpk011.chr",	0x00000, 0x20000,  CRC(4dc23ad8) SHA1(8e8cc699412dbb092e16e14518f407353f477ee1))
ROM_END

///////////////////////////////////////////////////////////////////////////
static void sc1_common_init(int reels)
{
	UINT8 *rom, i;

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	memset(Inputs, 0, sizeof(Inputs));
  
	Lamps_init(256);

	// setup n default 96 half step reels ///////////////////////////////////
	for ( i = 0; i < reels; i++ )
	{
		Stepper_init(i, STEPPER_48STEP_REEL);
	}
}

DRIVER_INIT(toppoker)
{
	sc1_common_init(3);
	decode_sc1(REGION_CPU1);			// decode main rom
	adder2_decode_char_roms();			// decode GFX roms

	Mechmtr_init(8);
  
	vfd_init(0,VFDTYPE_BFMBD1,0);
}

DRIVER_INIT(lotse)
{
	sc1_common_init(6);
	decode_sc1(REGION_CPU1);			// decode main rom
	Mechmtr_init(8);

	vfd_init(0,VFDTYPE_BFMBD1,0);
	vfd_init(1,VFDTYPE_BFMBD1,0);
}

///////////////////////////////////////////////////////////////////////////

DRIVER_INIT(rou029)
{
	sc1_common_init(6);
	Mechmtr_init(8);

	vfd_init(0,VFDTYPE_BFMBD1,0);
}

///////////////////////////////////////////////////////////////////////////

DRIVER_INIT(bfmesc)
{
	sc1_common_init(0);
	decode_sc1(REGION_CPU1);			// decode main rom
	Mechmtr_init(8);

	vfd_init(0,VFDTYPE_BFMBD1,0);
}

DRIVER_INIT(bfmclatt)
{
	sc1_common_init(6);
	decode_sc1(REGION_CPU1);			// decode main rom
	Mechmtr_init(8);

	vfd_init(0,VFDTYPE_BFMBD1,0);

	Scorpion1_SetSwitchState(3,2,1);
	Scorpion1_SetSwitchState(3,3,1);
	Scorpion1_SetSwitchState(3,6,1);
	Scorpion1_SetSwitchState(4,1,1);
}

//	  year, name,     parent,   machine,			input,            init,	   monitor, company,       fullname,                                    flags
GAME( 1988, bfmlotse, 0,        scorpion1,			scorpion1,		  lotse,   0,       "BFM/ELAM",    "Lotus SE (Dutch)",							GAME_NOT_WORKING|GAME_SUPPORTS_SAVE )
GAME( 1988, bfmroul,  0,        scorpion1,			scorpion1,		  rou029,  0,       "BFM/ELAM",    "Roulette (Dutch, Game Card 39-360-129?)",	GAME_NOT_WORKING|GAME_SUPPORTS_SAVE )
GAME( 1990, bfmclatt, 0,        scorpion1_nec_uk,	bfmclatt,		  bfmclatt,0,       "BFM",         "Club attraction (UK, Game Card 39-370-196)",GAME_NOT_WORKING|GAME_SUPPORTS_SAVE )
//Cobra
GAME( 1987, bfmesc,   0,        scorpion1_esc,		bfmesc_inputs,	  bfmesc,  0,       "BFM",         "Every Second Counts (UK)",					GAME_NOT_WORKING|GAME_SUPPORTS_SAVE )
//Adder
GAME( 1996, toppoker, 0,        scorpion1_adder2,	toppoker,		  toppoker,0,       "BFM/ELAM",    "Toppoker (Dutch, Game Card 95-750-899)",	GAME_NOT_WORKING|GAME_SUPPORTS_SAVE )
