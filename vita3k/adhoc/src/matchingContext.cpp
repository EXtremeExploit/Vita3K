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

#include <adhoc/state.h>
#include <kernel/state.h>

void SceNetAdhocMatchingContext::destroy(EmuEnvState &emuenv, SceUID thread_id, const char *export_name) {
    SceNetAdhocMatchingContext *prev = emuenv.adhoc.adhocMatchingContextsList; // empty header
    SceNetAdhocMatchingContext *current = emuenv.adhoc.adhocMatchingContextsList->next; // the first valid node

    // Context Pointer
    SceNetAdhocMatchingContext *item = emuenv.adhoc.adhocMatchingContextsList;

    // Iterate contexts
    for (; item != nullptr; item = item->next) {
        // Found matching ID
        if (item->id == this->id) {
            // Unlink Left (Beginning)
            if (prev == nullptr)
                emuenv.adhoc.adhocMatchingContextsList = item->next;

            // Unlink Left (Other)
            else
                prev->next = item->next;

            // Stop it first if it's still running
            if (item->isRunning) {
                item->isRunning = false;
                if (item->inputThread.joinable())
                    item->inputThread.join();

                if (item->eventThread.joinable())
                    item->eventThread.join();
            }
            break;
        }

        // Set Previous Reference
        prev = nullptr;
    }
    delete item;
};

void SceNetAdhocMatchingContext::notifyHandler(EmuEnvState &emuenv, int event, SceNetInAddr *peer, int optLen, void *opt) {
    if (!this->handler.entry)
        return;

    if (!this->handler.thread)
        this->handler.thread = emuenv.kernel.create_thread(emuenv.mem, "adhocHandlerThread");

    Ptr<SceNetInAddr> vPeer;
    if (peer) {
        vPeer = Ptr<SceNetInAddr>(alloc(emuenv.mem, sizeof(SceNetInAddr), "adhocHandlerPeer"));
        memcpy(vPeer.get(emuenv.mem), peer, sizeof(*peer));
    } else {
        vPeer = Ptr<SceNetInAddr>(0);
    }

    Ptr<char> vOpt;
    if (opt) {
        vOpt = Ptr<char>(alloc(emuenv.mem, optLen, "adhocHandlerOpt"));
        memcpy(vOpt.get(emuenv.mem), opt, optLen);
    } else {
        vOpt = Ptr<char>(0);
    }

    SceNetAdhocHandlerArguments handleArgs{
        .id = (uint32_t)this->id,
        .event = (uint32_t)event,
        .peer = vPeer.address(),
        .optlen = (uint32_t)optLen,
        .opt = vOpt.address()
    };

    auto data = Ptr<SceNetAdhocHandlerArguments>(alloc(emuenv.mem, sizeof(handleArgs), "handleArgs"));
    memcpy(data.get(emuenv.mem), &handleArgs, sizeof(handleArgs));

    this->handler.thread->run_guest_function(this->handler.entry.address(), sizeof(SceNetAdhocHandlerArguments), data);

    free(emuenv.mem, vPeer); // free peer
    free(emuenv.mem, vOpt); // free opt
    free(emuenv.mem, data); // free arguments
}

