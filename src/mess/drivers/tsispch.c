/******************************************************************************
*
*  Telesensory Systems Inc./Speech Plus
*  1500 and 2000 series
*  Prose 2020
*  By Jonathan Gevaryahu AKA Lord Nightmare and Kevin 'kevtris' Horton
*
*  Skeleton Driver
*
*  DONE:
*  Skeleton Written
*  Load cpu and dsp roms and mapper proms
*  Successful compile
*  Successful run
*  Correctly Interleave 8086 CPU roms
*  Debug LEDs hooked to popmessage
*  Correctly load UPD7720 roms as UPD7725 data - done, this is utterly disgusting code.
*  Attached i8251a uart at u15
*  Added dipswitch array S4
*  Attached 8259 PIC
   * IR0 = upd7720 p0 pin masked by something, not hooked up yet
   * IR1 = i8251 rxrdy
   * IR2 = i8251 txempty
   * IR3 = i8251 txrdy
   * IR4,5,6,7 unknown so far, not hooked up yet
*  Hooked the terminal to the i8251a uart at u15
*
*  TODO:
*  Attach the upd7720 serial output to the DAC; this requires fixing the upd7725 core!
*  Attach the other i8251a uart (assuming it is hooked to the hardware at all!)
*  Correctly implement UPD7720 cpu core to avoid needing revolting conversion code
*  Correct memory maps and io maps - partly done
*  8259 PIC: figure out where IR4-7 come from
*  Add other dipswitches and jumpers (these may actually just control clock dividers for the two 8251s)
*  Everything else
*
*  Notes:
*  Text in rom indicates there is a test mode 'activated by switch s4 dash 7'
*  When switch s4-7 is switched on, the hardware says, over and over:
*  "This is version 3.4.1 test mode, activated by switch s4 dash 7"
*  LEDS: bits 6,5,4,3 are debug leds on the board, and are placed in square
*    brackets to note this.
*  "led 7" may actually be the mask for interrupt for upd7720 to ip1
*  Bootup notes:
*    D3109: checks if 0x80 (S4-8) is set: if set, continue, else jump to D3123
*    D3123: write 0x1C (0 [0 0 1 1] 1 0 0) to LEDS
*    then jump to D32B0
*      D32B0: memory test routine:
*        This routine flood-fills memory from 0000-2FFF with 0xFF,
*        then, bytewise starting from 0000, shifts the value progresively
*        right by one, writes it and checks that it still matches,
*        i.e. read 0xFF, write 0x7f, read 0x7f, write 0x3f... etc.
*        Loop at D32E4.
*      D32E6: similar to D32B0, but rotates in 1 bits to 16 bit words,
*        though only the low byte is written, and only fills the 2BFF
*        down to 0000 region. (seems rather redundant, actually)
*        Loop at D3301.
*      D3311: write 0x0A (0 [0 0 0 1] 0 1 0) to LEDS
*      then jump to D3330
*      D3330: jump back to D312E
*    D312E: this is some unknown conditional code, usually goes to D314E
*      if BP is not 1, go to D314E and don't update leds (usually taken?)
*      if BP is 1 and SI is 0, delay for 8*65536 cycles. no delay if si!=0
*      write 0x0C (0 [0 0 0 1] 1 0 0) to LEDS
*    D314E: floodfill 0000-2BFF with 0x55 (rep at D315C)
*      check if bp was 1 and jump to D318F if it was
*      write 0x14 (0 [0 0 1 0] 1 0 0) to LEDS
*      call E3987: initialize UPD7720, return
*    D33D2: checksum the roms in 5? passes, loop at D33DA, test at D33E6 (which passes)
*      if test DID fail: write 0x10 (0 [0 0 1 0] 0 0 0) to LEDS
*        more stuff
*        write 0xFF to leds
*        more stuff
*        set up word table? not sure what its doing here...
*      if test does NOT fail (and it doesn't):
*        D3414: write 0x08 (0 [0 0 0 1] 0 0 0) to LEDS
*    D5E14: initialize PIC8259
*    <more stuff, wip>
*    D338A: write 0x12 0 [0 0 1 0] 0 1 0 to LEDS
*
*  F44B4: general in-operation LED status write
******************************************************************************/
#define ADDRESS_MAP_MODERN

