/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "libams.hpp"
#include "file_utils.hpp"
#include "mitm_manager.hpp"

namespace ams {

    namespace mitm {

        namespace {

            // Static heap strictly restricted to 128 KB (<= 2 MB constraint)
            alignas(0x40) constinit u8 g_heap_memory[0x20000];
            constinit lmem::HeapHandle g_heap_handle;
            constinit bool g_heap_initialized = false;
            constinit os::SdkMutex g_heap_init_mutex;

            lmem::HeapHandle GetHeapHandle() {
                if (AMS_UNLIKELY(!g_heap_initialized)) {
                    std::scoped_lock lk(g_heap_init_mutex);
                    if (AMS_LIKELY(!g_heap_initialized)) {
                        g_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
                        g_heap_initialized = true;
                    }
                }
                return g_heap_handle;
            }

            void *Allocate(size_t size) {
                return lmem::AllocateFromExpHeap(GetHeapHandle(), size);
            }

            void *AllocateWithAlign(size_t size, size_t align) {
                return lmem::AllocateFromExpHeap(GetHeapHandle(), size, align);
            }

            void Deallocate(void *p, size_t size) {
                AMS_UNUSED(size);
                return lmem::FreeToExpHeap(GetHeapHandle(), p);
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            R_ABORT_UNLESS(sm::Initialize());

            fs::InitializeForSystem();
            fs::SetAllocator(mitm::Allocate, mitm::Deallocate);
            fs::SetEnabledAutoAbort(false);

            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
        }

        void FinalizeSystemModule() {
            /* Cleanup if process is ever terminated */
        }

        void Startup() {}

    }

    ncm::ProgramId CurrentProgramId = {0x010000000000B0A7ul};

    namespace result {
        bool CallFatalOnResultAssertion = true;
    }

    void Main() {
        R_ABORT_UNLESS(FileUtils::Initialize());

        FileUtils::LogLine("[boxart-nx] Main thread starting...");

        MitmManager serverManager;
        R_ABORT_UNLESS(serverManager.RegisterServers());

        FileUtils::LogLine("[boxart-nx] MitM servers registered. Entering loop.");
        serverManager.LoopProcess();
    }
}

// Global operator new/delete overrides to use our static exp heap
void *operator new(size_t size) {
    return ams::mitm::Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &) noexcept {
    return ams::mitm::Allocate(size);
}

void operator delete(void *p) noexcept {
    ams::mitm::Deallocate(p, 0);
}

void operator delete(void *p, size_t size) noexcept {
    ams::mitm::Deallocate(p, size);
}

void *operator new[](size_t size) {
    return ams::mitm::Allocate(size);
}

void *operator new[](size_t size, const std::nothrow_t &) noexcept {
    return ams::mitm::Allocate(size);
}

void operator delete[](void *p) noexcept {
    ams::mitm::Deallocate(p, 0);
}

void operator delete[](void *p, size_t size) noexcept {
    ams::mitm::Deallocate(p, size);
}

void *operator new(size_t size, std::align_val_t align) {
    return ams::mitm::AllocateWithAlign(size, static_cast<size_t>(align));
}

void operator delete(void *p, std::align_val_t align) noexcept {
    AMS_UNUSED(align);
    ams::mitm::Deallocate(p, 0);
}
