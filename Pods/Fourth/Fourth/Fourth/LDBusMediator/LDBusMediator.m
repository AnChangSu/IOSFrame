//
//  LDMediator.m
//  LDBusMediator
//
//  Created by 庞辉 on 4/8/16.
//  Copyright © 2016 casa. All rights reserved.
//

#import "LDBusMediator.h"
#import "LDBusNavigator.h"
#import "LDBusConnectorPrt.h"
#import "LDBusMediatorTipViewController.h"
//#import "FirstDetailViewController.h"

NSString* const kLDRouteViewControllerKey = @"LDRouteViewController";
NSString *__nonnull const kLDRouteModeKey = @"kLDRouteType";

//全部保存各个模块的connector实例
static NSMutableDictionary<NSString *, id<LDBusConnectorPrt>> *g_connectorMap = nil;


@implementation LDBusMediator

#pragma mark- add anchang 

+(void)registerConnector:(nonnull id<LDBusConnectorPrt>)connector withName:(nonnull NSString*)contorName{
    if (![connector conformsToProtocol:@protocol(LDBusConnectorPrt)]) {
        return;
    }
    
    @synchronized(g_connectorMap) {
        if (g_connectorMap == nil){
            g_connectorMap = [[NSMutableDictionary alloc] initWithCapacity:5];
        }
        
//        NSString *connectorClsStr = NSStringFromClass([connector class]);
        if ([g_connectorMap objectForKey:contorName] == nil) {
            [g_connectorMap setObject:connector forKey:contorName];
        }
    }

}

#pragma mark - 向总控制中心注册挂接点

//modify anchang 直接使用字典key注册和获取
/*+(void)registerConnector:(nonnull id<LDBusConnectorPrt>)connector{
    if (![connector conformsToProtocol:@protocol(LDBusConnectorPrt)]) {
        return;
    }

    @synchronized(g_connectorMap) {
        if (g_connectorMap == nil){
            g_connectorMap = [[NSMutableDictionary alloc] initWithCapacity:5];
        }

        NSString *connectorClsStr = NSStringFromClass([connector class]);
        if ([g_connectorMap objectForKey:connectorClsStr] == nil) {
            [g_connectorMap setObject:connector forKey:connectorClsStr];
        }
    }
}*/


#pragma mark - 页面跳转接口
//add anchang
+(BOOL)routeToWebWithURL:(NSURL*)URL withParameters:(nullable NSDictionary *)params
{
    UIViewController *controller = [self viewControllerForURL:[NSURL URLWithString:@"IOSFrame://First_Detail"]];
    //    FirstDetailViewController *controller = [[FirstDetailViewController alloc] init];
    //    controller.startPage = URL.absoluteString;
    if ( [controller respondsToSelector:@selector(setStartPage:)] ) {
        [controller performSelector:@selector(setStartPage:) withObject:URL.absoluteString];
    }
    [[LDBusNavigator navigator] hookShowURLController:controller baseViewController:params[kLDRouteViewControllerKey] routeMode:params[kLDRouteModeKey]?[params[kLDRouteModeKey] intValue]:NavigationModePush];
    
    return YES;
}

//判断某个URL能否导航
+(BOOL)canRouteURL:(nonnull NSURL *)URL{
    if(!g_connectorMap || g_connectorMap.count <= 0) return NO;

    __block BOOL success = NO;
    //遍历connector不能并发
    [g_connectorMap enumerateKeysAndObjectsWithOptions:NSEnumerationReverse usingBlock:^(NSString * _Nonnull key, id<LDBusConnectorPrt>  _Nonnull connector, BOOL * _Nonnull stop) {
        if([connector respondsToSelector:@selector(canOpenURL:)]){
            if ([connector canOpenURL:URL]) {
                success = YES;
                *stop = YES;
            }
        }
    }];

    return success;
}


+(BOOL)routeURL:(nonnull NSURL *)URL{
    return [self routeURL:URL withParameters:nil];
}


+(BOOL)routeURL:(nonnull NSURL *)URL withParameters:(nullable NSDictionary *)params{
    if(!g_connectorMap || g_connectorMap.count <= 0) return NO;

    __block BOOL success = NO;
    __block int queryCount = 0;
    
    //add anchang 添加网页跳转
    if([URL.scheme isEqualToString:@"http"] || [URL.scheme isEqualToString:@"https"]){
       
        return [self routeToWebWithURL:URL withParameters:params];
    }
    
    NSDictionary *userParams = [self userParametersWithURL:URL andParameters:params];
    [g_connectorMap enumerateKeysAndObjectsWithOptions:NSEnumerationReverse usingBlock:^(NSString * _Nonnull key, id<LDBusConnectorPrt>  _Nonnull connector, BOOL * _Nonnull stop) {
        queryCount++;
        if([connector respondsToSelector:@selector(connectToOpenURL:params:)]){
            id returnObj = [connector connectToOpenURL:URL params:userParams];
            if(returnObj && [returnObj isKindOfClass:[UIViewController class]]){
                if ([returnObj isKindOfClass:[LDBusMediatorTipViewController class]]) {
                    LDBusMediatorTipViewController *tipController = (LDBusMediatorTipViewController *)returnObj;
                    if (tipController.isNotURLSupport) {
                        success = YES;
                    } else {
                        success = NO;
#if DEBUG
                        [tipController showDebugTipController:URL withParameters:params];
                        success = YES;
#endif
                    }
                } else if ([returnObj class] == [UIViewController class]){
                    success = YES;
                } else {
                    [[LDBusNavigator navigator] hookShowURLController:returnObj baseViewController:params[kLDRouteViewControllerKey] routeMode:params[kLDRouteModeKey]?[params[kLDRouteModeKey] intValue]:NavigationModePush];
                    success = YES;
                }

                *stop = YES;
            }
        }
    }];


#if DEBUG
    if (!success && queryCount == g_connectorMap.count) {
        [((LDBusMediatorTipViewController *)[UIViewController notFound]) showDebugTipController:URL withParameters:params];
        return NO;
    }
#endif

    return success;
}