/* Core includes */
#include "emu.h"
#include "includes/tsispch.h"
#include "cpu/upd7725/upd7725.h"
#include "cpu/i86/i86.h"
#include "machine/msm8251.h"
#include "machine/pic8259.h"
#include "machine/terminal.h"

// defines
#undef VERBOSE
#undef DEBUG_DSP
#undef DEBUG_DSP_W

/*
   Devices and handlers
 */

/*****************************************************************************
 USART 8251 and Terminal stuff
*****************************************************************************/
static WRITE_LINE_DEVICE_HANDLER( i8251_rxrdy_int )
{
	pic8259_ir1_w(device->machine->device("pic8259"), state);
}

static WRITE_LINE_DEVICE_HANDLER( i8251_txempty_int )
{
	pic8259_ir2_w(device->machine->device("pic8259"), state);
}

static WRITE_LINE_DEVICE_HANDLER( i8251_txrdy_int )
{
	pic8259_ir3_w(device->machine->device("pic8259"), state);
}

const msm8251_interface msm8251_config =
{
	DEVCB_NULL, // in rxd, serial
	DEVCB_NULL, // out txd, serial
	DEVCB_NULL, // in dsr
	DEVCB_NULL, // out dtr
	DEVCB_NULL, // out rts
	DEVCB_LINE(i8251_rxrdy_int), // out rxrdy
	DEVCB_LINE(i8251_txrdy_int), // out txrdy
	DEVCB_LINE(i8251_txempty_int), // out txempty
	DEVCB_NULL  // out syndet
};

WRITE8_MEMBER( tsispch_state::i8251_rxd )
{
	msm8251_receive_character(machine->device("pic8259"), data);
}
static GENERIC_TERMINAL_INTERFACE( tsispch_terminal_intf )
{
	DEVCB_DRIVER_MEMBER(tsispch_state, i8251_rxd)
};

