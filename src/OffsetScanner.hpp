#pragma once
#include <cstdint>

#include "MemoryManager.hpp"
#include "client_dll.hpp"
#include "offsets.hpp"

namespace offsets = cs2_dumper::offsets::client_dll;
namespace schemas = cs2_dumper::schemas::client_dll;

extern MemoryManager mem;
extern uintptr_t module_base;
extern size_t module_size;

extern uintptr_t g_ovEntityList;
extern uintptr_t g_ovLocalController;
extern uintptr_t g_ovViewMatrix;

struct ScannedOffset {
    int32_t value = 0;
    bool valid = false;
};

extern ScannedOffset g_fldTeamNum;
extern ScannedOffset g_fldHealth;
extern ScannedOffset g_fldGameSceneNode;
extern ScannedOffset g_fldPlayerPawnHandle;
extern ScannedOffset g_fldModelState;
extern ScannedOffset g_fldAttributeManager;
extern ScannedOffset g_fldItem;
extern ScannedOffset g_fldItemDefinitionIndex;
extern ScannedOffset g_fldWeaponServices;
extern ScannedOffset g_fldActiveWeapon;
extern ScannedOffset g_fldEntitySpottedState;

int32_t OffTeamNum();
int32_t OffHealth();
int32_t OffGameSceneNode();
int32_t OffPlayerPawnHandle();
int32_t OffModelState();
int32_t OffEntitySpottedState();
int32_t OffAttributeManager();
int32_t OffItem();
int32_t OffItemDefinitionIndex();
int32_t OffWeaponServices();
int32_t OffActiveWeapon();

void RefreshOffsets(uintptr_t base, size_t size);
