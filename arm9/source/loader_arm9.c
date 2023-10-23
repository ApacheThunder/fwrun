/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2010
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
------------------------------------------------------------------*/

// nds and dldi-specific stuff removed by nocebo

#define FWRUN_LOADER

#include <string.h>
#include <nds.h>
#include <nds/memory.h>
#include <nds/arm9/video.h>
#include <sys/stat.h>
#include <limits.h>

#include "main.h"
#include "fwparams.h"

#include "load_bin.h"

#include "loader_arm9.h"
#define LCDC_BANK_D (vu16*)0x06860000

#define BOOTLOADER_ARM7LOCATION 0x06020000

typedef struct {
    u32 __branch;
    fwunpackParams* params;
} strap7Args;

static void vramcpy(volatile void* dst, const void* src, int len) {
	u16* dst16 = (u16*)dst;
	u16* src16 = (u16*)src;
	for ( ; len > 0; len -= 2) { *dst16++ = *src16++; }
}	

void loader_run() {

	if (isDSiMode() && (REG_SCFG_EXT & BIT(31))) {
		REG_SCFG_CLK = 0x80;
		REG_SCFG_EXT = 0x83040000;
	}

	// Direct CPU access to VRAM bank D
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;
	// Load the loader/patcher into the correct address
	vramcpy(LCDC_BANK_D, load_bin, load_bin_size);

    // put strap7 args in place
    strap7Args* args = ((strap7Args*)LCDC_BANK_D);
    args->params = &params;

    nocashMessage("irqDisable()");
	irqDisable(IRQ_ALL);

	VRAM_D_CR = VRAM_ENABLE | VRAM_D_ARM7_0x06020000;
	
	REG_EXMEMCNT |= ARM7_OWNS_ROM | ARM7_OWNS_CARD;
	*((vu32*)0x02FFFFFC) = 0;
	*((vu32*)0x02FFFE04) = (u32)0xE59FF018;
	*((vu32*)0x02FFFE24) = (u32)0x02FFFE04;

    nocashMessage("resetARM7()");
    resetARM7(BOOTLOADER_ARM7LOCATION);

    nocashMessage("swiSoftReset()");
	swiSoftReset();

}