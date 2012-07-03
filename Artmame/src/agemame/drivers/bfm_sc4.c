/***************************************************************************

    bfm_sc4.c

    Bellfruit scorpion4 driver, (under heavy construction !!!)

    A.G.E Code Copyright (C) 2004-2006 by J. Wallace and the AGEMAME Development Team.
    Visit http://www.mameworld.net/agemame/ for more information.

    M.A.M.E Core Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team,
    used under license from http://mamedev.org

    18-07-2004: Re-Animator


Standard scorpion4 memorymap
+-------------------------------------------------------------------+
|0x800000 - 0x810000|64K RAM (battery backed up)                    |
+-------------------+-----------------------------------------------+
|0x810201           |I/O register Write, byte                       |
|0x810211           |I/O register Write, byte                       |
|0x810221           |I/O register Write, byte                       |
|0x810231           |I/O register Write, byte                       |
+-------------------+-----------------------------------------------+
|0x810241           |I/O register read byte (bit7 ready/busy signal)|
|0x8102e1           |I/O register read byte                         |
+-------------------+-----------------------------------------------+
|0x810361           |I/O register Write, byte                       |
|0x8102c1           |I/O register Write, byte                       |
|0x810281           |I/O register Write, byte                       |
|0x811391           |I/O register Write, byte                       |
|0x8112a1           |I/O register Write, byte                       |
+-------------------+-----------------------------------------------+
|0x810371           |I/O register Write, byte                       |
+-------------------+-----------------------------------------------+
|0x810001           |output matrix?                                 |
|0x811001           |output matrix?                                 |
+-------------------+-----------------------------------------------+
***************************************************************************/

#include "driver.h"
#include "vidhrdw/awpvid.h"
#include "machine/lamps.h"
#include "machine/steppers.h" // stepper motor
#include "machine/vacfdisp.h"  // vfd
#include "machine/mmtr.h"

// local prototypes ///////////////////////////////////////////////////////


// local vars /////////////////////////////////////////////////////////////

static UINT16 *nvram;	  // pointer to NV ram
static size_t	 nvram_size; // size of NV ram

static mame_timer *mtimer1;
static mame_timer *mtimer2;

static struct
{
	UINT16 mbar,		// sim07 base address
	scr[2];				// system control registers

	UINT16  PICR,		// PERIPHERAL INTERRUPT CONTROL REGISTER (PICR)
	PIVR,				// PROGRAMMABLE INTERRUPT VECTOR REGISTER (PIVR).
	LICR1,				// LATCHED INTERRUPT CONTROL REGISTER 1
	LICR2;				// LATCHED INTERRUPT CONTROL REGISTER 2

	UINT32   chipsel1,	// chipselect1  base address
	chipsel2,			// chipselect2  base address
	chipsel3,			// chipselect3  base address
	chipsel4;			// chipselect4  base address

	UINT8   cs1_fc,		// chipselect1  function code bits 
	cs2_fc,				// chipselect2  function code bits 
	cs3_fc,				// chipselect3  function code bits 
	cs4_fc;				// chipselect4  function code bits 
  
	struct 
	{
		unsigned short tmr,		// timer mode      reg ($120/$130)
					   trr,		// timer reference reg ($122/$132)
					   tcr,		// timer capture   reg ($124/$134)
					   tcn,		// timer counter       ($126/$136)
					   ter;		// timer event     reg ($129/$139)
		int running;
		int prescaler;

		double interval;

	 } timers[2];

} sim07;

static MACHINE_RESET( bfm_sc4 )
{
	memset(&sim07, 0, sizeof(sim07));
}

// user interface stuff ///////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////

/*static READ8_HANDLER( ram_r )
{
	return nvram[offset];
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( ram_w )
{
	nvram[offset] = data;
}*/

///////////////////////////////////////////////////////////////////////////

