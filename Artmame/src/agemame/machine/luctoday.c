/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "machine/7474.h"
#include "machine/8255ppi.h"
#include "includes/galaxian.h"



static int irq_line;
static mame_timer *int_timer;

static void galaxian_7474_9M_2_callback(void)
{
	/* Q bar clocks the other flip-flop,
       Q is VBLANK (not visible to the CPU) */
	TTL7474_clock_w(1, TTL7474_output_comp_r(0));
	TTL7474_update(1);
}

static void galaxian_7474_9M_1_callback(void)
{
	/* Q goes to the NMI line */
	cpunum_set_input_line(0, irq_line, TTL7474_output_r(1) ? CLEAR_LINE : ASSERT_LINE);
}

static const struct TTL7474_interface galaxian_7474_9M_2_intf =
{
	galaxian_7474_9M_2_callback
};

static const struct TTL7474_interface galaxian_7474_9M_1_intf =
{
	galaxian_7474_9M_1_callback
};


WRITE8_HANDLER( galaxian_nmi_enable_w )
{
	TTL7474_preset_w(1, data);
	TTL7474_update(1);
}


static void interrupt_timer(int param)
{
	/* 128V, 64V and 32V go to D */
	TTL7474_d_w(0, (param & 0xe0) != 0xe0);

	/* 16V clocks the flip-flop */
	TTL7474_clock_w(0, param & 0x10);

	param = (param + 0x10) & 0xff;

	timer_adjust(int_timer, cpu_getscanlinetime(param), param, 0);

	TTL7474_update(0);
}


static void machine_reset_common( int line )
{
	irq_line = line;

	/* initalize main CPU interrupt generator flip-flops */
	TTL7474_config(0, &galaxian_7474_9M_2_intf);
	TTL7474_preset_w(0, 1);
	TTL7474_clear_w (0, 1);

	TTL7474_config(1, &galaxian_7474_9M_1_intf);
	TTL7474_clear_w (1, 1);
	TTL7474_d_w     (1, 0);
	TTL7474_preset_w(1, 0);

	/* start a timer to generate interrupts */
	int_timer = timer_alloc(interrupt_timer);
	timer_adjust(int_timer, cpu_getscanlinetime(0), 0, 0);
}

MACHINE_RESET( galaxian )
{
	machine_reset_common(INPUT_LINE_NMI);
}



WRITE8_HANDLER( galaxian_coin_lockout_w )
{
	coin_lockout_global_w(~data & 1);
}


WRITE8_HANDLER( galaxian_coin_counter_w )
{
	coin_counter_w(offset, data & 0x01);
}

WRITE8_HANDLER( galaxian_coin_counter_1_w )
{
	coin_counter_w(1, data & 0x01);
}

WRITE8_HANDLER( galaxian_coin_counter_2_w )
{
	coin_counter_w(2, data & 0x01);
}


WRITE8_HANDLER( galaxian_leds_w )
{
	set_led_status(offset,data & 1);
}
