#****************************************************************************
#
#    Copyright (C) 2025 David Latham
#
#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
#    USA
#
# https://github.com/linuxplayground/z80-breakout
#
#****************************************************************************
TOP=.

all:: retro nouveau nabu

retro:
	make -C arch/RETRO

nouveau:
	make -C arch/NOUVEAU

nabu:
	make -C arch/NABU

clean::
	make -C arch/RETRO clean
	make -C arch/NOUVEAU clean
	make -C arch/NABU clean

world: clean all

copy: build/arch/RETRO/breakout.com
	cpmrm -f z80-retro-8k-8m /dev/loop0p1 0:breakout.com
	cpmcp -f z80-retro-8k-8m /dev/loop0p1 build/arch/RETRO/breakout.com 0:

