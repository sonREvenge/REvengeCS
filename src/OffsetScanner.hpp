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

// Manual pointer overrides, read from revengecs_offsets.cfg if present and
// otherwise populated by RefreshOffsets(). Zero means "use the compiled
// fallback in offsets::".
extern uintptr_t g_ovEntityList;
extern uintptr_t g_ovLocalController;
extern uintptr_t g_ovViewMatrix;

// A class field offset resolved live from CS2's schema system, with a flag
// distinguishing "not found" from a legitimately-zero offset.
struct ScannedOffset {
	int32_t value = 0;
	bool valid = false;
};

extern ScannedOffset g_fldTeamNum;
extern ScannedOffset g_fldHealth;
extern ScannedOffset g_fldGameSceneNode;
extern ScannedOffset g_fldPlayerPawnHandle;
extern ScannedOffset g_fldModelState;

int32_t OffTeamNum();
int32_t OffHealth();
int32_t OffGameSceneNode();
int32_t OffPlayerPawnHandle();
int32_t OffModelState();

// Re-scans client.dll and schemasystem.dll for every offset HackGame() needs,
// so the app keeps working across CS2 updates without a recompile. Falls back
// to the offsets baked in at build time for anything a scan doesn't find, and
// applies manual overrides from revengecs_offsets.cfg (if present) last.
void RefreshOffsets(uintptr_t base, size_t size);
