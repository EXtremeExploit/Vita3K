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

int adhocMatchingEventThread(EmuEnvState *emuenv, int id) {
    auto ctx = emuenv->adhoc.findMatchingContext(id);

    SceNetAdhocMatchingPipeMessage pipeMessage;
    while (::read(ctx->pipesFd[0], &pipeMessage, sizeof(pipeMessage)) >= 0) {
        int type = pipeMessage.type;
        auto peer = pipeMessage.peer;
        auto flags = pipeMessage.flags;
        switch (type) {
        case SCE_NET_ADHOC_MATCHING_EVENT_PACKET: { // Packet received
            // TODO

        } break;
        case SCE_NET_ADHOC_MATCHING_EVENT_UNK2: { // idk
            // TODO

        } break;
        case SCE_NET_ADHOC_MATCHING_EVENT_UNK3: { // idk
            // TODO

        } break;
        case SCE_NET_ADHOC_MATCHING_EVENT_HELLO_SEND: { // broadcast hello message to network
            // TODO: check if we are the max member count
            // If we are then dont broadcast the hello
            ctx->broadcastHello();
        } break;
        case SCE_NET_ADHOC_MATCHING_EVENT_UNK5: {
            // TODO

        } break;
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
                res = ::recvfrom(ctx->recvSocket, ctx->rxbuf, ctx->rxbuflen, 0, (sockaddr *)fromAddr, &fromAddrLen);
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
                auto rawPacket = new uint8_t[res];
                if (rawPacket == nullptr)
                    continue;

                // Copy the whole packet we received into the peer
                memcpy(rawPacket, ctx->rxbuf, res);
                foundTarget->rawPacketLength = res;
                foundTarget->rawPacket = rawPacket;
                foundTarget->packetLength = packetLength + 4;

                foundTarget->msg.type = SCE_NET_ADHOC_MATCHING_EVENT_PACKET;
                foundTarget->msg.peer = foundTarget;
                foundTarget->msg.flags = foundTarget->msg.flags | 1;
                *(uint *)&foundTarget->keepAliveInterval = ctx->keepAliveInterval;
                ::write(ctx->pipesFd[1], &foundTarget->msg, sizeof(foundTarget->msg));
            }
        }
    } while (true);

    return 0;
};
