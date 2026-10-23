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

/* Replays the same decoding procedures against whichever libfaad is linked.
 * gen_golden.c records what FAAD2 2.11.4 does; compat_test.c checks the shim. */
#include "neaacdec.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMPAT_REF_SAMPLES (1 << 17)  /* channel 0 kept per stream */
#define COMPAT_WINDOW 16384            /* samples stored in the golden files */

typedef struct {
    char **line;      /* run-length encoded: "<count> <text>" */
    size_t n, cap;
    short *ref;       /* channel 0, 16-bit, from the first frame */
    size_t ref_n;
    char last[512];
    size_t run;
} CompatOut;

static void co_flush(CompatOut *o)
{
    char buf[560];
    if (!o->run) return;
    snprintf(buf, sizeof buf, "%zu %s", o->run, o->last);
    if (o->n == o->cap) {
        o->cap = o->cap ? o->cap * 2 : 64;
        o->line = realloc(o->line, o->cap * sizeof *o->line);
    }
    o->line[o->n++] = strdup(buf);
    o->run = 0;
}

static void co_line(CompatOut *o, const char *text)
{
    if (o->run && strcmp(o->last, text) == 0) { o->run++; return; }
    co_flush(o);
    snprintf(o->last, sizeof o->last, "%s", text);
    o->run = 1;
}

static void co_free(CompatOut *o)
{
    size_t i;
    free(o->ref);
    for (i = 0; i < o->n; i++) free(o->line[i]);
    free(o->line);
    memset(o, 0, sizeof *o);
}

typedef struct { const unsigned char *p; size_t n; } CompatFrame;

/* kind: 0 ADTS via NeAACDecInit, 1 raw via NeAACDecInit2 (ASC from the ADTS header),
 * 2 DAB (ASC and length-prefixed frames in the file). seek: 0 none; 1 decode the
 * stream again after NeAACDecPostSeekReset(h, 0), whose first frame is silent; 2 the
 * same with NeAACDecPostSeekReset(h, -1), as a player seeks, so every frame has samples. */
static void compat_run(const unsigned char *d, size_t sz, int kind, int seek, int fmt, int downmix,
                       int dontup, CompatOut *o)
{
    NeAACDecHandle h = NeAACDecOpen();
    NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(h);
    unsigned long sr = 0;
    unsigned char ch = 0, asc[2];
    const unsigned char *asc_p = asc;
    size_t asc_n = 2, nf = 0, i, k;
    static CompatFrame fr[4096];
    mp4AudioSpecificConfig a;
    char text[512];
    int pass;
    long r;

    if (!o->ref) o->ref = malloc(COMPAT_REF_SAMPLES * sizeof(short));
    c->outputFormat = (unsigned char)fmt;
    c->downMatrix = (unsigned char)downmix;
    c->dontUpSampleImplicitSBR = (unsigned char)dontup;
    NeAACDecSetConfiguration(h, c);
    if (kind == 2) {
        size_t al = d[0] | d[1] << 8 | d[2] << 16 | (size_t)d[3] << 24, pos = 4 + al;
        asc_p = d + 4;
        asc_n = al;
        while (pos + 4 <= sz && nf < 4096) {
            size_t l = d[pos] | d[pos + 1] << 8 | d[pos + 2] << 16 | (size_t)d[pos + 3] << 24;
            fr[nf].p = d + pos + 4; fr[nf].n = l; nf++; pos += 4 + l;
        }
        r = NeAACDecInit2(h, (unsigned char *)asc_p, asc_n, &sr, &ch);
    } else {
        size_t off = 0, hdr = (d[1] & 1) ? 7 : 9;
        unsigned sfi = (d[2] >> 2) & 15, chn = ((d[2] & 1) << 2) | (d[3] >> 6);
        while (off + 7 <= sz && nf < 4096) {
            size_t l = ((d[off + 3] & 3) << 11) | (d[off + 4] << 3) | (d[off + 5] >> 5);
            if (!l || off + l > sz) break;
            fr[nf].p = d + off + (kind ? hdr : 0); fr[nf].n = l - (kind ? hdr : 0); nf++; off += l;
        }
        asc[0] = (unsigned char)((2 << 3) | (sfi >> 1));
        asc[1] = (unsigned char)(((sfi & 1) << 7) | (chn << 3));
        r = kind ? NeAACDecInit2(h, asc, 2, &sr, &ch) : NeAACDecInit(h, (unsigned char *)d, sz, &sr, &ch);
    }
    snprintf(text, sizeof text, "init ret %ld sr %lu ch %u", r, sr, ch);
    co_line(o, text);
    memset(&a, 0, sizeof a);
    snprintf(text, sizeof text, "asc ret %d", NeAACDecAudioSpecificConfig((unsigned char *)asc_p, asc_n, &a));
    co_line(o, text);
    snprintf(text, sizeof text, "asc ot %u sfi %u sf %lu cc %u sbr %d ps %d fl %u fus %d dss %d",
             a.objectTypeIndex, a.samplingFrequencyIndex, (unsigned long)a.samplingFrequency,
             a.channelsConfiguration, a.sbr_present_flag, a.ps_present_flag, a.frameLengthFlag,
             a.forceUpSampling, a.downSampledSBR);
    co_line(o, text);
    for (pass = 0; pass < (seek ? 2 : 1); pass++) {
        if (pass == 1) { NeAACDecPostSeekReset(h, seek == 2 ? -1 : 0); co_line(o, "seek"); o->ref_n = 0; }
        for (i = 0; i < nf; i++) {
            NeAACDecFrameInfo fi;
            void *p = NeAACDecDecode(h, &fi, (unsigned char *)fr[i].p, fr[i].n);
            int len = snprintf(text, sizeof text,
                "frame err %u samples %lu sr %lu ch %u sbr %u ps %u ot %u ht %u pos", fi.error,
                fi.samples, fi.samplerate, fi.channels, fi.sbr, fi.ps, fi.object_type, fi.header_type);
            for (k = 0; k < fi.channels && k < 8 && len < (int)sizeof text - 8; k++)
                len += snprintf(text + len, sizeof text - len, " %u", fi.channel_position[k]);
            co_line(o, text);
            if (p && fi.samples && fi.channels && fmt == FAAD_FMT_16BIT && !downmix && !dontup) {
                for (k = 0; k < fi.samples / fi.channels && o->ref_n < COMPAT_REF_SAMPLES; k++)
                    o->ref[o->ref_n++] = ((short *)p)[k * fi.channels];
            }
        }
    }
    co_flush(o);
    NeAACDecClose(h);
}

