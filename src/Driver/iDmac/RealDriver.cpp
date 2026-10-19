#include "Driver/iDmac/RealDriver.h"

#include "plog/Log.h"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace gc::driver {
namespace {

constexpr wchar_t kRealDriverFileName[] = L"iDmacDrv32_real.dll";

struct RealDriverState final {
    HMODULE loader_module{};
    RealIDmacApi api{};
    bool bound{};
    std::once_flag once;
};

RealDriverState& State() noexcept {
    static RealDriverState state;
    return state;
}

std::atomic_bool g_requested{false};

template <typename Fn>
bool Resolve(HMODULE module, const char* name, Fn& out) noexcept {
    const auto proc = GetProcAddress(module, name);
    if (proc == nullptr) {
        PLOG_ERROR << "RealDriver: original driver is missing export " << name;
        return false;
    }
    out = reinterpret_cast<Fn>(proc);
    return true;
}

bool BindOriginalDriver(RealDriverState& state) noexcept {
    try {
        std::vector<wchar_t> buffer(32768);
        const auto length = GetModuleFileNameW(state.loader_module, buffer.data(),
            static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size()) {
            PLOG_ERROR << "RealDriver: cannot locate loader module path error=" << GetLastError();
            return false;
        }
        const auto path = std::filesystem::path{std::wstring{buffer.data(), length}}
            .parent_path() / kRealDriverFileName;
        const HMODULE module = LoadLibraryW(path.c_str());
        if (module == nullptr) {
            PLOG_ERROR << "RealDriver: cannot load original driver path=" << path.string()
                << " error=" << GetLastError();
            return false;
        }
        auto& api = state.api;
        const bool resolved =
            Resolve(module, "iDmacDrvOpen", api.open) &&
            Resolve(module, "iDmacDrvClose", api.close) &&
            Resolve(module, "iDmacDrvDmaRead", api.dma_read) &&
            Resolve(module, "iDmacDrvDmaWrite", api.dma_write) &&
            Resolve(module, "iDmacDrvRegisterRead", api.register_read) &&
            Resolve(module, "iDmacDrvRegisterWrite", api.register_write) &&
            Resolve(module, "iDmacDrvRegisterBufferRead", api.register_buffer_read) &&
            Resolve(module, "iDmacDrvRegisterBufferWrite", api.register_buffer_write) &&
            Resolve(module, "iDmacDrvMemoryRead", api.memory_read) &&
            Resolve(module, "iDmacDrvMemoryWrite", api.memory_write) &&
            Resolve(module, "iDmacDrvMemoryBufferRead", api.memory_buffer_read) &&
            Resolve(module, "iDmacDrvMemoryBufferWrite", api.memory_buffer_write) &&
            Resolve(module, "iDmacDrvMemoryReadExt", api.memory_read_ext) &&
            Resolve(module, "iDmacDrvMemoryWriteExt", api.memory_write_ext) &&
            Resolve(module, "iDmacDrvProgramDownload", api.program_download);
        if (!resolved) return false;
        PLOG_INFO << "RealDriver: original iDmac driver bound path=" << path.string();
        return true;
    } catch (...) {
        PLOG_ERROR << "RealDriver: unexpected failure while binding original driver";
        return false;
    }
}

} // namespace

void RequestRealIDmacDriver(HMODULE loader_module) noexcept {
    State().loader_module = loader_module;
    g_requested.store(true, std::memory_order_release);
}

bool RealIDmacRequested() noexcept {
    return g_requested.load(std::memory_order_acquire);
}

const RealIDmacApi* RealIDmac() noexcept {
    if (!RealIDmacRequested()) return nullptr;
    auto& state = State();
    std::call_once(state.once, [&state] { state.bound = BindOriginalDriver(state); });
    return state.bound ? &state.api : nullptr;
}

} // namespace gc::driver
