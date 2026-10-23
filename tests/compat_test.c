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

/* Compares the shim with recorded FAAD2 2.11.4 behavior: Init, Init2 and
 * AudioSpecificConfig results, every NeAACDecFrameInfo field of every frame, and
 * the first samples of each stream, which must line up with FAAD2's. The golden
 * files come from gen_golden.c. */
#include "compat_driver.h"
#include <math.h>

/* Differences from FAAD2 that are deliberate. Each is a prefix of a case name and
 * the field that is not compared. */
static const struct { const char *prefix, *field, *why; } allowed[] = {
    /* FAAD2 filled in only two channel positions for 7.1 ADTS; its raw path and
     * the shim report all eight. */
    {"surround71_lc.adts.", " pos", "7.1 ADTS channel positions"},
    /* FAAD2 failed the frames of the second rate with errors that depend on the bits
     * (see rate_change_test in shim_test.c for what the shim does). */
    {"ratechange_lc.", "frame", "sampling rate change mid-stream"},
};

/* Output that is later than FAAD2's by a known number of samples. */
static const struct { const char *prefix; long late; const char *why; } delayed[] = {
    /* Implicit SBR with a core above 24 kHz comes back at the core rate. FAAD2 needed
     * no lead for it; libfaar's 2:1 filter needs 147 input frames of lookahead. */
    {"he48_implicit.", 73, "decimation to the core rate"},
};

static long expected_late(const char *name)
{
    size_t i;
    for (i = 0; i < sizeof delayed / sizeof *delayed; i++)
        if (strncmp(name, delayed[i].prefix, strlen(delayed[i].prefix)) == 0) return delayed[i].late;
    return 0;
}

/* Minimum signal-to-noise ratio, in dB, against FAAD2's output, about 3.5 dB
 * under what each stream measures so that compilers can differ. Decoder
 * arithmetic differs, noise substitution draws different random numbers, and
 * HE-AAC regenerates its high band. A sample of lag drops these by 20 dB or
 * more. */
static const struct { const char *fixture; double min_snr; } floors[] = {
    {"stereo22_lc", 52}, {"stereo24_lc", 64}, {"mono24_lc", 31}, {"stereo32_he", 22},
    {"stereo44_lc", 32}, {"stereo48_lc", 67}, {"surround51_lc", 83}, {"surround71_lc", 52},
    {"drc_ref23", 98}, {"drc_noref", 60}, {"lc_mono32", 98}, {"lc_st48", 96}, {"he2_st48", 14},
    {"lc8k_mono", 33}, {"lc11k_st", 23}, {"lc16k_st", 19}, {"lc22k_st", 16}, {"ratechange_lc", 16}, {"he48_implicit", 21},
};

#define COMPAT_LAGS 128

static unsigned failures;
#define FAIL(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); failures++; } while (0)

static char *strip(const char *line, const char *field, char *buf, size_t cap)
{
    char *at;
    snprintf(buf, cap, "%s", line);
    at = strstr(buf, field);
    if (at) *at = '\0';
    return buf;
}

/* Lines are "<count> <text>"; a skipped field "frame" drops the frame lines. */
static size_t keep_lines(char **in, size_t n, const char *skip, char **out)
{
    size_t i, m = 0;
    for (i = 0; i < n; i++)
        if (!skip || strcmp(skip, "frame") != 0 || !strstr(in[i], " frame "))
            out[m++] = in[i];
    return m;
}

static const char *ignored_field(const char *name)
{
    size_t i;
    for (i = 0; i < sizeof allowed / sizeof *allowed; i++)
        if (strncmp(name, allowed[i].prefix, strlen(allowed[i].prefix)) == 0) return allowed[i].field;
    return NULL;
}

