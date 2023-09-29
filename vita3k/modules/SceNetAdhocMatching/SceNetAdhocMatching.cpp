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

// #include "net/types.h"
#include <module/module.h>

#include <adhoc/state.h>

#include <util/tracy.h>
TRACY_MODULE_NAME(SceNetAdhocMatching);

// s_addr can be a macro on windows
#pragma push_macro("s_addr")
#undef s_addr

EXPORT(int, sceNetAdhocMatchingAbortSendData) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingCancelTargetWithOpt, int id, SceNetInAddr *target, int optLen, void *opt) {
    TRACY_FUNC(sceNetAdhocMatchingCancelTargetWithOpt, id, target, optLen, opt);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    if (!target)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->status != 2)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_RUNNING);

    auto foundTarget = ctx->findTargetByAddr(target->s_addr);
    if (!foundTarget)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_UNKNOWN_TARGET);

    if (((optLen < 0) || (SCE_NET_ADHOC_MATCHING_MAXOPTLEN < optLen)) || ((0 < optLen && (opt == nullptr))))
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_OPTLEN);

    switch (foundTarget->status) {
    case 1:
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        if (optLen > 0) {
            auto _opt = new char[optLen];
            memcpy(_opt, opt, optLen);
        }
        // falltrough
    default: {
        // TODO
    }
    }

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingCancelTarget, int id, SceNetInAddr *target) {
    TRACY_FUNC(sceNetAdhocMatchingCancelTarget, id, target);
    return CALL_EXPORT(sceNetAdhocMatchingCancelTargetWithOpt, id, target, 0, 0);
}

EXPORT(int, sceNetAdhocMatchingCreate, int mode, int maxnum, SceUShort16 port, int rxbuflen, unsigned int helloInterval, unsigned int keepaliveInterval, int initCount, unsigned int rexmtInterval, Ptr<void> handlerAddr) {
    TRACY_FUNC(sceNetAdhocMatchingCreate, mode, maxnum, port, rxbuflen, helloInterval, keepaliveInterval, initCount, rexmtInterval, handlerAddr)
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    if ((mode < SCE_NET_ADHOC_MATCHING_MODE_PARENT) || (SCE_NET_ADHOC_MATCHING_MODE_P2P < mode))
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MODE);

    if (maxnum < 2 || maxnum > 16)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MAXNUM);

    if (rxbuflen < maxnum * 4 + 4U)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_RXBUF_TOO_SHORT);

    if (((mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT) || (mode == SCE_NET_ADHOC_MATCHING_MODE_P2P)) && ((rexmtInterval == 0 || (helloInterval == 0))))
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
    if (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT)
        ctx->maxnum = maxnum;

    ctx->port = port;
    ctx->rxbuflen = rxbuflen;
    ctx->helloInterval = helloInterval;
    ctx->keepAliveInterval = keepaliveInterval;
    ctx->initCount = initCount;
    ctx->rexmtInterval = rexmtInterval;

    ctx->helloFuncInQueue = false;
    ctx->targets = nullptr;

    SceNetAdhocMatchingHandler handler{
        .entry = handlerAddr,
    };

    ctx->handler = handler;
    ctx->rxbuf = new char[ctx->rxbuflen];

    return ctx->id;
}

EXPORT(int, sceNetAdhocMatchingStop, int id) {
    TRACY_FUNC(sceNetAdhocMatchingStop, id);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_RUNNING)
        return 0; // The context is already stopping or its stopped already, nothing to do

    ctx->status = SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_STOPPING;

    // These 2 may take time because they wait for both threads to end
    ctx->unInitInputThread();
    ctx->unInitEventThread();

    if (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT || ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_P2P) {
        ctx->delTimedFunc(sendHelloReqToPipe);
        ctx->unInitHelloMsg();
        ctx->helloPipeMsg.flags &= 1U;
    }

    ctx->unInitSendSocket();
    ctx->status = SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_NOT_RUNNING;

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingDelete, int id) {
    TRACY_FUNC(sceNetAdhocMatchingDelete, id);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    // TODO: check if its running
    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_NOT_RUNNING)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_IS_RUNNING);

    delete ctx->rxbuf;
    ctx->rxbuf = nullptr;
    ctx->destroy(emuenv, thread_id, export_name);

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingGetHelloOpt, int id, int *optlen, void *opt) {
    TRACY_FUNC(sceNetAdhocMatchingGetHelloOpt, id, optlen, opt);

    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    if (optlen == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_CHILD)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MODE);

    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_RUNNING)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_NOT_RUNNING);

    ctx->getHelloOpt(optlen, opt);

    return 0;
}