bool SceNetAdhocMatchingContext::setHelloOpt(int optlen, const void *opt) {
    auto hi = new uint8_t[sizeof(SceNetAdhocMatchingHelloStart) + optlen + sizeof(SceNetAdhocMatchingHelloEnd)];
    this->totalHelloLength = sizeof(SceNetAdhocMatchingHelloStart) + optlen + sizeof(SceNetAdhocMatchingHelloEnd);

    if (this->hello != nullptr)
        delete this->hello;
    this->hello = hi;

    ((SceNetAdhocMatchingHelloStart *)hi)->unk00 = 1;
    ((SceNetAdhocMatchingHelloStart *)hi)->unk01 = 1;

    auto nRexmtInterval = htonl(this->rexmtInterval);
    memcpy(&((SceNetAdhocMatchingHelloStart *)hi)->nRexmtInterval, &nRexmtInterval, sizeof(nRexmtInterval));

    auto nHelloInterval = htonl(this->helloInterval);
    memcpy(&((SceNetAdhocMatchingHelloStart *)hi)->nHelloInterval, &nHelloInterval, sizeof(nHelloInterval));

    auto packetLength = htons(optlen + sizeof(nRexmtInterval) + sizeof(nHelloInterval));
    memcpy(&((SceNetAdhocMatchingHelloStart *)hi)->nPacketLength, &packetLength, sizeof(packetLength));

    memcpy(hi + sizeof(SceNetAdhocMatchingHelloStart), opt, optlen);

    uint32_t NUMBAONE = 1;
    memcpy(&((SceNetAdhocMatchingHelloEnd *)(hi + sizeof(SceNetAdhocMatchingHelloStart) + optlen))->unk1, &NUMBAONE, sizeof(NUMBAONE));
    memset(&((SceNetAdhocMatchingHelloEnd *)(hi + sizeof(SceNetAdhocMatchingHelloStart) + optlen))->unk2, 0, 12);

    return true;
};

bool SceNetAdhocMatchingContext::getHelloOpt(int *oOptlen, void *oOpt) {
    int optLen = this->totalHelloLength - sizeof(SceNetAdhocMatchingHelloStart);

    if (oOpt != nullptr && 0 < optLen) {
        // Whichever is less to not write more than app allocated for opt data
        if (*oOptlen < optLen)
            optLen = *oOptlen;

        memcpy(oOpt, this->hello + sizeof(SceNetAdhocMatchingHelloStart), optLen);
    }
    *oOptlen = optLen;

    return true;
};

bool SceNetAdhocMatchingContext::broadcastHello() {
    sockaddr_in send_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(this->port),
    };
    send_addr.sin_addr.s_addr = INADDR_BROADCAST;

    auto sendResult = ::sendto(this->sendSocket, &this->hello, this->totalHelloLength, 0, (sockaddr *)&send_addr, sizeof(send_addr));

    if (sendResult == EAGAIN)
        sendResult = 0;

    if (sendResult < 0)
        return false;
    // usleep(this->helloInterval);

    return true;
};

SceNetAdhocMatchingTarget *SceNetAdhocMatchingContext::findTargetByAddr(uint32_t addr) {
    SceNetAdhocMatchingTarget *item = this->targets;
    if (item != nullptr) {
        do {
            if (item->addr.s_addr == addr)
                return item;
            item = item->next;
        } while (item != nullptr);
    }
    return item;
};

bool SceNetAdhocMatchingContext::initSendSocket() {
    this->sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->sendSocket < 0)
        return false; // what to return here

    int flag = 1;
    if (setsockopt(this->sendSocket, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) < 0)
        return false; // what to return here

    return true;
}

bool SceNetAdhocMatchingContext::initEventHandler(EmuEnvState &emuenv) {
    auto pipesResult = pipe(this->pipesFd);
    if (pipesResult == -1) {
        assert(false);
        return false;
    }

    this->eventThread = std::thread(adhocMatchingEventThread, &emuenv, this->id);
    return true;
}

bool SceNetAdhocMatchingContext::initInputThread(EmuEnvState &emuenv) {
    this->recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->recvSocket < 0)
        return false;

    sockaddr_in recv_addr = {
        .sin_family = AF_INET,
        .sin_port = this->port
    };
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    auto bindResult = bind(this->recvSocket, (sockaddr *)&recv_addr, sizeof(recv_addr));

    if (bindResult < 0)
        close(this->recvSocket);

    this->inputThread = std::thread(adhocMatchingInputThread, &emuenv, this->id);
    return true;
}

bool SceNetAdhocMatchingContext::generateAddrsMsg(EmuEnvState &emuenv) {
    return true;
}

SceNetAdhocMatchingTarget *SceNetAdhocMatchingContext::newTarget(uint32_t addr) {
    auto target = new SceNetAdhocMatchingTarget();
    // TODO: init the target to status 1
    target->addr.s_addr = addr;

    return target;
}
