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

#include <emuenv/app_util.h>
#include <emuenv/state.h>
#include <io/state.h>
#include <net/functions.h>
#include <net/state.h>
#include <netinet/in.h>
#include <np/common.h>
#include <sys/socket.h>
#include <util/log.h>

#include <cstring>
#include <unistd.h>

bool init(NetState &state) {
    return true;
}

void adhocAuthThread(EmuEnvState *emuenv) {
    const int recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
    // const int sendSocket = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in bindAddr = {};
    bindAddr.sin_port = htons(33333);
    if (bind(recvSocket, (sockaddr *)&bindAddr, sizeof(bindAddr)) < 0) {
        LOG_CRITICAL("Could not bind adhoc recv socket to port 33333. Adhoc will not work");
        return;
    }
    // 1 second timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(recvSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    while (!emuenv->netctl.adhocShouldStop) {
        int bytes = 0;
        char buf[1024];
        sockaddr addr;
        socklen_t addrLen;

        do {
            sockaddr tmp_addr;
            socklen_t tmp_addrLen = sizeof(tmp_addr);
            memset(buf, 0, sizeof(buf));
            int tmp_bytes = recvfrom(recvSocket, buf, sizeof(buf), 0, &tmp_addr, &tmp_addrLen);
            if (tmp_bytes > 0) {
                char addrStr[17];
                inet_ntop(AF_INET, &((sockaddr_in *)&tmp_addr)->sin_addr, addrStr, sizeof(addrStr));
                LOG_CRITICAL("Received {} bytes from {}", tmp_bytes, addrStr);

                memcpy(&addr, &tmp_addr, sizeof(addr));
                memcpy(&addrLen, &tmp_addrLen, sizeof(addrLen));

                break;
            }
            bytes = tmp_bytes;
        } while (bytes > -1);

        if (std::string_view(buf) == "Hello, tell me about you c:") {
            LOG_CRITICAL("Received about request");
            SceNetCtlAdhocPeerInfo info;
            info.addr.s_addr = 0; // Set this 0, receiving end can fill it

            np::SceNpId npId;
            std::strncpy(npId.handle.data, emuenv->io.user_name.c_str(), SCE_SYSTEM_PARAM_USERNAME_MAXSIZE);
            npId.handle.term = '\0';
            std::fill(npId.handle.dummy, npId.handle.dummy + 3, 0);

            // Fill the unused stuffs to 0 (prevent some weird things happen)
            std::fill(npId.opt, npId.opt + 8, 0);
            std::fill(npId.reserved, npId.reserved + 8, 0);

            info.npId = npId;

            info.lastRecv = 0; // Doesnt matter, this is the server side
            info.appVer = 100; // TODO
            info.isValidNpId = true; // stub
            strcpy(info.username, npId.handle.data); // Stub
            std::strncpy(info.username, npId.handle.data, SCE_SYSTEM_PARAM_USERNAME_MAXSIZE);
            std::fill(info.padding, info.padding + sizeof(info.padding), 0);

            // Basically reply with the info of this instance
            auto res = sendto(recvSocket, &info, sizeof(info), 0, &addr, addrLen);
            if (res < 0) {
                char addrStr[17];
                inet_ntop(AF_INET, &((sockaddr_in *)&addr)->sin_addr, addrStr, sizeof(addrStr));
                auto port = ntohs(((sockaddr_in *)&addr)->sin_port);
                LOG_CRITICAL("Could not send own peer adhoc info to {}:{}, res = {}", addrStr, port, res);
                assert(false);
            }
        }
    }
}
