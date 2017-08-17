//
//  DetailPlugin.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/27.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "DetailPlugin.h"
#import <AVFoundation/AVFoundation.h>
#import "HLAudioPlayer.h"
#import "CDV.h"

@implementation DetailPlugin

-(void)scan:(CDVInvokedUrlCommand *)command
{
    [self.commandDelegate runInBackground:^{
        dispatch_async(dispatch_get_main_queue(), ^{
            [UIAlertController alertControllerWithTitle:@"原生弹窗" message:@"333" preferredStyle:UIAlertControllerStyleAlert];
        });
    }];
}

-(void)location:(CDVInvokedUrlCommand *)command
{
    NSString *locationStr = @"错误信息";
    
    [self.commandDelegate runInBackground:^{
        CDVPluginResult *result = [CDVPluginResult resultWithStatus:CDVCommandStatus_ERROR messageAsString:locationStr];
        [self.commandDelegate sendPluginResult:result callbackId:command.callbackId];
    }];
}

-(void)pay:(CDVInvokedUrlCommand *)command
{
    NSUInteger code = 1;
    NSString *tip = @"支付成功";
    NSArray *arguments = command.arguments;
    if (arguments.count < 4) {
        code = 2;
        tip = @"参数错误";
    }else{
        NSLog(@"从H5获取的支付参数:%@",arguments);
    }
    
    NSString *jsStr = [NSString stringWithFormat:@"payResult('%@',%lu)",tip,(unsigned long)code];
    [self.commandDelegate evalJs:jsStr];
}

-(void)share:(CDVInvokedUrlCommand *)command
{
    NSUInteger code = 1;
    NSString *tip = @"分享成功";
    NSArray *arguments = command.arguments;
    if ( arguments.count < 3 ) {
        code = 2;
        tip = @"参数错误";
        NSString *jsStr = [NSString stringWithFormat:@"shareResult('%@')",tip];
        [self.commandDelegate evalJs:jsStr];
    }
    
    NSLog(@"从H5获取的分享参数:%@",arguments);
    NSString *title = arguments[0];
    NSString *content = arguments[1];
    NSString *url = arguments[2];
    
    NSString *jsStr = [NSString stringWithFormat:@"shareResult('%@','%@','%@')",title,content,url];
    [self.commandDelegate evalJs:jsStr];

}

- (void)changeColor:(CDVInvokedUrlCommand *)command
{
    NSArray *arguments = command.arguments;
    if (arguments.count < 4) {
        return;
    }
    
    CGFloat r = [arguments[0] floatValue];
    CGFloat g = [arguments[1] floatValue];
    CGFloat b = [arguments[2] floatValue];
    CGFloat a = [arguments[3] floatValue];
    
    self.viewController.view.backgroundColor = [UIColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:a];
}

- (void)shake:(CDVInvokedUrlCommand *)command
{
    [self.commandDelegate runInBackground:^{
        AudioServicesPlaySystemSound (kSystemSoundID_Vibrate);
        [HLAudioPlayer playMusic:@"shake_sound_male.wav"];
    }];
}

- (void)playSound:(CDVInvokedUrlCommand *)command
{
    NSArray *arguments = command.arguments;
    if (arguments.count < 1) {
        return;
    }
    
    [self.commandDelegate runInBackground:^{
        NSString *fileName = arguments[0];
        [HLAudioPlayer playMusic:fileName];
    }];
}

@end
