//
//  MSCallTraceTimeCostModel.m
//  MSAPM
//
//  Created by guoxiaoliang on 2019/1/27.
//

#import "MSCallTraceTimeCostModel.h"

@implementation MSCallTraceTimeCostModel

- (NSString *)des {
    NSMutableString *str = [NSMutableString new];
    [str appendFormat:@"%2d| ",(int)_callDepth];
    [str appendFormat:@"%6.2f|",_timeCost * 1000.0];
    for (NSUInteger i = 0; i < _callDepth; i++) {
        [str appendString:@"  "];
    }
    [str appendFormat:@"%s[%@ %@]", (_isClassMethod ? "+" : "-"), _className, _methodName];
    return str;
}
@end
