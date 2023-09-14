// Vita3K emulator project
// Copyright (C) 2023 Vita3K team
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
#include <adhoc/state.h>
#include <thread>

SceNetAdhocMatchingContext *AdhocState::findMatchingContext(int id) {
    // Iterate Matching Context List
    SceNetAdhocMatchingContext *item = adhocMatchingContextsList;
    for (; item != nullptr; item = item->next) { // Found Matching ID
        if (item->id == id)
            return item;
    }

    // Context not found
    return nullptr;
};

int AdhocState::createAdhocMatchingContext(SceUShort16 port) {
    SceNetAdhocMatchingContext *item = adhocMatchingContextsList;

    // Check for port conflicts
    for (; item != nullptr; item = item->next) {
        if (item->port == port)
            return SCE_NET_ADHOC_MATCHING_ERROR_PORT_IN_USE;
    }

    // TODO: refactor this
    // Get free id
    int iVar2 = 1;
    if (matchingCtxCount != 15)
        iVar2 = matchingCtxCount + 1;

    int id;
    SceNetAdhocMatchingContext *paVar1;
    do {
        id = iVar2;
        if (id == matchingCtxCount) {
            return SCE_NET_ADHOC_MATCHING_ERROR_ID_NOT_AVAIL;
        }

        const auto paVar1 = findMatchingContext(id);
        if (paVar1 == nullptr) {
            if (id < 0) {
                matchingCtxCount = id;
                return id;
            }
            matchingCtxCount = id;
            const auto ctx = new SceNetAdhocMatchingContext();

            if (ctx == nullptr) {
                return SCE_NET_ADHOC_MATCHING_ERROR_NO_SPACE;
            }
            ctx->id = id;
            ctx->next = adhocMatchingContextsList;
            adhocMatchingContextsList = ctx;
            return id;
        }
        iVar2 = 1;
        if (id != 15) {
            iVar2 = id + 1;
        }
    } while (true);
}

int adhocMatchingEventThread(int matchingId) {
    return 0;
};

int adhocMatchingInputThread(int matchingId) {
    return 0;
};

int adhocMatchingCalloutThread(int matchingId) {
    return 0;
};

void SceNetAdhocMatchingContext::destroy(EmuEnvState &emuenv, SceUID thread_id, const char *export_name) {
    SceNetAdhocMatchingContext *item;

    SceNetAdhocMatchingContext *next = emuenv.adhoc.adhocMatchingContextsList;
    SceNetAdhocMatchingContext *pSVar3 = nullptr;
    if (emuenv.adhoc.adhocMatchingContextsList != nullptr) {

        do {
            auto item = next;
            next = item->next;
            // Find ourselves in the list
            if (item == this) {
                if (pSVar3 != nullptr) {
                    pSVar3->next = next;
                    next = emuenv.adhoc.adhocMatchingContextsList;
                }
                emuenv.adhoc.adhocMatchingContextsList = next->next;
                item->next = nullptr;
                break;
            }
            pSVar3 = item;
        } while (next != nullptr);
    }
    delete this;
    return;
};

int SceNetAdhocMatchingContext::initSendSocket(EmuEnvState &emuenv, SceUID thread_id, const char *export_name) {
    CALL_EXPORT(sceNetCtlAdhocGetInAddr, &emuenv.adhoc.addr);
    int s = CALL_EXPORT(sceNetSocket, "SceNetAdhocMatchingCallout", 2, SCE_NET_SOCK_DGRAM_P2P, SCE_NET_IPPROTO_IP);
    if (s < 0)
        return s;

    SceNetSockaddrIn addr = {
        .sin_len = 16,
        .sin_family = 2,
        .sin_port = CALL_EXPORT(sceNetHtons, 3658),
        .sin_vport = CALL_EXPORT(sceNetHtons, this->port + 1)
    };
    auto ret = CALL_EXPORT(sceNetBind, s, (SceNetSockaddr *)&addr, sizeof(SceNetSockaddr));
    if (ret < 0) {
        // TODO: uncomment shutdown once its implemented
        // CALL_EXPORT(sceNetShutdown, s, 0);
        CALL_EXPORT(sceNetSocketClose, s);
        return ret;
    }

    int optval = 1;
    CALL_EXPORT(sceNetSetsockopt, s, SCE_NET_SOL_SOCKET, SCE_NET_SO_BROADCAST, &optval, sizeof(optval));
    this->socket = s;
    return 0;
};

int SceNetAdhocMatchingContext::initEventHandler(EmuEnvState &emuenv, SceUID thread_id, const char *export_name) {
    auto msgPipe = CALL_EXPORT(sceKernelCreateMsgPipe, "SceNetAdhocMatchingEvent", 0x40, 0xc, 0x1000, 0);
    if (msgPipe < 0)
        return msgPipe;

    this->msgPipeUID = msgPipe;
    // Create the matching thread and pass the context id as the argument
    this->matchingEventThread = std::thread(adhocMatchingEventThread, this->id);
    return 0;
};

int SceNetAdhocMatchingContext::initInputRecv(EmuEnvState &emuenv, SceUID thread_id, const char *export_name) {
    int s = CALL_EXPORT(sceNetSocket, "SceNetAdhocMatchingRecv", 2, SCE_NET_SOCK_DGRAM_P2P, SCE_NET_IPPROTO_IP);
    if (s < 0)
        return s;

    this->matchingRecvSocket = s;

    SceNetSockaddrIn addr = {};
    addr.sin_len = 16;
    addr.sin_family = 2;
    addr.sin_port = CALL_EXPORT(sceNetHtons, 3658);
    addr.sin_vport = CALL_EXPORT(sceNetHtons, this->port);
    auto ret = CALL_EXPORT(sceNetBind, this->matchingRecvSocket, (SceNetSockaddr *)&addr, sizeof(SceNetSockaddr));
    if (ret < 0) {
        CALL_EXPORT(sceNetSocketClose, this->matchingRecvSocket);
        return ret;
    }

    this->inputThread = std::thread(adhocMatchingInputThread, this->id);
    return 0;
};

int SceNetAdhocMatchingContext::setHelloOpt(SceNetAdhocMatchingContext *ctx, int len, void *buf) {
    return 0;
};
