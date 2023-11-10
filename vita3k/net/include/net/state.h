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

#pragma once

#include <emuenv/app_util.h>
#include <net/epoll.h>
#include <net/socket.h>
#include <net/types.h>
#include <np/common.h>
#include <rtc/rtc.h>
#include <thread>
#include <util/types.h>

#include <array>

struct Socket;

typedef std::map<int, SocketPtr> NetSockets;
typedef std::map<int, EpollPtr> NetEpolls;

#define SCE_NETCTL_INFO_SSID_LEN_MAX 32
#define SCE_NETCTL_INFO_CONFIG_NAME_LEN_MAX 64

enum {
    SCE_NET_CTL_ERROR_NOT_INITIALIZED = 0x80412101,
    SCE_NET_CTL_ERROR_NOT_TERMINATED = 0x80412102,
    SCE_NET_CTL_ERROR_CALLBACK_MAX = 0x80412103,
    SCE_NET_CTL_ERROR_ID_NOT_FOUND = 0x80412104,
    SCE_NET_CTL_ERROR_INVALID_ID = 0x80412105,
    SCE_NET_CTL_ERROR_INVALID_CODE = 0x80412106,
    SCE_NET_CTL_ERROR_INVALID_ADDR = 0x80412107,
    SCE_NET_CTL_ERROR_NOT_CONNECTED = 0x80412108,
    SCE_NET_CTL_ERROR_NOT_AVAIL = 0x80412109,
    SCE_NET_CTL_ERROR_AUTO_CONNECT_DISABLED = 0x8041210a,
    SCE_NET_CTL_ERROR_AUTO_CONNECT_FAILED = 0x8041210b,
    SCE_NET_CTL_ERROR_NO_SUITABLE_SETTING_FOR_AUTO_CONNECT = 0x8041210c,
    SCE_NET_CTL_ERROR_DISCONNECTED_FOR_ADHOC_USE = 0x8041210d,
    SCE_NET_CTL_ERROR_DISCONNECT_REQ = 0x8041210e,
    SCE_NET_CTL_ERROR_INVALID_TYPE = 0x8041210f,
    SCE_NET_CTL_ERROR_AUTO_DISCONNECT = 0x80412110,
    SCE_NET_CTL_ERROR_INVALID_SIZE = 0x80412111,
    SCE_NET_CTL_ERROR_FLIGHT_MODE_ENABLED = 0x80412112,
    SCE_NET_CTL_ERROR_WIFI_DISABLED = 0x80412113,
    SCE_NET_CTL_ERROR_WIFI_IN_ADHOC_USE = 0x80412114,
    SCE_NET_CTL_ERROR_ETHERNET_PLUGOUT = 0x80412115,
    SCE_NET_CTL_ERROR_WIFI_DEAUTHED = 0x80412116,
    SCE_NET_CTL_ERROR_WIFI_BEACON_LOST = 0x80412117,
    SCE_NET_CTL_ERROR_DISCONNECTED_FOR_SUSPEND = 0x80412118,
    SCE_NET_CTL_ERROR_COMMUNICATION_ID_NOT_EXIST = 0x80412119,
    SCE_NET_CTL_ERROR_ADHOC_ALREADY_CONNECTED = 0x8041211a,
    SCE_NET_CTL_ERROR_DHCP_TIMEOUT = 0x8041211b,
    SCE_NET_CTL_ERROR_PPPOE_TIMEOUT = 0x8041211c,
    SCE_NET_CTL_ERROR_INSUFFICIENT_MEMORY = 0x8041211d,
    SCE_NET_CTL_ERROR_PSP_ADHOC_JOIN_TIMEOUT = 0x8041211e,
    SCE_NET_CTL_ERROR_UNKNOWN_DEVICE = 0x80412188
};

enum SceNetCtlState {
    SCE_NETCTL_STATE_DISCONNECTED,
    SCE_NETCTL_STATE_CONNECTING,
    SCE_NETCTL_STATE_FINALIZING,
    SCE_NETCTL_STATE_CONNECTED
};

