/**********************************************************************************


    PLAYER'S EDGE PLUS (PE+)

    Driver by Jim Stolis.

    Special thanks to smf for I2C EEPROM support.

    --- Technical Notes ---

    Name:    Player's Edge Plus (PP0516) Double Bonus Draw Poker.
    Company: IGT - International Gaming Technology
    Year:    1987

    Hardware:

    CPU =  INTEL 83c02       ; I8052 compatible
    VIDEO = ROCKWELL 6545    ; CRTC6845 compatible
    SND =  AY-3-8912         ; AY8910 compatible

    History:

    This form of video poker machine has the ability to use different game roms.  The operator
    changes the game by placing the rom at U68 on the motherboard.  This driver is currently valid
    for the PP0516 game rom, but should work with all other compatible game roms as cpu, video,
    sound, and inputs is concerned.  Some games can share the same color prom and graphic roms,
    but this is not always the case.  It is best to confirm the game, color and graphic combinations.

    The game code runs in two different modes, game mode and operator mode.  Game mode is what a
    normal player would see when playing.  Operator mode is for the machine operator to adjust
    machine settings and view coin counts.  The upper door must be open in order to enter operator
    mode and so it should be mapped to an input bank if you wish to support it.  The operator
    has two additional inputs (jackpot reset and self-test) to navigate with, along with the
    normal buttons available to the player.

    A normal machine keeps all coin counts and settings in a battery-backed ram, and will
    periodically update an external eeprom for an even more secure backup.  This eeprom
    also holds the current game state in order to recover the player from a full power failure.

***********************************************************************************/
#include "driver.h"
#include "sound/ay8910.h"
#include "cpu/i8051/i8051.h"
#include "machine/i2cmem.h"

#include "peplus.lh"
#include "pepp0158.lh"
#include "pepp0188.lh"
#include "peset038.lh"

static tilemap *bg_tilemap;

/* Pointers to External RAM */
static UINT8 *program_ram;
static UINT8 *cmos_ram;
static UINT8 *s1000_ram;
static UINT8 *s3000_ram;
static UINT8 *s5000_ram;
static UINT8 *s7000_ram;
static UINT8 *sb000_ram;
static UINT8 *sd000_ram;
static UINT8 *sf000_ram;

/* Variables used instead of CRTC6845 system */
static UINT8 vid_register = 0;
static UINT8 vid_low = 0;
static UINT8 vid_high = 0;

/* Holds upper video address and palette number */
static UINT8 *palette_ram;

/* IO Ports */
static UINT8 *io_port;

/* Coin, Door, Hopper and EEPROM States */
static UINT32 last_cycles;
static UINT8 coin_state = 0;
static UINT32 last_door;
static UINT8 door_open = 0;
static UINT32 last_coin_out;
static UINT8 coin_out_state = 0;
static int sda_dir = 0;

/* Static Variables */
#define CMOS_NVRAM_SIZE     0x1000
#define EEPROM_NVRAM_SIZE   0x200 // 4k Bit


/*****************
* NVRAM Handlers *
******************/

static NVRAM_HANDLER( peplus )
{
	if (read_or_write)
	{
		mame_fwrite(file, cmos_ram, CMOS_NVRAM_SIZE);
	}
	else
	{
		if (file)
		{
			mame_fread(file, cmos_ram, CMOS_NVRAM_SIZE);
		}
		else
		{
			memset(cmos_ram, 0, CMOS_NVRAM_SIZE);
		}
	}

	nvram_handler_i2cmem_0( machine, file, read_or_write );
}

/*****************
* Write Handlers *
******************/

static WRITE8_HANDLER( peplus_bgcolor_w )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (~data >> 0) & 0x01;
		bit1 = (~data >> 1) & 0x01;
		bit2 = (~data >> 2) & 0x01;
		r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (~data >> 3) & 0x01;
		bit1 = (~data >> 4) & 0x01;
		bit2 = (~data >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (~data >> 6) & 0x01;
		bit1 = (~data >> 7) & 0x01;
		bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		palette_set_color(Machine, (15 + (i*16)), r, g, b);
	}
}

/*
    ROCKWELL 6545 - Transparent Memory Addressing
    The current CRTC6845 driver does not support these
    additional registers (R18, R19, R31)
*/
static WRITE8_HANDLER( peplus_crtc_mode_w )
{
	/* Mode Control - Register 8 */
	/* Sets CRT to Transparent Memory Addressing Mode */
}

