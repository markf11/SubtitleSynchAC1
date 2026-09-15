#pragma once
#include <cstdint>
bool applyASMPatches();
namespace AudioHook { bool Install(uintptr_t address); }
namespace PauseHook { bool Install(uintptr_t address, unsigned playing); bool InstallGameHooks(); }
