//
//  MSCallTrace.m
//  Pods-MSAPM
//
//  Created by guoxiaoliang on 2019/1/26.
//

#import "MSCallTrace.h"
#import "MSCallLib.h"
#import "MSCallTraceTimeCostModel.h"
#import "TDDispatchAsync.h"
@implementation MSCallTrace

#pragma mark - Trace
#pragma mark - OC Interface

+ (instancetype)sharedInstance
{
    static MSCallTrace * instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[MSCallTrace alloc] init];
    });
    return instance;
}
//开始记录
+ (void)start; {
    msCallTraceStart();
}
+ (void)startWithMaxDepth:(int)depth {
    msCallConfigMaxDepth(depth);
    [MSCallTrace start];
}
+ (void)startWithMinCost:(double)ms{
    msCallConfigMinTime(ms * 1000);
    [MSCallTrace start];
}
+ (void)startWithMaxDepth:(int)depth minCost:(double)ms{
    msCallConfigMaxDepth(depth);
    msCallConfigMinTime(ms * 1000);
    [MSCallTrace start];
}
//停止记录
+ (void)stop {
    msCallTraceStop();
}
//保存和打印记录，如果不是短时间 stop 的话使用 saveAndClean
+ (void)save {
    NSMutableString *mStr = [NSMutableString new];
    NSArray<MSCallTraceTimeCostModel *> *arr = [self loadRecords];
    for (MSCallTraceTimeCostModel *model in arr) {
        //记录方法路径
        model.path = [NSString stringWithFormat:@"[%@ %@]",model.className,model.methodName];
        [self appendRecord:model to:mStr];
    }
    //NSLog(@"--------%@",mStr);
}
+ (void)asyncSave {
    
     //__weak typeof(self) weakSelf = self;
  //  [self asyncExecute:^{
        
        NSMutableString *mStr = [NSMutableString new];
        NSArray<MSCallTraceTimeCostModel *> *arr = [self loadRecords];
        for (MSCallTraceTimeCostModel *model in arr) {
            //记录方法路径
            model.path = [NSString stringWithFormat:@"[%@ %@]",model.className,model.methodName];
            [self appendRecord:model to:mStr];
        }
    
        if ([MSCallTrace sharedInstance].delegate && [[MSCallTrace sharedInstance].delegate respondsToSelector:@selector(callTraceObjectMethodCallInformation:)]) {
            [[MSCallTrace sharedInstance].delegate callTraceObjectMethodCallInformation:mStr];
        }
  //  }];
}
//停止保存打印并进行内存清理
+ (void)stopSaveAndClean{
    [MSCallTrace stop];
    [MSCallTrace save];
    msClearCallRecords();
}
+ (void)asyncExecute: (dispatch_block_t)block {
    assert(block != nil);
    if ([NSThread isMainThread]) {
        TDDispatchQueueAsyncBlockInUtility(block);
    } else {
        block();
    }
}
+ (void)appendRecord:(MSCallTraceTimeCostModel *)cost to:(NSMutableString *)mStr {
    [mStr appendFormat:@"%@\n path%@\n",[cost des],cost.path];
    if (cost.subCosts.count < 1) {
        cost.lastCall = YES;
    }
//    //记录到数据库中
//    [[[SMLagDB shareInstance] increaseWithClsCallModel:cost] subscribeNext:^(id x) {}];
    
    for (MSCallTraceTimeCostModel *model in cost.subCosts) {
        //记录方法的子方法的路径
        model.path = [NSString stringWithFormat:@"%@ - [%@ %@]",cost.path,model.className,model.methodName];
        [self appendRecord:model to:mStr];
    }
}
+ (NSArray<MSCallTraceTimeCostModel *>*)loadRecords {
    NSMutableArray<MSCallTraceTimeCostModel *> *arr = [NSMutableArray new];
    int num = 0;
    msCallRecord *records = msGetCallRecords(&num);
    for (int i = 0; i < num; i++) {
        msCallRecord *rd = &records[i];
        MSCallTraceTimeCostModel *model = [MSCallTraceTimeCostModel new];
        model.className = NSStringFromClass(rd->cls);
        model.methodName = NSStringFromSelector(rd->sel);
        model.isClassMethod = class_isMetaClass(rd->cls);
        model.timeCost = (double)rd->time / 1000000.0;
        model.callDepth = rd->depth;
        [arr addObject:model];
    }
//    NSUInteger count = arr.count;
//    for (NSUInteger i = 0; i < count; i++) {
//        MSCallTraceTimeCostModel *model = arr[i];
//        if (model.callDepth > 0) {
//            [arr removeObjectAtIndex:i];
//            //Todo:不需要循环，直接设置下一个，然后判断好边界就行
//            for (NSUInteger j = i; j < count - 1; j++) {
//                //下一个深度小的话就开始将后面的递归的往 sub array 里添加
//                if (arr[j].callDepth + 1 == model.callDepth) {
//                    NSMutableArray *sub = (NSMutableArray *)arr[j].subCosts;
//                    if (!sub) {
//                        sub = [NSMutableArray new];
//                        arr[j].subCosts = sub;
//                    }
//                    [sub insertObject:model atIndex:0];
//                }
//            }
//            i--;
//            count--;
//        }
//    }
    return arr;
}

@end