static WRITE8_HANDLER( peplus_crtc_register_w )
{
    vid_register = data;
}

static WRITE8_HANDLER( peplus_crtc_address_w )
{
	switch(vid_register) {
		case 0x12:  /* Update Address High */
			vid_high = data & 0x3f;
			break;
		case 0x13:  /* Update Address Low */
			vid_low = data;
			break;
	}
}

static WRITE8_HANDLER( peplus_crtc_display_w )
{
	UINT16 vid_address = (vid_high<<8) | vid_low;

	videoram[vid_address] = data;
	palette_ram[vid_address] = io_port[1];
	tilemap_mark_tile_dirty(bg_tilemap, vid_address);

	/* Transparent Memory Addressing increments the update address register */
	if (vid_low == 0xff) {
		vid_high++;
	}
	vid_low++;
}

static WRITE8_HANDLER( peplus_io_w )
{
	io_port[offset] = data;
}

static WRITE8_HANDLER( peplus_duart_w )
{
	// Used for Slot Accounting System Communication
}

static WRITE8_HANDLER( peplus_cmos_w )
{
	cmos_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_s1000_w )
{
	s1000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_s3000_w )
{
	s3000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_s5000_w )
{
	s5000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_s7000_w )
{
	s7000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_sb000_w )
{
	sb000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_sd000_w )
{
	sd000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_sf000_w )
{
	sf000_ram[offset] = data;
}

static WRITE8_HANDLER( peplus_output_bank_a_w )
{
	output_set_value("coinlockout",(data >> 0) & 1); /* Coin Lockout */
	output_set_value("diverter",(data >> 1) & 1); /* Diverter */
	output_set_value("bell",(data >> 2) & 1); /* Bell */
	output_set_value("na1",(data >> 3) & 1); /* N/A */
	output_set_value("hopper1",(data >> 4) & 1); /* Hopper 1 */
	output_set_value("hopper2",(data >> 5) & 1); /* Hopper 2 */
	output_set_value("na2",(data >> 6) & 1); /* N/A */
	output_set_value("na3",(data >> 7) & 1); /* N/A */

    coin_out_state = 0;
    if((data >> 4) & 1)
        coin_out_state = 1;
}

static WRITE8_HANDLER( peplus_output_bank_b_w )
{
	output_set_lamp_value(0,(data >> 0) & 1); /* Hold 2 3 4 Lt */
	output_set_lamp_value(1,(data >> 1) & 1); /* Deal Draw Lt */
	output_set_lamp_value(2,(data >> 2) & 1); /* Cash Out Lt */
	output_set_lamp_value(3,(data >> 3) & 1); /* Hold 1 Lt */
	output_set_lamp_value(4,(data >> 4) & 1); /* Bet Credits Lt */
	output_set_lamp_value(5,(data >> 5) & 1); /* Change Request Lt */
	output_set_lamp_value(6,(data >> 6) & 1); /* Door Open Lt */
	output_set_lamp_value(7,(data >> 7) & 1); /* Hold 5 Lt */
}

static WRITE8_HANDLER( peplus_output_bank_c_w )
{
	output_set_value("coininmeter",(data >> 0) & 1); /* Coin In Meter */
	output_set_value("coinoutmeter",(data >> 1) & 1); /* Coin Out Meter */
	output_set_value("coindropmeter",(data >> 2) & 1); /* Coin Drop Meter */
	output_set_value("jackpotmeter",(data >> 3) & 1); /* Jackpot Meter */
	output_set_value("billacceptor",(data >> 4) & 1); /* Bill Acceptor Enabled */
	output_set_value("sdsout",(data >> 5) & 1); /* SDS Out */
	output_set_value("na4",(data >> 6) & 1); /* N/A */
	output_set_value("gamemeter",(data >> 7) & 1); /* Game Meter */
}

static WRITE8_HANDLER(i2c_nvram_w)
{
	i2cmem_write(0, I2CMEM_SCL, BIT(data, 2));
	sda_dir = BIT(data, 1);
	i2cmem_write(0, I2CMEM_SDA, BIT(data, 0));
}


/****************
* Read Handlers *
****************/

static READ8_HANDLER( peplus_crtc_display_r )
{
	UINT16 vid_address = ((vid_high<<8) | vid_low) + 1;
    vid_high = (vid_address>>8) & 0x3f;
    vid_low = vid_address& 0xff;

    return 0x00;
}

