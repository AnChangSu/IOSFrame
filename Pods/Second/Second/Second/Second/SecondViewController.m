//
//  SecondViewController.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "SecondViewController.h"
#import "UIView+LayoutMethods.h"

#import "LDBusMediator.h"
#import "ModelUserInfoItemPrt.h"
#import "ModelUserInfoServicePrt.h"

#import "Model_ForFirst_info.h"
#import "RouterURLInfo.h"

NSString * const kCellIdentifier = @"kCellIdentifier";

static NSMutableDictionary *rootTabClassesDic = nil;

@interface SecondViewController () <UITableViewDelegate, UITableViewDataSource>

@property (nonatomic, strong) UITableView *tableView;
@property (nonatomic, strong) NSArray *dataSource;

@end

@implementation SecondViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
     self.navigationItem.title = @"second";
    [self.view addSubview:self.tableView];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [self.tableView fill];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - UITableViewDataSource
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return self.dataSource.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellIdentifier];
    cell.textLabel.text = self.dataSource[indexPath.row];
    return cell;
}

#pragma mark - UITableViewDelegate
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    if (indexPath.row == 0) {
        Model_ForFirst_info *item = [[Model_ForFirst_info alloc] init];
        item.name = @"name";
        item.school = @"school";
        item.token = @"token";
        [[LDBusMediator serviceForProtocol:@protocol(ModelUserInfoServicePrt)] modelUserInfo_deliveAprotocol:item];
    }
    
    //测试hookURLRouteBlock
    if (indexPath.row == 1) {
        [self setURLHookRouteBlock];
        [LDBusMediator routeURL:[NSURL URLWithString:@"IOSFrame://Fourth"]];
    }
    
    //测试无法找到url的tip提示
    if (indexPath.row == 2) {
        [LDBusMediator routeURL:[NSURL URLWithString:@"IOSFrame://testURL"]];
    }
    
    //添加FirstDetail页面
    if (indexPath.row == 3) {
//        [LDBusMediator routeURL:[NSURL URLWithString:CAT(RouterName,First_Detail_URL)]];
        [LDBusMediator routeURL:[NSURL URLWithString:@"https://www.baidu.com"]];
    }
    
    //添加FirstDetail页面
    if (indexPath.row == 4) {
        [LDBusMediator routeURL:[NSURL URLWithString:CAT(RouterName,First_Detail_URL)]];
    }

}

#pragma mark - getters and setters

- (NSArray *)dataSource
{
    if (_dataSource == nil) {
        _dataSource = @[@"sendUserInfoToFirstController",@"hookURLRouteBlock",@"url not found",@"push baidu",@"push index.html"];
    }
    return _dataSource;
}

- (UITableView *)tableView
{
    if (_tableView == nil) {
        _tableView = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
        _tableView.delegate = self;
        _tableView.dataSource = self;
        
        [_tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:kCellIdentifier];
    }
    return _tableView;
}

//拦截跳转，直接跳到tab方法
-(void)setURLHookRouteBlock{
    [[LDBusNavigator navigator] setHookRouteBlock:^BOOL(UIViewController * _Nonnull controller, UIViewController * _Nullable baseViewController, NavigationMode routeMode) {
        UIViewController *tabController = [self isViewControllerInTabContainer:controller];
        if (tabController) {
            [[LDBusNavigator navigator] showURLController:tabController baseViewController:baseViewController routeMode:NavigationModeShare];
            return YES;
        } else {
            return NO;
        }
    }];
}

-(UIViewController *)isViewControllerInTabContainer:(UIViewController *)controller{
    if (rootTabClassesDic == nil) {
        rootTabClassesDic = [[NSMutableDictionary alloc] initWithCapacity:2];
        UIViewController *rootViewContoller = [UIApplication sharedApplication].delegate.window.rootViewController;
        if (rootViewContoller && [rootViewContoller isKindOfClass:[UITabBarController class]]) {
            NSArray *tabControllers = ((UITabBarController *)rootViewContoller).viewControllers;
            [tabControllers enumerateObjectsUsingBlock:^(UIViewController *_Nonnull viewController, NSUInteger idx, BOOL * _Nonnull stop) {
                if ([viewController isKindOfClass:[UINavigationController class]]) {
                    viewController = [((UINavigationController *)viewController).viewControllers objectAtIndex:0];
                }
                
                [rootTabClassesDic setObject:viewController forKey:NSStringFromClass([viewController class])];
            }];
        }
    }
    
    if (rootTabClassesDic && rootTabClassesDic.count > 0) {
        NSString *controllerKey = NSStringFromClass([controller class]);
        if (controllerKey) {
            return [rootTabClassesDic objectForKey:controllerKey];
        } else {
            return nil;
        }
    } else {
        return nil;
    }
}


/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
