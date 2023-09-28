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

#include <module/module.h>

#include <kernel/state.h>
#include <net/state.h>
#include <net/types.h>
#include <rtc/rtc.h>
#include <util/lock_and_find.h>
#include <util/log.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#endif

#include <util/tracy.h>
TRACY_MODULE_NAME(SceNetCtl);

struct SceNetCtlAdhocPeerInfo;

EXPORT(int, sceNetCtlAdhocDisconnect) {
    TRACY_FUNC(sceNetCtlAdhocDisconnect);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetCtlAdhocGetPeerList, SceSize *peerInfoNum, SceNetCtlAdhocPeerInfo *peerInfo) {
    TRACY_FUNC(sceNetCtlAdhocGetPeerList, peerInfoNum, peerInfo);
    if (!peerInfoNum) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetCtlAdhocGetResult, int eventType, int *errorCode) {
    TRACY_FUNC(sceNetCtlAdhocGetResult, eventType, errorCode);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!errorCode) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    *errorCode = 0;
    return 0;
}

EXPORT(int, sceNetCtlAdhocGetState, int *state) {
    TRACY_FUNC(sceNetCtlAdhocGetState, state);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!state) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    *state = SCE_NETCTL_STATE_CONNECTED;
    return STUBBED("state = SCE_NETCTL_STATE_CONNECTED");
}

EXPORT(int, sceNetCtlAdhocRegisterCallback, Ptr<void> func, Ptr<void> arg, int *cid) {
    TRACY_FUNC(sceNetCtlAdhocRegisterCallback, func, arg, cid);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!func || !cid) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    const std::lock_guard<std::mutex> lock(emuenv.netctl.mutex);

    // Find the next available slot
    int next_id = 0;
    for (const auto &callback : emuenv.netctl.adhocCallbacks) {
        if (callback.pc == 0) {
            break;
        }
        next_id++;
    }

    if (next_id == 8) {
        return RET_ERROR(SCE_NET_CTL_ERROR_CALLBACK_MAX);
    }

    emuenv.netctl.adhocCallbacks[next_id].pc = func.address();
    emuenv.netctl.adhocCallbacks[next_id].arg = arg.address();
    *cid = next_id;
    return 0;
}

EXPORT(int, sceNetCtlAdhocUnregisterCallback, int cid) {
    TRACY_FUNC(sceNetCtlAdhocUnregisterCallback, cid);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if ((cid < 0) || (cid >= 8)) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ID);
    }

    const std::lock_guard<std::mutex> lock(emuenv.netctl.mutex);
    emuenv.netctl.adhocCallbacks[cid].pc = 0;
    emuenv.netctl.adhocCallbacks[cid].arg = 0;
    return 0;
}

EXPORT(int, sceNetCtlCheckCallback) {
    TRACY_FUNC(sceNetCtlCheckCallback);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (emuenv.net.state == 1) {
        return 0;
    }

    emuenv.net.state = 1;

    const ThreadStatePtr thread = lock_and_find(thread_id, emuenv.kernel.threads, emuenv.kernel.mutex);

    // TODO: Limit the number of callbacks called to 5
    // TODO: Check in which order the callbacks are executed

    for (auto &callback : emuenv.netctl.callbacks) {
        if (callback.pc != 0) {
            thread->run_callback(callback.pc, { SCE_NET_CTL_EVENT_TYPE_DISCONNECTED, callback.arg });
        }
    }

    for (auto &callback : emuenv.netctl.adhocCallbacks) {
        if (callback.pc != 0) {
            thread->run_callback(callback.pc, { SCE_NET_CTL_EVENT_TYPE_DISCONNECTED, callback.arg });
        }
    }

    return STUBBED("Stub");
}

EXPORT(int, sceNetCtlGetIfStat, int device, SceNetCtlIfStat *ifstat) {
    TRACY_FUNC(sceNetCtlGetIfStat, device, ifstat);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!ifstat) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    if (ifstat->size != sizeof(SceNetCtlIfStat)) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_SIZE);
    }

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetCtlGetNatInfo, SceNetCtlNatInfo *natinfo) {
    TRACY_FUNC(sceNetCtlGetNatInfo, natinfo);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!natinfo) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    return UNIMPLEMENTED();
}

EXPORT(int, sceNetCtlGetPhoneMaxDownloadableSize, SceInt64 *maxDownloadableSize) {
    TRACY_FUNC(sceNetCtlGetPhoneMaxDownloadableSize, maxDownloadableSize);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!maxDownloadableSize) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    *maxDownloadableSize = 0x7fffffffffffffffLL; // Unlimited
    return STUBBED("maxDownloadableSize = Unlimited");
}

