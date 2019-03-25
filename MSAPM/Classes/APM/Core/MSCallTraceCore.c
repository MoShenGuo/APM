//
//  MSCallTraceCore.c
//  Pods-MSAPM
//
//  Created by guoxiaoliang on 2019/1/25.
//

#include "MSCallTraceCore.h"

//#if (GCC_VERSION >= 40100)
///* 内存访问栅 */
//#define barrier()                 (__sync_synchronize())
///* 原子获取 */
//#define AO_GET(ptr)               ({ __typeof__(*(ptr)) volatile *_val = (ptr); barrier(); (*_val); })
///*原子设置，如果原值和新值不一样则设置*/
//#define AO_SET(ptr, value)        ((void)__sync_lock_test_and_set((ptr), (value)))
///* 原子交换，如果被设置，则返回旧值，否则返回设置值 */
//#define AO_SWAP(ptr, value)       ((__typeof__(*(ptr)))__sync_lock_test_and_set((ptr), (value)))
///* 原子比较交换，如果当前值等于旧值，则新值被设置，返回旧值，否则返回新值*/
//#define AO_CAS(ptr, comp, value)  ((__typeof__(*(ptr)))__sync_val_compare_and_swap((ptr), (comp), (value)))
///* 原子比较交换，如果当前值等于旧指，则新值被设置，返回真值，否则返回假 */
//#define AO_CASB(ptr, comp, value) (__sync_bool_compare_and_swap((ptr), (comp), (value)) != 0 ? true : false)
///* 原子清零 */
//#define AO_CLEAR(ptr)             ((void)__sync_lock_release((ptr)))
///* 通过值与旧值进行算术与位操作，返回新值 */
//#define AO_ADD_F(ptr, value)      ((__typeof__(*(ptr)))__sync_add_and_fetch((ptr), (value)))
//#define AO_SUB_F(ptr, value)      ((__typeof__(*(ptr)))__sync_sub_and_fetch((ptr), (value)))
//#define AO_OR_F(ptr, value)       ((__typeof__(*(ptr)))__sync_or_and_fetch((ptr), (value)))
//#define AO_AND_F(ptr, value)      ((__typeof__(*(ptr)))__sync_and_and_fetch((ptr), (value)))
//#define AO_XOR_F(ptr, value)      ((__typeof__(*(ptr)))__sync_xor_and_fetch((ptr), (value)))
///* 通过值与旧值进行算术与位操作，返回旧值 */
//#define AO_F_ADD(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_add((ptr), (value)))
//#define AO_F_SUB(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_sub((ptr), (value)))
//#define AO_F_OR(ptr, value)       ((__typeof__(*(ptr)))__sync_fetch_and_or((ptr), (value)))
//#define AO_F_AND(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_and((ptr), (value)))
//#define AO_F_XOR(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_xor((ptr), (value)))
//#else
//#error  \"can not supported atomic operation by gcc(v4.0.0+) buildin function.";
//#endif    /* if (GCC_VERSION >= 40100) */
///* 忽略返回值，算术和位操作 */
//#define AO_INC(ptr)                 ((void)AO_ADD_F((ptr), 1))
//#define AO_DEC(ptr)                 ((void)AO_SUB_F((ptr), 1))
//#define AO_ADD(ptr, val)            ((void)AO_ADD_F((ptr), (val)))
//#define AO_SUB(ptr, val)            ((void)AO_SUB_F((ptr), (val)))
//#define AO_OR(ptr, val)             ((void)AO_OR_F((ptr), (val)))
//#define AO_AND(ptr, val)            ((void)AO_AND_F((ptr), (val)))
//#define AO_XOR(ptr, val)            ((void)AO_XOR_F((ptr), (val)))
///* 通过掩码，设置某个位为1，并返还新的值 */
//#define AO_BIT_ON(ptr, mask)        AO_OR_F((ptr), (mask))
///* 通过掩码，设置某个位为0，并返还新的值 */
//#define AO_BIT_OFF(ptr, mask)       AO_AND_F((ptr), ~(mask))
///* 通过掩码，交换某个位，1变0，0变1，并返还新的值 */
//#define AO_BIT_XCHG(ptr, mask)      AO_XOR_F((ptr), (mask))


#pragma mark - fishhook

