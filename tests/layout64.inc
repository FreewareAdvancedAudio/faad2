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

    EQ(sizeof(NeAACDecConfiguration), 24);
    EQ(offsetof(NeAACDecConfiguration, defObjectType), 0);
    EQ(offsetof(NeAACDecConfiguration, defSampleRate), 8);
    EQ(offsetof(NeAACDecConfiguration, outputFormat), 16);
    EQ(offsetof(NeAACDecConfiguration, downMatrix), 17);
    EQ(offsetof(NeAACDecConfiguration, useOldADTSFormat), 18);
    EQ(offsetof(NeAACDecConfiguration, dontUpSampleImplicitSBR), 19);
    EQ(sizeof(NeAACDecFrameInfo), 104);
    EQ(offsetof(NeAACDecFrameInfo, bytesconsumed), 0);
    EQ(offsetof(NeAACDecFrameInfo, samples), 8);
    EQ(offsetof(NeAACDecFrameInfo, channels), 16);
    EQ(offsetof(NeAACDecFrameInfo, error), 17);
    EQ(offsetof(NeAACDecFrameInfo, samplerate), 24);
    EQ(offsetof(NeAACDecFrameInfo, sbr), 32);
    EQ(offsetof(NeAACDecFrameInfo, object_type), 33);
    EQ(offsetof(NeAACDecFrameInfo, header_type), 34);
    EQ(offsetof(NeAACDecFrameInfo, num_front_channels), 35);
    EQ(offsetof(NeAACDecFrameInfo, num_side_channels), 36);
    EQ(offsetof(NeAACDecFrameInfo, num_back_channels), 37);
    EQ(offsetof(NeAACDecFrameInfo, num_lfe_channels), 38);
    EQ(offsetof(NeAACDecFrameInfo, channel_position), 39);
    EQ(offsetof(NeAACDecFrameInfo, ps), 103);
    EQ(sizeof(mp4AudioSpecificConfig), 32);
    EQ(offsetof(mp4AudioSpecificConfig, objectTypeIndex), 0);
    EQ(offsetof(mp4AudioSpecificConfig, samplingFrequencyIndex), 1);
    EQ(offsetof(mp4AudioSpecificConfig, samplingFrequency), 8);
    EQ(offsetof(mp4AudioSpecificConfig, channelsConfiguration), 16);
    EQ(offsetof(mp4AudioSpecificConfig, frameLengthFlag), 17);
    EQ(offsetof(mp4AudioSpecificConfig, dependsOnCoreCoder), 18);
    EQ(offsetof(mp4AudioSpecificConfig, coreCoderDelay), 20);
    EQ(offsetof(mp4AudioSpecificConfig, extensionFlag), 22);
    EQ(offsetof(mp4AudioSpecificConfig, aacSectionDataResilienceFlag), 23);
    EQ(offsetof(mp4AudioSpecificConfig, aacScalefactorDataResilienceFlag), 24);
    EQ(offsetof(mp4AudioSpecificConfig, aacSpectralDataResilienceFlag), 25);
    EQ(offsetof(mp4AudioSpecificConfig, epConfig), 26);
    EQ(offsetof(mp4AudioSpecificConfig, sbr_present_flag), 27);
    EQ(offsetof(mp4AudioSpecificConfig, forceUpSampling), 28);
    EQ(offsetof(mp4AudioSpecificConfig, downSampledSBR), 29);
    EQ(offsetof(mp4AudioSpecificConfig, ps_present_flag), 30);