EXPORT(int, sceNetCtlInetGetInfo, int code, SceNetCtlInfo *info) {
    TRACY_FUNC(sceNetCtlInetGetInfo, code, info);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!info) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    switch (code) {
    case SCE_NETCTL_INFO_GET_IP_ADDRESS: {
        strcpy(info->ip_address, "127.0.0.1"); // placeholder in case gethostbyname can't find another ip
#ifdef _WIN32
        // TODO: windows has its own functions for getting the ipv4 addr of the host in a local network
        // what we are doing here right now is not enough filtering and its probably better to use them
        char devname[80];
        gethostname(devname, 80);
        struct hostent *resolved = gethostbyname(devname);
        for (int i = 0; resolved->h_addr_list[i] != nullptr; ++i) {
            struct in_addr addrIn;
            memcpy(&addrIn, resolved->h_addr_list[i], sizeof(uint32_t));
            char *addr = inet_ntoa(addrIn);
            if (strcmp(addr, "127.0.0.1") != 0) {
                strcpy(info->ip_address, addr);
                break;
            }
        }
#else
        struct ifaddrs *ifAddrStruct = NULL;
        struct ifaddrs *ifa = NULL;
        void *tmpAddrPtr = NULL;

        getifaddrs(&ifAddrStruct);

        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) {
                continue;
            }

            if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
                continue;
            if (ifa->ifa_flags)
                if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
                    // is a valid IP4 Address
                    tmpAddrPtr = &((sockaddr_in *)ifa->ifa_addr)->sin_addr;
                    char addressBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                    strcpy(info->ip_address, addressBuffer);
                    break;
                }
        }
        if (ifAddrStruct != NULL)
            freeifaddrs(ifAddrStruct);
#endif
        break;
    }
    case SCE_NETCTL_INFO_GET_DEVICE:
        info->device = 0; /*SCE_NET_CTL_DEVICE_WIRELESS*/
        // STUBBED("SCE_NETCTL_INFO_GET_DEVICE return SCE_NET_CTL_DEVICE_WIRELESS");
        break;
    case SCE_NETCTL_INFO_GET_RSSI_PERCENTAGE:
        info->rssi_percentage = 100;
        // STUBBED("code SCE_NETCTL_INFO_GET_RSSI_PERCENTAGE return 100%");
        break;
    default:
        switch (code) {
        case SCE_NETCTL_INFO_GET_CNF_NAME:
            STUBBED("code SCE_NETCTL_INFO_GET_CNF_NAME not implemented");
            break;
        case SCE_NETCTL_INFO_GET_ETHER_ADDR:
            STUBBED("code SCE_NETCTL_INFO_GET_ETHER_ADDR not implemented");
            break;
        case SCE_NETCTL_INFO_GET_MTU:
            STUBBED("code SCE_NETCTL_INFO_GET_MTU not implemented");
            break;
        case SCE_NETCTL_INFO_GET_LINK:
            STUBBED("code SCE_NETCTL_INFO_GET_LINK not implemented");
            break;
        case SCE_NETCTL_INFO_GET_BSSID:
            STUBBED("code SCE_NETCTL_INFO_GET_BSSID not implemented");
            break;
        case SCE_NETCTL_INFO_GET_SSID:
            STUBBED("code SCE_NETCTL_INFO_GET_SSID not implemented");
            break;
        case SCE_NETCTL_INFO_GET_WIFI_SECURITY:
            STUBBED("code SCE_NETCTL_INFO_GET_WIFI_SECURITY not implemented");
            break;
        case SCE_NETCTL_INFO_GET_RSSI_DBM:
            STUBBED("code SCE_NETCTL_INFO_GET_RSSI_DBM not implemented");
            break;
        case SCE_NETCTL_INFO_GET_CHANNEL:
            STUBBED("code SCE_NETCTL_INFO_GET_CHANNEL not implemented");
            break;
        case SCE_NETCTL_INFO_GET_IP_CONFIG:
            STUBBED("code SCE_NETCTL_INFO_GET_IP_CONFIG not implemented");
            break;
        case SCE_NETCTL_INFO_GET_DHCP_HOSTNAME:
            STUBBED("code SCE_NETCTL_INFO_GET_DHCP_HOSTNAME not implemented");
            break;
        case SCE_NETCTL_INFO_GET_PPPOE_AUTH_NAME:
            STUBBED("code SCE_NETCTL_INFO_GET_PPPOE_AUTH_NAME not implemented");
            break;
        case SCE_NETCTL_INFO_GET_NETMASK:
            STUBBED("code SCE_NETCTL_INFO_GET_NETMASK not implemented");
            break;
        case SCE_NETCTL_INFO_GET_DEFAULT_ROUTE:
            STUBBED("code SCE_NETCTL_INFO_GET_DEFAULT_ROUTE not implemented");
            break;
        case SCE_NETCTL_INFO_GET_PRIMARY_DNS:
            STUBBED("code SCE_NETCTL_INFO_GET_PRIMARY_DNS not implemented");
            break;
        case SCE_NETCTL_INFO_GET_SECONDARY_DNS:
            STUBBED("code SCE_NETCTL_INFO_GET_SECONDARY_DNS not implemented");
            break;
        case SCE_NETCTL_INFO_GET_HTTP_PROXY_CONFIG:
            STUBBED("code SCE_NETCTL_INFO_GET_HTTP_PROXY_CONFIG not implemented");
            break;
        case SCE_NETCTL_INFO_GET_HTTP_PROXY_SERVER:
            STUBBED("code SCE_NETCTL_INFO_GET_HTTP_PROXY_SERVER not implemented");
            break;
        case SCE_NETCTL_INFO_GET_HTTP_PROXY_PORT:
            STUBBED("code SCE_NETCTL_INFO_GET_HTTP_PROXY_PORT not implemented");
            break;
        default:
            LOG_ERROR("Unknown code:{}", log_hex(code));
        }
    }
    return 0;
}