+(nullable UIViewController *)viewControllerForURL:(nonnull NSURL *)URL{
    return [self viewControllerForURL:URL withParameters:nil];
}


+(nullable UIViewController *)viewControllerForURL:(nonnull NSURL *)URL withParameters:(nullable NSDictionary *)params{
    if(!g_connectorMap || g_connectorMap.count <= 0) return nil;

    __block UIViewController *returnObj = nil;
    __block int queryCount = 0;
    NSDictionary *userParams = [self userParametersWithURL:URL andParameters:params];
    //modify anchang 直接使用key注册和获取connector
    [g_connectorMap enumerateKeysAndObjectsWithOptions:NSEnumerationReverse usingBlock:^(NSString * _Nonnull key, id<LDBusConnectorPrt>  _Nonnull connector, BOOL * _Nonnull stop) {
        queryCount++;
        if([connector respondsToSelector:@selector(connectToOpenURL:params:)]){
            returnObj = [connector connectToOpenURL:URL params:userParams];
            if(returnObj && [returnObj isKindOfClass:[UIViewController class]]){
                *stop = YES;
            }
        }
    }];
    NSArray *array = [URL.host componentsSeparatedByString:@"_"];
    id<LDBusConnectorPrt> connector = [g_connectorMap objectForKey:[array objectAtIndex:0]];
    if ( [connector respondsToSelector:@selector(connectToOpenURL:params:)] ) {
        returnObj = [connector connectToOpenURL:URL params:userParams];
    }


#if DEBUG
    if (!returnObj && queryCount == g_connectorMap.count) {
        [((LDBusMediatorTipViewController *)[UIViewController notFound]) showDebugTipController:URL withParameters:params];
        return nil;
    }
#endif


    if (returnObj) {
        if ([returnObj isKindOfClass:[LDBusMediatorTipViewController class]]) {
#if DEBUG
            [((LDBusMediatorTipViewController *)returnObj) showDebugTipController:URL withParameters:params];
#endif
            return nil;
        } else if([returnObj class] == [UIViewController class]){
            return nil;
        } else {
            return returnObj;
        }
    }

    return nil;
}


/**
 * 从url获取query参数放入到参数列表中
 */
+(NSDictionary *)userParametersWithURL:(nonnull NSURL *)URL andParameters:(nullable NSDictionary *)params{
    NSArray *pairs = [URL.query componentsSeparatedByString:@"&"];
    NSMutableDictionary *userParams = [[NSMutableDictionary alloc] init];
    for (NSString *pair in pairs) {
        NSArray *kv = [pair componentsSeparatedByString:@"="];
        if (kv.count == 2) {
            NSString *key = [kv objectAtIndex:0];
            NSString *value = [self URLDecodedString:[kv objectAtIndex:1]];
            [userParams setObject:value forKey:key];
        }
    }
    [userParams addEntriesFromDictionary:params];
    return [NSDictionary dictionaryWithDictionary:userParams];
}


/**
 * 对url的value部分进行urlDecoding
 */
+(nonnull NSString *)URLDecodedString:(nonnull NSString *)urlString
{
    NSString *result = urlString;
#if __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_9_0
    result = (__bridge_transfer NSString *)CFURLCreateStringByReplacingPercentEscapesUsingEncoding(kCFAllocatorDefault,
                                                                                                             (__bridge CFStringRef)urlString,
                                                                                                             CFSTR(""),
                                                                                                             kCFStringEncodingUTF8);
#else
    result = [urlString stringByRemovingPercentEncoding];
#endif
    return result;
}


#pragma mark - 服务调用接口

+(nullable id)serviceForProtocol:(nonnull Protocol *)protocol{
    if(!g_connectorMap || g_connectorMap.count <= 0) return nil;

    __block id returnServiceImp = nil;
    [g_connectorMap enumerateKeysAndObjectsWithOptions:NSEnumerationReverse usingBlock:^(NSString * _Nonnull key, id<LDBusConnectorPrt>  _Nonnull connector, BOOL * _Nonnull stop) {
        if([connector respondsToSelector:@selector(connectToHandleProtocol:)]){
            returnServiceImp = [connector connectToHandleProtocol:protocol];
            if(returnServiceImp){
                *stop = YES;
            }
        }
    }];

    return returnServiceImp;
}

@end
