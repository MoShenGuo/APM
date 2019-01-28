//
//  MSCallTraceCore.h
//  Pods-MSAPM
//
//  Created by guoxiaoliang on 2019/1/25.
//

#ifndef MSCallTraceCore_h
#define MSCallTraceCore_h

#include <stdio.h>

#include <objc/objc.h>

typedef struct {
    __unsafe_unretained Class cls;
    SEL sel;
    uint64_t time; // us (1/1000 ms)
    int depth;
} msCallRecord;

extern void msCallTraceStart();
extern void msCallTraceStop();

extern void msCallConfigMinTime(uint64_t us); //default 1000
extern void msCallConfigMaxDepth(int depth);  //default 3

extern msCallRecord *msGetCallRecords(int *num);
extern void msClearCallRecords();

#endif /* MSCallTraceCore_h */