EXPORT(int, sceNetCtlAdhocGetInAddr, SceNetInAddr *inaddr) {
    TRACY_FUNC(sceNetCtlAdhocGetInAddr, inaddr);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!inaddr) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    SceNetCtlInfo info;
    CALL_EXPORT(sceNetCtlInetGetInfo, SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
    if(std::string_view(info.ip_address).starts_with("127."))
        return RET_ERROR(-1); // TODO
        


    inet_pton(AF_INET, info.ip_address, &inaddr->s_addr);
    return 0;
}

EXPORT(int, sceNetCtlInetGetResult, int eventType, int *errorCode) {
    TRACY_FUNC(sceNetCtlInetGetResult, eventType, errorCode);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!errorCode) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    *errorCode = 0;
    return 0;
}

EXPORT(int, sceNetCtlInetGetState, int *state) {
    TRACY_FUNC(sceNetCtlInetGetState, state);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!state) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    *state = SCE_NETCTL_STATE_CONNECTED;
    return STUBBED("state = SCE_NETCTL_STATE_CONNECTED");
}

EXPORT(int, sceNetCtlInetRegisterCallback, Ptr<void> func, Ptr<void> arg, int *cid) {
    TRACY_FUNC(sceNetCtlInetRegisterCallback, func, arg, cid);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if (!func || !cid) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ADDR);
    }

    const std::lock_guard<std::mutex> lock(emuenv.netctl.mutex);

    // Find the next available slot
    int next_id = 0;
    for (const auto &callback : emuenv.netctl.callbacks) {
        if (callback.pc == 0) {
            break;
        }
        next_id++;
    }

    if (next_id == 8) {
        return RET_ERROR(SCE_NET_CTL_ERROR_CALLBACK_MAX);
    }

    emuenv.netctl.callbacks[next_id].pc = func.address();
    emuenv.netctl.callbacks[next_id].arg = arg.address();
    *cid = next_id;
    return 0;
}

EXPORT(int, sceNetCtlInetUnregisterCallback, int cid) {
    TRACY_FUNC(sceNetCtlInetUnregisterCallback, cid);
    if (!emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_INITIALIZED);
    }

    if ((cid < 0) || (cid >= 8)) {
        return RET_ERROR(SCE_NET_CTL_ERROR_INVALID_ID);
    }

    const std::lock_guard<std::mutex> lock(emuenv.netctl.mutex);
    emuenv.netctl.callbacks[cid].pc = 0;
    emuenv.netctl.callbacks[cid].arg = 0;

    return 0;
}

EXPORT(int, sceNetCtlInit) {
    TRACY_FUNC(sceNetCtlInit);
    if (emuenv.netctl.inited) {
        return RET_ERROR(SCE_NET_CTL_ERROR_NOT_TERMINATED);
    }

    const std::lock_guard<std::mutex> lock(emuenv.netctl.mutex);
    emuenv.netctl.adhocCallbacks.fill({ 0, 0 });
    emuenv.netctl.callbacks.fill({ 0, 0 });

    emuenv.netctl.inited = true;
    return STUBBED("Stub");
}

EXPORT(void, sceNetCtlTerm) {
    TRACY_FUNC(sceNetCtlTerm);
    STUBBED("Stub");
    emuenv.netctl.inited = false;
}
