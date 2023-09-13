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

#pragma once

#include <emuenv/state.h>
#include <kernel/types.h>
#include <mem/ptr.h>
#include <module/module.h>
#include <net/types.h>
#include <util/types.h>

#include <map>
#include <string>
#include <vector>

#define SCE_NET_ADHOC_MATCHING_MAXNUM			16
#define SCE_NET_ADHOC_MATCHING_MAXOPTLEN		9196
#define SCE_NET_ADHOC_MATCHING_MAXDATALEN		9204
#define SCE_NET_ADHOC_MATCHING_MAXHELLOOPTLEN	1426


DECL_EXPORT(SceInt32, sceNetCtlAdhocGetInAddr, SceNetInAddr *inaddr);
DECL_EXPORT(SceInt32, sceNetSocket, const char *name, int domain, SceNetSocketType type, SceNetProtocol protocol);
DECL_EXPORT(unsigned short int, sceNetHtons, unsigned short int n);
DECL_EXPORT(int, sceNetBind, int sid, const SceNetSockaddr *addr, unsigned int addrlen);
DECL_EXPORT(int, sceNetSetsockopt, int sid, SceNetProtocol level, SceNetSocketOption optname, const int *optval, unsigned int optlen);
DECL_EXPORT(int, sceNetGetMacAddress, SceNetEtherAddr *addr, int flags);
DECL_EXPORT(int, sceNetSocketClose, int sid);
DECL_EXPORT(int, sceNetShutdown, int eid, int how);
DECL_EXPORT(int, sceKernelCreateMsgPipe, const char *name, uint32_t type, uint32_t attr, SceSize bufSize, const SceKernelCreateMsgPipeOpt *opt);

enum SceNetAdhocMatchingErrorCode {
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MODE = 0x80413101,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_PORT = 0x80413102,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_MAXNUM = 0x80413103,
    SCE_NET_ADHOC_MATCHING_ERROR_RXBUF_TOO_SHORT = 0x80413104,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_OPTLEN = 0x80413105,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ARG = 0x80413106,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ID = 0x80413107,
    SCE_NET_ADHOC_MATCHING_ERROR_ID_NOT_AVAIL = 0x80413108,
    SCE_NET_ADHOC_MATCHING_ERROR_NO_SPACE = 0x80413109,
    SCE_NET_ADHOC_MATCHING_ERROR_IS_RUNNING = 0x8041310a,
    SCE_NET_ADHOC_MATCHING_ERROR_NOT_RUNNING = 0x8041310b,
    SCE_NET_ADHOC_MATCHING_ERROR_UNKNOWN_TARGET = 0x8041310c,
    SCE_NET_ADHOC_MATCHING_ERROR_TARGET_NOT_READY = 0x8041310d,
    SCE_NET_ADHOC_MATCHING_ERROR_EXCEED_MAXNUM = 0x8041310e,
    SCE_NET_ADHOC_MATCHING_ERROR_REQUEST_IN_PROGRESS = 0x8041310f,
    SCE_NET_ADHOC_MATCHING_ERROR_ALREADY_ESTABLISHED = 0x80413110,
    SCE_NET_ADHOC_MATCHING_ERROR_BUSY = 0x80413111,
    SCE_NET_ADHOC_MATCHING_ERROR_ALREADY_INITIALIZED = 0x80413112,
    SCE_NET_ADHOC_MATCHING_ERROR_NOT_INITIALIZED = 0x80413113,
    SCE_NET_ADHOC_MATCHING_ERROR_PORT_IN_USE = 0x80413114,
    SCE_NET_ADHOC_MATCHING_ERROR_STACKSIZE_TOO_SHORT = 0x80413115,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_DATALEN = 0x80413116,
    SCE_NET_ADHOC_MATCHING_ERROR_NOT_ESTABLISHED = 0x80413117,
    SCE_NET_ADHOC_MATCHING_ERROR_DATA_BUSY = 0x80413118,
    SCE_NET_ADHOC_MATCHING_ERROR_INVALID_ALIGNMENT = 0x80413119
};

enum SceAdhocMatchingMode {
    SCE_ADHOC_MATCHING_MODE_P2P,
    SCE_ADHOC_MATCHING_MODE_PARENT,
    SCE_ADHOC_MATCHING_MODE_CHILD,
    SCE_ADHOC_MATCHING_MODE_UDP,
    SCE_ADHOC_MATCHING_MODE_MAX
};

enum AdhocMatchingEventType {
    SCE_NET_ADHOC_MATCHING_EVENT_HELLO = 1,
    SCE_NET_ADHOC_MATCHING_EVENT_REQUEST = 2,
    SCE_NET_ADHOC_MATCHING_EVENT_LEAVE = 3,
    SCE_NET_ADHOC_MATCHING_EVENT_DENY = 4,
    SCE_NET_ADHOC_MATCHING_EVENT_CANCEL = 5,
    SCE_NET_ADHOC_MATCHING_EVENT_ACCEPT = 6,
    SCE_NET_ADHOC_MATCHING_EVENT_ESTABLISHED = 7,
    SCE_NET_ADHOC_MATCHING_EVENT_TIMEOUT = 8,
    SCE_NET_ADHOC_MATCHING_EVENT_ERROR = 9,
    SCE_NET_ADHOC_MATCHING_EVENT_BYE = 10,
    SCE_NET_ADHOC_MATCHING_EVENT_DATA = 11,
    SCE_NET_ADHOC_MATCHING_EVENT_DATA_ACK = 12,
    SCE_NET_ADHOC_MATCHING_EVENT_DATA_TIMEOUT = 13
};

struct SceNetAdhocMatchingHandler {
    Ptr<void> entry;
};

struct SceNetAdhocMatchingContext {
    SceNetAdhocMatchingContext *next;
    int id;
    int mode;
    int maxnum;
    SceUShort16 port;
    int rxbuflen;
    void *rxbuf;
    unsigned int helloInterval;
    unsigned int keepAliveInterval;
    int initCount;
    unsigned int rexmtInterval;
    SceNetAdhocMatchingHandler handler;
    int socket;
    SceNetEtherAddr mac;
    std::thread matchingEventThread;
    std::thread inputThread;
    SceUID msgPipeUID;
    int matchingRecvSocket;

    int initSendSocket(EmuEnvState &emuenv, SceUID thread_id, const char *export_name);
    int initEventHandler(EmuEnvState &emuenv, SceUID thread_id, const char *export_name);
    int initInputRecv(EmuEnvState &emuenv, SceUID thread_id, const char *export_name);
};

struct AdhocState {
    bool inited = false;
    SceUID next_uid = 0;
    SceNetInAddr addr;
    SceNetAdhocMatchingContext *adhocMatchingContextsList = NULL;
    SceUID matchingCtxCount = 1;

    SceNetAdhocMatchingContext *findMatchingContext(int id);
    int createAdhocMatchingContext(SceUShort16 port);
};
