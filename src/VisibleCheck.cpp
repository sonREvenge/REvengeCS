#include "VisibleCheck.hpp"

#include <windows.h>
#include <unordered_map>

#include "OffsetScanner.hpp"

static std::unordered_map<uintptr_t, bool> s_cache;

static uintptr_t s_cachedLocalPawn = 0;
static int       s_cachedLocalIdx = 0;

static int FindLocalControllerIndex(uintptr_t entityList, uintptr_t localPawn) {
    if (!entityList || !localPawn) return 0;

    uintptr_t localController =
        mem.Read<uintptr_t>(module_base + (g_ovLocalController ? g_ovLocalController : offsets::dwLocalPlayerController));
    if (!localController) return 0;

    for (int i = 1; i < 64; i++) {
        uintptr_t entry = mem.Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9)) + 16);
        if (!entry) continue;
        uintptr_t ctrl = mem.Read<uintptr_t>(entry + 112 * (i & 0x1FF));
        if (ctrl == localController) return i;
    }
    return 0;
}

static bool CheckSpottedByMask(uintptr_t pawn, int localIndex) {
    if (!pawn || localIndex <= 0) return false;

    uintptr_t stateAddr = pawn + OffEntitySpottedState();
    uint32_t mask[2];
    mask[0] = mem.Read<uint32_t>(stateAddr + 0xC);
    mask[1] = mem.Read<uint32_t>(stateAddr + 0x10);

    int bitIdx = localIndex - 1;
    if (bitIdx < 32)
        return (mask[0] >> bitIdx) & 1;
    else
        return (mask[1] >> (bitIdx - 32)) & 1;
}

void InitVisibleCheck(uintptr_t, size_t) {
}

void ShutdownVisibleCheck() {
    s_cache.clear();
    s_cachedLocalPawn = 0;
    s_cachedLocalIdx = 0;
}

bool IsPawnVisible(uintptr_t pawn, uintptr_t localPawn,
                   uintptr_t entityList, int) {
    if (!pawn || !localPawn) return false;

    {
        auto it = s_cache.find(pawn);
        if (it != s_cache.end()) return it->second;
    }

    if (localPawn != s_cachedLocalPawn) {
        s_cachedLocalPawn = localPawn;
        s_cachedLocalIdx = FindLocalControllerIndex(entityList, localPawn);
    }
    int localIdx = s_cachedLocalIdx;

    if (localIdx > 0 && CheckSpottedByMask(pawn, localIdx)) {
        s_cache[pawn] = true;
        return true;
    }

    s_cache[pawn] = false;
    return false;
}

void ClearVisibleCache() { s_cache.clear(); }

bool GetVisibleCache(uintptr_t pawn) {
    auto it = s_cache.find(pawn);
    return it != s_cache.end() && it->second;
}
