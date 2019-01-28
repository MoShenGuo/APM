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
@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    NSLog(@"---------------\n");
   // [MSCallTrace save];
            struct  timeval start;
    
            struct  timeval end;
    
           
    
            unsigned  long diff;
    
            gettimeofday(&start,NULL);
    
           // delay(10);
           sleep(5);
    
            gettimeofday(&end,NULL);
    
    diff = (end.tv_sec % 100) * 1000000 + end.tv_usec - ((start.tv_sec % 100) * 1000000 + start.tv_usec);
    NSLog(@"diff---=%lu",diff);

}

@end
//int delay(int time)
//
//{
//    
//    int i,j;
//    
//    
//    
//    for(i =0;i<time;i++) {
//        
//        for(j=0;j<5000;j++)
//            
//            ;
//    }
//    
//}