static READ8_HANDLER( peplus_crtc_lpen1_r )
{
    return 0x40;
}

static READ8_HANDLER( peplus_crtc_lpen2_r )
{
    UINT8 ret_val = 0x00;
    UINT8 x_val = readinputportbytag("TOUCH_X");
    UINT8 y_val = (0x19 - readinputportbytag("TOUCH_Y"));
    UINT16 t_val = y_val * 0x28 + (x_val+1);

	switch(vid_register) {
		case 0x10:  /* Light Pen Address High */
			ret_val = (t_val >> 8) & 0x3f;
			break;
		case 0x11:  /* Light Pen Address Low */
			ret_val = t_val & 0xff;
			break;
	}

    return ret_val;
}

static READ8_HANDLER( peplus_io_r )
{
    return io_port[offset];
}

static READ8_HANDLER( peplus_duart_r )
{
	// Used for Slot Accounting System Communication
	return 0x00;
}

static READ8_HANDLER( peplus_cmos_r )
{
	switch (offset)
	{
		case 0x00db:
			if ((readinputportbytag("DOOR") & 0x01) == 0) {
				cmos_ram[offset] = 0x00;
			}
			break;
		case 0x0b8d:
			if ((readinputportbytag("DOOR") & 0x02) == 1) {
				cmos_ram[offset] = 0x01;
			}
			break;
	}

	return cmos_ram[offset];
}

static READ8_HANDLER( peplus_s1000_r )
{
	return s1000_ram[offset];
}

static READ8_HANDLER( peplus_s3000_r )
{
	return s3000_ram[offset];
}

static READ8_HANDLER( peplus_s5000_r )
{
	return s5000_ram[offset];
}

static READ8_HANDLER( peplus_s7000_r )
{
	return s7000_ram[offset];
}

static READ8_HANDLER( peplus_sb000_r )
{
	return sb000_ram[offset];
}

static READ8_HANDLER( peplus_sd000_r )
{
	return sd000_ram[offset];
}

static READ8_HANDLER( peplus_sf000_r )
{
	return sf000_ram[offset];
}

/* External RAM Callback for I8052 */
static READ32_HANDLER( peplus_external_ram_iaddr )
{
	if (mem_mask == 0xff) {
		return (io_port[2] << 8) | offset;
	} else {
		return offset;
	}
}

/* Last Color in Every Palette is bgcolor */
static READ8_HANDLER( peplus_bgcolor_r )
{
	return palette_get_color(Machine, 15); // Return bgcolor from First Palette
}

static READ8_HANDLER( peplus_dropdoor_r )
{
	return 0x00; // Drop Door 0x00=Closed 0x02=Open
}

static READ8_HANDLER( peplus_watchdog_r )
{
	return 0x00; // Watchdog
}

static READ8_HANDLER( peplus_input_bank_a_r )
{
/*
        Bit 0 = COIN DETECTOR A
        Bit 1 = COIN DETECTOR B
        Bit 2 = COIN DETECTOR C
        Bit 3 = COIN OUT
        Bit 4 = HOPPER FULL
        Bit 5 = DOOR OPEN
        Bit 6 = LOW BATTERY
        Bit 7 = I2C EEPROM SDA
*/
	UINT8 bank_a = 0x50; // Turn Off Low Battery and Hopper Full Statuses
	UINT8 coin_optics = 0x00;
    UINT8 coin_out = 0x00;
	UINT32 curr_cycles = activecpu_gettotalcycles();

	UINT8 sda = 0;
	if(!sda_dir)
	{
		sda = i2cmem_read(0, I2CMEM_SDA);
	}

	if ((readinputportbytag("SENSOR") & 0x01) == 0x01 && coin_state == 0) {
		coin_state = 1; // Start Coin Cycle
		last_cycles = activecpu_gettotalcycles();
	} else {
		/* Process Next Coin Optic State */
		if (curr_cycles - last_cycles > 20000 && coin_state != 0) {
			coin_state++;
			if (coin_state > 5)
				coin_state = 0;
			last_cycles = activecpu_gettotalcycles();
		}
	}

	switch (coin_state)
	{
		case 0x00: // No Coin
			coin_optics = 0x00;
			break;
		case 0x01: // Optic A
			coin_optics = 0x01;
			break;
		case 0x02: // Optic AB
			coin_optics = 0x03;
			break;
		case 0x03: // Optic ABC
			coin_optics = 0x07;
			break;
		case 0x04: // Optic BC
			coin_optics = 0x06;
			break;
		case 0x05: // Optic C
			coin_optics = 0x04;
			break;
	}

	if (curr_cycles - last_door > 6000) { // Guessing with 6000
		if ((readinputportbytag("DOOR") & 0x01) == 0x01) {
			door_open = (!door_open & 0x01);
		} else {
			door_open = 1;
		}
		last_door = activecpu_gettotalcycles();
	}

	if (curr_cycles - last_coin_out > 60000) { // Guessing with 60000
		if (coin_out_state == 1) {
            coin_out = 0x08;
		}
        coin_out_state = 0;
		last_coin_out = activecpu_gettotalcycles();
	}

	bank_a = (sda<<7) | bank_a | (door_open<<5) | coin_optics | coin_out;

	return bank_a;
}


