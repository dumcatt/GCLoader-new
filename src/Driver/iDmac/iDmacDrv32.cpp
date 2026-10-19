#include <windows.h>
#include "Driver/iDmac/RealDriver.h"
#include "Input/Polling/InputPollingRuntime.h"
#include "Driver/iDmac/RegisterOpTypes.h"
#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include <format>
#include <string>

// When real cabinet I/O is enabled, every export forwards to the original
// iDmacDrv32 driver and the emulated input runtime never starts.

extern "C" __declspec(dllexport) DWORD __cdecl iDmacDrvOpen(
    int deviceId,
    LPVOID outBuffer,
    LPVOID lpSomeFlag) {
    PLOG_VERBOSE << "iDmacDrvOpen";
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        if (real == nullptr) {
            MessageBoxA(
                nullptr,
                "Original iDmacDrv32 driver could not be loaded. See loader-log.txt.",
                "GCLoader Driver Error",
                MB_OK | MB_ICONERROR);
            return ERROR_DEVICE_NOT_AVAILABLE;
        }
        return real->open(deviceId, outBuffer, lpSomeFlag);
    }
    *static_cast<DWORD*>(outBuffer) = 284;
    *static_cast<DWORD*>(lpSomeFlag) = 0;

    const auto input_result = gc::input::OpenInputPollingRuntime();
    if (!input_result.success) {
        const std::string message =
            "Input initialization failed: " + input_result.message;
        PLOG_ERROR << message;
        MessageBoxA(
            nullptr,
            message.c_str(),
            "GCLoader Input Error",
            MB_OK | MB_ICONERROR);
        return ERROR_DEVICE_NOT_AVAILABLE;
    }
    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) DWORD __cdecl iDmacDrvClose(
    int deviceId,
    LPVOID lpWriteAccess) {
    PLOG_VERBOSE << "iDmacDrvClose";
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->close(deviceId, lpWriteAccess) : ERROR_DEVICE_NOT_AVAILABLE;
    }
    gc::input::CloseInputPollingRuntime();
    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvDmaRead(int a1, LPVOID lp, UINT_PTR ucb, LPVOID a4) {
    PLOG_VERBOSE << "iDmacDrvDmaRead" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->dma_read(a1, lp, ucb, a4) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvDmaWrite(int a1, void* lp, UINT_PTR ucb, LPVOID a4) {
    PLOG_VERBOSE << "iDmacDrvDmaWrite" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->dma_write(a1, lp, ucb, a4) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvRegisterRead(int DeviceId, DWORD CommandCode, LPVOID OutBuffer, LPVOID DeviceResult) {
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->register_read(DeviceId, CommandCode, OutBuffer, DeviceResult) : 0;
    }
    auto readType = static_cast<RegisterReadType>(CommandCode);
    DWORD result;
    switch (readType)
    {
    case RegisterReadType::DMAC_ID:
        result = 0x00010201;
        break;
    case RegisterReadType::FIO_NODE_0_STATUS:
        result = 0x00FF00FF;
        break;
    case RegisterReadType::FIO_NODE_1_STATUS:
        result = 0x00FF0000;
        break;
    case RegisterReadType::FIO_NODE_0_INPUT:
        result = gc::input::ReadPublishedInput();
        break;
    case RegisterReadType::FIO_NODE0_ANALOG1:
        result = 0;
        break;
    // Seems to be volume for GC
    case RegisterReadType::FIO_NODE0_ANALOG2:
        result = 0xFF;
        break;
    case RegisterReadType::FIO_NODE0_ANALOG3:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE_1_INPUT:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE1_ANALOG1:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE1_ANALOG2:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE1_ANALOG3:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE_0_COINSLOT_1:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE_0_COINSLOT_2:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE_1_COINSLOT_1:
        result = 0;
        break;
    case RegisterReadType::FIO_NODE_1_COINSLOT_2:
        result = 0;
        break;
    case RegisterReadType::FIO_HUB_PORT_1:
        result = 0x0001823C;
        break;
    case RegisterReadType::FIO_HUB_PORT_2:
    case RegisterReadType::FIO_HUB_PORT_3:
    case RegisterReadType::FIO_HUB_PORT_4:
    case RegisterReadType::FIO_GC_UNKNOWN_1:
    case RegisterReadType::FIO_GC_UNKNOWN_2:
    case RegisterReadType::FIO_GC_UNKNOWN_3:
    case RegisterReadType::FIO_GC_UNKNOWN_4:
    case RegisterReadType::FIO_GC_UNKNOWN_5:
    case RegisterReadType::FIO_GC_UNKNOWN_6:
    case RegisterReadType::FIO_GC_UNKNOWN_7:
    case RegisterReadType::FIO_GC_UNKNOWN_8:
        result = 0;
        break;
    default:
        result = 0;
    }
    *static_cast<DWORD*>(OutBuffer) = result;
    *static_cast<DWORD*>(DeviceResult) = 0;
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvRegisterWrite(int deviceId, DWORD CommandCode, int unused, LPVOID DeviceResult) {
    PLOG_VERBOSE << std::format("iDmacDrvRegisterWrite, command {:#x}", CommandCode) << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->register_write(deviceId, CommandCode, unused, DeviceResult) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvRegisterBufferRead(int a1, DWORD BytesReturned, LPVOID lp, UINT_PTR ucb, LPVOID a5) {
    PLOG_VERBOSE << "iDmacDrvRegisterBufferRead" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->register_buffer_read(a1, BytesReturned, lp, ucb, a5) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvRegisterBufferWrite(int a1, DWORD BytesReturned, void* lp, UINT_PTR ucb, LPVOID a5) {
    PLOG_VERBOSE << "iDmacDrvRegisterBufferWrite" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->register_buffer_write(a1, BytesReturned, lp, ucb, a5) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvMemoryRead(int a1, DWORD BytesReturned, LPVOID lp, LPVOID a4) {
    PLOG_VERBOSE << "iDmacDrvMemoryRead" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->memory_read(a1, BytesReturned, lp, a4) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvMemoryWrite(int a1, DWORD BytesReturned, int a3, LPVOID lp) {
    PLOG_VERBOSE << "iDmacDrvMemoryWrite" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->memory_write(a1, BytesReturned, a3, lp) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvMemoryBufferRead(int a1, DWORD BytesReturned, LPVOID lp, UINT_PTR ucb, LPVOID a5) {
    PLOG_VERBOSE << std::format("iDmacDrvMemoryBufferRead, a1: {:#x},a2: {:#x},a3: {:#x},a4: {:#x},a5: {:#x}",
                                a1, BytesReturned, reinterpret_cast<DWORD>(lp), ucb, reinterpret_cast<DWORD>(a5)) << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->memory_buffer_read(a1, BytesReturned, lp, ucb, a5) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvMemoryBufferWrite(int a1, int a2, void* lp, UINT_PTR ucb, LPVOID a5) {
    PLOG_VERBOSE << std::format("iDmacDrvMemoryBufferWrite, a1: {:#x},a2: {:#x},a3: {:#x},a4: {:#x},a5: {:#x}",
                                a1, a2, reinterpret_cast<DWORD>(lp), ucb, reinterpret_cast<DWORD>(a5)) << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->memory_buffer_write(a1, a2, lp, ucb, a5) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvMemoryReadExt(int a1, DWORD BytesReturned, int a3, LPVOID lp, DWORD nOutBufferSize, LPVOID a6) {
    PLOG_VERBOSE << "iDmacDrvMemoryReadExt" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->memory_read_ext(a1, BytesReturned, a3, lp, nOutBufferSize, a6) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvMemoryWriteExt(int a1, int a2, int a3, void* Src, rsize_t DstSize, LPVOID lp) {
    PLOG_VERBOSE << "iDmacDrvMemoryWriteExt" << std::endl;
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->memory_write_ext(a1, a2, a3, Src, DstSize, lp) : 0;
    }
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl iDmacDrvProgramDownload() {
    if (gc::driver::RealIDmacRequested()) {
        const auto* real = gc::driver::RealIDmac();
        return real != nullptr ? real->program_download() : 0;
    }
    return 0;
}
