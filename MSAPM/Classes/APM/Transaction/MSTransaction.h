//
//  MSTransaction.h
//  MSAPM
//
//  Created by guoxiaoliang on 2019/1/27.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
/*!
 *  @brief  任务封装
 */
@interface MSTransaction : NSObject

+ (void)begin;
+ (void)commit;

@end

NS_ASSUME_NONNULL_END
