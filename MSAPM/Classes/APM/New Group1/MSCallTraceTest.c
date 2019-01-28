////
////  MSCallTrace.c
////  Pods-MSAPM
////
////  Created by guoxiaoliang on 2019/1/25.
////
//
//#include "MSCallTrace.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stddef.h>
//#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/time.h>
//#include <objc/message.h>
//#include <objc/runtime.h>
//#include <dispatch/dispatch.h>
//#include <pthread.h>
//
//#include <dlfcn.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/types.h>
//#include <mach-o/dyld.h>
//#include <mach-o/loader.h>
//#include <mach-o/nlist.h>
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stddef.h>
//#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/time.h>
//#include <objc/message.h>
//#include <objc/runtime.h>
//#include <dispatch/dispatch.h>
//#include <pthread.h>
//
//#include <mach/vm_types.h>
//#ifdef __LP64__
//typedef struct mach_header_64 mach_header_t;
//typedef struct segment_command_64 segment_command_t;
//typedef struct section_64 section_t;
//typedef struct nlist_64 nlist_t;
//#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT_64
//#else
//typedef struct mach_header mach_header_t;
//typedef struct segment_command segment_command_t;
//typedef struct section section_t;
//typedef struct nlist nlist_t;
//#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT
//#endif
//
//#ifndef SEG_DATA_CONST
//#define SEG_DATA_CONST  "__DATA_CONST"
//#endif
//
//static bool _call_record_enabled = true;
//static uint64_t _min_time_cost = 1000; //us
//static int _max_call_depth = 3;
//static pthread_key_t _thread_key;
//__unused static id (*orig_objc_msgSend)(id, SEL, ...);
//
//#ifdef __aarch64__
//typedef struct
//{
//    vm_address_t addr;
//    unsigned long isTrace;
//    vm_address_t lr;
//    vm_address_t fp;
//    vm_address_t key;
//} ReturnPayload;
//// Shared structures.
//typedef struct CallRecord_ {
//    void *objc;
//    void *method;
//    void *fp;
//    void *lr;
//} CallRecord;
//#else
//typedef struct
//{
//    vm_address_t addr;
//    unsigned long isTrace;
//    vm_address_t ret;
//    vm_address_t bp;
//    vm_address_t key;
//} ReturnPayload;
//// Shared structures.
//typedef struct CallRecord_ {
//    void *objc;
//    void *method;
//    void *bp;
//    void *ret;
//} CallRecord;
//
//#endif
///*
// -ARM64
// http://infocenter.arm.com/help/topic/com.arm.doc.den0024a/DEN0024A_v8_architecture_PG.pdf (7.2.1 Floating-point) (4.6.1 Floating-point register organization in AArch64)
// use struct and union to describe diagram in the above link, nice!
// -X86
// https://en.wikipedia.org/wiki/X86_calling_conventions
// RDI, RSI, RDX, RCX, R8, R9, XMM0–7
// */
//// x86_64 is XMM, arm64 is q
//typedef union FPMReg_ {
//    __int128_t q;
//    struct {
//        double d1; // Holds the double (LSB).
//        double d2;
//    } d;
//    struct {
//        float f1; // Holds the float (LSB).
//        float f2;
//        float f3;
//        float f4;
//    } f;
//} FPReg;
//// just ref how to backup/restore registers
//struct RegState_ {
//    uint64_t bp;
//    uint64_t ret;
//    union {
//        uint64_t arr[7];
//        struct {
//            uint64_t rax;
//            uint64_t rdi;
//            uint64_t rsi;
//            uint64_t rdx;
//            uint64_t rcx;
//            uint64_t r8;
//            uint64_t r9;
//        } regs;
//    } general;
//    uint64_t _; // for align
//    
//    union {
//        FPReg arr[8];
//        struct {
//            FPReg xmm0;
//            FPReg xmm1;
//            FPReg xmm2;
//            FPReg xmm3;
//            FPReg xmm4;
//            FPReg xmm5;
//            FPReg xmm6;
//            FPReg xmm7;
//        } regs;
//    } floating;
//};
//typedef struct pa_list_ {
//    struct RegState_ *regs; // Registers saved when function is called.
//    unsigned char *stack; // Address of current argument.
//    int ngrn; // The Next General-purpose Register Number.
//    int nsrn; // The Next SIMD and Floating-point Register Number.
//} pa_list;
//__attribute__((__naked__)) static void fake_objc_msgSend_safe()
//{
//    // test for direct jmp
//    // __asm__ volatile(
//    //     "jmpq *%0\n":
//    //     : "r" (orig_objc_msgSend)
//    //     :);
//    // backup registers
//    __asm__ volatile(
//                     "subq $(16*8+8), %%rsp\n" // +8 for alignment
//                     "movdqa    %%xmm0, (%%rsp)\n"
//                     "movdqa    %%xmm1, 0x10(%%rsp)\n"
//                     "movdqa    %%xmm2, 0x20(%%rsp)\n"
//                     "movdqa    %%xmm3, 0x30(%%rsp)\n"
//                     "movdqa    %%xmm4, 0x40(%%rsp)\n"
//                     "movdqa    %%xmm5, 0x50(%%rsp)\n"
//                     "movdqa    %%xmm6, 0x60(%%rsp)\n"
//                     "movdqa    %%xmm7, 0x70(%%rsp)\n"
//                     "pushq %%rax\n" // stack align
//                     "pushq %%r9\n" // might be xmm parameter count
//                     "pushq %%r8\n"
//                     "pushq %%rcx\n"
//                     "pushq %%rdx\n"
//                     "pushq %%rsi\n"
//                     "pushq %%rdi\n"
//                     "pushq %%rax\n"
//                     // origin rsp, contain `ret address`, how to use leaq, always wrong.
//                     "movq %%rsp, %%rax\n"
//                     "addq $(16*8+8+8+7*8), %%rax\n"
//                     "pushq (%%rax)\n"
//                     "pushq %%rax\n" ::
//                     :);
//    // prepare args for func
//    __asm__ volatile(
//                     "movq %%rsp, %%rdi\n"
//                     "callq __Z10hookBeforeP9RegState_\n" ::
//                     :);
//    // get value from `ReturnPayload`
//    __asm__ volatile(
//                     "movq (%%rax), %%r10\n"
//                     "movq 8(%%rax), %%r11\n" ::
//                     :);
//    // restore registers
//    __asm__ volatile(
//                     "popq %%rax\n"
//                     "popq (%%rax)\n"
//                     "popq    %%rax\n"
//                     "popq    %%rdi\n"
//                     "popq    %%rsi\n"
//                     "popq    %%rdx\n"
//                     "popq    %%rcx\n"
//                     "popq    %%r8\n"
//                     "popq    %%r9\n"
//                     "popq %%rax\n" // stack align
//                     "movdqa    (%%rsp), %%xmm0\n"
//                     "movdqa    0x10(%%rsp), %%xmm1\n"
//                     "movdqa    0x20(%%rsp), %%xmm2\n"
//                     "movdqa    0x30(%%rsp), %%xmm3\n"
//                     "movdqa    0x40(%%rsp), %%xmm4\n"
//                     "movdqa    0x50(%%rsp), %%xmm5\n"
//                     "movdqa    0x60(%%rsp), %%xmm6\n"
//                     "movdqa    0x70(%%rsp), %%xmm7\n"
//                     "addq $(16*8+8), %%rsp\n" ::
//                     :);
//    // go to the original objc_msgSend
//    __asm__ volatile(
//                     // "br x9\n"
//                     "cmpq $0, %%r11\n"
//                     "jne Lthroughx\n"
//                     "jmpq *%%r10\n"
//                     "Lthroughx:\n"
//                     // trick to jmp
//                     "jmp NextInstruction\n"
//                     "Begin:\n"
//                     "popq %%r11\n"
//                     "movq %%r11, (%%rsp)\n"
//                     "jmpq *%%r10\n"
//                     "NextInstruction:\n"
//                     "call Begin" ::
//                     :);
//    //-----------------------------------------------------------------------------
//    // after objc_msgSend we parse the result.
//    // backup registers
//    __asm__ volatile(
//                     "pushq %%r10\n" // stack align
//                     "push    %%rbp\n"
//                     "movq    %%rsp, %%rbp\n"
//                     "subq    $(16*8), %%rsp\n" // +8 for alignment
//                     "movdqa    %%xmm0, -0x80(%%rbp)\n"
//                     "push    %%r9\n" // might be xmm parameter count
//                     "movdqa    %%xmm1, -0x70(%%rbp)\n"
//                     "push    %%r8\n"
//                     "movdqa    %%xmm2, -0x60(%%rbp)\n"
//                     "push    %%rcx\n"
//                     "movdqa    %%xmm3, -0x50(%%rbp)\n"
//                     "push    %%rdx\n"
//                     "movdqa    %%xmm4, -0x40(%%rbp)\n"
//                     "push    %%rsi\n"
//                     "movdqa    %%xmm5, -0x30(%%rbp)\n"
//                     "push    %%rdi\n"
//                     "movdqa    %%xmm6, -0x20(%%rbp)\n"
//                     "push    %%rax\n"
//                     "movdqa    %%xmm7, -0x10(%%rbp)\n"
//                     "pushq 0x8(%%rbp)\n"
//                     "movq %%rbp, %%rax\n"
//                     "addq $8, %%rax\n"
//                     "pushq %%rax\n" ::
//                     :);
//    // prepare args for func
//    __asm__ volatile(
//                     "movq %%rsp, %%rdi\n"
//                     // "callq __Z9hookAfterP9RegState_\n"
//                     "callq *%0\n"
//                     "movq %%rax, %%r10\n"
//                     :
//                     : "r"(func_ptr)
//                     : "%rax");
//    // restore registers
//    __asm__ volatile(
//                     "pop %%rax\n"
//                     "pop 8(%%rbp)\n"
//                     "movdqa    -0x80(%%rbp), %%xmm0\n"
//                     "pop    %%rax\n"
//                     "movdqa    -0x70(%%rbp), %%xmm1\n"
//                     "pop    %%rdi\n"
//                     "movdqa    -0x60(%%rbp), %%xmm2\n"
//                     "pop    %%rsi\n"
//                     "movdqa    -0x50(%%rbp), %%xmm3\n"
//                     "pop    %%rdx\n"
//                     "movdqa    -0x40(%%rbp), %%xmm4\n"
//                     "pop    %%rcx\n"
//                     "movdqa    -0x30(%%rbp), %%xmm5\n"
//                     "pop    %%r8\n"
//                     "movdqa    -0x20(%%rbp), %%xmm6\n"
//                     "pop    %%r9\n"
//                     "movdqa    -0x10(%%rbp), %%xmm7\n"
//                     "leave\n"
//                     // go to the original objc_msgSend
//                     "movq %%r10, (%%rsp)\n"
//                     "ret\n" ::
//                     :);
//}
///*
// arg1: object-address(need to parse >> class)
// arg2: method string address
// arg3: method signature
// */
//vm_address_t hookBefore(struct RegState_ *rs)
//{
//    gettimeofday(&stat,NULL);
//    // TODO: parse args
//    pa_list args = (pa_list){
//        rs,
//        reinterpret_cast<unsigned char *>(rs->bp),
//        2,
//        0};
//    ThreadCallStack *cs = getThreadCallStack(threadKey);
//    ReturnPayload *rp = cs->rp;
//    vm_address_t xaddr = (uint64_t)orig_objc_msgSend;
//    rp->addr = xaddr;
//    rp->ret = rs->ret;
//    rp->bp = rs->bp;
//    rp->isTrace = 0;
//    rp->key = rs->ret & rs->bp;
//    if (1 || pthread_main_np())
//    {
//        /* 
//         pay attention!!! as `objc_msgSend` invoked so often, the `filter` code snippet that use `c` to write it must be fast!!!.
//         */
//        do
//        {   
//            // check method string address, can be narrow the scope.
//            char *methodSelector = reinterpret_cast<char *>(rs->general.regs.rsi);
//            if(!(methodSelector && checkAddressRange((vm_address_t)methodSelector, macho_load_addr, macho_load_end_addr)))
//                break;
//            // check object's class address
//            vm_address_t class_addr = macho_objc_getClass(rs->general.regs.rdi);
//            if (!(class_addr && checkAddressRange(class_addr, macho_load_addr, macho_load_end_addr)))
//                break;
//            // check `call count filter`
//            if(check_freq_filter((unsigned long)methodSelector))
//                break;
//            
//            // // test for wechat
//            // if(!strncmp(methodSelector, "onRevokeMsg", 9))
//            //     debug_break();
//            // check exclude
//            // check method_name first, no need parse object.
//            if (!checkLogFilters_MethodName(NULL, methodSelector, FT_EXINLUDE))
//                break;
//            objc_class_info_t *xobjc_class_info;
//            xobjc_class_info = mem->parse_OBJECT(rs->general.regs.rdi);
//            if(!xobjc_class_info)
//                break;
//            
//            // check class name, need parse object
//            if (!checkLogFilters_ClassName(xobjc_class_info->class_name, NULL, FT_EXINLUDE))
//                break;
//            objc_method_info_t * objc_method_info;
//            objc_method_info = search_method_name(&(xobjc_class_info->methods), methodSelector);
//            if (!objc_method_info)
//                break;
//            if(check_thread_filter(cs->thread))
//                break;
//            // add to `method call count cache`
//            add_freq_filter((unsigned long)methodSelector, objc_method_info);
//            // may be can pass into 'objc_method_info_t' without 'class_name' and 'method_name'
//            CallRecord *cr = pushCallRecord(xobjc_class_info->class_name, methodSelector, reinterpret_cast<void *>(rs->bp), reinterpret_cast<void *>(rs->ret), cs);
//            if(!cr)
//                break;
//            printCallRcord(cr, cs);
//            rp->isTrace = 1;
//        } while(0);
//    }
//    gettimeofday(&end,NULL);
//    time_cost += (end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
//    return reinterpret_cast<unsigned long>(rp);
//}
//vm_address_t hookAfter(struct RegState_ *rs)
//{
//    pa_list args = (pa_list){
//        rs,
//        reinterpret_cast<unsigned char *>(rs->bp),
//        2,
//        0};
//    ThreadCallStack *cs = getThreadCallStack(threadKey);
//    CallRecord *cr = popCallRecordSafe(cs, (void *)rs->bp);
//    if (cr)
//    {
//        // printCallRcordReturnValue(cr, cs, rs);
//        return reinterpret_cast<unsigned long>(cr->ret);
//    }
//    else
//    {
//        cr = popCallRecord(cs);
//        return reinterpret_cast<unsigned long>(cr->ret);
//    }
//}