#if 0
static WRITE16_HANDLER( m683xxx_sim07_w )
{
	switch ( offset )
	{
	case 0x02:  // 0x0000F2 - Module Base Address Register (MBAR)
	logerror("SIM07.mbar = %04X\n", data);
	sim07.mbar = data;
	break;

	case 0x04:  // 0x0000F4 - System Control Register (SCR)
	logerror("SIM07.scr[0] = %04X\n", data);
	sim07.scr[0] = data;
	break;

	case 0x06:  // 0x0000F6 - System Control Register (SCR)

	logerror("SIM07.scr[1] = %04X\n", data);
	sim07.scr[1] = data;
	break;

	case 0x00:  // 0x0000F0 -		 reserved - No external bus access
	default:	// 0x0000F8 - 0xFF reserved - No external bus access
	logerror("SIM07.unknown reg(%02X) = %04X\n", offset + 0xF0, data);
	}
}
#endif

static READ16_HANDLER( sim07_mbar_r )
{
	logerror("read MBAR\n");
	return sim07.mbar;
}

///////////////////////////////////////////////////////////////////////////


static WRITE16_HANDLER(sim07_w)
{
	UINT16 reg = offset<<1, option;
	UINT32 ba;
	UINT8  read;

	switch ( reg )
	{
	case 0x020: // MBASE + $020 : Latched Interrupt Control reg1
	sim07.LICR1 = data;
	logerror("SIM07: LICR1 = %04X\n", data);
	break;

	case 0x022: // MBASE + $022 : Latched Interrupt Control reg2
	sim07.LICR2 = data;
	logerror("SIM07: LICR2 = %04X\n", data);
	break;

	case 0x024: // MBASE + $024 : Peripheral Interrupt Control Reg
	logerror("SIM07: PICR = %04X\n", data);
	sim07.PICR = data;
	break;

	case 0x026: // MBASE + $027 : Peripheral Interrupt Control Reg
	logerror("SIM07: PIVR = %02X\n", (data>>8) & 0x00FF );
	sim07.PIVR = (data>>8) & 0x00FF;
	break;

  /////////////////////////////////////////////////////////////////////////
	case 0x040 :	// base register 0
	ba   = (data>>2 & 0x7FF) << 13;
	read = data & 0x01;

	sim07.chipsel1 = (data>>2 & 0x7FF) << 13;
	sim07.cs1_fc   = (data>>13) & 0x07;

	logerror("SIM07: base register 0 = %04X (base %08X, rw=%d)\n", data,(UINT32)ba, read);
	break;

	case 0x042 :	// option register 0
	logerror("SIM07: option register 0 = %04X\n", data);

	option = data>>13;

	if ( option == 0x07 ) logerror("  extern DTACK\n");
	else                  logerror("  %d waitstates\n", option);
	break;

	case 0x044 :	// base register 1
	ba   = (data>>2 & 0x7FF) << 13;
	read = data & 0x01;

	sim07.chipsel2 = (data>>2 & 0x7FF) << 13;
	sim07.cs2_fc   = (data>>13) & 0x07;

	logerror("SIM07: base register 1 = %04X (base %08X, rw=%d)\n", data,(UINT32)ba, read);
	break;

	case 0x046 :	// option register 1
	logerror("SIM07: option register 1 = %04X\n", data);
	option = data>>13;

	if ( option == 0x07 ) logerror("  extern DTACK\n");
	else                  logerror("  %d waitstates\n", option);
	break;

	case 0x048 :	// base register 2
	ba   = (data>>2 & 0x7FF) << 13;
	read = data & 0x01;

	sim07.chipsel3 = (data>>2 & 0x7FF) << 13;
	sim07.cs3_fc   = (data>>13) & 0x07;

	logerror("SIM07: base register 2 = %04X (base %08X, rw=%d)\n", data,(UINT32)ba, read);
	break;

	case 0x04A :	// option register 2
	logerror("SIM07: option register 2 = %04X\n", data);
	option = data>>13;

	if ( option == 0x07 ) logerror("  extern DTACK\n");
	else                  logerror("  %d waitstates\n", option);
	break;

	case 0x04C :	// base register 3
	ba   = (data>>2 & 0x7FF) << 13;
	read = data & 0x01;

	sim07.chipsel4 = (data>>2 & 0x7FF) << 13;
	sim07.cs4_fc   = (data>>13) & 0x07;

	logerror("SIM07: base register 3 = %04X (base %08X, rw=%d)\n", data,(UINT32)ba, read);
	break;

	case 0x04E :	// option register 3
	logerror("SIM07: option register 3 = %04X\n", data);
	option = data>>13;

	if ( option == 0x07 ) logerror("  extern DTACK\n");
	else                  logerror("  %d waitstates\n", option);
	break;
  /////////////////////////////////////////////////////////////////////////

	case 0x120: 
	{
		long clock = 16000000L;

		sim07.timers[0].tmr = data;
		sim07.timers[0].prescaler = (data>>8)+1;

		logerror("  timer1: mode=%04X (ps=%d)\n", data, sim07.timers[0].prescaler);


		if ( data & 0x01 )
		{ // timer enable

			sim07.timers[0].interval = TIME_IN_HZ( clock / ((double)sim07.timers[0].trr * sim07.timers[0].prescaler) );

			if ( mtimer1 )
			{
				logerror("start timer1 %f",sim07.timers[0].interval);
				//mame_timer_adjust(mtimer1, double_to_mame_time(sim07.timers[0].interval), 0, double_to_mame_time(sim07.timers[0].interval));
				timer_reset(mtimer1, sim07.timers[0].interval );
			}
		}
	}
	break;
	
	case 0x122: 
	sim07.timers[0].trr = data;

	logerror("  timer1: ref cnt = %04X\n", data);
	break;

	case 0x124:
	sim07.timers[0].tcr = data;
	break;
	
	case 0x126:
	sim07.timers[0].tcn = data;
	break;
	
	case 0x129:
	sim07.timers[0].ter = data;
	break;

	case 0x12A: // MBASE + $12A : watchdog reference register (WRR)
	logerror("SIM07: watchdog reference reg = %04X\n", data );
	break;

	case 0x12C: // MBASE + $12C : watchdog counter register (WCR)
	logerror("SIM07: watchdog counter reg = %04X\n", data );
	break;
 
 	case 0x010: // port A control reg
	if (ACCESSING_LSB)
	{
		logerror("SIM07: port A control reg = %04X (mask=%04X)\n",data, mem_mask);
	}
	break;

	case 0x016: // port B control register
	logerror("SIM07: port B control reg = %04X (mask=%04X)\n",data, mem_mask);
	break;

	case 0x018: // port B Data direction register
	logerror("SIM07: port B data direction reg = %04X (mask=%04X)\n",data, mem_mask);
	break;

	case 0x01A: // port B Data register
	logerror("SIM07: port B data reg = %04X (mask=%04X)\n",data, mem_mask);
	break;

	default:
	logerror("SIM07: write reg %04X(%04X) = %04X mask=%04X(%d)\n", reg,offset,data, mem_mask,ACCESSING_LSB);
}

}

