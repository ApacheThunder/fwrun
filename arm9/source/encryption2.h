/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ENCRYPTION2_H
#define ENCRYPTION2_H

#include <nds/ndstypes.h>
void init_keycode2 (u32 idcode, u32 level, u32 modulo, int iCardDevice);
void crypt_64bit_down2 (u32* ptr);
void crypt_64bit_up2 (u32* ptr);

#endif

