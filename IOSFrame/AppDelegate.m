//
//  AppDelegate.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "AppDelegate.h"
#import "RouterURLInfo.h"
#import "LDBusMediator.h"
#import "JPEngine.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

-(void)loadJSPath{
    [JPEngine startEngine];
    NSString *sourcePath = [[NSBundle mainBundle] pathForResource:@"demo" ofType:@"js"];
    NSString *script = [NSString stringWithContentsOfFile:sourcePath encoding:NSUTF8StringEncoding error:nil];
    [JPEngine evaluateScript:script];
}

-(void)loadTabController
{
    UITabBarController *rootTabBarController = [[UITabBarController alloc] init];
    
    //tab1
    NSString *str = CAT(RouterName, First_URL);
    //    NSLog(@"_____%@______",str1);
    UIViewController *controller1 = [LDBusMediator viewControllerForURL:[NSURL URLWithString:str]];
    UINavigationController *navTab1 = [[UINavigationController alloc] initWithRootViewController:controller1];
    navTab1.title = @"first";
    //tab2
    str = CAT(RouterName, Second_RUL);
    UIViewController *controller2 = [LDBusMediator viewControllerForURL:[NSURL URLWithString:str]];
    UINavigationController *navTab2 = [[UINavigationController alloc] initWithRootViewController:controller2];
    navTab2.title = @"second";
    //tab3
    str = CAT(RouterName, Third_URL);
    UIViewController *controller3 = [LDBusMediator viewControllerForURL:[NSURL URLWithString:str]];
    UINavigationController *navTab3 = [[UINavigationController alloc] initWithRootViewController:controller3];
    navTab3.title = @"third";
    //tab4
    str = CAT(RouterName, Fourth_URL);
    str = @"IOSFrame://Fourth/detail";
    UIViewController *controller4 = [LDBusMediator viewControllerForURL:[NSURL URLWithString:str]];
    UINavigationController *navTab4 = [[UINavigationController alloc] initWithRootViewController:controller4];
    navTab4.title = @"fourth";
    
    rootTabBarController.viewControllers = @[navTab1,navTab2,navTab3,navTab4];
    
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.backgroundColor = [UIColor whiteColor];
    self.window.rootViewController = rootTabBarController;
    [self.window makeKeyAndVisible];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    
   
    // Override point for customization after application launch.
    [self loadJSPath];
    [self loadTabController];
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}


- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}


@end
