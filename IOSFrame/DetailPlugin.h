//
//  DetailPlugin.h
//  IOSFrame
//
//  Created by 安昌 on 2017/7/27.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "CDV.h"

@interface DetailPlugin : CDVPlugin

-(void)scan:(CDVInvokedUrlCommand*)command;

-(void)location:(CDVInvokedUrlCommand*)command;

-(void)pay:(CDVInvokedUrlCommand*)command;

-(void)share:(CDVInvokedUrlCommand*)command;

-(void)changeColor:(CDVInvokedUrlCommand*)command;

-(void)shake:(CDVInvokedUrlCommand*)command;

-(void)playSound:(CDVInvokedUrlCommand*)command;

@end
