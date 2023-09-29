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

#include <emuenv/state.h>
#include <kernel/state.h>
#include <net/types.h>
#include <sys/socket.h>
#include <thread>
#include <util/types.h>

#include <cstring>

bool SceNetAdhocMatchingContext::initSendSocket(EmuEnvState &emuenv, SceUID thread_id) {
    SceNetInAddr ownAddr;
    CALL_EXPORT(sceNetCtlAdhocGetInAddr, &ownAddr);
    this->ownAddress = ownAddr.s_addr;

    this->sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->sendSocket < 0)
        return false; // what to return here

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(this->port + 1)
    };

    auto bindResult = bind(this->sendSocket, (sockaddr *)&addr, sizeof(addr));
    if (bindResult < 0) {
        shutdown(this->sendSocket, 0);
        close(this->sendSocket);
        return false;
    }

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

#ifdef _WIN32
    recv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

    auto bindResult = bind(this->recvSocket, (sockaddr *)&recv_addr, sizeof(recv_addr));

    if (bindResult < 0)
        close(this->recvSocket);

    this->inputThread = std::thread(adhocMatchingInputThread, &emuenv, this->id);
    return true;
}

bool SceNetAdhocMatchingContext::initCalloutThread(EmuEnvState &emuenv) {
    if (this->calloutSyncing.calloutThreadIsRunning)
        return false;

    this->calloutSyncing.calloutShouldExit = false;

    this->calloutSyncing.calloutThread = std::thread(adhocMatchingCalloutThread, &emuenv, this->id);
    this->calloutSyncing.calloutThreadIsRunning = true;
    return true;
};

void SceNetAdhocMatchingContext::unInitInputThread() {
    // TODO!: abort all recvSocket operations
    if (this->inputThread.joinable())
        this->inputThread.join();

    shutdown(this->recvSocket, SHUT_RDWR);
    close(this->recvSocket);
    this->recvSocket = 0;
}

void SceNetAdhocMatchingContext::unInitEventThread() {
    // TODO: abort all pipe operations and make read operation return negative value on the pipe

    if (this->eventThread.joinable())
        this->eventThread.join();

    // TODO: delete pipe here
}

void SceNetAdhocMatchingContext::unInitSendSocket() {
    // TODO: abort send socket operations
    shutdown(this->sendSocket, 0);
    close(this->sendSocket);
}

void SceNetAdhocMatchingContext::unInitAddrMsg() {
    if (this->addrMsgLen <= 0)
        return;

    this->addrMsgLen = 0;
    delete this->addrMsg;
    this->addrMsg = 0;
}

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
            break;
        }

        // Set Previous Reference
        prev = nullptr;
    }
    delete item;
};

int SceNetAdhocMatchingContext::addTimedFunc(int (*entry)(void *), void *arg, uint64_t timeFromNow) {
    if (!this->calloutSyncing.calloutThreadIsRunning)
        return -1;

    std::lock_guard guard(this->calloutSyncing.calloutMutex);

    auto funcIt = this->calloutSyncing.functions.find(entry);
    if (funcIt != this->calloutSyncing.functions.end())
        return -1; // function is already one the lists

    uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    auto execAt = now + timeFromNow;

    SceNetAdhocMatchingCalloutFunction func = {
        .execAt = execAt,
        .args = arg,
    };

    this->calloutSyncing.functions.insert({ entry, func });

    return 0;
};

void SceNetAdhocMatchingContext::unInitHelloMsg() {
    if (this->totalHelloLength <= 0)
        return;

    delete this->hello;
    this->hello = nullptr;
    this->totalHelloLength = 0;
}
bool SceNetAdhocMatchingContext::searchTimedFunc(int (*entry)(void *)) {
    std::lock_guard guard(this->calloutSyncing.calloutMutex);

    auto funcIt = this->calloutSyncing.functions.find(entry);
    if (funcIt == this->calloutSyncing.functions.end())
        return false; // function is not on the list, nothing do do c:
    return true;
}

int SceNetAdhocMatchingContext::delTimedFunc(int (*entry)(void *)) {
    if (!this->calloutSyncing.calloutThreadIsRunning)
        return -1;

    std::lock_guard guard(this->calloutSyncing.calloutMutex);

    auto funcIt = this->calloutSyncing.functions.find(entry);
    if (funcIt == this->calloutSyncing.functions.end())
        return 0; // function is not on the list, nothing do do c:

    this->calloutSyncing.functions.erase(funcIt->first);

    return 0;
}

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