#ifdef __aarch64__

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#ifdef __LP64__
typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct section_64 section_t;
typedef struct nlist_64 nlist_t;
#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT_64
#else
typedef struct mach_header mach_header_t;
typedef struct segment_command segment_command_t;
typedef struct section section_t;
typedef struct nlist nlist_t;
#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT
#endif

#ifndef SEG_DATA_CONST
#define SEG_DATA_CONST  "__DATA_CONST"
#endif

struct rebinding {
    const char *name;
    void *replacement;
    void **replaced;
};
/*
 * For each rebinding in rebindings, rebinds references to external, indirect
 * symbols with the specified name to instead point at replacement for each
 * image in the calling process as well as for all future images that are loaded
 * by the process. If rebind_functions is called more than once, the symbols to
 * rebind are added to the existing list of rebindings, and if a given symbol
 * is rebound more than once, the later rebinding will take precedence.
 */
static int fish_rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel);



struct rebindings_entry {
    struct rebinding *rebindings;
    size_t rebindings_nel;
    struct rebindings_entry *next;
};

static struct rebindings_entry *_rebindings_head;

static int prepend_rebindings(struct rebindings_entry **rebindings_head,
                              struct rebinding rebindings[],
                              size_t nel) {
    struct rebindings_entry *new_entry = (struct rebindings_entry *) malloc(sizeof(struct rebindings_entry));
    if (!new_entry) {
        return -1;
    }
    new_entry->rebindings = (struct rebinding *) malloc(sizeof(struct rebinding) * nel);
    if (!new_entry->rebindings) {
        free(new_entry);
        return -1;
    }
    memcpy(new_entry->rebindings, rebindings, sizeof(struct rebinding) * nel);
    new_entry->rebindings_nel = nel;
    new_entry->next = *rebindings_head;
    *rebindings_head = new_entry;
    return 0;
}

static void perform_rebinding_with_section(struct rebindings_entry *rebindings,
                                           section_t *section,
                                           intptr_t slide,
                                           nlist_t *symtab,
                                           char *strtab,
                                           uint32_t *indirect_symtab) {
    uint32_t *indirect_symbol_indices = indirect_symtab + section->reserved1;
    void **indirect_symbol_bindings = (void **)((uintptr_t)slide + section->addr);
    for (uint i = 0; i < section->size / sizeof(void *); i++) {
        uint32_t symtab_index = indirect_symbol_indices[i];
        if (symtab_index == INDIRECT_SYMBOL_ABS || symtab_index == INDIRECT_SYMBOL_LOCAL ||
            symtab_index == (INDIRECT_SYMBOL_LOCAL   | INDIRECT_SYMBOL_ABS)) {
            continue;
        }
        uint32_t strtab_offset = symtab[symtab_index].n_un.n_strx;
        char *symbol_name = strtab + strtab_offset;
        bool symbol_name_longer_than_1 = symbol_name[0] && symbol_name[1];
        struct rebindings_entry *cur = rebindings;
        while (cur) {
            for (uint j = 0; j < cur->rebindings_nel; j++) {
                if (symbol_name_longer_than_1 &&
                    strcmp(&symbol_name[1], cur->rebindings[j].name) == 0) {
                    if (cur->rebindings[j].replaced != NULL &&
                        indirect_symbol_bindings[i] != cur->rebindings[j].replacement) {
                        *(cur->rebindings[j].replaced) = indirect_symbol_bindings[i];
                    }
                    indirect_symbol_bindings[i] = cur->rebindings[j].replacement;
                    goto symbol_loop;
                }
            }
            cur = cur->next;
        }
    symbol_loop:;
    }
}

