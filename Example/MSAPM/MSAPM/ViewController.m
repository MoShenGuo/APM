//
//  ViewController.m
//  MSAPM
//
//  Created by guoxiaoliang on 2019/1/25.
//  Copyright © 2019年 Apple. All rights reserved.
//

#import "ViewController.h"
#include<stdio.h>

#include<sys/time.h>

#include<unistd.h>

#import <sys/stat.h>
#import <dlfcn.h>
#import <mach-o/dyld.h>
#import "FTDeviceSupportHandler.h"
@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
//    checkCydia();
//    checkInject();
    // Do any additional setup after loading the view, typically from a nib.
    [FTDeviceSupportHandler findPrivateFramework];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    NSLog(@"-------touchesBegan--------\n");

    struct timeval now;
     gettimeofday(&now, NULL);
    CFAbsoluteTime startTime =CFAbsoluteTimeGetCurrent();
    for (int i = 0; i <= 1000000; i++) {
        [self testMonitor];
    }
    CFAbsoluteTime linkTime = (CFAbsoluteTimeGetCurrent() - startTime);
    NSLog(@"time=%f",(linkTime - startTime));
}
- (void)testMonitor{
    
}

//使用stat系列函数检测Cydia等工具
void checkCydia(void)
{
    struct stat stat_info;
    if (0 == stat("/Applications/Cydia.app", &stat_info)) {
        NSLog(@"Device is jailbroken \n");
    }
}
//这个方法没有效果
//stat是不是出自系统库，有没有被攻击者换掉：
void checkInject(void)
{
    int ret ;
    Dl_info dylib_info;
    int (*func_stat)(const char *, struct stat *) = stat;
    if ((ret = dladdr(func_stat, &dylib_info))) {
        NSLog(@"lib :%s \n", dylib_info.dli_fname);
    }
}
//列出所有已链接的动态库：通常情况下，会包含越狱机的输出结果会包含字符串： Library/MobileSubstrate/MobileSubstrate.dylib 。
//尽量不要，appStore可能审核不过
void checkDylibs(void)
{
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0 ; i < count; ++i) {
        NSString *name = [[NSString alloc]initWithUTF8String:_dyld_get_image_name(i)];
        if ([name containsString:@"Library/MobileSubstrate/MobileSubstrate.dylib"]) {
             NSLog(@"--越狱 \n");
            //退出软件
           // exit(0);
        }
        ///NSLog(@"--%@ \n", name);
    }
}
//检测当前程序运行的环境变量：未越狱设备返回结果是null，越狱设备有值`
void printEnv(void)
{
    char *env = getenv("DYLD_INSERT_LIBRARIES");
    if (env) {
        NSLog(@"越狱");
    }
   // NSLog(@"%s", env);
}

//判断手机是否越狱
- (BOOL)mgjpf_isJailbroken
{

    //以下检测的过程是越往下，越狱越高级
    
    //    /Applications/Cydia.app, /privte/var/stash
    BOOL jailbroken = NO;
    NSString *cydiaPath = @"/Applications/Cydia.app";
    NSString *aptPath = @"/private/var/lib/apt/";
    if ([[NSFileManager defaultManager] fileExistsAtPath:cydiaPath]) {
        jailbroken = YES;
    }
    if ([[NSFileManager defaultManager] fileExistsAtPath:aptPath]) {
        jailbroken = YES;
    }
    
    //可能存在hook了NSFileManager方法，此处用底层C stat去检测
    struct stat stat_info;
    if (0 == stat("/Library/MobileSubstrate/MobileSubstrate.dylib", &stat_info)) {
        jailbroken = YES;
    }
    if (0 == stat("/Applications/Cydia.app", &stat_info)) {
        jailbroken = YES;
    }
    if (0 == stat("/var/lib/cydia/", &stat_info)) {
        jailbroken = YES;
    }
    if (0 == stat("/var/cache/apt", &stat_info)) {
        jailbroken = YES;
    }
    //    /Library/MobileSubstrate/MobileSubstrate.dylib 最重要的越狱文件，几乎所有的越狱机都会安装MobileSubstrate
    //    /Applications/Cydia.app/ /var/lib/cydia/绝大多数越狱机都会安装
    //    /var/cache/apt /var/lib/apt /etc/apt
    //    /bin/bash /bin/sh
    //    /usr/sbin/sshd /usr/libexec/ssh-keysign /etc/ssh/sshd_config
    
    //可能存在stat也被hook了，可以看stat是不是出自系统库，有没有被攻击者换掉
    //这种情况出现的可能性很小
    //这个方法
    int ret;
    Dl_info dylib_info;
    int (*func_stat)(const char *,struct stat *) = stat;
    if ((ret = dladdr(func_stat, &dylib_info))) {
        //NSLog(@"lib:%s",dylib_info.dli_fname);      //如果不是系统库，肯定被攻击了
        //两者相等返回0  其余不相等返回非0,
        if (strcmp(dylib_info.dli_fname, "/usr/lib/system/libsystem_kernel.dylib")) {   //不相等，肯定被攻击了，相等为0
            jailbroken = YES;
        }
    }
    
    //还可以检测链接动态库，看下是否被链接了异常动态库，但是此方法存在appStore审核不通过的情况，这里不作罗列
    //通常，越狱机的输出结果会包含字符串： Library/MobileSubstrate/MobileSubstrate.dylib——之所以用检测链接动态库的方法，是可能存在前面的方法被hook的情况。这个字符串，前面的stat已经做了
    
    //如果攻击者给MobileSubstrate改名，但是原理都是通过DYLD_INSERT_LIBRARIES注入动态库
    //那么可以，检测当前程序运行的环境变量
    char *env = getenv("DYLD_INSERT_LIBRARIES");
    if (env != NULL) {
        jailbroken = YES;
    }
    
    return jailbroken;
}
@end