/*****************************************************************************
 PIC 8259 stuff
*****************************************************************************/
static WRITE_LINE_DEVICE_HANDLER( pic8259_set_int_line )
{
	cputag_set_input_line(device->machine, "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

static const struct pic8259_interface pic8259_config =
{
	DEVCB_LINE(pic8259_set_int_line)
};

static IRQ_CALLBACK(irq_callback)
{
	return pic8259_acknowledge(device->machine->device("pic8259"));
}

/*****************************************************************************
 LED/dipswitch stuff
*****************************************************************************/
READ8_MEMBER( tsispch_state::dsw_r )
{
	UINT8 data;
	/* the only dipswitch I'm really sure about is s4-7 which enables the test mode
	 * The switches are, for normal operation on my unit:
	 * ON ON OFF OFF OFF OFF OFF OFF
	 * which makes this register read 0xFC
	 * When s4-7 is turned on, it reads 0xBC
	 */
	data = input_port_read(space.machine, "s4");
	return data;
}

WRITE8_MEMBER( tsispch_state::led_w )
{
	/* These are the 4 debug leds on the pcb inside the case.
	   They are called on the silkscreen, unimaginatively, '6','5','4',and '3', and
	   are connected to bits 6, 5, 4, and 3 of this register.
	   Bit 6 also connects to the 'talking' LED on the front panel.
	   When 0 is written to a bit, the led turns on.
	   When the unit is idle, leds 5 and 3 are on. (bits 6 and 4 are set)
	   Bit 7 doesn't seem to be used so far. it may be a reset line for something.
	   Bits 0,1,2 may be a startup test status counter;
	     when the unit starts up, they have 1, 2, 3 writen to them sequentiall
	     as each test passes; note that tests 1 and 2 (ram tests and rom test?)
	     both pass while test 3 fails... assuming this is indeed a counter.
	*/
	tsispch_state *state = machine->driver_data<tsispch_state>();
	state->statusLED = data;
#ifdef VERBOSE
	logerror("8086: LED status: %02X\n", data);
#endif
	popmessage("LED status: (%x) %x %x %x %x (%x %x %x)\n", BIT(data,7), BIT(data,6), BIT(data,5), BIT(data,4), BIT(data,3), BIT(data,2), BIT(data,1), BIT(data,0));
}

/*****************************************************************************
 UPD77P20 stuff
*****************************************************************************/
READ16_MEMBER( tsispch_state::dsp_data_r )
{
	upd7725_device *upd7725 = machine->device<upd7725_device>("dsp");
#ifdef DEBUG_DSP
	UINT8 temp;
	temp = upd7725->snesdsp_read(true);
	fprintf(stderr, "dsp data read: %02x\n", temp);
	return temp;
#else
	return upd7725->snesdsp_read(true);
#endif
}

WRITE16_MEMBER( tsispch_state::dsp_data_w )
{
	upd7725_device *upd7725 = machine->device<upd7725_device>("dsp");
#ifdef DEBUG_DSP_W
	fprintf(stderr, "dsp data write: %02x\n", data);
#endif
	upd7725->snesdsp_write(true, data);
}

READ16_MEMBER( tsispch_state::dsp_status_r )
{
	upd7725_device *upd7725 = machine->device<upd7725_device>("dsp");
#ifdef DEBUG_DSP
	UINT8 temp;
	temp = upd7725->snesdsp_read(false);
	fprintf(stderr, "dsp status read: %02x\n", temp);
	return temp;
#else
	return upd7725->snesdsp_read(false);
#endif
}

WRITE16_MEMBER( tsispch_state::dsp_status_w )
{
	fprintf(stderr, "warning: upd772x status register should never be written to!\n");
	upd7725_device *upd7725 = machine->device<upd7725_device>("dsp");
	upd7725->snesdsp_write(false, data);
}

/*****************************************************************************
 Reset and Driver Init
*****************************************************************************/
void tsispch_state::machine_reset()
{
	// clear fifos (TODO: memset would work better here...)
	int i;
	for (i=0; i<32; i++) infifo[i] = 0;
	infifo_tail_ptr = infifo_head_ptr = 0;
	cpu_set_irq_callback(machine->device("maincpu"), irq_callback);
	// below seem to be unnecessary?
	//cputag_set_input_line(machine, "dsp", INPUT_LINE_RESET, ASSERT_LINE);
	//cputag_set_input_line(machine, "dsp", INPUT_LINE_RESET, CLEAR_LINE);
	fprintf(stderr,"machine reset\n");
}

DRIVER_INIT( prose2k )
{
	UINT8 *dspsrc = (UINT8 *)machine->region("dspprgload")->base();
	UINT32 *dspprg = (UINT32 *)machine->region("dspprg")->base();
	fprintf(stderr,"driver init\n");
    // unpack 24 bit 7720 data into 32 bit space and shuffle it so it can run as 7725 code
	// data format as-is in dspsrc: (L = always 0, X = doesn't matter)
	// source upd7720                  dest upd7725
	// bit 7  6  5  4  3  2  1  0      bit 7  6  5  4  3  2  1  0
	// for OP/RT:
	// b1  15 16 17 18 19 20 21 22 ->      22 21 20 19 18 17 16 15
	// b2  L  8  9  10 11 12 13 14 ->      14 13 12 L  11 10 9  8
	// b3  0  1  2  3  4  5  6  7  ->      7  6  5  4  3  2  1  0
	// for JP:
	// b1  15 16 17 18 19 20 21 22 ->      22 21 20 19 18 17 16 15
	// b2  L  8  9  10 11 12 13 14 ->      14 13 L  L  L  12 11 10
	// b3  0  1  2  3  4  5  6  7  ->      9  8  7  6  5  4  X  X
	// for LD:
	// b1  15 16 17 18 19 20 21 22 ->      22 21 20 19 18 17 16 15
	// b2  L  8  9  10 11 12 13 14 ->      14 13 12 11 10 9  8  7
	// b3  0  1  2  3  4  5  6  7  ->      6  5  X  X  3  2  1  0
	UINT8 byte1t;
	UINT16 byte23t;
        for (int i = 0; i < 0x600; i+= 3)
        {
			byte1t = BITSWAP8(dspsrc[0+i], 0, 1, 2, 3, 4, 5, 6, 7);
			// here's where things get disgusting: if the first byte was an OP or RT, do the following:
			if ((byte1t&0x80) == 0x00) // op or rt instruction
			{
				byte23t = BITSWAP16( (((UINT16)dspsrc[1+i]<<8)|dspsrc[2+i]), 8, 9, 10, 15, 11, 12, 13, 14, 0, 1, 2, 3, 4, 5, 6, 7);
			}
			else if ((byte1t&0xC0) == 0x80) // jp instruction
			{
				byte23t = BITSWAP16( (((UINT16)dspsrc[1+i]<<8)|dspsrc[2+i]), 8, 9, 15, 15, 15, 10, 11, 12, 13, 14, 0, 1, 2, 3, 6, 7);
			}
			else // ld instruction
			{
				byte23t = BITSWAP16( (((UINT16)dspsrc[1+i]<<8)|dspsrc[2+i]), 8, 9, 10, 11, 12, 13, 14, 0, 1, 2, 3, 3, 4, 5, 6, 7);
			}

            *dspprg = byte1t<<24 | byte23t<<8;
            dspprg++;
        }
}

/******************************************************************************
 Address Maps
******************************************************************************/
/* The address map of the prose 2020 is controlled by 2 proms, see the rom section
   for details on those.
   (x = ignored; * = selects address within this range; s = selects one of a pair of chips)
   A19 A18 A17 A16  A15 A14 A13 A12  A11 A10 A9 A8  A7 A6 A5 A4  A3 A2 A1 A0
     0   0   x   x    0   x   0   *    *   *  *  *   *  *  *  *   *  *  *  s  6264*2 SRAM first half
     0   0   x   x    0   x   1   0    *   *  *  *   *  *  *  *   *  *  *  s  6264*2 SRAM 3rd quarter
     0   0   x   x    0   x   1   1    0   0  0  x   x  x  x  x   x  x  *  x  iP8251A @ U15
     0   0   x   x    0   x   1   1    0   0  1  x   x  x  x  x   x  x  *  x  AMD P8259A PIC
     0   0   x   x    0   x   1   1    0   1  0  x   x  x  x  x   x  x  x  *  LEDS and dipswitches?
     0   0   x   x    0   x   1   1    0   1  1  x   x  x  x  x   x  x  *  x  UPD77P20 data/status
     0   0   x   x    0   x   1   1    1   x  x                               Open bus, verified (returns 0x00EA)
     0   0   x   x    1   x                                                   Open bus? (or maybe status?) (returns 0xFA,B,C,FFF)
     0   1                                                                    Open bus, verfiied (returns 0x00EA)
     1   0                                                                    Open bus, verified (returns 0x00EA)
     1   1   0   *    *   *   *   *    *   *  *  *   *  *  *  *   *  *  *  s  ROMs 2 and 3
     1   1   1   *    *   *   *   *    *   *  *  *   *  *  *  *   *  *  *  s  ROMs 0 and 1
*/
static ADDRESS_MAP_START(i8086_mem, ADDRESS_SPACE_PROGRAM, 16, tsispch_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x02FFF) AM_MIRROR(0x34000) AM_RAM // verified; 6264*2 sram, only first 3/4 used
	AM_RANGE(0x03000, 0x03001) AM_MIRROR(0x341FC) AM_DEVREADWRITE8_LEGACY("i8251a_u15", msm8251_data_r, msm8251_data_w, 0x00FF)
	AM_RANGE(0x03002, 0x03003) AM_MIRROR(0x341FC) AM_DEVREADWRITE8_LEGACY("i8251a_u15", msm8251_status_r, msm8251_control_w, 0x00FF)
	AM_RANGE(0x03200, 0x03203) AM_MIRROR(0x341FC) AM_DEVREADWRITE8_LEGACY("pic8259", pic8259_r, pic8259_w, 0x00FF) // AMD P8259 PIC @ U5 (reads as 04 and 7c, upper byte is open bus)
	AM_RANGE(0x03400, 0x03401) AM_MIRROR(0x341FE) AM_READ8(dsw_r, 0x00FF) // verified, read from dipswitch s4
	AM_RANGE(0x03400, 0x03401) AM_MIRROR(0x341FE) AM_WRITE8(led_w, 0xFF00) // verified, write to the 4 leds, plus 4 other mystery bits
	AM_RANGE(0x03600, 0x03601) AM_MIRROR(0x341FC) AM_READWRITE(dsp_data_r, dsp_data_w) // verified; UPD77P20 data reg r/w
	AM_RANGE(0x03602, 0x03603) AM_MIRROR(0x341FC) AM_READWRITE(dsp_status_r, dsp_status_w) // verified; UPD77P20 status reg r
	AM_RANGE(0xc0000, 0xfffff) AM_ROM // verified
ADDRESS_MAP_END

// Technically the IO line of the i8086 is completely ignored (it is running in 8086 MIN mode,I believe, which may ignore IO)
static ADDRESS_MAP_START(i8086_io, ADDRESS_SPACE_IO, 16, tsispch_state)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(dsp_prg_map, ADDRESS_SPACE_PROGRAM, 32, tsispch_state)
    AM_RANGE(0x0000, 0x01ff) AM_ROM AM_REGION("dspprg", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(dsp_data_map, ADDRESS_SPACE_DATA, 16, tsispch_state)
    AM_RANGE(0x0000, 0x01ff) AM_ROM AM_REGION("dspdata", 0)
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( prose2k )
PORT_START("s4") // dipswitch array s4
	PORT_DIPNAME( 0x01, 0x00, "S4-1") PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "S4-2") PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "S4-3") PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "S4-4") PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "S4-5") PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "S4-6") PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Self Test") PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "S4-8") PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static MACHINE_CONFIG_START( prose2k, tsispch_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8086, 4000000) /* TODO: correct clock speed? theres a 16mhz xtal... */
    MCFG_CPU_PROGRAM_MAP(i8086_mem)
    MCFG_CPU_IO_MAP(i8086_io)

    MCFG_CPU_ADD("dsp", UPD7725, 8000000) /* TODO: correct clock (may be correct, since theres a 24mhz xtal) and correct dsp type is UPD77P20 */
    MCFG_CPU_PROGRAM_MAP(dsp_prg_map)
    MCFG_CPU_DATA_MAP(dsp_data_map)

    /* PIC 8259 */
    MCFG_PIC8259_ADD("pic8259", pic8259_config)

    /* uarts */
    MCFG_MSM8251_ADD("i8251a_u15", msm8251_config)

    /* sound hardware */
    //MCFG_SPEAKER_STANDARD_MONO("mono")
    //MCFG_SOUND_ADD("dac", DAC, 0) /* TODO: correctly figure out how the DAC works; apparently it is connected to the serial output of the upd7720, which will be fun to connect up */
    //MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

    MCFG_FRAGMENT_ADD( generic_terminal )
    MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,tsispch_terminal_intf)