static void rebind_symbols_for_image(struct rebindings_entry *rebindings,
                                     const struct mach_header *header,
                                     intptr_t slide) {
    Dl_info info;
    if (dladdr(header, &info) == 0) {
        return;
    }
    
    segment_command_t *cur_seg_cmd;
    segment_command_t *linkedit_segment = NULL;
    struct symtab_command* symtab_cmd = NULL;
    struct dysymtab_command* dysymtab_cmd = NULL;
    
    uintptr_t cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_LINKEDIT) == 0) {
                linkedit_segment = cur_seg_cmd;
            }
        } else if (cur_seg_cmd->cmd == LC_SYMTAB) {
            symtab_cmd = (struct symtab_command*)cur_seg_cmd;
        } else if (cur_seg_cmd->cmd == LC_DYSYMTAB) {
            dysymtab_cmd = (struct dysymtab_command*)cur_seg_cmd;
        }
    }
    
    if (!symtab_cmd || !dysymtab_cmd || !linkedit_segment ||
        !dysymtab_cmd->nindirectsyms) {
        return;
    }
    
    // Find base symbol/string table addresses
    uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
    nlist_t *symtab = (nlist_t *)(linkedit_base + symtab_cmd->symoff);
    char *strtab = (char *)(linkedit_base + symtab_cmd->stroff);
    
    // Get indirect symbol table (array of uint32_t indices into symbol table)
    uint32_t *indirect_symtab = (uint32_t *)(linkedit_base + dysymtab_cmd->indirectsymoff);
    
    cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_DATA) != 0 &&
                strcmp(cur_seg_cmd->segname, SEG_DATA_CONST) != 0) {
                continue;
            }
            for (uint j = 0; j < cur_seg_cmd->nsects; j++) {
                section_t *sect =
                (section_t *)(cur + sizeof(segment_command_t)) + j;
                if ((sect->flags & SECTION_TYPE) == S_LAZY_SYMBOL_POINTERS) {
                    perform_rebinding_with_section(rebindings, sect, slide, symtab, strtab, indirect_symtab);
                }
                if ((sect->flags & SECTION_TYPE) == S_NON_LAZY_SYMBOL_POINTERS) {
                    perform_rebinding_with_section(rebindings, sect, slide, symtab, strtab, indirect_symtab);
                }
            }
        }
    }
}

static void _rebind_symbols_for_image(const struct mach_header *header,
                                      intptr_t slide) {
    rebind_symbols_for_image(_rebindings_head, header, slide);
}

int rebind_symbols_image(void *header,
                         intptr_t slide,
                         struct rebinding rebindings[],
                         size_t rebindings_nel) {
    struct rebindings_entry *rebindings_head = NULL;
    int retval = prepend_rebindings(&rebindings_head, rebindings, rebindings_nel);
    rebind_symbols_for_image(rebindings_head, (const struct mach_header *) header, slide);
    if (rebindings_head) {
        free(rebindings_head->rebindings);
    }
    free(rebindings_head);
    return retval;
}

static int fish_rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel) {
    int retval = prepend_rebindings(&_rebindings_head, rebindings, rebindings_nel);
    if (retval < 0) {
        return retval;
    }
    // If this was the first call, register callback for image additions (which is also invoked for
    // existing images, otherwise, just run on existing images
    //首先是遍历 dyld 里的所有的 image，取出 image header 和 slide。注意第一次调用时主要注册 callback
    if (!_rebindings_head->next) {
        _dyld_register_func_for_add_image(_rebind_symbols_for_image);
    } else {
        uint32_t c = _dyld_image_count();
        for (uint32_t i = 0; i < c; i++) {
            _rebind_symbols_for_image(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i));
        }
    }
    return retval;
}
int rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel) {
    int retval = prepend_rebindings(&_rebindings_head, rebindings, rebindings_nel);
    if (retval < 0) {
        return retval;
    }
    // If this was the first call, register callback for image additions (which is also invoked for
    // existing images, otherwise, just run on existing images
    if (!_rebindings_head->next) {
        _dyld_register_func_for_add_image(_rebind_symbols_for_image);
    } else {
        uint32_t c = _dyld_image_count();
        for (uint32_t i = 0; i < c; i++) {
            _rebind_symbols_for_image(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i));
        }
    }
    return retval;
}

#pragma mark - Record

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#include <pthread.h>

static bool _call_record_enabled = true;
static uint64_t _min_time_cost = 1000; //us
static int _max_call_depth = 3;
static pthread_key_t _thread_key;
__unused static id (*orig_objc_msgSend)(id, SEL, ...);

static msCallRecord *_msCallRecords;
//static int otp_record_num;
//static int otp_record_alloc;
static int _msRecordNum;
static int _msRecordAlloc;

