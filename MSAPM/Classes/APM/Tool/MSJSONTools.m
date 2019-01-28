//
//  MSJSONTools.m
//  Pods-MSAPMSDK
//
//  Created by dingtao on 2018/9/5.
//

#import "MSJSONTools.h"
#import <objc/runtime.h>

@implementation MSJSONTools

+ (NSString*)dictionaryToJson:(NSDictionary *)dict
{
    NSMutableString *string = [[NSMutableString alloc] init];
    if (dict.count == 0) {
        return [string copy];
    }

    [dict enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop) {
        if ([obj isKindOfClass:[NSString class]]) {
            NSString *str = (NSString *)obj;
            str = [str stringByReplacingOccurrencesOfString:@"\"" withString:@""];
            str = [str stringByReplacingOccurrencesOfString:@"{" withString:@""];
            str = [str stringByReplacingOccurrencesOfString:@"}" withString:@""];
            [string appendFormat:@"%@:%@\\n", key, str];
        }
    }];

    return string;
}


+ (NSString *)stringFromDict:(NSDictionary *)dict {
    NSMutableString *string = [[NSMutableString alloc] init];
    if (dict.count == 0) {
        return [string copy];
    }

    [string appendFormat:@"{"];
    [dict enumerateKeysAndObjectsUsingBlock:^(id key, id _Nonnull obj, BOOL * _Nonnull stop) {
        if ([key isEqualToString:@"TCPObjectId"]) {
            #ifdef DEBUG
            NSLog(@"%@", obj);
            #else
            #endif
        }
        if ([obj isKindOfClass:[NSString class]]) {
            [string appendFormat:@"%@,", [NSString stringWithFormat:@"\"%@\": \"%@\"",key,obj]];
        }
        else if ([obj isKindOfClass:[NSNumber class]]) {
            [string appendFormat:@"%@,", [NSString stringWithFormat:@"\"%@\": %@",key,obj]];
        }
        else if ([obj isKindOfClass:[NSNull class]]) {
            [string appendFormat:@"%@,", [NSString stringWithFormat:@"\"%@\": \"%s\"",key,""]];
        }
        else if ([obj isKindOfClass:[NSDictionary class]]) {
            [string appendFormat:@"%@,", [NSString stringWithFormat:@"\"%@\": %@",key,[self stringFromDict:obj]]];
        }
    }];
    string = [[NSMutableString alloc] initWithString:[string substringToIndex:string.length-1]];
    [string appendFormat:@"}"];
    return [string copy];
}

+ (NSString *)prettyStringFromDict:(NSDictionary *)dict {
    NSMutableString *string = [[NSMutableString alloc] init];
    [string appendFormat:@"{"];
    [dict enumerateKeysAndObjectsUsingBlock:^(id key, id _Nonnull obj, BOOL * _Nonnull stop) {
        if ([obj isKindOfClass:[NSString class]]) {
            [string appendFormat:@"%@,\n", [NSString stringWithFormat:@"\"%@\": \"%@\"",key,obj]];
        }
        else if ([obj isKindOfClass:[NSNumber class]]) {
            [string appendFormat:@"%@,\n", [NSString stringWithFormat:@"\"%@\": %@",key,obj]];
        }
        else if ([obj isKindOfClass:[NSNull class]]) {
            [string appendFormat:@"%@,\n", [NSString stringWithFormat:@"\"%@\": \"%s\"",key,""]];
        }
        else if ([obj isKindOfClass:[NSDictionary class]]) {
            [string appendFormat:@"%@,\n", [NSString stringWithFormat:@"\"%@\":  %@",key,[self prettyStringFromDict:obj]]];
        }
    }];
    string = [[NSMutableString alloc] initWithString:[string substringToIndex:string.length-2]];
    [string appendFormat:@"}"];
    return [string copy];
}

+ (NSDictionary *)dicFromObject:(NSObject *)object {
    NSMutableDictionary *dic = [NSMutableDictionary dictionary];
    unsigned int count;
    objc_property_t *propertyList = class_copyPropertyList([object class], &count);
    for (int i = 0; i < count; i++) {
        objc_property_t property = propertyList[i];
        const char *cName = property_getName(property);
        NSString *name = [NSString stringWithUTF8String:cName];
        NSObject *value = [object valueForKey:name];//valueForKey返回的数字和字符串都是对象

        if ([value isKindOfClass:[NSString class]] || [value isKindOfClass:[NSNumber class]]) {
            //string , bool, int ,NSinteger
            if ([value isKindOfClass:NSClassFromString(@"__NSCFBoolean")]) {
                [dic setObject: [(NSNumber *)value integerValue]!= 0 ? @"true" : @"false" forKey:name];
            } else {
                [dic setObject:value forKey:name];
            }
        } else if ([value isKindOfClass:[NSArray class]] || [value isKindOfClass:[NSDictionary class]]) {
            //数组或字典
            [dic setObject:[self arrayOrDicWithObject:(NSArray*)value] forKey:name];
        } else if (value == nil) {
//            //null
//            [dic setObject:[NSNull null] forKey:name];
        } else {
            //model
            [dic setObject:[self dicFromObject:value] forKey:name];
        }
    }
    // 释放内存
    free(propertyList);

    return [dic copy];
}

//将可能存在model数组转化为普通数组
+ (id)arrayOrDicWithObject:(id)origin {
    if ([origin isKindOfClass:[NSArray class]]) {
        //数组
        NSMutableArray *array = [NSMutableArray array];
        for (NSObject *object in origin) {
            if ([object isKindOfClass:[NSString class]] || [object isKindOfClass:[NSNumber class]]) {
                //string , bool, int ,NSinteger
                [array addObject:object];
            } else if ([object isKindOfClass:[NSArray class]] || [object isKindOfClass:[NSDictionary class]]) {
                //数组或字典
                [array addObject:[self arrayOrDicWithObject:(NSArray *)object]];
            } else {
                //model
                [array addObject:[self dicFromObject:object]];
            }
        }
        return [array copy];
    } else if ([origin isKindOfClass:[NSDictionary class]]) {
        //字典
        NSDictionary *originDic = (NSDictionary *)origin;
        NSMutableDictionary *dic = [NSMutableDictionary dictionary];
        for (NSString *key in originDic.allKeys) {
            id object = [originDic objectForKey:key];
            if ([object isKindOfClass:[NSString class]] || [object isKindOfClass:[NSNumber class]]) {
                //string , bool, int ,NSinteger
                [dic setObject:object forKey:key];
                if ([object isKindOfClass:NSClassFromString(@"_")]) {
                    [dic setObject: object != 0 ? @"false" : @"true" forKey:key];
                }
            } else if ([object isKindOfClass:[NSArray class]] || [object isKindOfClass:[NSDictionary class]]) {
                //数组或字典
                [dic setObject:[self arrayOrDicWithObject:object] forKey:key];
            } else {
                //model
                [dic setObject:[self dicFromObject:object] forKey:key];
            }
        }
        return [dic copy];
    }
    return [NSNull null];
}
@end