MACHINE_CONFIG_END

/******************************************************************************
 ROM Definitions
******************************************************************************/
ROM_START( prose2k )
	ROM_REGION(0x100000,"maincpu", 0)
	// prose 2000/2020 firmware version 3.4.1
	ROMX_LOAD( "v3.4.1__2000__2.u22",   0xc0000, 0x10000, CRC(201D3114) SHA1(549EF1AA28D5664D4198CBC1826B31020D6C4870),ROM_SKIP(1))
	ROMX_LOAD( "v3.4.1__2000__3.u45",   0xc0001, 0x10000, CRC(190C77B6) SHA1(2B90B3C227012F2085719E6283DA08AFB36F394F),ROM_SKIP(1))
	ROMX_LOAD( "v3.4.1__2000__0.u21",   0xe0000, 0x10000, CRC(3FAE874A) SHA1(E1D3E7BA309B29A9C3EDBE3D22BECF82EAE50A31),ROM_SKIP(1))
	ROMX_LOAD( "v3.4.1__2000__1.u44",   0xe0001, 0x10000, CRC(BDBB0785) SHA1(6512A8C2641E032EF6BB0889490D82F5D4399575),ROM_SKIP(1))

	// TSI/Speech plus DSP firmware v3.12 8/9/88, NEC UPD77P20
	ROM_REGION( 0x600, "dspprgload", 0) // packed 24 bit data
	ROM_LOAD( "v3.12__8-9-88__dsp_prog.u29", 0x0000, 0x0600, CRC(9E46425A) SHA1(80A915D731F5B6863AEEB448261149FF15E5B786))
	ROM_REGION( 0x800, "dspprg", ROMREGION_ERASEFF) // for unpacking 24 bit data into 32 bit data which cpu core can understand
	ROM_REGION( 0x400, "dspdata", 0)
	ROM_LOAD( "v3.12__8-9-88__dsp_data.u29", 0x0000, 0x0400, CRC(F4E4DD16) SHA1(6E184747DB2F26E45D0E02907105FF192E51BABA))

	// mapping proms:
	// All are am27s19 32x8 TriState PROMs (equivalent to 82s123/6331)
	// L - always low; H - always high
	// U77: unknown (what does this do?)
	//      input is A19 for I4, A18 for I3, A15 for I2, A13 for I1, A12 for I0
	//      output bits 0bLLLLzyxH
	//      bit - function
	//      7, 6, 5, 4 - seem unconnected?
	//      3 - connection unknown, low in the RAM, ROM, and Peripheral memory areas
	//      2 - connection unknown, low in the RAM and ROM memory areas
	//      1 - unknown, always high
	//      0 - unknown, always high
	//
	// U79: SRAM and peripheral mapping:
	//      input is A19 for I4, A18 for I3, A15 for I2, A13 for I1, A12 for I0, same as U77
	//      On the Prose 2000 board dumped, only bits 3 and 0 are used;
	//      bits 7-4 are always low, bits 2 and 1 are always high.
	//      SRAMS are only populated in U61 and U64.
	//      output bits 0bLLLLyHHx
	//      7,6,5,4 - seem unconnected?
	//      3 - to /EN3 (pin 4) of 74S138N at U80
	//          AND to EN1 (pin 6) of 74S138N at U78
	//          i.e. one is activated when pin is high and other when pin is low
	//          The 74S138N at U80:
	//              /EN2 - pulled to GND
	//              EN1 - pulled to VCC through resistor R5
	//              inputs: S0 - A9; S1 - A10; S2 - A11
	//              /Y0 - /CS (pin 11) of iP8251A at U15
	//              /Y1 - /CS (pin 1) of AMD 8259A at U4
	//              /Y2 - pins 1, 4, 9 (1A, 2A, 3A inputs) of 74HCT32 Quad OR gate at U58 <wip, involves LEDS?>
	//              /Y3 - pin 26 (/CS) of UPD77P20 at U29
	//              /Y4 thru /Y7 - seem unconnected SO FAR? <wip>
	//          The 74S138N at U78: <wip>
	//              /EN3 - ?
	//              /EN2 - ?
	//              inputs: S0 - A18; S1 - A19; S2 - Pulled to GND
	//              /Y0 - ?
	//              /Y1 - ?
	//              /Y2 - ?
	//              /Y3 - connects somewhere
	//              /Y4-/Y7 - never used since S2 is pulled to GND
	//      2 - to /CS1 on 6264 SRAMs at U63 and U66
	//      1 - to /CS1 on 6264 SRAMs at U62 and U65
	//      0 - to /CS1 on 6264 SRAMs at U61 and U64
	//
	// U81: maps ROMS: input is A19-A15 for I4,3,2,1,0
	//      On the Prose 2000 board dumped, only bits 6 and 5 are used,
	//      the rest are always high; maps roms 0,1,2,3 to C0000-FFFFF.
	//      The Prose 2000 board has empty unpopulated sockets for roms 4-15;
	//      if present these would be driven by a different prom in this location.
	//      bit - function
	//      7 - to /CE of ROMs 14(U28) and 15(U51)
	//      6 - to /CE of ROMs 0(U21) and 1(U44)
	//      5 - to /CE of ROMs 2(U22) and 3(U45)
	//      4 - to /CE of ROMs 4(U23) and 5(U46)
	//      3 - to /CE of ROMs 6(U24) and 7(U47)
	//      2 - to /CE of ROMs 8(U25) and 9(U48)
	//      1 - to /CE of ROMs 10(U26) and 11(U49)
	//      0 - to /CE of ROMs 12(U27) and 13(U50)
	ROM_REGION(0x1000, "proms", 0)
	ROM_LOAD( "am27s19.u77", 0x0000, 0x0020, CRC(A88757FC) SHA1(9066D6DBC009D7A126D75B8461CA464DDF134412))
	ROM_LOAD( "am27s19.u79", 0x0020, 0x0020, CRC(A165B090) SHA1(BFC413C79915C68906033741318C070AD5DD0F6B))
	ROM_LOAD( "am27s19.u81", 0x0040, 0x0020, CRC(62E1019B) SHA1(ACADE372EDB08FD0DCB1FA3AF806C22C47081880))
	ROM_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME	PARENT	COMPAT	MACHINE		INPUT	INIT	COMPANY     FULLNAME            FLAGS */
COMP( 1985, prose2k,	0,		0,		prose2k,		prose2k,	prose2k,	"Telesensory Systems Inc/Speech Plus",	"Prose 2000/2020",	GAME_NOT_WORKING | GAME_NO_SOUND )