typedef struct {
    id self; //通过 object_getClass 能够得到 Class 再通过 NSStringFromClass 能够得到类名
    Class cls;
    SEL cmd; //通过 NSStringFromSelector 方法能够得到方法名
    uint64_t time; //us
    uintptr_t lr; // link register
} thread_call_record;

typedef struct {
    thread_call_record *stack;
    int allocated_length;
    int index;
    bool is_main_thread;
} thread_call_stack;
static inline thread_call_stack * get_thread_call_stack() {
    thread_call_stack *cs = (thread_call_stack *)pthread_getspecific(_thread_key);
    if (cs == NULL) {
        cs = (thread_call_stack *)malloc(sizeof(thread_call_stack));
        cs->stack = (thread_call_record *)calloc(128, sizeof(thread_call_record));
        cs->allocated_length = 64;
        cs->index = -1;
        cs->is_main_thread = pthread_main_np();
        pthread_setspecific(_thread_key, cs);
    }
    return cs;
}

static void release_thread_call_stack(void *ptr) {
    thread_call_stack *cs = (thread_call_stack *)ptr;
    if (!cs) return;
    if (cs->stack) free(cs->stack);
    free(cs);
}

static inline void push_call_record(id _self, Class _cls, SEL _cmd, uintptr_t lr) {
    thread_call_stack *cs = get_thread_call_stack();
    if (cs) {
        int nextIndex = (++cs->index);
        if (nextIndex >= cs->allocated_length) {
            cs->allocated_length += 64;
            /*
             void *realloc( void *ptr, size_t new_size );
             重新分配给定的内存区域。它必须是之前为 malloc() 、 calloc() 或 realloc() 所分配，并且仍未被 free 或 realloc 的调用所释放。否则，结果未定义。
             重新分配按以下二者之一执行：
             a) 可能的话，扩张或收缩 ptr 所指向的已存在内存。内容在新旧大小中的较小者范围内保持不变。若扩张范围，则数组新增部分的内容是未定义的。
             b) 分配一个大小为 new_size 字节的新内存块，并复制大小等于新旧大小中较小者的内存区域，然后释放旧内存块。
             若无足够内存，则不释放旧内存块，并返回空指针。
             
             若 ptr 是 NULL ，则行为与调用 malloc(new_size) 相同。
             ptr    -    指向需要重新分配的内存区域的指针
             new_size    -    数组的新大小（字节数）
             */
            //重新分配内存
            cs->stack = (thread_call_record *)realloc(cs->stack, cs->allocated_length * sizeof(thread_call_record));
        }
        thread_call_record *newRecord = &cs->stack[nextIndex];
        newRecord->self = _self;
        newRecord->cls = _cls;
        newRecord->cmd = _cmd;
        newRecord->lr = lr;
       
        if (cs->is_main_thread && _call_record_enabled) {
            struct timeval now;
            //int gettimeofday(struct timeval*tv, struct timezone *tz);
            /*
             struct timeval{
             long int tv_sec; // 秒数
             long int tv_usec; // 微秒数
             }
             */
            gettimeofday(&now, NULL);
            newRecord->time = (now.tv_sec % 100) * 1000000 + now.tv_usec;
        }
    }
}

static inline uintptr_t pop_call_record() {
    thread_call_stack *cs = get_thread_call_stack();

    int curIndex = cs->index;
    int nextIndex = cs->index--;
    thread_call_record *pRecord = &cs->stack[nextIndex];
    
    if (cs->is_main_thread && _call_record_enabled) {
        struct timeval now;
        /*gettimeofday 系统调用可以获取系统当前挂钟时间（Wall-Clock Time）。它的第一个参数是一个指向 struct timeval 类型空间的指针。这个结构可以表示一个以秒为单位的时间。这个值被分为两个域，tv_sec 表示整秒数，而 tv_usec 表示剩余的微秒部分。整个 struct timeval 值表示的是从 Unix ''epoch''（UTC 时间 1970 年 1 月 1 日）开始到当前流逝的时间
         用到精确到毫秒的时间
         注意了其中的(int64_t)类型转换对于32位的系统是必须的，否则乘上1000会溢出
         int64_t ts = (int64_t)tv.tv_sec*1000 + tv.tv_usec/1000;
        */
        gettimeofday(&now, NULL);
        uint64_t time = (now.tv_sec % 100) * 1000000 + now.tv_usec;
        if (time < pRecord->time) {
            time += 100 * 1000000;
        }
        //记录数据保存在_msCallRecords中
        //计算出耗时
        uint64_t cost = time - pRecord->time;
        if (cost > _min_time_cost && cs->index < _max_call_depth) {
            //如果没有回调记录 创建了一个
            if (!_msCallRecords) {
                _msRecordAlloc = 1024;
                _msCallRecords = malloc(sizeof(msCallRecord) * _msRecordAlloc);
            }
            
            //记录
            _msRecordNum++;
            if (_msRecordNum >= _msRecordAlloc) {
                _msRecordAlloc += 1024;
                _msCallRecords = realloc(_msCallRecords, sizeof(msCallRecord) * _msRecordAlloc);
            }
            msCallRecord *log = &_msCallRecords[_msRecordNum - 1];
            log->cls = pRecord->cls;
            log->depth = curIndex;
            log->sel = pRecord->cmd;
            log->time = cost;
        }
    }
    return pRecord->lr;
}