enum SceNetCtlInfoType {
    SCE_NETCTL_INFO_GET_CNF_NAME = 1,
    SCE_NETCTL_INFO_GET_DEVICE,
    SCE_NETCTL_INFO_GET_ETHER_ADDR,
    SCE_NETCTL_INFO_GET_MTU,
    SCE_NETCTL_INFO_GET_LINK,
    SCE_NETCTL_INFO_GET_BSSID,
    SCE_NETCTL_INFO_GET_SSID,
    SCE_NETCTL_INFO_GET_WIFI_SECURITY,
    SCE_NETCTL_INFO_GET_RSSI_DBM,
    SCE_NETCTL_INFO_GET_RSSI_PERCENTAGE,
    SCE_NETCTL_INFO_GET_CHANNEL,
    SCE_NETCTL_INFO_GET_IP_CONFIG,
    SCE_NETCTL_INFO_GET_DHCP_HOSTNAME,
    SCE_NETCTL_INFO_GET_PPPOE_AUTH_NAME,
    SCE_NETCTL_INFO_GET_IP_ADDRESS,
    SCE_NETCTL_INFO_GET_NETMASK,
    SCE_NETCTL_INFO_GET_DEFAULT_ROUTE,
    SCE_NETCTL_INFO_GET_PRIMARY_DNS,
    SCE_NETCTL_INFO_GET_SECONDARY_DNS,
    SCE_NETCTL_INFO_GET_HTTP_PROXY_CONFIG,
    SCE_NETCTL_INFO_GET_HTTP_PROXY_SERVER,
    SCE_NETCTL_INFO_GET_HTTP_PROXY_PORT,
};

typedef union SceNetCtlInfo {
    char cnf_name[SCE_NETCTL_INFO_CONFIG_NAME_LEN_MAX + 1];
    unsigned int device;
    SceNetEtherAddr ether_addr;
    unsigned int mtu;
    unsigned int link;
    SceNetEtherAddr bssid;
    char ssid[SCE_NETCTL_INFO_SSID_LEN_MAX + 1];
    unsigned int wifi_security;
    unsigned int rssi_dbm;
    unsigned int rssi_percentage;
    unsigned int channel;
    unsigned int ip_config;
    char dhcp_hostname[256];
    char pppoe_auth_name[128];
    char ip_address[16];
    char netmask[16];
    char default_route[16];
    char primary_dns[16];
    char secondary_dns[16];
    unsigned int http_proxy_config;
    char http_proxy_server[256];
    unsigned int http_proxy_port;
} SceNetCtlInfo;

struct SceNetCtlNatInfo {
    SceSize size;
    int stun_status;
    int nat_type;
    SceNetInAddr mapped_addr;
};

struct SceNetCtlIfStat {
    SceSize size;
    SceUInt32 totalSec;
    SceUInt64 txBytes;
    SceUInt64 rxBytes;
    SceRtcTick resetTick;
    SceUInt32 reserved[8];
};

struct SceNetCtlAdhocPeerInfo {
    SceNetInAddr addr;
    np::SceNpId npId;
    SceUInt64 lastRecv;
    int appVer;
    SceBool isValidNpId;
    char username[SCE_SYSTEM_PARAM_USERNAME_MAXSIZE];
    uint8_t padding[7];
};

struct NetState {
    bool inited = false;
    int next_id = 0;
    NetSockets socks;
    int next_epoll_id = 0;
    NetEpolls epolls;
    int state = -1;
    int resolver_id = 0;
};

struct NetCtlState {
    std::array<SceNetCtlCallback, 8> adhocCallbacks;
    std::array<SceNetCtlCallback, 8> callbacks;
    bool inited = false;
    bool inAdhocMatchingMode = false;
    bool adhocShouldStop = false;
    std::vector<SceNetCtlAdhocPeerInfo> adhocPeers;
    std::thread adhocAuthThread;
    std::mutex mutex;
};
