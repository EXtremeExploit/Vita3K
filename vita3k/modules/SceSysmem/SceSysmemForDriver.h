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

#include <module/module.h>

BRIDGE_DECL(ksceGUIDClose)
BRIDGE_DECL(ksceGUIDReferObject)
BRIDGE_DECL(ksceGUIDReferObjectWithClass)
BRIDGE_DECL(ksceGUIDReferObjectWithClassLevel)
BRIDGE_DECL(ksceGUIDReleaseObject)
BRIDGE_DECL(ksceKernelAddressSpaceVAtoPABySW)
BRIDGE_DECL(ksceKernelAllocHeapMemory)
BRIDGE_DECL(ksceKernelAllocHeapMemoryFromGlobalHeap)
BRIDGE_DECL(ksceKernelAllocHeapMemoryFromGlobalHeapWithOpt)
BRIDGE_DECL(ksceKernelAllocHeapMemoryWithOpt1)
BRIDGE_DECL(ksceKernelAllocHeapMemoryWithOption)
BRIDGE_DECL(ksceKernelAllocMemBlock)
BRIDGE_DECL(ksceKernelAllocMemBlockWithInfo)
BRIDGE_DECL(ksceKernelCreateClass)
BRIDGE_DECL(ksceKernelCreateHeap)
BRIDGE_DECL(ksceKernelCreateUidObj2)
BRIDGE_DECL(ksceKernelCreateUidObjForUid)
BRIDGE_DECL(ksceKernelCreateUserUidForClass)
BRIDGE_DECL(ksceKernelCreateUserUidForName)
BRIDGE_DECL(ksceKernelCreateUserUidForNameWithClass)
BRIDGE_DECL(ksceKernelDeleteHeap)
BRIDGE_DECL(ksceKernelFindMemBlock)
BRIDGE_DECL(ksceKernelFindMemBlockByAddr)
BRIDGE_DECL(ksceKernelFindMemBlockByAddrForDefaultSize)
BRIDGE_DECL(ksceKernelFindMemBlockByAddrForPid)
BRIDGE_DECL(ksceKernelFindMemBlockForPid)
BRIDGE_DECL(ksceKernelFirstDifferentBlock32User)
BRIDGE_DECL(ksceKernelFirstDifferentBlock64User)
BRIDGE_DECL(ksceKernelFirstDifferentBlock64UserForPid)
BRIDGE_DECL(ksceKernelFirstDifferentIntUserForPid)
BRIDGE_DECL(ksceKernelFreeHeapMemory)
BRIDGE_DECL(ksceKernelFreeHeapMemoryFromGlobalHeap)
BRIDGE_DECL(ksceKernelFreeMemBlock)
BRIDGE_DECL(ksceKernelGUIDGetObject)
BRIDGE_DECL(ksceKernelGetClassForPidForUid)
BRIDGE_DECL(ksceKernelGetClassForUid)
BRIDGE_DECL(ksceKernelGetMemBlockBase)
BRIDGE_DECL(ksceKernelGetMemBlockMappedBase)
BRIDGE_DECL(ksceKernelGetMemBlockPARange)
BRIDGE_DECL(ksceKernelGetMemBlockPaddrListForUid)
BRIDGE_DECL(ksceKernelGetMemBlockVBase)
BRIDGE_DECL(ksceKernelGetNameForPidByUid)
BRIDGE_DECL(ksceKernelGetNameForUid)
BRIDGE_DECL(ksceKernelGetNameForUid2)
BRIDGE_DECL(ksceKernelGetObjectForPidForUid)
BRIDGE_DECL(ksceKernelGetObjectForUidForAttr)
BRIDGE_DECL(ksceKernelGetObjectForUidForClassTree)
BRIDGE_DECL(ksceKernelGetPaddrListForLargePage)
BRIDGE_DECL(ksceKernelGetPaddrListForSmallPage)
BRIDGE_DECL(ksceKernelGetPaddrPair)
BRIDGE_DECL(ksceKernelGetPaddrPairForLargePage)
BRIDGE_DECL(ksceKernelGetPaddrPairForSmallPage)
BRIDGE_DECL(ksceKernelGetPhysicalMemoryType)
BRIDGE_DECL(ksceKernelGetPidContext)
BRIDGE_DECL(ksceKernelGetUidClass)
BRIDGE_DECL(ksceKernelIsPaddrWithinSameSectionForUid)
BRIDGE_DECL(ksceKernelKernelUidForUserUidForClass)
BRIDGE_DECL(ksceKernelMapBlockUserVisible)
BRIDGE_DECL(ksceKernelMapBlockUserVisibleWithFlag)
BRIDGE_DECL(ksceKernelMapUserBlock)
BRIDGE_DECL(ksceKernelMemBlockDecRefCounterAndReleaseUid)
BRIDGE_DECL(ksceKernelMemBlockGetInfoEx)
BRIDGE_DECL(ksceKernelMemBlockGetInfoExForVisibilityLevel)
BRIDGE_DECL(ksceKernelMemBlockGetSomeSize)
BRIDGE_DECL(ksceKernelMemBlockIncRefCounterAndReleaseUid)
BRIDGE_DECL(ksceKernelMemBlockRelease)
BRIDGE_DECL(ksceKernelMemBlockType2Memtype)
BRIDGE_DECL(ksceKernelMemBlockTypeGetPrivileges)
BRIDGE_DECL(ksceKernelMemRangeRelease)
BRIDGE_DECL(ksceKernelMemRangeReleaseForPid)
BRIDGE_DECL(ksceKernelMemRangeReleaseWithPerm)
BRIDGE_DECL(ksceKernelMemRangeRetain)
BRIDGE_DECL(ksceKernelMemRangeRetainForPid)
BRIDGE_DECL(ksceKernelMemRangeRetainWithPerm)
BRIDGE_DECL(ksceKernelMemcpyKernelToUser)
BRIDGE_DECL(ksceKernelMemcpyKernelToUserForPid)
BRIDGE_DECL(ksceKernelMemcpyKernelToUserForPidUnchecked)
BRIDGE_DECL(ksceKernelMemcpyUserToKernel)
BRIDGE_DECL(ksceKernelMemcpyUserToKernelForPid)
BRIDGE_DECL(ksceKernelMemcpyUserToUser)
BRIDGE_DECL(ksceKernelMemcpyUserToUserForPid)
BRIDGE_DECL(ksceKernelOpenUidForName)
BRIDGE_DECL(ksceKernelProcModeVAtoPA)
BRIDGE_DECL(ksceKernelProcUserMap)
BRIDGE_DECL(ksceKernelRemapBlock)
BRIDGE_DECL(ksceKernelRoMemcpyKernelToUserForPid)
BRIDGE_DECL(ksceKernelSetNameForPidForUid)
BRIDGE_DECL(ksceKernelSetObjectForUid)
BRIDGE_DECL(ksceKernelStrncpyKernelToUser)
BRIDGE_DECL(ksceKernelStrncpyUserForPid)
BRIDGE_DECL(ksceKernelStrncpyUserToKernel)
BRIDGE_DECL(ksceKernelStrnlenUser)
BRIDGE_DECL(ksceKernelStrnlenUserForPid)
BRIDGE_DECL(ksceKernelSwitchPidContext)
BRIDGE_DECL(ksceKernelSwitchVmaForPid)
BRIDGE_DECL(ksceKernelUnmapMemBlock)
BRIDGE_DECL(ksceKernelUserMap)
BRIDGE_DECL(ksceKernelVARangeToPAVector)
BRIDGE_DECL(ksceKernelVAtoPA)
BRIDGE_DECL(kscePUIDClose)
BRIDGE_DECL(kscePUIDOpenByGUID)
BRIDGE_DECL(kscePUIDtoGUID)