static const char *const compat_aac[] = {"stereo22_lc", "stereo24_lc", "mono24_lc", "stereo32_he",
    "stereo44_lc", "stereo48_lc", "surround51_lc", "surround71_lc", "drc_ref23", "drc_noref",
    "lc8k_mono", "lc11k_st", "lc16k_st", "lc22k_st", "ratechange_lc", "he48_implicit"};
static const char *const compat_dab[] = {"lc_mono32", "lc_st48", "he2_st48"};
#define COMPAT_AAC 16
#define COMPAT_FIXTURES 19

typedef struct { int fixture, kind, seek, variant; char name[96]; } CompatCase;
static const char *const compat_variant[] = {"s16", "flt", "dm", "dont"};
static const char *const compat_seek_suffix[] = {"", ".seek", ".seekc"};

/* Enumerate the cases in a fixed order. Returns the count. */
static size_t compat_cases(CompatCase *out)
{
    size_t n = 0;
    int f, ki, seek, v;
    for (f = 0; f < COMPAT_FIXTURES; f++) {
        int dab = f >= COMPAT_AAC, nk = dab ? 1 : 2, nv = dab ? 1 : 4;
        const char *nm = dab ? compat_dab[f - COMPAT_AAC] : compat_aac[f];
        for (ki = 0; ki < nk; ki++) for (seek = 0; seek < 3; seek++) for (v = 0; v < nv; v++) {
            if (seek && v) continue;
            out[n].fixture = f; out[n].kind = dab ? 2 : ki; out[n].seek = seek; out[n].variant = v;
            snprintf(out[n].name, sizeof out[n].name, "%s.%s.%s%s", nm,
                     dab ? "dab" : ki ? "raw" : "adts", compat_variant[v], compat_seek_suffix[seek]);
            n++;
        }
    }
    return n;
}

static void compat_exec(const CompatCase *cs, const unsigned char *d, size_t sz, CompatOut *o)
{
    int v = cs->variant;
    compat_run(d, sz, cs->kind, cs->seek, v == 1 ? FAAD_FMT_FLOAT : FAAD_FMT_16BIT, v == 2, v == 3, o);
}

static unsigned char *compat_slurp(const char *path, size_t *n)
{
    unsigned char *d;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); rewind(f);
    d = malloc(*n ? *n : 1);
    if (fread(d, 1, *n, f) != *n) { free(d); d = NULL; }
    fclose(f);
    return d;
}

static const char *compat_fixture_name(int f) { return f >= COMPAT_AAC ? compat_dab[f - COMPAT_AAC] : compat_aac[f]; }
static void compat_fixture_path(char *buf, size_t cap, const char *dir, int f)
{
    if (f >= COMPAT_AAC) snprintf(buf, cap, "%s/dab/%s.dab", dir, compat_dab[f - COMPAT_AAC]);
    else snprintf(buf, cap, "%s/%s.aac", dir, compat_aac[f]);
}