bool SceNetAdhocMatchingContext::setHelloOpt(int optlen, void *opt) {
    auto hi = new char[sizeof(SceNetAdhocMatchingHelloStart) + optlen + sizeof(SceNetAdhocMatchingHelloEnd)];
    this->totalHelloLength = sizeof(SceNetAdhocMatchingHelloStart) + optlen + sizeof(SceNetAdhocMatchingHelloEnd);

    if (this->hello != nullptr)
        delete this->hello;
    this->hello = hi;

    ((SceNetAdhocMatchingHelloStart *)hi)->one = 1;
    ((SceNetAdhocMatchingHelloStart *)hi)->packetType = SCE_NET_ADHOC_MATCHING_PACKET_TYPE_HELLO;

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

bool SceNetAdhocMatchingContext::broadcastHello() {
    sockaddr_in send_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(this->port),
    };
#ifdef _WIN32
    send_addr.sin_addr.S_un.S_addr = INADDR_BROADCAST;
#else
    send_addr.sin_addr.s_addr = INADDR_BROADCAST;
#endif

    auto sendResult = sendto(this->sendSocket, &this->hello, this->totalHelloLength, 0, (sockaddr *)&send_addr, sizeof(send_addr));

    if (sendResult == EAGAIN)
        sendResult = 0;

    if (sendResult < 0)
        return false;

    LOG_CRITICAL("sent hello to broadcast :D");

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

SceNetAdhocMatchingTarget *SceNetAdhocMatchingContext::newTarget(uint32_t addr) {
    auto target = new SceNetAdhocMatchingTarget();
    // TODO: init the target to status 1
    target->addr.s_addr = addr;

    // TODO: do more stuff here

    return target;
}

int SceNetAdhocMatchingContext::countTargetsWithStatusOrBetter(int status) {
    int i = 0;
    SceNetAdhocMatchingTarget *target;
    for (target = this->targets; target != nullptr; target = target->next) {
        if (target->status >= status)
            i++;
    }
    return i;
}

void SceNetAdhocMatchingContext::processPacketFromPeer(SceNetAdhocMatchingTarget *target) {
    SceNetAdhocMatchingPacketType packetType;
    memcpy(&packetType, target->rawPacket + 1, sizeof(packetType));

    SceNetAdhocMatchingMode mode = (SceNetAdhocMatchingMode)this->mode;
    if (mode == SCE_NET_ADHOC_MATCHING_MODE_PARENT) {
        // If we received any of these 2 packets and we are the parent, ignore them, we dont care
        if (packetType == SCE_NET_ADHOC_MATCHING_PACKET_TYPE_HELLO || packetType == SCE_NET_ADHOC_MATCHING_PACKET_TYPE_ADDRS)
            return;
    } else {
        if (mode != SCE_NET_ADHOC_MATCHING_MODE_CHILD) {
            bool peerIsHigherAddrThanUs = false; // TODO: not stub this to false
            if (peerIsHigherAddrThanUs) {
                if (packetType == SCE_NET_ADHOC_MATCHING_PACKET_TYPE_ADDRS)
                    return;
            } else {
                if (packetType == SCE_NET_ADHOC_MATCHING_PACKET_TYPE_UNK7)
                    return;
            }
        }
    }

    if ((packetType == SCE_NET_ADHOC_MATCHING_PACKET_TYPE_UNK2 || packetType == SCE_NET_ADHOC_MATCHING_PACKET_TYPE_UNK3) && target->rawPacketLength - target->packetLength > 15) {
        // TODO
    }

    // TODO
}

void SceNetAdhocMatchingContext::getAddressesWithStatusOrBetter(int status, SceNetInAddr *addrs, int *pCount) {
    int count = 0;
    SceNetInAddr *dst = addrs;

    for (auto target = this->targets; target != nullptr; target = target->next) {
        if (target->status >= status) {
            if (addrs) {
                if (*pCount <= count)
                    break;
                memcpy(&dst, &target->addr, sizeof(target->addr));
            }
            dst++;
            count++;
        }
    }
    *pCount = count;
}

void SceNetAdhocMatchingContext::generateAddrsMsg() {
    auto count = this->countTargetsWithStatusOrBetter(5);
    auto addrMsgLen = count * sizeof(uint32_t) + sizeof(SceNetAdhocMatchingAddrMsgStart);
    SceUShort16 totalCount = count + 1;

    SceNetAdhocMatchingAddrMsgStart *msg = (SceNetAdhocMatchingAddrMsgStart *)new char[addrMsgLen];

    msg->one = 1;
    msg->type = SCE_NET_ADHOC_MATCHING_PACKET_TYPE_ADDRS;

    auto nPacketLength = htons((totalCount & 0b0011111111111111) << 2);
    memcpy(&msg->packetLength, &nPacketLength, sizeof(nPacketLength));
    memcpy(&msg->ownAddress, &this->ownAddress, sizeof(this->ownAddress));

    getAddressesWithStatusOrBetter(5, (SceNetInAddr *)(msg + sizeof(SceNetAdhocMatchingAddrMsgStart)), &count);

    if (this->addrMsg)
        delete this->addrMsg;

    this->addrMsgLen = addrMsgLen;
    this->addrMsg = (char *)msg;
}
void SceNetAdhocMatchingContext::getMembers(unsigned int *membersNum, SceNetAdhocMatchingMember *members) {
    uint uVar1;
    int currentSize;
    uint otherI;
    SceNetAdhocMatchingMember *dst;
    int totalSize;
    SceSize i;

    // TODO: touch this so it doesnt look like trash
    totalSize = this->addrMsgLen;
    uVar1 = *membersNum;
    otherI = 0;
    currentSize = 4;
    dst = members;
    i = 0;
    if (4 < totalSize) {
        do {
            if ((members) && (otherI < uVar1)) {
                memcpy(dst, this->addrMsg + currentSize, 4);
                totalSize = this->addrMsgLen;
            }
            otherI++;
            currentSize += 4;
            dst++;
            i = otherI;
        } while (currentSize < totalSize);
    }
    *membersNum = i;
}