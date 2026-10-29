# FAAD2 compatibility library

`libfaad.so.2` and the legacy `NeAACDec*` API, implemented as a thin shim over
[FAAD3](https://github.com/FreewareAdvancedAudio/faac). Applications that link
FAAD2 keep working without being rebuilt; the decoding is done by libfaad3, and
libfaar resamples where FAAD2 did.

This repository only builds the shim. The decoder (libfaad3), the resampler
(libfaar) and the `faad` command-line tool live in the
[FAAC repository](https://github.com/FreewareAdvancedAudio/faac).

## Do you need this?

- **Your application calls `NeAACDec*` and you cannot change it yet:** yes. Install
  the shim and keep `<neaacdec.h>` and `-lfaad`.
- **You are writing new code:** no. Use libfaad3 directly (`<faad3.h>`,
  `pkg-config --cflags --libs faad3`). See [Migrating](#migrating-to-libfaad3).
- **You need MAIN, LTP, SSR, LD, error-resilient AAC, DRM, LATM, ADIF or a
  fixed-point build:** this is not the library. See [Limits](#limits).

## Building

The shim builds against installed libfaad3 and libfaar (for example the FAAC
development packages):

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

Link with `pkg-config --cflags --libs faad2`. The build keeps the `faad3` and
`faar` pkg-config dependencies, and `-DBUILD_SHARED_LIBS=OFF` builds a static
library; static consumers get the extra link flags from
`pkg-config --libs --static faad2`.

| Need | How |
|---|---|
| Compile libfaad3 and libfaar into the shim | `git submodule update --init external/faac`, then `-DFAAD3_USE_BUNDLED_SOURCE=ON` |
| Use targets from a parent CMake project | `-DFAAD3_CMAKE_TARGET=name -DFAAR_CMAKE_TARGET=name` |
| Bazel (Bazel 7; WORKSPACE files are not supported by Bazel 9) | `git submodule update --init external/faac`, then `bazel build //:faad` |
| Run the tests | `-DFAAD_BUILD_TESTS=ON` (plus `-DFAAD3_USE_BUNDLED_SOURCE=ON` if libfaad3 is not installed), then `ctest --test-dir build` |

Bazel builds libfaar as `@faac//:faar`, with the same double-precision filter kernels
as Meson's default. The historic library variant labels are kept for downstream build
files; they all use the same shim and do not enable fixed-point decoding. The fuzz
targets use the workspace's Python 3.11 toolchain, and the workspace setup also needs
Python 3.11 on the host for the pinned `rules_fuzzing` wheel installer.

## What it decodes

AAC-LC, HE-AAC v1 (SBR), HE-AAC v2 (PS) and DAB+, as ADTS or as raw access units
with an AudioSpecificConfig (ASC).

### Limits

- Not supported: MAIN, LTP, SSR, LD and error-resilient profiles, DRM, LATM, ADIF,
  fixed-point decoding, and raw AAC without an ASC.
- `NeAACDecInitDRM` is exported by every build, not only the DRM one, but always fails.
- `useOldADTSFormat` is ignored, and `defObjectType` and `defSampleRate` do not
  select stream parameters.
- `NeAACDecGetCapabilities` reports only `LC_DEC_CAP` (FAAD2 also reported MAIN, LTP,
  LD and error resilience), so callers that fall back to another decoder when a
  capability is missing now do.
- FAAD3 decodes in floating point, so there is no equivalent of FAAD2's fixed-point
  build for processors without an FPU. Such targets can use libhelix-aac, a
  fixed-point decoder for AAC-LC and HE-AAC v1 from RealNetworks' Helix project. It has
  its own API and license, and lacks parametric stereo, 960-sample frames and the MAIN
  and LTP profiles. It is packaged for Arduino and ESP boards as
  [codec-helix](https://github.com/pschatzmann/codec-helix) and as
  [ESP8266Audio's libhelix-aac](https://github.com/earlephilhower/ESP8266Audio/tree/master/src/libhelix-aac).

## Behaviour compared with FAAD2

### Kept

- The first decoded frame reports no samples.
- FAAD2 channel order and positions, the 5/6-channel downmix, the legacy error-number
  table, and the ADTS sync search of up to 768 bytes.
- Output format and `downMatrix` take effect when the decoder initializes or after
  `NeAACDecPostSeekReset`. `dontUpSampleImplicitSBR` takes effect at `NeAACDecInit` or
  `NeAACDecInit2` only.
- Dynamic range control uses full compression. Streams with a programme reference level
  are normalized to -20 dBFS. A stream with `dynamic_range_info` but no reference level
  plays 20 dB quieter, as in FAAD2.

### Different

| Area | Shim behaviour |
|---|---|
| AAC-LC without SBR data at or below 24 kHz | Upsampled to twice the rate, as FAAD2 did when it assumed implicit SBR, and reported as `NO_SBR_UPSAMPLED`. libfaar does the resampling, so the samples differ from FAAD2's QMF upsampling, but the output lags by 962 samples per channel, as FAAD2's did. `dontUpSampleImplicitSBR` turns this off. A stream that carries SBR data is returned as before |
| Implicit SBR with an AAC-LC core above 24 kHz (HE-AAC without signalling, for example a 48 kHz core) | The core rate, as FAAD2 returned by default (`SBR_DOWNSAMPLED`), by decimating libfaad3's full-rate output with libfaar. libfaar's 2:1 filter needs 147 input frames of lookahead, so the output is 73 samples later than FAAD2's. Once aligned, the signal matches FAAD2 to about 24 dB. Streams at or below 24 kHz, the usual HE-AAC, are unaffected. `dontUpSampleImplicitSBR` returns the full SBR rate for ADTS streams; raw streams are decimated either way, as in FAAD2 |
| Sampling-rate change in the middle of an ADTS stream | FAAD2 configured itself from the first header and failed every frame written for the other rate, with errors that depend on the bits (32 was the usual one). The shim returns error 32 and no samples for those frames and decodes the others |
| 7.1 in an ADTS stream | `channel_position` lists all eight positions; FAAD2 filled in only two. Initialization reports the legacy channel configuration value (two for ADTS, seven for raw/MP4), while decoded frames report eight channels |
| Corrupt or false-sync ADTS frames | Use the legacy error table but may report a different error than FAAD2 |
| 32-bit output | Clamps at positive full scale instead of wrapping |
| After `NeAACDecPostSeekReset` | Decodes exactly as a new decoder would. FAAD2 carried the overlap and filter history across the seek, so its first frames afterwards differ |

### Sample accuracy

Samples are not bit-identical to FAAD2's. Signal-to-noise ratio against FAAD2 on the
test streams:

| Streams | SNR |
|---|---|
| AAC-LC at 32 kHz and above | 59 to 105 dB |
| Upsampled streams (the interpolation filters differ) | 21 to 68 dB |
| HE-AAC (the high band is regenerated) | 14 to 28 dB |
| Stereo written by FAAC with noise substitution (PNS) | 20 to 45 dB |

PNS streams score lower because FAAD3 draws different random numbers, so the
noise-filled bands are uncorrelated while the rest of the spectrum agrees closely;
levels agree to within 0.1 dB.

## Tests

Enable them with `-DFAAD_BUILD_TESTS=ON` and run `ctest --test-dir build`. There are
three suites: `shim_test` (API regression), `compat_test` and `faad_header_test`.

`tests/compat_test.c` replays 19 fixtures (16 AAC-LC and HE-AAC streams and 3 DAB+
streams) through the shim, in ADTS and raw form, with each output format, downmix,
`dontUpSampleImplicitSBR` and two kinds of seek. It compares them with what FAAD2
2.11.4 did, recorded in `tests/golden`:

- The results of `NeAACDecInit`, `NeAACDecInit2` and `NeAACDecAudioSpecificConfig`, and
  every field of `NeAACDecFrameInfo`, must match apart from the differences listed in
  the test.
- The first samples of each stream must line up with FAAD2's to the sample, so the delay
  of HE-AAC, DAB and upsampled streams is the same (implicit SBR above 24 kHz is exactly
  73 samples later, as above).
- Each stream must stay within a signal-to-noise ratio set for it.

The golden files only change when the cases do. To regenerate them, build the original
library from commit `0fb0973` and run the recorder against it. The committed recording
was made on x86_64 Linux:

```sh
git worktree add --detach /tmp/faad2-2.11.4 0fb0973
cmake -S /tmp/faad2-2.11.4 -B /tmp/faad2-2.11.4/build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/faad2-2.11.4/build -j
cc tests/gen_golden.c -Itests -I/tmp/faad2-2.11.4/include -L/tmp/faad2-2.11.4/build \
    -lfaad -lm -Wl,-rpath,/tmp/faad2-2.11.4/build -o gen_golden
./gen_golden tests/data tests/golden
```

Six of the fixtures were written with FAAC 4f68217 (`faac --rate keep -a`);
`tests/gen_fixtures.py path/to/faac tests/data` rebuilds them byte for byte from
synthesized audio. The others came with the first version of the shim's regression
tests.

## Migrating to libfaad3

Existing applications can keep using `<neaacdec.h>` and link `libfaad.so.2`. The legacy
`<faad.h>` is still installed; it defines `FAAD2_VERSION` and includes `<neaacdec.h>`,
and its compile-time notice points to libfaad3.

For new code, include `<faad3.h>` and link `libfaad3`. The calling sequence is
`faad_config_init()`, `faad_decoder_open()` and `faad_decode_frame()`. Provide the ASC
when opening a raw stream, and use the returned status, format-change flags and stream
info in place of the legacy error and frame structures.

The [FAAD3 API and migration guide](https://github.com/FreewareAdvancedAudio/faac/blob/master/docs/libfaad.md)
has the function mappings, examples and changed behaviour. The `libfaad.3` suffix of the
legacy manual in `docs/` names manual section 3, not FAAD version 3.

## Repository layout

| Path | Contents |
|---|---|
| `libfaad/shim.c` | The legacy API implemented on FAAD3 and FAAR |
| `include` | Public FAAD2 compatibility headers |
| `docs` | Legacy API manual and compatibility notes |
| `tests` | API regression tests, the FAAD2 comparison (`compat_test.c` and the recording in `golden`) and AAC fixtures |
| `fuzz` | Decoder API fuzzing entry points |
| `external/faac` | FAAC source submodule (libfaad3 and libfaar) for vendored builds |
| `bazel` | BUILD file for the FAAC sources |

## License

The FAAD2 code in this project is under the GNU General Public License, version 2 or
later:

```
FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

The "appropriate copyright message" mentioned in section 2c of the GPLv2
must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"

Commercial non-GPL licensing of FAAD2 code may be available from Nero AG.
```

The shim (`libfaad/shim.c`) and the tests are Copyright (C) 2026 Nils Schimmelmann,
under the GNU General Public License, version 2 or later. The error message table and
downmix coefficients in `libfaad/shim.c` come from FAAD2 and keep Nero's copyright and
the message above.

libfaad3 and libfaar are separate dependencies under the GNU Lesser General Public
License, version 2.1; see their source and copyright notices when building or
distributing the combined library.

Use of AAC software may require payment of patent royalties. You are responsible for
determining and meeting any obligations that apply to your use; this project provides
no patent warranty or indemnity.