void before_objc_msgSend(id self, SEL _cmd, uintptr_t lr) {
    push_call_record(self, object_getClass(self), _cmd, lr);
}

uintptr_t after_objc_msgSend() {
    return pop_call_record();
}

//replacement objc_msgSend (arm64)
// https://blog.nelhage.com/2010/10/amd64-and-va_arg/
// http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf
// https://developer.apple.com/library/ios/documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARM64FunctionCallingConventions.html
/*
 ARM64架构的处理器有31个64bit的整数寄存器，分别被标记为x0 - x30。每一个寄存器也可以分离开来只使用低32bit，标记为w0 - w30。其中x0 - x7是用来传递函数的参数的。这意味着objc_msgSend接收的self参数放在x0上，_cmd参数放在x1上。
 SP寄存器其实就是 x31，在指令编码中，使用 SP/WSP来进行对SP寄存器的访问。
 一般来说 arm64上 x0 – x7 分别会存放方法的前 8 个参数
 如果参数个数超过了8个，多余的参数会存在栈上，新方法会通过栈来读取。
 方法的返回值一般都在 x0 上。
 如果方法返回值是一个较大的数据结构时，结果会存在 x8 执行的地址上。
 sp指向栈顶，也就是低地址.fp指向当前frame的栈底，也就是高地址
 "stp x8, x9, [sp, #-16]!\n":将x8,x9保存到[sp, #-16]地址上去,[sp, #-16]!表示sp缓存的地址偏移16个字节位置,地址后面跟着一个感叹号，这是一个非常有趣的特性。它指示寄存器write-back，寄存器会先更新自己的值，之后再进行其他操作。上面的这条指令会先sp -= 16并保存到x12中
 "mov x12, %0\n" :: "r"(value):将value值存入x12中
 ldp x8, x9, [sp], #16: 从sp地址取出 16 byte数据，分别存入x8, x9. 然后 sp+=16;
 sub sp, sp, #16 将sp- #16赋值给sp
 add sp, sp, #16 将sp + #16赋值给sp
 stp x8, x9, [sp, #-16]!:把 x8, x9的值存到 sp-16的地址上，并且把 sp-=16.
 blr  x12:跳转到由x12目标寄存器指定的地址处，同时将下一条指令存放到X30寄存器中
 ret;    // 返回指令，这一步直接执行lr的指令。
 */
