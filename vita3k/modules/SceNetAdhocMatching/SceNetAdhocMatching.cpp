// Vita3K emulator project
// Copyright (C) 2024 Vita3K team
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

#include "net/types.h"
#include <cstdint>
#include <cstring>
#include <module/module.h>

#include <adhoc/state.h>

#include <util/tracy.h>
TRACY_MODULE_NAME(SceNetAdhocMatching);

EXPORT(int, sceNetAdhocMatchingAbortSendData) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingCancelTarget) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingCancelTargetWithOpt) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingCreate, int mode, int maxnum, SceUShort16 port, int rxbuflen, unsigned int helloInterval, unsigned int keepaliveInterval, int initCount, unsigned int rexmtInterval, Ptr<void> handlerAddr) {
    TRACY_FUNC(sceNetAdhocMatchingCreate, mode, maxnum, port, rxbuflen, helloInterval, keepaliveInterval, initCount, rexmtInterval, handlerAddr)
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    if ((mode < SCE_ADHOC_MATCHING_MODE_PARENT) || (SCE_ADHOC_MATCHING_MODE_P2P < mode))
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MODE);

    if (maxnum < 2 || maxnum > 16)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MAXNUM);

    if (rxbuflen < maxnum * 4 + 4U)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_RXBUF_TOO_SHORT);

    if (((mode == SCE_ADHOC_MATCHING_MODE_PARENT) || (mode == SCE_ADHOC_MATCHING_MODE_P2P)) && ((rexmtInterval == 0 || (helloInterval == 0))))
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    if (keepaliveInterval < 0 || initCount == 0)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    const auto id = emuenv.adhoc.createAdhocMatchingContext(port);

    if (id < 0)
        return RET_ERROR(id);

    const auto ctx = emuenv.adhoc.findMatchingContext(id);
    ctx->mode = mode;

    // Children have 2 peers max (parent and itself)
    ctx->maxnum = 2;
    if (ctx->mode == SCE_ADHOC_MATCHING_MODE_PARENT)
        ctx->maxnum = maxnum;
    ctx->port = port;
    ctx->rxbuflen = rxbuflen;
    ctx->helloInterval = helloInterval;
    ctx->keepAliveInterval = keepaliveInterval;
    ctx->initCount = initCount;
    ctx->rexmtInterval = rexmtInterval;

    SceNetEtherAddr localmac = {};
    CALL_EXPORT(sceNetGetMacAddress, &localmac, 0);
    ctx->mac = localmac;

    SceNetAdhocMatchingHandler handler{
        .entry = handlerAddr,
    };

    ctx->handler = handler;

    ctx->rxbuf = new uint8_t[ctx->rxbuflen];

    return ctx->id;
}

EXPORT(int, sceNetAdhocMatchingDelete, int id) {
    TRACY_FUNC(sceNetAdhocMatchingDelete, id);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingGetHelloOpt) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingGetMembers) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingInit, SceSize poolsize, void *poolptr) {
    TRACY_FUNC(sceNetAdhocMatchingInit, poolsize, poolptr);

    if (emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_ALREADY_INITIALIZED);

    if (!poolptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    emuenv.adhoc.inited = true;

    return STUBBED("Stub adhoc init to true");
}

EXPORT(int, sceNetAdhocMatchingSelectTarget) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingSendData) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingSetHelloOpt) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingStart, int id, int threadPriority, int threadStackSize, int threadCpuAffinityMask, int helloOptlen, const void *helloOpt) {
    TRACY_FUNC(sceNetAdhocMatchingStart, id, threadPriority, threadStackSize, threadCpuAffinityMask, helloOptlen, helloOpt);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (((helloOptlen < 0) || (SCE_NET_ADHOC_MATCHING_MAXHELLOOPTLEN < helloOptlen)) || ((0 < helloOptlen && (!helloOpt)))) {
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_OPTLEN);
    }

    auto ret = ctx->initSendSocket(emuenv, thread_id, export_name);
    if (ret < 0)
        return ret;

    // thread stuff should go here, but we are ignoring it as we are running the thread natively
    ret = ctx->initEventHandler(emuenv, thread_id, export_name);
    if (ret < 0)
        return ret;

    return 0;
}

EXPORT(int, sceNetAdhocMatchingStop, int id) {
    TRACY_FUNC(sceNetAdhocMatchingStop, id);
    return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingTerm) {
    TRACY_FUNC(sceNetAdhocMatchingTerm);

    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    for (uint8_t i = 0; i < 15; i++) {
        CALL_EXPORT(sceNetAdhocMatchingStop, i);
    }

    // for (auto &ctx : emuenv.adhoc.matchingContexts) {
    //     delete ctx.second;
    // }

    emuenv.adhoc.inited = false;

    return UNIMPLEMENTED();
}
