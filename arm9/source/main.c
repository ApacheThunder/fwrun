// fwrun
// entrypoint

#include "main.h"

#include <nds.h>
#include <string.h>
#include <fat.h>

#include "fwpatch.h"
#include "loader_arm9.h"
#include "prefcompat.h"
#include "encryption.h"
#include "read_card.h"
#include "nds_card.h"
#include "tonccpy.h"

#ifdef EMBEDDED_FIRMWARE
#include "firmware_bin.h"
#else
#define NEED_FAT
#endif
#ifdef EMBEDDED_BIOS7
#include "bios7_bin.h"
#else
#define NEED_FAT
#endif

fwunpackParams params;
FILE* image;

void hang() {
	while(1) swiWaitForVBlank();
}

int fwRead() {

#ifndef EMBEDDED_BIOS7
	printf("reading bios7.bin...");
	FILE* bios7 = fopen("bios7.bin", "rb");
	if (!bios7)bios7 = fopen("nds/bios7.bin", "rb");
	if (!bios7) return 1;
	fseek(bios7, BIOS7_KEY1_OFFSET, SEEK_SET);
	params.key1data = malloc(KEY1_SIZE);
	fread(params.key1data, 1, KEY1_SIZE, bios7);
	fclose(bios7);
#else
	printf("using embedded bios7...");
	params.key1data = bios7_bin + BIOS7_KEY1_OFFSET;
#endif
	printf("got key1 (%X,%X)\n", params.key1data, swiCRC16(0xFFFF, params.key1data, KEY1_SIZE));

	printf("reading header...");
	image = fopen("firmware.bin", "rb");
	if (!image)image = fopen("/nds/firmware.bin", "rb");
	if (!image) return 1;
	/*fseek(fw_bin, 0, SEEK_END);
	size_t fw_size = ftell(fw_bin);
	fseek(fw_bin, 0, SEEK_SET);*/
	params.fwhdr = malloc(sizeof(fwHeader));
	fread(params.fwhdr, 1, sizeof(fwHeader), image);
	//fclose(fw_bin);
	printf("0x%06X\n", sizeof(fwHeader));

	return 0;
}


void InitGUI() {
	// Enable console
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	REG_BG0CNT_SUB = BG_MAP_BASE(4) | BG_COLOR_16 | BG_TILE_BASE(6) | BG_PRIORITY(0);
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 4, 6, false, true);
	// Start console with white on white for invisible text. ShowText function will undo this if halt occurs.
	// Previous text is preserved this way so that load process can be shown if error occurs.
	// Otherwise it will be a smooth transition into firmware boot. :D
	// Use Debug compile flag to always show text.
	BG_PALETTE_SUB[0] = RGB15(31,31,31);
	BG_PALETTE_SUB[255] = RGB15(31,31,31);
}

void ShowText() {
	BG_PALETTE_SUB[0] = RGB15(31,31,31);
	BG_PALETTE_SUB[255] = RGB15(0,0,0);
}

int main(void) {

	InitGUI();
	#ifdef DEBUGMODE
	ShowText();
	#endif

	params.isDsi = isDSiMode();
	params.hasCart = 0;
	
	printf("fwrun\n\n");
	memset(&params, sizeof params, 1);

	if (!fatInitDefault()) {
		ShowText();
		printf("fat init failure!\n");
		hang();
		return 1;
	}

	if (fwRead()) {
		ShowText();
		printf("error!\n");
		hang();
		return 1;
	}

	printf("1) begin unpacking\n");
	fwunpack_stage1();

	printf("2) patch bootcode\n");
	if (fwpatch()) {
		ShowText();
		printf("patching failed!\n");
		hang();
		return 1;
	}

	printf("3) finish unpacking\n");
	fwunpack_stage3();

	printf("4) patch preferences\n");
	patch_preferences();

	printf("5) pass control to firmware\n");
	
	consoleClear();
	
	if (!isDSiMode()) {
		ShowText();
		ALIGN(4) u32 ndsHeader[0x80];
		getHeader (ndsHeader);
		bool noCart = false;
		printf("Remove DS Card\nPress B to skip...");
		do {
			swiWaitForVBlank();
			scanKeys();
			if(keysDown() & KEY_B) { noCart = true; break; }
			getHeader (ndsHeader);
		} while (ndsHeader[0] != 0xFFFFFFFF);
		consoleClear();
		printf("Insert DS Card\nPress B to skip...");
		do {
			swiWaitForVBlank();
			scanKeys();
			if(keysDown() & KEY_B) { noCart = true; break; }
			getHeader (ndsHeader);
		} while (ndsHeader[0] == 0xFFFFFFFF);
		if (!noCart) {
			params.hasCart = 0x00000001;
			for(int i = 0; i < 30; i++)swiWaitForVBlank();
		}
		consoleClear();
	} else if (REG_SCFG_EXT & BIT(31)) {
		bool CartWasMissing = (REG_SCFG_MC == 0x11);
		if (!CartWasMissing) {
			ALIGN(4) sNDSHeaderExt* ndsHeaderExt = (sNDSHeaderExt*)malloc(sizeof(sNDSHeaderExt));
			if (REG_SCFG_MC == 0x10)enableSlot1();
			cardInit(ndsHeaderExt);
			params.hasCart = 0x00000001;
		}
	}
	
	
	loader_run();

	return 0;
}

