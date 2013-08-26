/*
    Copyright 2005-2011 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents related
    to the source code ("Material") are owned by Intel Corporation or its
    suppliers or licensors.  Title to the Material remains with Intel
    Corporation or its suppliers and licensors.  The Material is protected
    by worldwide copyright laws and treaty provisions.  No part of the
    Material may be used, copied, reproduced, modified, published, uploaded,
    posted, transmitted, distributed, or disclosed in any way without
    Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under such
    intellectual property rights must be express and approved by Intel in
    writing.
*/

#ifndef __TBB_machine_H
#error Do not include this file directly; include tbb_machine.h instead
#endif

#include <stdint.h>
#include <unistd.h>

// This file is for PowerPC with compilers supporting GNU inline-assembler syntax (currently GNU g++ and IBM XL).

// Motivation for use of "#if defined(__powerpc64__) || defined(__ppc64__)" to detect a 64-bit environment:
// IBM XL documents both __powerpc64__ and __PPC64__, and these also appear to work on g++ (documentation?)
// Apple documents __ppc64__ (with __ppc__ only 32-bit, which is not portable even to other environments using g++)
inline int32_t __TBB_machine_cmpswp4 (volatile void *ptr, int32_t value, int32_t comparand )
{
    int32_t result;

    __asm__ __volatile__("sync\n"
                         "0: lwarx %0,0,%2\n\t"  /* load w/ reservation */
                         "cmpw %0,%4\n\t"        /* compare against comparand */
                         "bne- 1f\n\t"           /* exit if not same */
                         "stwcx. %3,0,%2\n\t"    /* store new_value */
                         "bne- 0b\n"             /* retry if reservation lost */
                         "1: sync"               /* the exit */
                          : "=&r"(result), "=m"(* (int32_t*) ptr)
                          : "r"(ptr), "r"(value), "r"(comparand), "m"(* (int32_t*) ptr)
                          : "cr0", "memory");
    return result;
}

#if defined(__powerpc64__) || defined(__ppc64__)

inline int64_t __TBB_machine_cmpswp8 (volatile void *ptr, int64_t value, int64_t comparand )
{
    int64_t result;
    __asm__ __volatile__("sync\n"
                         "0: ldarx %0,0,%2\n\t"  /* load w/ reservation */
                         "cmpd %0,%4\n\t"        /* compare against comparand */
                         "bne- 1f\n\t"           /* exit if not same */
                         "stdcx. %3,0,%2\n\t"    /* store new_value */
                         "bne- 0b\n"             /* retry if reservation lost */
                         "1: sync"               /* the exit */
                          : "=&r"(result), "=m"(* (int64_t*) ptr)
                          : "r"(ptr), "r"(value), "r"(comparand), "m"(* (int64_t*) ptr)
                          : "cr0", "memory");
    return result;
}
#else
// Except for special circumstances, 32-bit builds are meant to run on actual 32-bit hardware
// A locked implementation would also be a possibility
#define __TBB_64BIT_ATOMICS 0
#endif /* 64bit CAS */

#define __TBB_BIG_ENDIAN 1

#if defined(__powerpc64__) || defined(__ppc64__)
#define __TBB_WORDSIZE 8
#define __TBB_CompareAndSwapW(P,V,C) __TBB_machine_cmpswp8(P,V,C)
#else
#define __TBB_WORDSIZE 4
#define __TBB_CompareAndSwapW(P,V,C) __TBB_machine_cmpswp4(P,V,C)
#endif

#define __TBB_CompareAndSwap4(P,V,C) __TBB_machine_cmpswp4(P,V,C)
#if __TBB_64BIT_ATOMICS
#define __TBB_CompareAndSwap8(P,V,C) __TBB_machine_cmpswp8(P,V,C)
#endif
#define __TBB_full_memory_fence() __asm__ __volatile__("sync": : :"memory")
#define __TBB_release_consistency_helper() __asm__ __volatile__("lwsync": : :"memory")

#if !__IBMCPP__
// "1501-230 (S) Internal compiler error; please contact your Service Representative"
static inline intptr_t __TBB_machine_lg( uintptr_t x ) {
    // TODO: assumes sizeof(uintptr_t)<=8 resp. 4
    #if defined(__powerpc64__) || defined(__ppc64__)
    __asm__ __volatile__ ("cntlzd %0,%0" : "+r"(x)); // counting starts at 2^63
    return 63-static_cast<intptr_t>(x);
    #else
    __asm__ __volatile__ ("cntlzw %0,%0" : "+r"(x)); // counting starts at 2^31 (on 64-bit hardware, higher-order bits are ignored)
    return 31-static_cast<intptr_t>(x);
    #endif
}
#define __TBB_Log2(V) __TBB_machine_lg(V)
#endif

#define __TBB_Byte uint32_t // TODO: would this ever not be aligned without an alignment specification?

inline bool __TBB_machine_trylockbyte( __TBB_Byte &flag ) {
    return __TBB_machine_cmpswp4(&flag,1,0)==0;
}
#define __TBB_TryLockByte(P) __TBB_machine_trylockbyte(P)