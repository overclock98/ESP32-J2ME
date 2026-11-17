
#include "flint_mutex.h"
#include "flint_system_api.h"
#include <stdint.h>

extern "C" bool __atomic_test_and_set(volatile void *p, int model) {
    uintptr_t addr = (uintptr_t)p;
    volatile uint32_t *aligned_ptr = (volatile uint32_t *)(addr & ~3);
    uint32_t shift = (addr & 3) * 8;
    uint32_t mask = 1U << shift;
    uint32_t old = __sync_fetch_and_or(aligned_ptr, mask);
    return (old & mask) != 0;
}

extern "C" void __atomic_clear(volatile void *p, int model) {
    uintptr_t addr = (uintptr_t)p;
    volatile uint32_t *aligned_ptr = (volatile uint32_t *)(addr & ~3);
    uint32_t shift = (addr & 3) * 8;
    uint32_t mask = ~(0xFFU << shift);
    __sync_and_and_fetch(aligned_ptr, mask);
}

FMutex::FMutex(void) {
    locked.clear();
    lockNest = 0;
    lockThread = NULL;
}

void FMutex::lock(void) {
    void *currentThread = FlintAPI::Thread::getCurrentThread();
    while(1) {
        while(atomic_flag_test_and_set_explicit(&locked, memory_order_acquire))
            FlintAPI::Thread::yield();
        if(lockThread == NULL) {
            lockNest = 1;
            lockThread = currentThread;
            atomic_flag_clear_explicit(&locked, memory_order_release);
            return;
        }
        else if(lockThread == currentThread) {
            lockNest++;
            atomic_flag_clear_explicit(&locked, memory_order_release);
            return;
        }
        atomic_flag_clear_explicit(&locked, memory_order_release);
        FlintAPI::Thread::yield();
    }
}

void FMutex::unlock(void) {
    void *currentThread = FlintAPI::Thread::getCurrentThread();
    while(atomic_flag_test_and_set_explicit(&locked, memory_order_acquire))
        FlintAPI::Thread::yield();
    if(currentThread == lockThread && lockNest > 0) {
        if(--lockNest == 0)
            lockThread = NULL;
    }
    atomic_flag_clear_explicit(&locked, memory_order_release);
}
