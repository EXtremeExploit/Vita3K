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

#include "module/module.h"

#include <adhoc/state.h>
#include <bits/chrono.h>
#include <chrono>

int sendHelloReqToPipe(void *arg) {
    SceNetAdhocMatchingContext *ctx = (SceNetAdhocMatchingContext *)arg;
    if ((ctx->helloPipeMsg.flags & 1) == 0) {
        ctx->helloPipeMsg.type = SCE_NET_ADHOC_MATCHING_EVENT_HELLO_SEND;
        ctx->helloPipeMsg.flags = ctx->helloPipeMsg.flags | 1;
        ctx->helloPipeMsg.peer = nullptr;
        write(ctx->pipesFd[1], &ctx->helloPipeMsg, sizeof(ctx->helloPipeMsg));
        LOG_CRITICAL("sent hello event req");
    }
    LOG_CRITICAL("sent hello event req end");
    return 0;
}

int adhocMatchingEventThread(EmuEnvState *emuenv, int id) {
    auto ctx = emuenv->adhoc.findMatchingContext(id);

    SceNetAdhocMatchingPipeMessage pipeMessage;
    while (read(ctx->pipesFd[0], &pipeMessage, sizeof(pipeMessage)) >= 0) {
        int type = pipeMessage.type;
        auto peer = pipeMessage.peer;
        auto flags = pipeMessage.flags;
        LOG_CRITICAL("received event:{}", type);
        switch (type) {
        case SCE_NET_ADHOC_MATCHING_EVENT_PACKET: { // Packet received
            peer->msg.flags = peer->msg.flags & 0xfffffffe;
            ctx->processPacketFromPeer(peer);
            if (peer->rawPacket)
                delete peer->rawPacket;
            peer->rawPacket = 0;
            peer->rawPacketLength = 0;
            break;
        }
        case SCE_NET_ADHOC_MATCHING_EVENT_UNK2: { // idk
            // TODO
            break;
        }
        case SCE_NET_ADHOC_MATCHING_EVENT_UNK3: { // idk
            // TODO
            break;
        }
        case SCE_NET_ADHOC_MATCHING_EVENT_HELLO_SEND: { // broadcast hello message to network
            ctx->helloPipeMsg.flags = ctx->helloPipeMsg.flags & 0xfffffffe;
            int num = ctx->countTargetsWithStatusOrBetter(3);
            // also count ourselves
            if (num + 1 < ctx->maxnum)
                ctx->broadcastHello();

            if (ctx->searchTimedFunc(sendHelloReqToPipe)) {
                ctx->delTimedFunc(sendHelloReqToPipe); // not run it, we are gonna reschedule it
            };
            ctx->addTimedFunc(sendHelloReqToPipe, ctx, ctx->helloInterval);
            break;
        }
        case SCE_NET_ADHOC_MATCHING_EVENT_UNK5: {
            // TODO
            break;
        }
        }
    }

    return 0;
};

int adhocMatchingInputThread(EmuEnvState *emuenv, int id) {
    auto ctx = emuenv->adhoc.findMatchingContext(id);

    sockaddr_in *fromAddr;
    socklen_t fromAddrLen = sizeof(*fromAddr);

    do {
        int res;
        SceUShort16 packetLength;
        do {
            do {
                res = recvfrom(ctx->recvSocket, ctx->rxbuf, ctx->rxbuflen, 0, (sockaddr *)fromAddr, &fromAddrLen);
                if (res < 0) {
                    assert(false);
                    return 0; // exit the thread immediatly
                }

                // Ignore packets of our own (own broadcast) and make sure the first 4 bytes is host byte order 1
            } while (fromAddr->sin_addr.s_addr == ctx->ownAddress || *ctx->rxbuf != 1);
            // we have a packet :D, but may be unfinished, check how long it is and see if we can get the remaining

            SceUShort16 nPacketLength; // network byte order of packet length
            memcpy(&nPacketLength, ctx->rxbuf + 2, 2);
            SceUShort16 packetLenght = ntohs(nPacketLength); // ACTUALLY the packet length fr this time
        } while (res < packetLength + 4);
        // We received the whole packet, we can now commence the parsing and the fun
        auto foundTarget = ctx->findTargetByAddr(fromAddr->sin_addr.s_addr);
        if (foundTarget == nullptr) {
            uint8_t targetMode = *(ctx->rxbuf + 1);
            if (((targetMode == SCE_NET_ADHOC_MATCHING_MODE_CHILD) && ((ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT || ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_P2P))) || ((targetMode == SCE_NET_ADHOC_MATCHING_MODE_PARENT && ((ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_CHILD || (ctx->mode == SCE_NET_ADHOC_MATCHING_MODE_P2P)))))) {
                foundTarget = ctx->newTarget(fromAddr->sin_addr.s_addr);
            }

            if (((foundTarget->msg.flags & 1U) == 0)) {
                auto rawPacket = new char[res];
                if (rawPacket == nullptr)
                    continue;

                // Copy the whole packet we received into the peer
                memcpy(rawPacket, ctx->rxbuf, res);
                foundTarget->rawPacketLength = res;
                foundTarget->rawPacket = rawPacket;
                foundTarget->packetLength = packetLength + 4;
                foundTarget->keepAliveInterval = ctx->keepAliveInterval;

                foundTarget->msg.type = SCE_NET_ADHOC_MATCHING_EVENT_PACKET;
                foundTarget->msg.peer = foundTarget;
                foundTarget->msg.flags = foundTarget->msg.flags | 1;
                write(ctx->pipesFd[1], &foundTarget->msg, sizeof(foundTarget->msg));
            }
        }
    } while (true);

    return 0;
};

int adhocMatchingCalloutThread(EmuEnvState *emuenv, int id) {
    auto ctx = emuenv->adhoc.findMatchingContext(id);

    do {
        ctx->calloutSyncing.calloutMutex.lock();

        auto &list = ctx->calloutSyncing.functions;
        if (list.empty()) {
            LOG_CRITICAL("timed funcs is empty, waiting 100us");
            usleep(100); // sleep for 100us to wait for the list to populate, mainly to not waste cpu
        }
        uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        for (auto &func : list) {
            if (func.second.execAt < now)
                continue;
            ctx->calloutSyncing.calloutMutex.unlock();
            LOG_CRITICAL("running timed func");
            (func.first)(func.second.args);
            // running callout func
            func.second.ran = true;
            ctx->calloutSyncing.calloutMutex.lock();
        };

        // delete functions that ran
        for (auto &func : list) {
            if (func.second.ran)
                list.erase(func.first);
        };

        ctx->calloutSyncing.calloutMutex.lock();
    } while (ctx->calloutSyncing.calloutShouldExit == false);

    return 0;
};