static READ16_HANDLER(sim07_r)
{
  UINT16 reg = offset<<1;
  UINT16 result = 0;

  logerror("SIM07: read reg %04X (%04X) mask=%04X(%d)\n", reg,offset,mem_mask,ACCESSING_LSB);

  return result;
}

static void sim07_t1_timeout(int which)
{
	unsigned short mode = sim07.timers[0].tmr;

	if ( mode & 0x0001 )
	{ // timer enabled

		logerror("**timer1** mature %f\n",sim07.timers[0].interval);

		if ( mode & 0x0010 )
		{ // IRQ enabled
			int t1_irq = (sim07.PICR>>12)&0x07; 

			logerror(" irq %d\n", t1_irq );
			if ( t1_irq )
			{
				cpunum_set_input_line(0, t1_irq, ASSERT_LINE);
			}
		}
	#if 0
		if ( mode & 0x0020 )
		{ // toggle output

		}
		else
		{ // active low pulse
		}
	#endif

	if ( mode & 0x0008 )
		{ // re-start timer
			if ( mtimer1 )
			{
				timer_reset(mtimer1, sim07.timers[0].interval );
			}
		}
	}
}

static void sim07_t2_timeout(int which)
{
}

// machine start (called only once) ////////////////////////////////////////

static MACHINE_START( bfm_sc4 )
{
	Lamps_init(256);

	Mechmtr_init(8);

	Stepper_init(0, STEPPER_48STEP_REEL);
	Stepper_init(1, STEPPER_48STEP_REEL);
	Stepper_init(2, STEPPER_48STEP_REEL);
	Stepper_init(3, STEPPER_48STEP_REEL);
	Stepper_init(4, STEPPER_48STEP_REEL);
	Stepper_init(5, STEPPER_48STEP_REEL);

	mtimer1 = mame_timer_alloc(sim07_t1_timeout);
	mtimer2 = mame_timer_alloc(sim07_t2_timeout);

	if ( mtimer1 ) timer_reset(mtimer1, TIME_NEVER);
	if ( mtimer2 ) timer_reset(mtimer2, TIME_NEVER);
	return 0;
}


