#pragma once
#include <cstdint>

void InitVisibleCheck(uintptr_t clientBase, size_t clientSize);
void ShutdownVisibleCheck();
bool IsPawnVisible(uintptr_t pawn, uintptr_t localPawn,
                   uintptr_t entityList, int pawnIndex);
void ClearVisibleCache();
bool GetVisibleCache(uintptr_t pawn);
