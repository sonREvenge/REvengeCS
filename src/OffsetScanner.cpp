#include "OffsetScanner.hpp"

#include <fstream>
#include <string>
#include <vector>
#include <set>

MemoryManager mem;
uintptr_t module_base = 0;
size_t module_size = 0;

static const char* kOffsFile = "revengecs_offsets.cfg";
uintptr_t g_ovEntityList = 0;
uintptr_t g_ovLocalController = 0;
uintptr_t g_ovViewMatrix = 0;

ScannedOffset g_fldTeamNum;
ScannedOffset g_fldHealth;
ScannedOffset g_fldGameSceneNode;
ScannedOffset g_fldPlayerPawnHandle;
ScannedOffset g_fldModelState;
ScannedOffset g_fldAttributeManager;
ScannedOffset g_fldItem;
ScannedOffset g_fldItemDefinitionIndex;
ScannedOffset g_fldWeaponServices;
ScannedOffset g_fldActiveWeapon;
ScannedOffset g_fldEntitySpottedState;

int32_t OffTeamNum()          { return g_fldTeamNum.valid ? g_fldTeamNum.value : schemas::C_BaseEntity::m_iTeamNum; }
int32_t OffHealth()           { return g_fldHealth.valid ? g_fldHealth.value : schemas::C_BaseEntity::m_iHealth; }
int32_t OffGameSceneNode()    { return g_fldGameSceneNode.valid ? g_fldGameSceneNode.value : schemas::C_BaseEntity::m_pGameSceneNode; }
int32_t OffPlayerPawnHandle() { return g_fldPlayerPawnHandle.valid ? g_fldPlayerPawnHandle.value : schemas::CCSPlayerController::m_hPlayerPawn; }
int32_t OffModelState()       { return g_fldModelState.valid ? g_fldModelState.value : schemas::CSkeletonInstance::m_modelState; }
int32_t OffAttributeManager() { return g_fldAttributeManager.valid ? g_fldAttributeManager.value : schemas::C_EconEntity::m_AttributeManager; }
int32_t OffItem()             { return g_fldItem.valid ? g_fldItem.value : schemas::C_AttributeContainer::m_Item; }
int32_t OffItemDefinitionIndex() { return g_fldItemDefinitionIndex.valid ? g_fldItemDefinitionIndex.value : schemas::C_EconItemView::m_iItemDefinitionIndex; }
int32_t OffWeaponServices()   { return g_fldWeaponServices.valid ? g_fldWeaponServices.value : schemas::C_BasePlayerPawn::m_pWeaponServices; }
int32_t OffActiveWeapon()     { return g_fldActiveWeapon.valid ? g_fldActiveWeapon.value : schemas::CPlayer_WeaponServices::m_hActiveWeapon; }
int32_t OffEntitySpottedState() { return g_fldEntitySpottedState.valid ? g_fldEntitySpottedState.value : schemas::C_CSPlayerPawn::m_entitySpottedState; }

static void LoadOffsets() {
    std::ifstream f(kOffsFile); if (!f) return;
    std::string name; unsigned long long val;
    while (f >> name >> std::hex >> val) {
        if (name == "dwEntityList") g_ovEntityList = (uintptr_t)val;
        else if (name == "dwLocalPlayerController") g_ovLocalController = (uintptr_t)val;
        else if (name == "dwViewMatrix") g_ovViewMatrix = (uintptr_t)val;
    }
}

static std::string ReadCString(uintptr_t addr, size_t maxLen) {
    if (!addr) return {};
    std::vector<char> buf(maxLen + 1, 0);
    mem.ReadBytes(addr, buf.data(), maxLen);
    buf[maxLen] = 0;
    return std::string(buf.data());
}

