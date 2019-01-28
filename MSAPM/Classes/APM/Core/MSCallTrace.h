//
//  MSCallTrace.h
//  Pods-MSAPM
//
//  Created by guoxiaoliang on 2019/1/26.
//

#import <Foundation/Foundation.h>
#import "MSCallTraceCore.h"
NS_ASSUME_NONNULL_BEGIN

@protocol MSCallTraceDelegate <NSObject>

@optional
- (void)callTraceObjectMethodCallInformation:(NSString *)objectMethods;

@end
@interface MSCallTrace : NSObject
@property(nonatomic,weak)id<MSCallTraceDelegate>delegate;
+ (instancetype)sharedInstance;
+ (void)start; //开始记录
+ (void)startWithMaxDepth:(int)depth;
+ (void)startWithMinCost:(double)ms;
+ (void)startWithMaxDepth:(int)depth minCost:(double)ms;
+ (void)stop; //停止记录
+ (void)save; //保存和打印记录，如果不是短时间 stop 的话使用 saveAndClean
+ (void)stopSaveAndClean; //停止保存打印并进行内存清理
+ (void)asyncSave;
//int smRebindSymbols(struct smRebinding rebindings[], size_t rebindings_nel);
@end

NS_ASSUME_NONNULL_END
