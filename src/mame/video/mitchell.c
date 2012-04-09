/***************************************************************************

 Pang Video Hardware

***************************************************************************/

#include "emu.h"
#include "sound/okim6295.h"
#include "includes/mitchell.h"

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static TILE_GET_INFO( get_tile_info )
{
	mitchell_state *state = machine.driver_data<mitchell_state>();
	UINT8 attr = state->m_colorram[tile_index];
	int code = state->m_videoram[2 * tile_index] + (state->m_videoram[2 * tile_index + 1] << 8);
	SET_TILE_INFO(
			0,
			code,
			attr & 0x7f,
			(attr & 0x80) ? TILE_FLIPX : 0);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( pang )
{
	mitchell_state *state = machine.driver_data<mitchell_state>();

	state->m_bg_tilemap = tilemap_create(machine, get_tile_info, tilemap_scan_rows, 8, 8, 64, 32);
	state->m_bg_tilemap->set_transparent_pen(15);

	/* OBJ RAM */
	state->m_objram = auto_alloc_array_clear(machine, UINT8, state->m_videoram_size);

	/* Palette RAM */
	state->m_generic_paletteram_8.allocate(2 * machine.total_colors());

	state->save_pointer(NAME(state->m_objram), state->m_videoram_size);
}



/***************************************************************************

  Memory handlers

***************************************************************************/

/***************************************************************************
  OBJ / CHAR RAM HANDLERS (BANK 0 = CHAR, BANK 1=OBJ)
***************************************************************************/

WRITE8_MEMBER(mitchell_state::pang_video_bank_w)
{

	/* Bank handler (sets base pointers for video write) (doesn't apply to mgakuen) */
	m_video_bank = data;
}

WRITE8_MEMBER(mitchell_state::mstworld_video_bank_w)
{

	/* Monsters World seems to freak out if more bits are used.. */
	m_video_bank = data & 1;
}


WRITE8_MEMBER(mitchell_state::mgakuen_videoram_w)
{

	m_videoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset / 2);
}

READ8_MEMBER(mitchell_state::mgakuen_videoram_r)
{
	return m_videoram[offset];
}

WRITE8_MEMBER(mitchell_state::mgakuen_objram_w)
{
	m_objram[offset] = data;
}

READ8_MEMBER(mitchell_state::mgakuen_objram_r)
{
	return m_objram[offset];
}

WRITE8_MEMBER(mitchell_state::pang_videoram_w)
{

	if (m_video_bank)
		mgakuen_objram_w(space, offset, data);
	else
		mgakuen_videoram_w(space, offset, data);
}

READ8_MEMBER(mitchell_state::pang_videoram_r)
{

	if (m_video_bank)
		return mgakuen_objram_r(space, offset);
	else
		return mgakuen_videoram_r(space, offset);
}

/***************************************************************************
  COLOUR RAM
****************************************************************************/

WRITE8_MEMBER(mitchell_state::pang_colorram_w)
{

	m_colorram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}

READ8_MEMBER(mitchell_state::pang_colorram_r)
{
	return m_colorram[offset];
}

/***************************************************************************
  PALETTE HANDLERS (COLOURS: BANK 0 = 0x00-0x3f BANK 1=0x40-0xff)
****************************************************************************/

WRITE8_MEMBER(mitchell_state::pang_gfxctrl_w)
{

logerror("PC %04x: pang_gfxctrl_w %02x\n",cpu_get_pc(&space.device()),data);
{
#if 0
	char baf[40];
	sprintf(baf,"%02x",data);
	popmessage(baf);
#endif
}

	/* bit 0 is unknown (used, maybe back color enable?) */

	/* bit 1 is coin counter */
	coin_counter_w(space.machine(), 0, data & 2);

	/* bit 2 is flip screen */
	if (m_flipscreen != (data & 0x04))
	{
		m_flipscreen = data & 0x04;
		machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	}

	/* bit 3 is unknown (used, e.g. marukin pulses it on the title screen) */

	/* bit 4 selects OKI M6295 bank */
	if (m_oki != NULL)
		m_oki->set_bank_base((data & 0x10) ? 0x40000 : 0x00000);

	/* bit 5 is palette RAM bank selector (doesn't apply to mgakuen) */
	m_paletteram_bank = data & 0x20;

	/* bits 6 and 7 are unknown, used in several places. At first I thought */
	/* they were bg and sprites enable, but this screws up spang (screen flickers */
	/* every time you pop a bubble). However, not using them as enable bits screws */
	/* up marukin - you can see partially built up screens during attract mode. */
}

