//
//  MSJSONTools.h
//  Pods-TDWAPMSDK
//
//  Created by dingtao on 2018/9/5.
//

#import <Foundation/Foundation.h>

@interface MSJSONTools: NSObject

+ (NSString*)dictionaryToJson:(NSDictionary *)dic;

+ (NSDictionary *)dicFromObject:(NSObject *)object;

+ (NSString *)stringFromDict:(NSDictionary *)dict;

+ (NSString *)prettyStringFromDict:(NSDictionary *)dict;

@end
