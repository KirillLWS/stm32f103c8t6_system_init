#ifndef CMSIS_COMPILER_MINIMAL_GCC_H
#define CMSIS_COMPILER_MINIMAL_GCC_H

/* ===== Compiler attributes ===== */

#define __ASM            __asm
#define __INLINE         inline
#define __STATIC_INLINE  static inline
#define __STATIC_FORCEINLINE __attribute__((always_inline)) static inline
#define __NO_RETURN      __attribute__((noreturn))
#define __WEAK           __attribute__((weak))
#define __PACKED         __attribute__((packed))
#define __ALIGNED(x)     __attribute__((aligned(x)))

/* ===== Memory barriers (ARMv7-M) ===== */

#define __NOP()          __ASM volatile ("nop")
#define __WFI()          __ASM volatile ("wfi")
#define __WFE()          __ASM volatile ("wfe")
#define __SEV()          __ASM volatile ("sev")

#define __ISB()          __ASM volatile ("isb 0xF":::"memory")
#define __DSB()          __ASM volatile ("dsb 0xF":::"memory")
#define __DMB()          __ASM volatile ("dmb 0xF":::"memory")

/* ===== Special register access ===== */

#define __enable_irq()   __ASM volatile ("cpsie i" ::: "memory")
#define __disable_irq()  __ASM volatile ("cpsid i" ::: "memory")

/* ===== Stack pointer access ===== */

__STATIC_INLINE uint32_t __get_MSP(void)
{
    uint32_t result;
    __ASM volatile ("mrs %0, msp" : "=r" (result));
    return result;
}

__STATIC_INLINE void __set_MSP(uint32_t topOfMainStack)
{
    __ASM volatile ("msr msp, %0" : : "r" (topOfMainStack) : );
}

__STATIC_INLINE uint32_t __get_PSP(void)
{
    uint32_t result;
    __ASM volatile ("mrs %0, psp" : "=r" (result));
    return result;
}

__STATIC_INLINE void __set_PSP(uint32_t topOfProcStack)
{
    __ASM volatile ("msr psp, %0" : : "r" (topOfProcStack) : );
}

#endif