/* Lines of one case in the golden file, starting after its "case" line. */
static void check_case(const CompatCase *cs, const unsigned char *d, size_t sz, char **golden, size_t g_n,
                       const char *dir)
{
    CompatOut o;
    size_t i;
    const char *skip = ignored_field(cs->name);
    const long late = expected_late(cs->name);
    char *mine[1024], *theirs[1024];
    size_t mn, tn;
    memset(&o, 0, sizeof o);
    compat_exec(cs, d, sz, &o);
    mn = keep_lines(o.line, o.n < 1024 ? o.n : 1024, skip, mine);
    tn = keep_lines(golden, g_n < 1024 ? g_n : 1024, skip, theirs);
    if (mn != tn) FAIL("%s: %zu lines, FAAD2 had %zu", cs->name, mn, tn);
    for (i = 0; i < mn && i < tn; i++) {
        char a[600], b[600];
        const char *x = mine[i], *y = theirs[i];
        if (skip && strcmp(skip, "frame") != 0) x = strip(x, skip, a, sizeof a), y = strip(y, skip, b, sizeof b);
        if (strcmp(x, y) != 0) { FAIL("%s line %zu:\n  FAAD2 %s\n  shim  %s", cs->name, i, theirs[i], mine[i]); break; }
    }
    if (o.ref_n) {
        char path[1024];
        size_t n, k;
        int32_t start;
        short *ref, *file;
        double sig = 0, err = 0, snr;
        size_t j;
        snprintf(path, sizeof path, "%s/golden/%s%s.pcm", dir, compat_fixture_name(cs->fixture),
                 compat_seek_suffix[cs->seek]);
        file = (short *)compat_slurp(path, &n);
        if (!file || n < sizeof start) FAIL("%s: cannot read %s", cs->name, path);
        else {
            memcpy(&start, file, sizeof start);
            ref = file + sizeof start / sizeof(short);
            n = (n - sizeof start) / sizeof(short);
            if ((size_t)start + n + (size_t)late > o.ref_n) FAIL("%s: %zu samples, FAAD2 had at least %zu", cs->name, o.ref_n, (size_t)start + n);
            for (k = 0; k < n && (size_t)start + k + (size_t)late < o.ref_n; k++) {
                double e = (double)ref[k] - o.ref[(size_t)start + k + (size_t)late];
                sig += (double)ref[k] * ref[k];
                err += e * e;
            }
            snr = err > 0 ? 10 * log10(sig / err) : 200;
            /* The delay must be FAAD2's to the sample: the correlation of the window
             * peaks at lag 0. The signal-to-noise floors alone would let a one-sample
             * shift of a noisy stream pass. */
            if (sig > 0) {
                long lag, best = 0;
                double peak = -2;
                for (lag = -COMPAT_LAGS; lag <= COMPAT_LAGS; lag++) {
                    double xy = 0, yy = 0;
                    for (k = 0; k < n; k++) {
                        long at = (long)start + (long)k + lag;
                        double y = at >= 0 && (size_t)at < o.ref_n ? o.ref[at] : 0;
                        xy += (double)ref[k] * y;
                        yy += y * y;
                    }
                    if (yy > 0 && xy / sqrt(yy) > peak) peak = xy / sqrt(yy), best = lag;
                }
                if (best != late) FAIL("%s: output is %ld samples late against FAAD2, expected %ld", cs->name, best, late);
            }
            for (j = 0; j < sizeof floors / sizeof *floors; j++)
                if (strcmp(floors[j].fixture, compat_fixture_name(cs->fixture)) == 0 && snr < floors[j].min_snr)
                    FAIL("%s: SNR %.1f dB against FAAD2, expected at least %.0f (samples out of line?)",
                         cs->name, snr, floors[j].min_snr);
            if (getenv("COMPAT_VERBOSE")) fprintf(stderr, "%s snr %.1f\n", cs->name, snr);
            free(file);
        }
    }
    co_free(&o);
}

int main(int argc, char **argv)
{
    static CompatCase cases[512];
    size_t n = compat_cases(cases), g_n = 0, g_cap = 0, checked = 0;
    char path[1024], data_dir[1024], text[700], **glines = NULL;
    FILE *g;
    if (argc != 2) { fprintf(stderr, "usage: %s tests-directory\n", argv[0]); return 1; }
    snprintf(data_dir, sizeof data_dir, "%s/data", argv[1]);
    snprintf(path, sizeof path, "%s/golden/faad2.golden", argv[1]);
    g = fopen(path, "r");
    if (!g) { fprintf(stderr, "cannot read %s\n", path); return 1; }
    /* Split the golden file into cases and check each as it completes. */
    {
        char *name = NULL;
        size_t ci = 0;
        for (;;) {
            int more = fgets(text, sizeof text, g) != NULL;
            if (more) text[strcspn(text, "\n")] = '\0';
            if (!more || strncmp(text, "case ", 5) == 0) {
                if (name) {
                    for (ci = 0; ci < n && strcmp(cases[ci].name, name) != 0; ci++) ;
                    if (ci == n) FAIL("golden case %s is not known", name);
                    else {
                        size_t sz;
                        unsigned char *d;
                        compat_fixture_path(path, sizeof path, data_dir, cases[ci].fixture);
                        d = compat_slurp(path, &sz);
                        if (!d) FAIL("cannot read fixture for %s", name);
                        else { check_case(&cases[ci], d, sz, glines, g_n, argv[1]); checked++; free(d); }
                    }
                    for (ci = 0; ci < g_n; ci++) free(glines[ci]);
                    g_n = 0;
                    free(name);
                    name = NULL;
                }
                if (!more) break;
                name = strdup(text + 5);
            } else {
                if (g_n == g_cap) { g_cap = g_cap ? g_cap * 2 : 64; glines = realloc(glines, g_cap * sizeof *glines); }
                glines[g_n++] = strdup(text);
            }
        }
    }
    fclose(g);
    free(glines);
    if (checked != n) FAIL("checked %zu cases, expected %zu", checked, n);
    if (failures) fprintf(stderr, "%u compatibility checks failed\n", failures);
    else puts("All FAAD2 compatibility checks passed");
    return failures ? 1 : 0;
}