/// 函数调用，value传入函数地址
#define call(b, value) \
__asm volatile ("stp x8, x9, [sp, #-16]!\n"); \
__asm volatile ("mov x12, %0\n" :: "r"(value)); \
__asm volatile ("ldp x8, x9, [sp], #16\n"); \
__asm volatile (#b " x12\n");
/// 保存寄存器参数信息
#define save() \
__asm volatile ( \
"stp x8, x9, [sp, #-16]!\n" \
"stp x6, x7, [sp, #-16]!\n" \
"stp x4, x5, [sp, #-16]!\n" \
"stp x2, x3, [sp, #-16]!\n" \
"stp x0, x1, [sp, #-16]!\n");
/// 还原寄存器参数信息
#define load() \
__asm volatile ( \
"ldp x0, x1, [sp], #16\n" \
"ldp x2, x3, [sp], #16\n" \
"ldp x4, x5, [sp], #16\n" \
"ldp x6, x7, [sp], #16\n" \
"ldp x8, x9, [sp], #16\n" );

#define link(b, value) \
__asm volatile ("stp x8, lr, [sp, #-16]!\n"); \
__asm volatile ("sub sp, sp, #16\n"); \
call(b, value); \
__asm volatile ("add sp, sp, #16\n"); \
__asm volatile ("ldp x8, lr, [sp], #16\n");

#define ret() __asm volatile ("ret\n");
/// msgSend必须使用汇编实现
//__naked__修饰的函数告诉编译器在函数调用的时候不使用栈保存参数信息，同时函数返回地址会被保存到LR寄存器上。由于msgSend本身就是用这个修饰符的，因此在记录函数调用的出入栈操作中，必须保证能够保存以及还原寄存器数据。msgSend利用x0 - x9的寄存器存储参数信息，可以手动使用sp寄存器来存储和还原这些参数信息：
__attribute__((__naked__))
static void hook_Objc_msgSend() {
    // Save parameters.
    /// 保存寄存器参数信息
    save()
    //lr 是link register中的值
    //mov x2, lr: 将x2设置为lr
    __asm volatile ("mov x2, lr\n");
    //将设置x3设置为x4
    __asm volatile ("mov x3, x4\n");
    
    /// 函数调用，value传入函数地址
    // Call our before_objc_msgSend.
    //blr 到before_objc_msgSend方法执行
    call(blr, &before_objc_msgSend)
    
    /// 还原寄存器参数信息
    // Load parameters.
    load()
    
     /// 函数调用，value传入函数地址
    // Call through to the original objc_msgSend.
    //跳转到orig_objc_msgSend指向地址的方法执行
    call(blr, orig_objc_msgSend)
    
    /// 保存寄存器参数信息
    // Save original objc_msgSend return value.
    save()
    
     /// 函数调用，value传入函数地址
    // Call our after_objc_msgSend.
    call(blr, &after_objc_msgSend)
    
    // restore lr
    //恢复lr
    __asm volatile ("mov lr, x0\n");
    
    /// 还原寄存器参数信息
    // Load original objc_msgSend return value.
    load()
    
    // return
    // __asm volatile ("ret\n");
    ret()
}

#pragma mark public

void msCallTraceStart() {
    _call_record_enabled = true;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        pthread_key_create(&_thread_key, &release_thread_call_stack);
        fish_rebind_symbols((struct rebinding[6]){
            {"objc_msgSend", (void *)hook_Objc_msgSend, (void **)&orig_objc_msgSend},
        }, 1);
    });
}
void observe_Objc_msgSend() {
    struct rebinding msgSend_rebinding = { "objc_msgSend", hook_Objc_msgSend, (void *)&orig_objc_msgSend };
    rebind_symbols((struct rebinding[1]){msgSend_rebinding}, 1);
}
void msCallTraceStop() {
    _call_record_enabled = false;
}

void msCallConfigMinTime(uint64_t us) {
    _min_time_cost = us;
}
void msCallConfigMaxDepth(int depth) {
    _max_call_depth = depth;
}

msCallRecord *msGetCallRecords(int *num) {
    if (num) {
        *num = _msRecordNum;
    }
    return _msCallRecords;
}
//清除线程里有数据
void msClearThreadDataRecords() {
    thread_call_stack *cs = (thread_call_stack *)pthread_getspecific(_thread_key);
    if (!cs) return;
    if (cs->stack) free(cs->stack);
    free(cs);
     pthread_setspecific(_thread_key, NULL);
}
void msClearCallRecords() {
    if (_msCallRecords) {
        free(_msCallRecords);
        _msCallRecords = NULL;
    }
    _msRecordNum = 0;
    //清除线程数据
   // msClearThreadDataRecords();
}

#else
void msCallTraceStart() {}
void msCallTraceStop() {}
void msCallConfigMinTime(uint64_t us) {
}
void msCallConfigMaxDepth(int depth) {
}
msCallRecord *msGetCallRecords(int *num) {
    if (num) {
        *num = 0;
    }
    return NULL;
}
void msClearCallRecords() {}

#endif
