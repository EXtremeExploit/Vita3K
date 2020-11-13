// Vita3K emulator project
// Copyright (C) 2018 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "SceAtrac.h"

EXPORT(int, sceAtracAddStreamData) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracCreateDecoderGroup) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracDecode, int atracHandle, void *pOutputBuffer, SceUInt32 *pOutputSamples, SceUInt32 *pDecoderStatus) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracDeleteDecoderGroup) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetContentInfo, int atracHandle, void *pContentInfo) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetDecoderGroupInfo) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetDecoderStatus) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetInternalError) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetLoopInfo) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetNextOutputPosition) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetOutputSamples) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetOutputableSamples) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetRemainSamples) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetStreamInfo) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetSubBufferInfo) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracGetVacantSize) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracIsSubBufferNeeded, int atracHandle) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracQueryDecoderGroupMemSize) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracReleaseHandle) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracResetNextOutputPosition, int atracHandle, SceUInt32 resetSample) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracSetDataAndAcquireHandle, SceUChar8 *pucBuffer, SceUInt32 uiReadSize, SceUInt32 uiBufferSize) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracSetLoopNum, int atracHandle, int loopNum) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracSetOutputSamples, int atracHandle, SceUInt32 outputSamples) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAtracSetSubBuffer) {
    return UNIMPLEMENTED();
}

BRIDGE_IMPL(sceAtracAddStreamData)
BRIDGE_IMPL(sceAtracCreateDecoderGroup)
BRIDGE_IMPL(sceAtracDecode)
BRIDGE_IMPL(sceAtracDeleteDecoderGroup)
BRIDGE_IMPL(sceAtracGetContentInfo)
BRIDGE_IMPL(sceAtracGetDecoderGroupInfo)
BRIDGE_IMPL(sceAtracGetDecoderStatus)
BRIDGE_IMPL(sceAtracGetInternalError)
BRIDGE_IMPL(sceAtracGetLoopInfo)
BRIDGE_IMPL(sceAtracGetNextOutputPosition)
BRIDGE_IMPL(sceAtracGetOutputSamples)
BRIDGE_IMPL(sceAtracGetOutputableSamples)
BRIDGE_IMPL(sceAtracGetRemainSamples)
BRIDGE_IMPL(sceAtracGetStreamInfo)
BRIDGE_IMPL(sceAtracGetSubBufferInfo)
BRIDGE_IMPL(sceAtracGetVacantSize)
BRIDGE_IMPL(sceAtracIsSubBufferNeeded)
BRIDGE_IMPL(sceAtracQueryDecoderGroupMemSize)
BRIDGE_IMPL(sceAtracReleaseHandle)
BRIDGE_IMPL(sceAtracResetNextOutputPosition)
BRIDGE_IMPL(sceAtracSetDataAndAcquireHandle)
BRIDGE_IMPL(sceAtracSetLoopNum)
BRIDGE_IMPL(sceAtracSetOutputSamples)
BRIDGE_IMPL(sceAtracSetSubBuffer)