static uintptr_t ScanRipRelative(uintptr_t base, size_t size, const std::vector<int>& prefix, const std::vector<int>& suffix) {
    std::vector<int> pattern;
    pattern.reserve(prefix.size() + 4 + suffix.size());
    pattern.insert(pattern.end(), prefix.begin(), prefix.end());
    pattern.insert(pattern.end(), 4, -1);
    pattern.insert(pattern.end(), suffix.begin(), suffix.end());

    uintptr_t matchAddr = mem.FindPattern(base, base + size, pattern);
    if (!matchAddr) return 0;

    uintptr_t dispAddr = matchAddr + prefix.size();
    int32_t disp = mem.Read<int32_t>(dispAddr);
    uintptr_t ripEnd = dispAddr + sizeof(int32_t);
    return ripEnd + (intptr_t)disp;
}

static std::vector<uintptr_t> WalkClassBindings(uintptr_t hashAddr) {
    std::vector<uintptr_t> result;
    std::set<uintptr_t> seen;

    int32_t blocksAllocated = mem.Read<int32_t>(hashAddr + 0xC);
    int32_t peakAllocated = mem.Read<int32_t>(hashAddr + 0x10);

    constexpr int kBucketCount = 256;
    constexpr uintptr_t kBucketsStart = 0x60;
    constexpr uintptr_t kBucketSize = 0x18;
    constexpr int kMaxChainLen = 4096;

    for (int b = 0; b < kBucketCount && (int32_t)result.size() < blocksAllocated; b++) {
        uintptr_t nodePtr = mem.Read<uint64_t>(hashAddr + kBucketsStart + b * kBucketSize + 0x10);
        for (int guard = 0; nodePtr && guard < kMaxChainLen; guard++) {
            uintptr_t data = mem.Read<uint64_t>(nodePtr + 0x10);
            uintptr_t next = mem.Read<uint64_t>(nodePtr + 0x8);
            if (data && seen.insert(data).second) result.push_back(data);
            if ((int32_t)result.size() >= blocksAllocated) break;
            nodePtr = next;
        }
    }

    uintptr_t blobPtr = mem.Read<uint64_t>(hashAddr + 0x20);
    int unallocCollected = 0;
    for (int guard = 0; blobPtr && guard < kMaxChainLen && unallocCollected < peakAllocated; guard++) {
        uintptr_t data = mem.Read<uint64_t>(blobPtr + 0x10);
        uintptr_t next = mem.Read<uint64_t>(blobPtr + 0x0);
        if (data && seen.insert(data).second) result.push_back(data);
        unallocCollected++;
        blobPtr = next;
    }

    return result;
}

static uintptr_t FindSchemaSystem() {
    ModuleInfo schemaModule = mem.GetModuleInfo(L"schemasystem.dll");
    if (!schemaModule.base) return 0;
    return ScanRipRelative(schemaModule.base, schemaModule.size, { 0x4C, 0x8D, 0x35 }, { 0x0F, 0x28, 0x45 });
}

static uintptr_t FindClientTypeScope(uintptr_t schemaSystemAddr) {
    int32_t count = mem.Read<int32_t>(schemaSystemAddr + 0x190);
    uintptr_t arrayBase = mem.Read<uint64_t>(schemaSystemAddr + 0x198);
    if (!arrayBase || count <= 0 || count > 64) return 0;

    for (int32_t i = 0; i < count; i++) {
        uintptr_t typeScopePtr = mem.Read<uint64_t>(arrayBase + i * 8);
        if (!typeScopePtr) continue;
        if (ReadCString(typeScopePtr + 0x8, 32) == "client.dll") return typeScopePtr;
    }
    return 0;
}

static uintptr_t FindClassBinding(uintptr_t typeScopePtr, const char* className) {
    for (uintptr_t bindingAddr : WalkClassBindings(typeScopePtr + 0x560)) {
        uintptr_t nameAddr = mem.Read<uint64_t>(bindingAddr + 0x8);
        if (ReadCString(nameAddr, 64) == className) return bindingAddr;
    }
    return 0;
}