EXPORT(int, sceNetAdhocMatchingGetMembers, int id, unsigned int *membersCount, SceNetAdhocMatchingMember *members) {
    TRACY_FUNC(sceNetAdhocMatchingGetMembers, id, membersCount, members);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    if (!membersCount)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_RUNNING)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_RUNNING);

    ctx->getMembers(membersCount, members);

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingSelectTarget, int id, SceNetInAddr *target, int optlen, void *opt) {
    TRACY_FUNC(sceNetAdhocMatchingSelectTarget, id, target, optlen, opt);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    if (!target)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_RUNNING)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_RUNNING);

    auto foundTarget = ctx->findTargetByAddr(target->s_addr);
    if (!foundTarget)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_UNKNOWN_TARGET);

    if (((optlen < 0) || (SCE_NET_ADHOC_MATCHING_MAXOPTLEN < optlen)) || ((0 < optlen && (opt == (void *)0x0))))
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_OPTLEN);

    auto membersCount = ctx->countTargetsWithStatusOrBetter(3);
    switch (foundTarget->status) {
    case 1:
        if (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT)
            return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_TARGET_NOT_READY);
        // TODO
        break;
    case 2:
        if (membersCount + 1 >= ctx->maxnum)
            return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_EXCEED_MAXNUM);
        // TODO
        break;
    case 3:
    case 4:
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_REQUEST_IN_PROGRESS);
    case 5:
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_ALREADY_ESTABLISHED);
    }

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingSendData, int id, SceNetInAddr *target, int length, void *data) {
    TRACY_FUNC(sceNetAdhocMatchingSendData, id, target, length, data);
    return UNIMPLEMENTED();
}

EXPORT(int, sceNetAdhocMatchingSetHelloOpt, int id, int optlen, void *opt) {
    TRACY_FUNC(sceNetAdhocMatchingSetHelloOpt, id, optlen, opt);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_CHILD)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MODE);

    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_RUNNING)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_RUNNING);

    if (((optlen < 0) || (SCE_NET_ADHOC_MATCHING_MAXHELLOOPTLEN < optlen)) || ((0 < optlen && (opt == nullptr))))
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_OPTLEN);

    ctx->setHelloOpt(optlen, opt);

    return 0;
}

EXPORT(int, sceNetAdhocMatchingStart, int id, int threadPriority, int threadStackSize, int threadCpuAffinityMask, int helloOptlen, void *helloOpt) {
    TRACY_FUNC(sceNetAdhocMatchingStart, id, threadPriority, threadStackSize, threadCpuAffinityMask, helloOptlen, helloOpt);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    SceNetAdhocMatchingContext *ctx = emuenv.adhoc.findMatchingContext(id);
    if (ctx == nullptr)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID);

    if (ctx->status != SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_NOT_RUNNING)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_IS_RUNNING);

    if (((helloOptlen < 0) || (SCE_NET_ADHOC_MATCHING_MAXHELLOOPTLEN < helloOptlen)) || ((0 < helloOptlen && (!helloOpt)))) {
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_INVALID_OPTLEN);
    }

    const int setupSendSocketOk = ctx->initSendSocket(emuenv, thread_id);
    if (!setupSendSocketOk)
        return RET_ERROR(-1);

    const int setupEventHandlerOk = ctx->initEventHandler(emuenv);
    if (!setupEventHandlerOk) {
        ctx->unInitSendSocket();
        return RET_ERROR(-1);
    }

    const int setupInputThread = ctx->initInputThread(emuenv);
    if (!setupInputThread) {
        ctx->unInitSendSocket();
        ctx->unInitEventThread();
        return RET_ERROR(-1);
    }

    const int setupCallout = ctx->initCalloutThread(emuenv);
    if (!setupCallout) {
        ctx->unInitSendSocket();
        ctx->unInitEventThread();
        ctx->unInitInputThread();
        return RET_ERROR(-1);
    }

    if (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT || ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_P2P) {
        ctx->setHelloOpt(helloOptlen, helloOpt);
        // TODO: do a bit more here
        ctx->addTimedFunc(sendHelloReqToPipe, ctx, ctx->helloInterval);
    }
    // Init addrs msg
    ctx->generateAddrsMsg();

    ctx->status = SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_RUNNING;

    return 0;
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

EXPORT(int, sceNetAdhocMatchingTerm) {
    TRACY_FUNC(sceNetAdhocMatchingTerm);
    if (!emuenv.adhoc.inited)
        return RET_ERROR(SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(emuenv.adhoc.mutex);

    for (int i = 0; i < SCE_NET_ADHOC_MATCHING_MAXNUM - 1; i++) {
        CALL_EXPORT(sceNetAdhocMatchingStop, i);
        auto ctx = emuenv.adhoc.findMatchingContext(i);
        if ((ctx != nullptr) && (ctx->status == SCE_NET_ADHOC_MATCHING_CONTEXT_STATUS_NOT_RUNNING)) {
            delete (ctx->rxbuf);
            ctx->rxbuf = nullptr;
            ctx->destroy(emuenv, thread_id, export_name);
        }
    }

    emuenv.adhoc.inited = false;

    return UNIMPLEMENTED();
}

#pragma pop_macro("s_addr")
