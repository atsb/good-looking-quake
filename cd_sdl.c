/*
	cd_sdl.c
	$Id: cd_sdl.c,v 1.19 2010/01/27 16:22:42 sezero Exp $

	Copyright (C) 1996-1997  Id Software, Inc.
	Taken from the Twilight project with modifications
	to make it work with Hexen II: Hammer of Thyrion.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		51 Franklin St, Fifth Floor,
		Boston, MA  02110-1301  USA
*/


#include <SDL2/SDL.h>

/* SDL dropped support for
   cd audio since v1.3.0 */
#include "cd_null.c"
