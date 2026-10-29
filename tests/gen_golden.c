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

/* Records what FAAD2 2.11.4 does for every case in compat_driver.h.
 * Build against the original libfaad (commit 0fb0973), not the shim:
 *   cc gen_golden.c -I<faad2-2.11.4>/include -L<build> -lfaad -lm -o gen_golden
 *   gen_golden data-directory golden-directory
 * Needs only to be rerun when the cases change. */
#include "compat_driver.h"

int main(int argc, char **argv)
{
    static CompatCase cases[512];
    size_t n = compat_cases(cases), i, j;
    char path[1024];
    FILE *g;
    if (argc != 3) { fprintf(stderr, "usage: %s data-dir golden-dir\n", argv[0]); return 1; }
    snprintf(path, sizeof path, "%s/faad2.golden", argv[2]);
    g = fopen(path, "w");
    if (!g) return 1;
    for (i = 0; i < n; i++) {
        size_t sz;
        CompatOut o;
        unsigned char *d;
        compat_fixture_path(path, sizeof path, argv[1], cases[i].fixture);
        d = compat_slurp(path, &sz);
        if (!d) { fprintf(stderr, "cannot read %s\n", path); return 1; }
        memset(&o, 0, sizeof o);
        compat_exec(&cases[i], d, sz, &o);
        fprintf(g, "case %s\n", cases[i].name);
        for (j = 0; j < o.n; j++) fprintf(g, "%s\n", o.line[j]);
        if (o.ref_n) {
            /* A window that opens shortly before the signal does, so a stream that
             * lags or leads is out of line with it. Stored as the start index (int32)
             * followed by the samples. */
            FILE *p;
            size_t first = 0, start, len;
            int32_t start32;
            while (first < o.ref_n && !o.ref[first]) first++;
            start = first > 1024 ? first - 1024 : 0;
            /* FAAD2 carries decoder state across a seek, so its first samples after one
             * are not reproducible; the shim starts clean (see seek_state_test). */
            if (cases[i].seek == 2 && start < o.ref_n / 2) start = o.ref_n / 2 < 8192 ? o.ref_n / 2 : 8192;
            len = o.ref_n - start < COMPAT_WINDOW ? o.ref_n - start : COMPAT_WINDOW;
            start32 = (int32_t)start;
            snprintf(path, sizeof path, "%s/%s%s.pcm", argv[2], compat_fixture_name(cases[i].fixture),
                     compat_seek_suffix[cases[i].seek]);
            p = fopen(path, "wb");
            fwrite(&start32, sizeof start32, 1, p);
            fwrite(o.ref + start, sizeof(short), len, p);
            fclose(p);
        }
        co_free(&o);
        free(d);
    }
    fclose(g);
    return 0;
}
