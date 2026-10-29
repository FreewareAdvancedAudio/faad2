#!/usr/bin/env python3
# Copyright (C) 2026 Nils Schimmelmann
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
"""Rebuilds the AAC fixtures that FAAC wrote: lc8k_mono, lc11k_st, lc16k_st,
lc22k_st, ratechange_lc and he48_implicit in tests/data.

    gen_fixtures.py path/to/faac output-directory

Needs numpy and the faac tool of FAAC 4f68217 (the fixtures were written with
that version; other versions encode differently, and the golden files only
describe the committed streams). The other fixtures in tests/data came with
the first version of the shim's regression tests.
"""
import subprocess
import sys
import wave

import numpy as np


def lc_signal(rng, rate, channels, seconds):
    """Tone, vibrato tone, a high tone and amplitude-modulated noise, plus a
    little independent noise per extra channel."""
    n = int(rate * seconds)
    t = np.arange(n) / rate
    base = (0.35 * np.sin(2 * np.pi * 440 * t)
            + 0.2 * np.sin(2 * np.pi * 1250 * t * (1 + 0.1 * np.sin(2 * np.pi * 3 * t)))
            + 0.15 * np.sin(2 * np.pi * min(3300, rate * 0.3) * t))
    base += 0.05 * rng.standard_normal(n) * (0.5 + 0.5 * np.sin(2 * np.pi * 5 * t))
    chans = [base * (1 if c == 0 else 0.8) + 0.03 * rng.standard_normal(n) * c
             for c in range(channels)]
    return np.clip(np.stack(chans, 1), -1, 1)


def he_signal(rng, rate, seconds):
    n = int(rate * seconds)
    t = np.arange(n) / rate
    x = (0.3 * np.sin(2 * np.pi * 440 * t) + 0.2 * np.sin(2 * np.pi * 5000 * t)
         + 0.1 * np.sin(2 * np.pi * 21000 * t) + 0.03 * rng.standard_normal(n))
    return np.stack([x, x * 0.9], 1).clip(-1, 1)


def save_wav(path, rate, samples):
    with wave.open(path, "wb") as w:
        w.setnchannels(samples.shape[1])
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes((samples * 30000).astype("<i2").tobytes())


def encode(faac, wav, aac, *options):
    subprocess.run([faac, "--rate", "keep", "-a", *options, "-o", aac, wav],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    faac, out = sys.argv[1], sys.argv[2]

    rng = np.random.default_rng(7)
    for name, rate, channels, seconds in (("lc8k_mono", 8000, 1, 1.2), ("lc11k_st", 11025, 2, 1.2),
                                          ("lc16k_st", 16000, 2, 1.2), ("lc22k_st", 22050, 2, 0.8)):
        save_wav(f"{out}/{name}.wav", rate, lc_signal(rng, rate, channels, seconds))
        encode(faac, f"{out}/{name}.wav", f"{out}/{name}.aac", "-b", "32")

    # A stream that changes sampling rate part-way through.
    with open(f"{out}/ratechange_lc.aac", "wb") as joined:
        for name in ("lc22k_st", "lc16k_st"):
            joined.write(open(f"{out}/{name}.aac", "rb").read())

    # HE-AAC whose ADTS header claims an AAC-LC core at 48 kHz (implicit SBR
    # with a core above 24 kHz).
    save_wav(f"{out}/he48.wav", 96000, he_signal(np.random.default_rng(3), 96000, 1.0))
    encode(faac, f"{out}/he48.wav", f"{out}/he48_implicit.aac",
           "--object-type", "he-aac-v1", "-b", "64")


if __name__ == "__main__":
    main()