/****************************
* Video/Character functions *
****************************/

static void get_bg_tile_info(int tile_index)
{
	int pr = palette_ram[tile_index];
	int vr = videoram[tile_index];

	int code = ((pr & 0x0f)*256) | vr;
	int color = (pr>>4) & 0x0f;

	SET_TILE_INFO(0, code, color, 0);
}

static VIDEO_START( peplus )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 40, 25);
	palette_ram = auto_malloc(0x3000);
	memset(palette_ram, 0, 0x3000);
    return 0;
}

static VIDEO_UPDATE( peplus )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	return 0;
}

static PALETTE_INIT( peplus )
{
/*  prom bits
    7654 3210
    ---- -xxx   red component.
    --xx x---   green component.
    xx-- ----   blue component.
*/
	int i;

	for (i = 0;i < machine->drv->total_colors;i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (~color_prom[i] >> 0) & 0x01;
		bit1 = (~color_prom[i] >> 1) & 0x01;
		bit2 = (~color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (~color_prom[i] >> 3) & 0x01;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (~color_prom[i] >> 6) & 0x01;
		bit1 = (~color_prom[i] >> 7) & 0x01;
		bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		palette_set_color(machine, i, r, g, b);
	}
}


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout charlayout =
{
	8,8,    /* 8x8 characters */
	0x1000, /* 4096 characters */
	4,  /* 4 bitplanes */
	{ 0x1000*8*8*3, 0x1000*8*8*2, 0x1000*8*8*1, 0x1000*8*8*0 }, /* bitplane offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


/******************************
* Graphics Decode Information *
******************************/

static const gfx_decode peplus_gfxdecodeinfo[] =
{
    { REGION_GFX1, 0x00000, &charlayout,     0, 64*2 },
    { -1 }
};

/**************
* Driver Init *
***************/

static DRIVER_INIT( peplus )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	//program_ram[0x5e7e] = 0x01; // Enable Autohold Feature
	program_ram[0x9a24] = 0x22; // RET - Disable Memory Test
	program_ram[0xd61d] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( pepp0158 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	//program_ram[0x5ffe] = 0x01; // Enable Autohold Feature
	program_ram[0xa19f] = 0x22; // RET - Disable Memory Test
	program_ram[0xddea] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( pepp0188 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	//program_ram[0x742f] = 0x01; // Enable Autohold Feature
	program_ram[0x9a8d] = 0x22; // RET - Disable Memory Test
	program_ram[0xf429] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( peset038 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0x302] = 0x22;  // RET - Disable Memory Test
	program_ram[0x289f] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( pebe0014 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0x75e7] = 0x22; // RET - Disable Memory Test
	program_ram[0xc3ab] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( peke1012 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0x59e7] = 0x22; // RET - Disable Memory Test
	program_ram[0xbe01] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( peps0615 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0x84be] = 0x22; // RET - Disable Memory Test
	program_ram[0xbfd8] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( peps0716 )
{
	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0x7f99] = 0x22; // RET - Disable Memory Test
	program_ram[0xbaa9] = 0x22; // RET - Disable Program Checksum
}

static DRIVER_INIT( pexp0019 )
{
    UINT8 *super_data = memory_region(REGION_USER1);

    /* Distribute Superboard Data */
    memcpy(s1000_ram, &super_data[0], 0x1000);
    memcpy(s3000_ram, &super_data[0x3000], 0x1000);
    memcpy(s5000_ram, &super_data[0x5000], 0x1000);
    memcpy(s7000_ram, &super_data[0x7000], 0x1000);
    memcpy(sb000_ram, &super_data[0xb000], 0x1000);
    memcpy(sd000_ram, &super_data[0xd000], 0x1000);
    memcpy(sf000_ram, &super_data[0xf000], 0x1000);

	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0xc1e4] = 0x22; // RET - Disable Memory Test
	program_ram[0xc15f] = 0x22; // RET - Disable Program Checksum
    program_ram[0xc421] = 0x22; // RET - Disable 2nd Memory Test
}

static DRIVER_INIT( pexs0006 )
{
    UINT8 *super_data = memory_region(REGION_USER1);

    /* Distribute Superboard Data */
    memcpy(s1000_ram, &super_data[0], 0x1000);
    memcpy(s3000_ram, &super_data[0x3000], 0x1000);
    memcpy(s5000_ram, &super_data[0x5000], 0x1000);
    memcpy(s7000_ram, &super_data[0x7000], 0x1000);
    memcpy(sb000_ram, &super_data[0xb000], 0x1000);
    memcpy(sd000_ram, &super_data[0xd000], 0x1000);
    memcpy(sf000_ram, &super_data[0xf000], 0x1000);

	/* External RAM callback */
	i8051_set_eram_iaddr_callback(peplus_external_ram_iaddr);

    /* EEPROM is a X2404P 4K-bit Serial I2C Bus */
	i2cmem_init(0, I2CMEM_SLAVE_ADDRESS, 8, EEPROM_NVRAM_SIZE, NULL);

	// For testing only, cannot stay in final driver
	program_ram[0x9bd4] = 0x22; // RET - Disable Memory Test
    program_ram[0x9e9c] = 0x22; // RET - Disable 2nd Memory Test
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( peplus_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_ROM AM_BASE(&program_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( peplus_datamap, ADDRESS_SPACE_DATA, 8 )
	// Battery-backed RAM
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_READWRITE(peplus_cmos_r, peplus_cmos_w) AM_BASE(&cmos_ram)

	// Superboard Data
	AM_RANGE(0x1000, 0x1fff) AM_RAM AM_READWRITE(peplus_s1000_r, peplus_s1000_w) AM_BASE(&s1000_ram)

	// CRT Controller
	AM_RANGE(0x2008, 0x2008) AM_WRITE(peplus_crtc_mode_w)
	AM_RANGE(0x2080, 0x2080) AM_READ(peplus_crtc_lpen1_r) AM_WRITE(peplus_crtc_register_w)
	AM_RANGE(0x2081, 0x2081) AM_READ(peplus_crtc_lpen2_r) AM_WRITE(peplus_crtc_address_w)
	AM_RANGE(0x2083, 0x2083) AM_READ(peplus_crtc_display_r) AM_WRITE(peplus_crtc_display_w)

    // Superboard Data
	AM_RANGE(0x3000, 0x3fff) AM_RAM AM_READWRITE(peplus_s3000_r, peplus_s3000_w) AM_BASE(&s3000_ram)

	// Sound and Dipswitches
	AM_RANGE(0x4000, 0x4000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x4004, 0x4004) AM_READ(input_port_3_r) AM_WRITE(AY8910_write_port_0_w)

    // Superboard Data
	AM_RANGE(0x5000, 0x5fff) AM_RAM AM_READWRITE(peplus_s5000_r, peplus_s5000_w) AM_BASE(&s5000_ram)

	// Background Color Latch
	AM_RANGE(0x6000, 0x6000) AM_READ(peplus_bgcolor_r) AM_WRITE(peplus_bgcolor_w)

    // Bogus Location for Video RAM
	AM_RANGE(0x06001, 0x06400) AM_RAM AM_BASE(&videoram)

    // Superboard Data
	AM_RANGE(0x7000, 0x7fff) AM_RAM AM_READWRITE(peplus_s7000_r, peplus_s7000_w) AM_BASE(&s7000_ram)

	// Input Bank A, Output Bank C
	AM_RANGE(0x8000, 0x8000) AM_READ(peplus_input_bank_a_r) AM_WRITE(peplus_output_bank_c_w)

	// Drop Door, I2C EEPROM Writes
	AM_RANGE(0x9000, 0x9000) AM_READ(peplus_dropdoor_r) AM_WRITE(i2c_nvram_w)

	// Input Banks B & C, Output Bank B
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r) AM_WRITE(peplus_output_bank_b_w)

    // Superboard Data
	AM_RANGE(0xb000, 0xbfff) AM_RAM AM_READWRITE(peplus_sb000_r, peplus_sb000_w) AM_BASE(&sb000_ram)

	// Output Bank A
	AM_RANGE(0xc000, 0xc000) AM_READ(peplus_watchdog_r) AM_WRITE(peplus_output_bank_a_w)

    // Superboard Data
	AM_RANGE(0xd000, 0xdfff) AM_RAM AM_READWRITE(peplus_sd000_r, peplus_sd000_w) AM_BASE(&sd000_ram)

	// DUART
	AM_RANGE(0xe000, 0xe005) AM_READWRITE(peplus_duart_r, peplus_duart_w)

    // Superboard Data
	AM_RANGE(0xf000, 0xffff) AM_RAM AM_READWRITE(peplus_sf000_r, peplus_sf000_w) AM_BASE(&sf000_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( peplus_iomap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))

	// I/O Ports
	AM_RANGE(0x00, 0x03) AM_READ(peplus_io_r) AM_WRITE(peplus_io_w) AM_BASE(&io_port)
ADDRESS_MAP_END

/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( peplus_poker )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Hold 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x05, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Hold 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Hold 4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Hold 5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x50, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_BUTTON15 ) PORT_NAME("Bill Acceptor") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DOOR")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

	PORT_START_TAG("SENSOR")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("Coin In") PORT_IMPULSE(1)

	PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Line Frequency" )
	PORT_DIPSETTING(    0x01, "60HZ" )
	PORT_DIPSETTING(    0x00, "50HZ" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( peplus_keno )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x05, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Erase") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("Light Pen") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x50, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_BUTTON15 ) PORT_NAME("Bill Acceptor") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DOOR")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

	PORT_START_TAG("SENSOR")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("Coin In") PORT_IMPULSE(1)

	PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Line Frequency" )
	PORT_DIPSETTING(    0x01, "60HZ" )
	PORT_DIPSETTING(    0x00, "50HZ" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

    PORT_START_TAG("TOUCH_X")
    PORT_BIT( 0xff, 0x08, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00, 0x28) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(13)
    PORT_START_TAG("TOUCH_Y")
	PORT_BIT( 0xff, 0x08, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00, 0x19) PORT_CROSSHAIR(Y, -1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(13)
INPUT_PORTS_END

/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( peplus )
	// basic machine hardware
	MDRV_CPU_ADD_TAG( "main", I8052, 3686400*2 )
	MDRV_CPU_PROGRAM_MAP( peplus_map, 0 )
	MDRV_CPU_DATA_MAP( peplus_datamap, 0 )
	MDRV_CPU_IO_MAP( peplus_iomap, 0 )
	MDRV_CPU_VBLANK_INT( irq0_line_hold, 1 )

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME( DEFAULT_60HZ_VBLANK_DURATION )

	MDRV_NVRAM_HANDLER( peplus )

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((52+1)*8, (31+1)*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 25*8-1)

	MDRV_GFXDECODE( peplus_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(16*16)

	MDRV_PALETTE_INIT(peplus)
	MDRV_VIDEO_START(peplus)
	MDRV_VIDEO_UPDATE(peplus)

	// sound hardware
	MDRV_SOUND_ADD(AY8910, 20000000/12)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END

/*************************
*        Rom Load        *
*************************/

ROM_START( peplus )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pp0516.u68",   0x00000, 0x10000, CRC(d9da6e13) SHA1(421678d9cb42daaf5b21074cc3900db914dd26cf) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END

ROM_START( pepp0158 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pp0158.u68",   0x00000, 0x10000, CRC(5976cd19) SHA1(6a461ea9ddf78dffa3cf8b65903ebf3127f23d45) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END

ROM_START( pepp0188 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pp0188.u68",   0x00000, 0x10000, CRC(cf36a53c) SHA1(99b578538ab24d9ff91971b1f77599272d1dbfc6) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END

ROM_START( peset038 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "set038.u68",   0x00000, 0x10000, CRC(9c4b1d1a) SHA1(8a65cd1d8e2d74c7b66f4dfc73e7afca8458e979) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg740.u72",	 0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",	 0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",	 0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",	 0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) )
ROM_END

ROM_START( pebe0014 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "be0014.u68",   0x00000, 0x10000, CRC(232b32b7) SHA1(a3af9414577642fedc23b4c1911901cd31e9d6e0) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg2036.u72",	 0x00000, 0x8000, CRC(0a168d06) SHA1(7ed4fb5c7bcacab077bcec030f0465c6eaf3ce1c) )
	ROM_LOAD( "mgo-cg2036.u73",	 0x08000, 0x8000, CRC(826b4090) SHA1(34390484c0faffe9340fd93d273b9292d09f97fd) )
	ROM_LOAD( "mbo-cg2036.u74",	 0x10000, 0x8000, CRC(46aac851) SHA1(28d84b49c6cebcf2894b5a15d935618f84093caa) )
	ROM_LOAD( "mxo-cg2036.u75",	 0x18000, 0x8000, CRC(60204a56) SHA1(2e3420da9e79ba304ca866d124788f84861380a7) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap707.u50", 0x0000, 0x0100, CRC(9851ba36) SHA1(5a0a43c1e212ae8c173102ede9c57a3d95752f99) )
ROM_END

ROM_START( peke1012 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ke1012.u68",   0x00000, 0x10000, CRC(470e8c10) SHA1(f8a65a3a73477e9e9d2f582eeefa93b470497dfa) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) // BAD DUMPS
	ROM_LOAD( "mro-cg1267.u72",	 0x00000, 0x8000, CRC(16498b57) SHA1(9c22726299af7204c4be1c6d8afc4c1b512ad918) )
	ROM_LOAD( "mgo-cg1267.u73",	 0x08000, 0x8000, CRC(80847c5a) SHA1(8422cd13a91c3c462af5efcfca8615e7eeaa2e52) )
	ROM_LOAD( "mbo-cg1267.u74",	 0x10000, 0x8000, CRC(ce7af8a7) SHA1(38675122c764b8fa9260246ea99ac0f0750da277) )
	ROM_LOAD( "mxo-cg1267.u75",	 0x18000, 0x8000, CRC(3aac0d4a) SHA1(764da54bdb2f2c49551cf1d10286de9450abad2f) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap1267.u50", 0x0000, 0x0100, CRC(7051db57) SHA1(76751a3cc47d506983205decb07e99ca0c178a42) )
ROM_END

ROM_START( peps0615 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ps0615.u68",   0x00000, 0x10000, CRC(d27dd6ab) SHA1(b3f065f507191682edbd93b07b72ed87bf6ae9b1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg2246.u72",	 0x00000, 0x8000, CRC(7c08c355) SHA1(2a154b81c6d9671cea55a924bffb7f5461747142) )
	ROM_LOAD( "mgo-cg2246.u73",	 0x08000, 0x8000, CRC(b3c16487) SHA1(c97232fadd086f604eaeb3cd3c2d1c8fe0dcfa70) )
	ROM_LOAD( "mbo-cg2246.u74",	 0x10000, 0x8000, CRC(e61331f5) SHA1(4364edc625d64151cbae40780b54cb1981086647) )
	ROM_LOAD( "mxo-cg2246.u75",	 0x18000, 0x8000, CRC(f0f4a27d) SHA1(3a10ab196aeaa5b50d47b9d3c5b378cfadd6fe96) )

	ROM_REGION( 0x100, REGION_PROMS, 0 ) // WRONG CAP
	ROM_LOAD( "cap2234.u50", 0x0000, 0x0100, CRC(a930e283) SHA1(61bce50fa13b3e980ece3e72d068835e19bd5049) )
ROM_END

ROM_START( peps0716 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ps0716.u68",   0x00000, 0x10000, CRC(7615d7b6) SHA1(91fe62eec720a0dc2ebf48835065148f19499d16) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg2266.u72",	 0x00000, 0x8000, CRC(590accd8) SHA1(4e1c963c50799eaa49970e25ecf9cb01eb6b09e1) )
	ROM_LOAD( "mgo-cg2266.u73",	 0x08000, 0x8000, CRC(b87ffa05) SHA1(92126b670b9cabeb5e2cc35b6e9c458088b18eea) )
	ROM_LOAD( "mbo-cg2266.u74",	 0x10000, 0x8000, CRC(e3df30e1) SHA1(c7d2ae9a7c7e53bfb6197b635efcb5dc231e4fe0) )
	ROM_LOAD( "mxo-cg2266.u75",	 0x18000, 0x8000, CRC(56271442) SHA1(61ad0756b9f6412516e46ef6625a4c3899104d4e) )

	ROM_REGION( 0x100, REGION_PROMS, 0 ) // WRONG CAP
	ROM_LOAD( "cap2265.u50", 0x0000, 0x0100, CRC(dfb82a2f) SHA1(c96947bb475bf4497ff2e44053941625a3a7bf62) )
ROM_END

ROM_START( pexp0019 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "xp000019.u67",   0x00000, 0x10000, CRC(8ac876eb) SHA1(105b4aee2692ccb20795586ccbdf722c59db66cf) )

    ROM_REGION( 0x10000, REGION_USER1, 0 )
    ROM_LOAD( "x002025p.u66",   0x00000, 0x10000, CRC(f3dac423) SHA1(e9394d330deb3b8a1001e57e72a506cd9098f161) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg2185.u77",	 0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) )
	ROM_LOAD( "mgo-cg2185.u78",	 0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",	 0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",	 0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x100, REGION_PROMS, 0 ) // WRONG CAP
	ROM_LOAD( "cap2234.u43", 0x0000, 0x0100, CRC(a930e283) SHA1(61bce50fa13b3e980ece3e72d068835e19bd5049) )
ROM_END

ROM_START( pexs0006 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "xs000006.u67",   0x00000, 0x10000, CRC(4b11ca18) SHA1(f64a1fbd089c01bc35a5484e60b8834a2db4a79f) )

    ROM_REGION( 0x10000, REGION_USER1, 0 )
    ROM_LOAD( "x000998s.u66",   0x00000, 0x10000, CRC(e29d4346) SHA1(93901ff65c8973e34ac1f0dd68bb4c4973da5621) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mro-cg2361.u77",	 0x00000, 0x8000, CRC(c0eba866) SHA1(8f217aeb3e8028a5633d95e5612f1b55e601650f) )
	ROM_LOAD( "mgo-cg2361.u78",	 0x08000, 0x8000, CRC(345eaea2) SHA1(18ebb94a323e1cf671201db8b9f85d4f30d8b5ec) )
	ROM_LOAD( "mbo-cg2361.u79",	 0x10000, 0x8000, CRC(fa130af6) SHA1(aca5e52e00bc75a4801fd3f6c564e62ed4251d8e) )
	ROM_LOAD( "mxo-cg2361.u80",	 0x18000, 0x8000, CRC(7de1812c) SHA1(c7e23a10f20fc8b618df21dd33ac456e1d2cfe33) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "cap2361.u43", 0x0000, 0x0100, CRC(051aea66) SHA1(2abf32caaeb821ca50a6398581de69bbfe5930e9) )
ROM_END

/*************************
*      Game Drivers      *
*************************/

/*    YEAR  NAME      PARENT  MACHINE  INPUT          INIT      ROT    COMPANY                                  FULLNAME                                                      FLAGS   */
GAME( 1987, peplus,   0,      peplus,  peplus_poker,  peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0516) Double Bonus Poker",             0 )
GAME( 1987, pepp0158, 0,      peplus,  peplus_poker,  pepp0158, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0158) 4 of a Kind Bonus Poker",        0 )
GAME( 1987, pepp0188, 0,      peplus,  peplus_poker,  pepp0188, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0188) Standard Draw Poker",            0 )
GAME( 1987, peset038, 0,      peplus,  peplus_poker,  peset038, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (Set038) Set Chip",                       0 )
GAME( 1994, pebe0014, 0,      peplus,  peplus_poker,  pebe0014, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (BE0014) Blackjack",                      0 )
GAME( 1994, peke1012, 0,      peplus,  peplus_keno,   peke1012, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (KE1012) Keno",                           0 )
GAME( 1996, peps0615, 0,      peplus,  peplus_poker,  peps0615, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0615) Chaos Slots",                    0 )
GAME( 1996, peps0716, 0,      peplus,  peplus_poker,  peps0716, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0716) Quarter Mania Slots",            0 )
GAME( 1995, pexp0019, 0,      peplus,  peplus_poker,  pexp0019, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XP000019) Deuces Wild Poker",            0 )
GAME( 1997, pexs0006, 0,      peplus,  peplus_poker,  pexs0006, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XS000006) Triple Triple Diamond Slots",  0 )
