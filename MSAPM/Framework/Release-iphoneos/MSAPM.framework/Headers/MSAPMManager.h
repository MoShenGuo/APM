//
//  MSAPMManager.h
//  MSAPM
//
//  Created by guoxiaoliang on 2019/1/27.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
static NSString *pathFlag = @"filePath";
//监控指标
typedef enum _TDMonitoringIndicators
{
    _TDMonitoringIndicatorsALL = 1,//所有的
    _TDMonitoringIndicatorsBase = 2,//基本性能数据
    _TDMonitoringIndicatorsFPS = 3,//帧率FPS
    _TDMonitoringIndicatorsNetwork = 4,//网络
    _TDMonitoringIndicatorsCaton = 5,//卡顿
    _TDMonitoringIndicatorsCrash = 6,//崩溃
} TDMonitoringIndicators;
@interface MSAPMManager : NSObject

+ (instancetype)sharedInstance;

//异步获取数据,生命周期方法名
- (void)asyncExecuteClassName:(NSString *)className withStartTime:(NSString *)startTime withEndTime:(NSString *)endTime withHookMethod:(NSString *)hookMethod withUniqueIdentifier:(NSString *)uniqueIdentifier;

//定时将数据字符串写入沙盒文件 兼容之前写main分支代码
- (void)startToCollectPerformanceData;
//停止监控性能
- (void)stopAppPerformanceMonitor;
//清空txt文件缓存
- (void)clearTxt;
//改变监控指标状态 Indicators:监控指标,isStartMonitor:监控是否开启与关闭
- (void)didChangeMonitoringIndicators: (TDMonitoringIndicators)Indicators withChangeStatus:(BOOL)isStartMonitor;
- (void)writeSandbox;

@end

NS_ASSUME_NONNULL_END
