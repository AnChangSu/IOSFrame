//
//  FirstViewUserInfo.m
//  IOSFrame
//
//  Created by 安昌 on 2017/8/14.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "FirstViewUserInfo.h"

@implementation FirstViewUserInfo

@synthesize name;
@synthesize school;
@synthesize token;

-(nonnull NSString*)description
{
    return [NSString stringWithFormat:@"__%@__%@__%@",name,school,token];
}

-(nonnull instancetype)initWIthName:(nullable NSString*)tname School:(nullable NSString*)tschool token:(nonnull NSString *)ttoken
{
    if ( self = [super init] ) {
        self.name = tname;
        self.school = school;
        self.token = token;
    }
    
    return self;
}

@end