WRITE8_MEMBER(mitchell_state::pangbl_gfxctrl_w)
{

logerror("PC %04x: pang_gfxctrl_w %02x\n",cpu_get_pc(&space.device()),data);
{
#if 0
	char baf[40];
	sprintf(baf,"%02x",data);
	popmessage(baf);
#endif
}

	/* bit 0 is unknown (used, maybe back color enable?) */

	/* bit 1 is coin counter */
	coin_counter_w(machine(), 0, data & 2);

	/* bit 2 is flip screen */
	if (m_flipscreen != (data & 0x04))
	{
		m_flipscreen = data & 0x04;
		machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	}

	/* bit 3 is unknown (used, e.g. marukin pulses it on the title screen) */

	/* bit 4 selects OKI M6295 bank, nop'ed here */

	/* bit 5 is palette RAM bank selector (doesn't apply to mgakuen) */
	m_paletteram_bank = data & 0x20;

	/* bits 6 and 7 are unknown, used in several places. At first I thought */
	/* they were bg and sprites enable, but this screws up spang (screen flickers */
	/* every time you pop a bubble). However, not using them as enable bits screws */
	/* up marukin - you can see partially built up screens during attract mode. */
}

WRITE8_MEMBER(mitchell_state::mstworld_gfxctrl_w)
{

logerror("PC %04x: pang_gfxctrl_w %02x\n",cpu_get_pc(&space.device()),data);
{
	char baf[40];
	sprintf(baf,"%02x",data);
//  popmessage(baf);
}

	/* bit 0 is unknown (used, maybe back color enable?) */

	/* bit 1 is coin counter */
	coin_counter_w(machine(), 0, data & 2);

	/* bit 2 is flip screen */
	if (m_flipscreen != (data & 0x04))
	{
		m_flipscreen = data & 0x04;
		machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	}

	/* bit 3 is unknown (used, e.g. marukin pulses it on the title screen) */

	/* bit 4 here does not select OKI M6295 bank: mstworld has its own z80 + sound banking */

	/* bit 5 is palette RAM bank selector (doesn't apply to mgakuen) */
	m_paletteram_bank = data & 0x20;

	/* bits 6 and 7 are unknown, used in several places. At first I thought */
	/* they were bg and sprites enable, but this screws up spang (screen flickers */
	/* every time you pop a bubble). However, not using them as enable bits screws */
	/* up marukin - you can see partially built up screens during attract mode. */
}

WRITE8_MEMBER(mitchell_state::pang_paletteram_w)
{

	if (m_paletteram_bank)
		paletteram_xxxxRRRRGGGGBBBB_byte_le_w(space, offset + 0x800, data);
	else
		paletteram_xxxxRRRRGGGGBBBB_byte_le_w(space, offset, data);
}

READ8_MEMBER(mitchell_state::pang_paletteram_r)
{

	if (m_paletteram_bank)
		return m_generic_paletteram_8[offset + 0x800];

	return m_generic_paletteram_8[offset];
}

WRITE8_MEMBER(mitchell_state::mgakuen_paletteram_w)
{
	paletteram_xxxxRRRRGGGGBBBB_byte_le_w(space, offset, data);
}

READ8_MEMBER(mitchell_state::mgakuen_paletteram_r)
{
	return m_generic_paletteram_8[offset];
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	mitchell_state *state = machine.driver_data<mitchell_state>();
	int offs, sx, sy;

	/* the last entry is not a sprite, we skip it otherwise spang shows a bubble */
	/* moving diagonally across the screen */
	for (offs = 0x1000 - 0x40; offs >= 0; offs -= 0x20)
	{
		int code = state->m_objram[offs];
		int attr = state->m_objram[offs + 1];
		int color = attr & 0x0f;
		sx = state->m_objram[offs + 3] + ((attr & 0x10) << 4);
		sy = ((state->m_objram[offs + 2] + 8) & 0xff) - 8;
		code += (attr & 0xe0) << 3;
		if (state->m_flipscreen)
		{
			sx = 496 - sx;
			sy = 240 - sy;
		}
		drawgfx_transpen(bitmap,cliprect,machine.gfx[1],
				 code,
				 color,
				 state->m_flipscreen, state->m_flipscreen,
				 sx,sy,15);
	}
}

SCREEN_UPDATE_IND16( pang )
{
	mitchell_state *state = screen.machine().driver_data<mitchell_state>();

	bitmap.fill(0, cliprect);
	state->m_bg_tilemap->draw(bitmap, cliprect, 0, 0);
	draw_sprites(screen.machine(), bitmap, cliprect);
	return 0;
}
