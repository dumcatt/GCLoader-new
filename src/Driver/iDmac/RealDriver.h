#pragma once

#include <windows.h>

namespace gc::driver {

// Typed copies of the iDmacDrv32 export surface, used to forward calls to the
// original cabinet driver when real cabinet I/O is enabled.
struct RealIDmacApi final {
    using OpenFn = DWORD(__cdecl*)(int, LPVOID, LPVOID);
    using CloseFn = DWORD(__cdecl*)(int, LPVOID);
    using DmaReadFn = int(__cdecl*)(int, LPVOID, UINT_PTR, LPVOID);
    using DmaWriteFn = int(__cdecl*)(int, void*, UINT_PTR, LPVOID);
    using RegisterReadFn = int(__cdecl*)(int, DWORD, LPVOID, LPVOID);
    using RegisterWriteFn = int(__cdecl*)(int, DWORD, int, LPVOID);
    using RegisterBufferReadFn = int(__cdecl*)(int, DWORD, LPVOID, UINT_PTR, LPVOID);
    using RegisterBufferWriteFn = int(__cdecl*)(int, DWORD, void*, UINT_PTR, LPVOID);
    using MemoryReadFn = int(__cdecl*)(int, DWORD, LPVOID, LPVOID);
    using MemoryWriteFn = int(__cdecl*)(int, DWORD, int, LPVOID);
    using MemoryBufferReadFn = int(__cdecl*)(int, DWORD, LPVOID, UINT_PTR, LPVOID);
    using MemoryBufferWriteFn = int(__cdecl*)(int, int, void*, UINT_PTR, LPVOID);
    using MemoryReadExtFn = int(__cdecl*)(int, DWORD, int, LPVOID, DWORD, LPVOID);
    using MemoryWriteExtFn = int(__cdecl*)(int, int, int, void*, rsize_t, LPVOID);
    using ProgramDownloadFn = int(__cdecl*)();

    OpenFn open{};
    CloseFn close{};
    DmaReadFn dma_read{};
    DmaWriteFn dma_write{};
    RegisterReadFn register_read{};
    RegisterWriteFn register_write{};
    RegisterBufferReadFn register_buffer_read{};
    RegisterBufferWriteFn register_buffer_write{};
    MemoryReadFn memory_read{};
    MemoryWriteFn memory_write{};
    MemoryBufferReadFn memory_buffer_read{};
    MemoryBufferWriteFn memory_buffer_write{};
    MemoryReadExtFn memory_read_ext{};
    MemoryWriteExtFn memory_write_ext{};
    ProgramDownloadFn program_download{};
};

// Records that the game process should use the original driver. Called once
// from game startup; the driver is not loaded until the game first opens it.
void RequestRealIDmacDriver(HMODULE loader_module) noexcept;

[[nodiscard]] bool RealIDmacRequested() noexcept;

// Returns the bound original driver, loading it on first use. Returns nullptr
// when the original driver could not be loaded or resolved; the failure is
// logged. Only meaningful when RealIDmacRequested() is true.
[[nodiscard]] const RealIDmacApi* RealIDmac() noexcept;

} // namespace gc::driver
