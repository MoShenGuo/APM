//
//  MSCallStack.h
//  MSAPM
//
//  Created by guoxiaoliang on 2019/1/27.
//

#import <Foundation/Foundation.h>
#import "MSCallLib.h"
NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MSCallStackType) {
    MSCallStackTypeAll,     //全部线程
    MSCallStackTypeMain,    //主线程
    MSCallStackTypeCurrent  //当前线程
};
@interface MSCallStack : NSObject

+ (NSString *)callStackWithType:(MSCallStackType)type;

extern NSString *msStackOfThread(thread_t thread);
@end

NS_ASSUME_NONNULL_END