static bool FindSchemaField(uintptr_t classBindingAddr, const char* fieldName, int32_t& outOffset) {
    int16_t fieldCount = mem.Read<int16_t>(classBindingAddr + 0x24);
    uintptr_t fieldsBase = mem.Read<uint64_t>(classBindingAddr + 0x30);
    if (!fieldsBase || fieldCount <= 0 || fieldCount > 1024) return false;

    for (int16_t i = 0; i < fieldCount; i++) {
        uintptr_t fieldAddr = fieldsBase + i * 0x20;
        uintptr_t nameAddr = mem.Read<uint64_t>(fieldAddr + 0x0);
        if (ReadCString(nameAddr, 64) == fieldName) {
            outOffset = mem.Read<int32_t>(fieldAddr + 0x10);
            return true;
        }
    }
    return false;
}

static void RefreshSchemaOffsets() {
    uintptr_t schemaSystemAddr = FindSchemaSystem();
    if (!schemaSystemAddr) return;

    uintptr_t typeScope = FindClientTypeScope(schemaSystemAddr);
    if (!typeScope) return;

    int32_t offset;

    if (uintptr_t baseEntity = FindClassBinding(typeScope, "C_BaseEntity")) {
        if (FindSchemaField(baseEntity, "m_iTeamNum", offset))        g_fldTeamNum = { offset, true };
        if (FindSchemaField(baseEntity, "m_iHealth", offset))         g_fldHealth = { offset, true };
        if (FindSchemaField(baseEntity, "m_pGameSceneNode", offset))  g_fldGameSceneNode = { offset, true };
    }
    if (uintptr_t controller = FindClassBinding(typeScope, "CCSPlayerController")) {
        if (FindSchemaField(controller, "m_hPlayerPawn", offset))     g_fldPlayerPawnHandle = { offset, true };
    }
    if (uintptr_t skeleton = FindClassBinding(typeScope, "CSkeletonInstance")) {
        if (FindSchemaField(skeleton, "m_modelState", offset))        g_fldModelState = { offset, true };
    }
    if (uintptr_t csPlayer = FindClassBinding(typeScope, "C_CSPlayerPawn")) {
        if (FindSchemaField(csPlayer, "m_entitySpottedState", offset)) g_fldEntitySpottedState = { offset, true };
    }
    if (uintptr_t econEntity = FindClassBinding(typeScope, "C_EconEntity")) {
        if (FindSchemaField(econEntity, "m_AttributeManager", offset))  g_fldAttributeManager = { offset, true };
    }
    if (uintptr_t attrContainer = FindClassBinding(typeScope, "C_AttributeContainer")) {
        if (FindSchemaField(attrContainer, "m_Item", offset))           g_fldItem = { offset, true };
    }
    if (uintptr_t econItemView = FindClassBinding(typeScope, "C_EconItemView")) {
        if (FindSchemaField(econItemView, "m_iItemDefinitionIndex", offset)) g_fldItemDefinitionIndex = { offset, true };
    }
    if (uintptr_t playerPawn = FindClassBinding(typeScope, "C_BasePlayerPawn")) {
        if (FindSchemaField(playerPawn, "m_pWeaponServices", offset))        g_fldWeaponServices = { offset, true };
    }
    if (uintptr_t weaponServices = FindClassBinding(typeScope, "CPlayer_WeaponServices")) {
        if (FindSchemaField(weaponServices, "m_hActiveWeapon", offset))      g_fldActiveWeapon = { offset, true };
    }
}

void RefreshOffsets(uintptr_t base, size_t size) {
    LoadOffsets();

    if (uintptr_t va = ScanRipRelative(base, size, { 0x48, 0x89, 0x0D }, { 0xE9, -1, -1, -1, -1, 0xCC }))
        g_ovEntityList = va - base;

    if (uintptr_t va = ScanRipRelative(base, size, { 0x48, 0x8D, 0x0D }, { 0x48, 0xC1, 0xE0, 0x06 }))
        g_ovViewMatrix = va - base;

    if (uintptr_t va = ScanRipRelative(base, size, { 0x48, 0x8B, 0x05 }, { 0x41, 0x89, 0xBE }))
        g_ovLocalController = va - base;

    RefreshSchemaOffsets();
}
