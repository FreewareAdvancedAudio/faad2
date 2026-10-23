# Freeware Advanced Audio (AAC) Decoder including SBR decoding

This repository provides the legacy FAAD2 `NeAACDec*` API and `libfaad.so.2`
ABI as a compatibility shim over FAAD3. It builds the library; the decoder
implementation and `faad` command-line tool are maintained in the
[FAAC repository](https://github.com/FreewareAdvancedAudio/faac).

The original FAAD2 decoder supported AAC-LC, HE-AAC, MAIN and LTP profiles.
This shim keeps the legacy interface for existing applications while using
FAAD3 to decode the supported AAC-LC and HE-AAC streams. It retains FAAD2
channel order and positions for multichannel output.

### Key features

- Legacy `NeAACDec*` API and `libfaad.so.2` ABI
- AAC-LC, HE-AAC v1 (SBR), and HE-AAC v2 (PS) decoding through FAAD3
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

FAAD3 is a separate dependency with its own license; see its source and
copyright notices when building or distributing the combined library.

Use of AAC software may require payment of patent royalties. You are
responsible for determining and meeting any obligations that apply to your
use; this project provides no patent warranty or indemnity.

## Directory structure

- `docs` — legacy API manual and compatibility notes
- `include` — public FAAD2 compatibility headers
- `libfaad/shim.c` — implementation of the legacy API on FAAD3
- `external/faac` — optional FAAC source submodule for vendored builds
- `tests` — API regression tests and AAC fixtures
- `fuzz` — decoder API fuzzing entry points

## Building

For Debian and other package builds, install the FAAD3 development package and
configure with CMake:

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

The default build links to the installed FAAD3 library and retains its
`faad3` pkg-config dependency. Use `-DBUILD_SHARED_LIBS=OFF` for a static
library. Static consumers also need FAAD3; `pkg-config --libs --static faad2`
supplies the required link flags.

For a source-vendored build, initialize the pinned FAAC submodule and enable
`-DFAAD3_USE_BUNDLED_SOURCE=ON` to compile FAAD3 into the FAAD2 library. A
parent project can instead provide its own CMake target with
`-DFAAD3_CMAKE_TARGET=target_name`.

```sh
git submodule update --init external/faac
```

For Bazel builds, initialize the FAAC submodule before building:

```sh
git submodule update --init external/faac
bazel build //:faad
```

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
decoder initializes, or after `NeAACDecPostSeekReset`.

Dynamic range control uses full compression. Streams carrying a programme
reference level are normalized to -20 dBFS. A stream with `dynamic_range_info`
but no programme reference level is played 20 dB quieter, matching FAAD2's
behavior.

The shim does not support MAIN/LTP/SSR/LD/ER profiles, DRM, fixed-point
decoding, LATM, ADIF, or raw AAC without an AudioSpecificConfig (ASC).
`NeAACDecInitDRM` remains exported but always fails. The
`useOldADTSFormat` and `dontUpSampleImplicitSBR` fields are ignored;
`defObjectType` and `defSampleRate` do not select stream parameters.

Known differences from FAAD2 include:

- LC-only streams at or below 24 kHz are reported and emitted at their real
  rate; FAAD2 doubled the rate and upsampled them.
- For implicit SBR in a raw/MP4 configuration at or below 24 kHz,
  `NeAACDecInit2` reports the core rate while decoded frames carry the doubled
  rate. ADTS is reported correctly.
- For 7.1, initialization reports the legacy channel configuration value
  (two for ADTS, seven for raw/MP4); decoded frames report eight channels.
- Corrupt or false-sync ADTS frames use the legacy error table, but may report
  a different error than FAAD2.
- 32-bit output clamps at positive full scale instead of wrapping.
- Samples differ because decoder arithmetic differs. On noise-substituted
  content, AAC decoders typically agree at about 50 dB; HE-AAC agreement with
  FAAD2 is about 20 dB because the high band is regenerated.

## Migrating applications

Existing applications can continue using `<neaacdec.h>` and link to
`libfaad.so.2` through this shim. For new code, include `<faad3.h>` and link
to `libfaad3` (or use `pkg-config --cflags --libs faad3`). The FAAD3 calling
sequence uses `faad_config_init()`, `faad_decoder_open()`, and
`faad_decode_frame()`; provide ASC when opening a raw stream and use returned
status, format-change flags, and stream information instead of legacy error
and frame structures.

See the [FAAD3 API and migration guide](https://github.com/FreewareAdvancedAudio/faac/blob/master/docs/libfaad.md)
for function mappings, examples, and details of changed behavior. The
`libfaad.3` suffix identifies manual section 3, not FAAD version 3; the FAAD3
library is named `libfaad3`.
