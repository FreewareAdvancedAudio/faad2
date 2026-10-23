/*
** Copyright (C) 2026 Nils Schimmelmann
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* The generated legacy header <faad.h> must still compile, carry the version and
 * bring in the NeAACDec API. */
#include <faad.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    NeAACDecHandle h;

    if (strlen(FAAD2_VERSION) == 0 || FAAD2_VERSION[0] < '0' || FAAD2_VERSION[0] > '9') {
        fprintf(stderr, "FAAD2_VERSION is \"%s\"\n", FAAD2_VERSION);
        return 1;
    }
    if (!(NeAACDecGetCapabilities() & LC_DEC_CAP))
        return 1;
    h = NeAACDecOpen();
    if (!h)
        return 1;
    NeAACDecClose(h);
    puts("The legacy faad.h works");
    return 0;
}
