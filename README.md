# Freeware Advanced Audio (AAC) Decoder including SBR decoding

This repository provides the legacy FAAD2 `NeAACDec*` API and `libfaad.so.2`
ABI as a compatibility shim over FAAD3. It builds the library; the decoder
(libfaad3), the resampler (libfaar) and the `faad` command-line tool are
maintained in the [FAAC repository](https://github.com/FreewareAdvancedAudio/faac).

The original FAAD2 decoder supported AAC-LC, HE-AAC, MAIN and LTP profiles.
This shim keeps the legacy interface for existing applications while using
FAAD3 to decode the supported AAC-LC and HE-AAC streams. It retains FAAD2
channel order and positions for multichannel output.

### Key features

- Legacy `NeAACDec*` API and `libfaad.so.2` ABI
- AAC-LC, HE-AAC v1 (SBR), and HE-AAC v2 (PS) decoding through FAAD3
- Streams FAAD2 resampled keep their rate and delay: low-rate AAC-LC is upsampled
  to twice its rate and implicit SBR above 24 kHz comes back at the core rate,
  both with libfaar
- Compared with FAAD2 2.11.4 stream by stream in the tests: results of every
  call, delay to the sample, and signal-to-noise ratio
- CMake and Bazel library builds
- Existing applications can keep their FAAD2 interface while migrating to
  FAAD3 at their own pace

## Project migration

The original FAAD2 decoder sources and command-line frontend are no longer in
this repository. `libfaad/shim.c` implements the compatibility API; new decoder
work and the `faad` command-line tool live in the FAAC repository. The API
porting guide is linked below under Migrating applications.

## Copyrights

For the FAAD2 code in this project, the following license applies:

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

The shim (`libfaad/shim.c`) and the tests are Copyright (C) 2026 Nils
Schimmelmann, under the GNU General Public License, version 2 or later. The
error message table and downmix coefficients in `libfaad/shim.c` come from
FAAD2 and keep Nero's copyright and the message above.

libfaad3 and libfaar are separate dependencies under the GNU Lesser General
Public License, version 2.1; see their source and copyright notices when
building or distributing the combined library.

Use of AAC software may require payment of patent royalties. You are
responsible for determining and meeting any obligations that apply to your
use; this project provides no patent warranty or indemnity.

## Directory structure

- `docs` — legacy API manual and compatibility notes
- `include` — public FAAD2 compatibility headers
- `libfaad/shim.c` — implementation of the legacy API on FAAD3 and FAAR
- `external/faac` — FAAC source submodule (libfaad3 and libfaar) for vendored builds
- `bazel` — BUILD file for the FAAC sources
- `tests` — API regression tests, the comparison with FAAD2 (`compat_test.c` and
  the recording in `golden`) and AAC fixtures
- `fuzz` — decoder API fuzzing entry points

## Building

For Debian and other package builds, install the FAAC development files (libfaad3
and libfaar) and configure with CMake:

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

The default build links to the installed FAAD3 and FAAR libraries and retains
their `faad3` and `faar` pkg-config dependencies. Use `-DBUILD_SHARED_LIBS=OFF`
for a static library. Static consumers also need FAAD3 and FAAR;
`pkg-config --libs --static faad2` supplies the required link flags.

For a source-vendored build, initialize the pinned FAAC submodule and enable
`-DFAAD3_USE_BUNDLED_SOURCE=ON` to compile FAAD3 and FAAR into the FAAD2
library. A parent project can instead provide its own CMake targets with
`-DFAAD3_CMAKE_TARGET=target_name` and `-DFAAR_CMAKE_TARGET=target_name`.

```sh
git submodule update --init external/faac
```

For Bazel builds (Bazel 7; the WORKSPACE files are not supported by Bazel 9),
initialize the FAAC submodule before building:

```sh
git submodule update --init external/faac
bazel build //:faad
```

Bazel builds libfaar as `@faac//:faar`, with the same double-precision filter
kernels as Meson's default.

The repository retains the historic library variant labels for downstream
build files; they use the same shim implementation. They do not enable
fixed-point decoding. Fuzz targets use the workspace's Python 3.11 toolchain;
workspace dependency setup also requires Python 3.11 on the host for the
pinned `rules_fuzzing` wheel installer.

## Compatibility and limitations

The shim preserves selected FAAD2 behaviors, including the first decoded
frame reporting no samples, channel order and positions, the 5/6-channel
downmix, the legacy error-number table, and ADTS sync search up to 768 bytes.
The configured output format and `downMatrix` setting take effect when the
decoder initializes, or after `NeAACDecPostSeekReset`. `dontUpSampleImplicitSBR`
takes effect at `NeAACDecInit` or `NeAACDecInit2` only.

Dynamic range control uses full compression. Streams carrying a programme
reference level are normalized to -20 dBFS. A stream with `dynamic_range_info`
but no programme reference level is played 20 dB quieter, matching FAAD2's
behavior.

The shim does not support MAIN/LTP/SSR/LD/ER profiles, DRM, fixed-point
decoding, LATM, ADIF, or raw AAC without an AudioSpecificConfig (ASC).
`NeAACDecInitDRM` remains exported but always fails. The
`useOldADTSFormat` field is ignored; `defObjectType` and `defSampleRate` do
not select stream parameters.

FAAD3 decodes in floating point, so this library has no equivalent of FAAD2's
fixed-point build for processors without an FPU. Such targets can use
libhelix-aac, a fixed-point decoder for AAC-LC and HE-AAC v1 (SBR) from
RealNetworks' Helix project, which has its own API and license and lacks
parametric stereo, 960-sample frames and the MAIN and LTP profiles. It is
packaged for Arduino and ESP boards as
[codec-helix](https://github.com/pschatzmann/codec-helix) and as
[ESP8266Audio's libhelix-aac](https://github.com/earlephilhower/ESP8266Audio/tree/master/src/libhelix-aac).

Known differences from FAAD2 include:

- AAC-LC streams without SBR data at or below 24 kHz are upsampled to twice
  their rate, as FAAD2 did when it assumed implicit SBR, and report
  `NO_SBR_UPSAMPLED`. libfaar does the resampling, so the samples differ from
  FAAD2's QMF upsampling, but the output lags by 962 samples per channel, as
  FAAD2's did. `dontUpSampleImplicitSBR` turns this off. A stream that does
  carry SBR data is returned as before.
- A change of sampling rate in the middle of an ADTS stream: FAAD2 configured
  itself from the first header and failed every frame written for the other rate,
  with errors that depend on the bits (32 was the usual one). The shim returns
  error 32 and no samples for those frames and decodes the others.
- Implicit SBR with an AAC-LC core above 24 kHz (HE-AAC written without
  signalling, for example a 48 kHz core): FAAD2 returned the core rate by
  default (`SBR_DOWNSAMPLED`). The shim now does too, decimating libfaad3's
  full-rate output with libfaar. libfaar's 2:1 filter needs 147 input frames of
  lookahead, so the output is 73 samples later than FAAD2's; FAAD2's 32-band
  synthesis needed no such lead. The aligned signal matches FAAD2 to about
  24 dB. Streams at or below 24 kHz, which are the usual HE-AAC, are unaffected.
  `dontUpSampleImplicitSBR` returns the full SBR rate for ADTS streams; raw
  streams are decimated either way, as FAAD2 did.
- `NeAACDecGetCapabilities` reports only `LC_DEC_CAP`; FAAD2 also reported MAIN,
  LTP, LD and error resilience. Callers that fall back to another decoder when
  a capability is missing now do so.
- `NeAACDecInitDRM` is exported by every build, not only the DRM one.
- For 7.1 in an ADTS stream, `channel_position` lists all eight positions;
  FAAD2 filled in only two.
- For 7.1, initialization reports the legacy channel configuration value
  (two for ADTS, seven for raw/MP4); decoded frames report eight channels.
- Corrupt or false-sync ADTS frames use the legacy error table, but may report
  a different error than FAAD2.
- 32-bit output clamps at positive full scale instead of wrapping.
- Samples differ from FAAD2's. On the test streams the signal-to-noise ratio
  against FAAD2 is 59 to 105 dB for AAC-LC at 32 kHz and above, 21 to 68 dB for
  the streams that are upsampled, whose interpolation filters differ, and 14 to
  28 dB for HE-AAC, whose high band is regenerated. Stereo streams written
  by FAAC with noise substitution (PNS) measure 20 to 45 dB: FAAD3 draws
  different random numbers, so the noise-filled bands are uncorrelated while the
  rest of the spectrum agrees closely, and levels agree to within 0.1 dB.
- After `NeAACDecPostSeekReset` the shim decodes exactly as a new decoder
  would. FAAD2 carried the overlap and filter history of the stream across the
  seek, so its first frames afterwards differ.

## Compatibility tests

`tests/compat_test.c` replays 19 fixtures (16 AAC-LC and HE-AAC streams and
3 DAB+ streams) through the shim, in ADTS and raw form, with each output format,
downmix, `dontUpSampleImplicitSBR` and two kinds of seek, and compares them with
what FAAD2 2.11.4 did, recorded in `tests/golden`. The results of
`NeAACDecInit`, `NeAACDecInit2` and `NeAACDecAudioSpecificConfig` and every
field of `NeAACDecFrameInfo` must match, apart from the differences listed in
the test. The first samples of each stream must also line up with FAAD2's to the
sample, so the delay of HE-AAC, DAB and upsampled streams is the same (implicit
SBR above 24 kHz is exactly 73 samples later, as above), and they must stay
within a signal-to-noise ratio set for each stream.

The golden files only change when the cases do. To regenerate them, build the
original library from commit `0fb0973` and run the recorder against it. The
committed recording was made on x86_64 Linux:

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
synthesized audio. The others came with the first version of the shim's
regression tests.

## Migrating applications

Existing applications can continue using `<neaacdec.h>` and link to
`libfaad.so.2` through this shim. The legacy `<faad.h>` is still installed; it
defines `FAAD2_VERSION` and includes `<neaacdec.h>`, and its compile-time notice
points to libfaad3. For new code, include `<faad3.h>` and link
to `libfaad3` (or use `pkg-config --cflags --libs faad3`). The FAAD3 calling
sequence uses `faad_config_init()`, `faad_decoder_open()`, and
`faad_decode_frame()`; provide ASC when opening a raw stream and use returned
status, format-change flags, and stream information instead of legacy error
and frame structures.

See the [FAAD3 API and migration guide](https://github.com/FreewareAdvancedAudio/faac/blob/master/docs/libfaad.md)
for function mappings, examples, and details of changed behavior. The
`libfaad.3` suffix identifies manual section 3, not FAAD version 3; the FAAD3
library is named `libfaad3`.