// memory map for reading scorpion4 board /////////////////////////////////

static ADDRESS_MAP_START( memmap, ADDRESS_SPACE_PROGRAM, 16 )

	AM_RANGE(0x0000F2, 0x0000F3) AM_READ(sim07_mbar_r)

	AM_RANGE(0x800000, 0x80ffff) AM_WRITE(MWA16_RAM) AM_BASE(&nvram) AM_SIZE(&nvram_size) //

	AM_RANGE(0x800000, 0x80ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)

	AM_RANGE(0xFFF000, 0xFFF000+0xFFF)  AM_WRITE(sim07_w)
	AM_RANGE(0xFFF000, 0xFFF000+0xFFF)  AM_READ(sim07_r)

ADDRESS_MAP_END


// machine driver for scorpion4 board /////////////////////////////////////

static MACHINE_DRIVER_START( scorpion4 )

	MDRV_MACHINE_START(bfm_sc4)  // main scorpion4 board initialisation
	MDRV_MACHINE_RESET(bfm_sc4)  // main scorpion4 board initialisation
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(memmap, 0)  // setup read and write memorymap

	//MDRV_CPU_PERIODIC_INT(timer_irq, TIME_IN_HZ(1000) )	// generate 1000 IRQ's per second
	MDRV_DEFAULT_LAYOUT(layout_awpvid)
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(288, 29)
	MDRV_SCREEN_VISIBLE_AREA(0, 288-1, 0, 29-1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_VIDEO_START( awpvideo)
	MDRV_VIDEO_UPDATE(awpvideo)

	MDRV_PALETTE_LENGTH(22)
	MDRV_COLORTABLE_LENGTH(22)
	MDRV_PALETTE_INIT(awpvideo)

MACHINE_DRIVER_END

// input ports for scorpion4 board ////////////////////////////////////////

INPUT_PORTS_START( scorpion4_inputs )

INPUT_PORTS_END


// ROM definition /////////////////////////////////////////////////////////

ROM_START( thegame )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "95406248.lo", 0x00001, 0x80000, CRC(2edb055b) SHA1(38bffa8e53ae9ad11a8544830cd2624c36edfe14))
	ROM_LOAD16_BYTE( "95406249.hi", 0x00000, 0x80000, CRC(09fdf1dc) SHA1(a9ec270dde5e260ef556b68f061f85319e0b985c))
ROM_END

ROM_START( maz_pole )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "p7012s13.lo", 0x00001, 0x80000, CRC(3ef0e8ce) SHA1(a9c12c8a8c47a2a69559cffc9b9a74f3ceb6ebe8))
	ROM_LOAD16_BYTE( "p7012s13.hi", 0x00000, 0x80000, CRC(83367285) SHA1(4ba6bfc0d33607eadcb0600d0318cf682840008c))
ROM_END

GAME( 199?, thegame, 0,scorpion4, scorpion4_inputs, 0,		  0,       "BFM/Eurocoin",  "The Game (Dutch)",	GAME_NOT_WORKING | GAME_NO_SOUND)
GAME( 199?, maz_pole,0,scorpion4, scorpion4_inputs, 0,		  0,       "Mazooma",    	"Poleposition",		GAME_NOT_WORKING | GAME_NO_SOUND)
