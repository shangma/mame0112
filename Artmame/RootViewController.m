//
//  RootViewController.m
//  Artnestopia
//
//  Created by arthur on 20/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "RootViewController.h"
#import <QuartzCore/QuartzCore.h>
#import <pthread.h>
#import "Helper.h"
#import "TouchInputView.h"

extern void audio_close(void);
extern void audio_open(void);

RootViewController *rootViewController = nil;
TouchInputView *inputView = nil;
ScreenView *screenView = nil;

CGRect get_screen_rect(UIInterfaceOrientation toInterfaceOrientation);
CGRect get_screen_rect(UIInterfaceOrientation toInterfaceOrientation)
{
    CGRect r = [[UIScreen mainScreen] applicationFrame];
    if (UIInterfaceOrientationIsLandscape(toInterfaceOrientation)) {
        CGFloat tmp = r.size.width;
        r.size.width = r.size.height;
        r.size.height = tmp;
    }
    extern int screen_width, screen_height;
    int tmp_width = r.size.width;
    int tmp_height = ((((tmp_width * screen_height) / screen_width)+7)&~7);
    if(tmp_height > r.size.height)
    {
        tmp_height = r.size.height;
        tmp_width = ((((tmp_height * screen_width) / screen_height)+7)&~7);
    }
    NSLog(@"tmp_width %d tmp_height %d", tmp_width, tmp_height);
    r.origin.x = ((int)r.size.width - tmp_width) / 2;             
    r.origin.y = ((int)r.size.height - tmp_height) / 2;
    r.size.width = tmp_width;
    r.size.height = tmp_height;    
    return r;
}

@implementation RootViewController

@synthesize path = _path;
@synthesize cornerLabel = _cornerLabel;
@synthesize debugLabel = _debugLabel;

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void)dealloc
{
    NSLog(@"dealloc RootViewController");
    extern int loaded;
    if (loaded) {
        audio_close();
        loaded = 0;
        extern void unload_game(void);
        unload_game();
    }
    rootViewController = nil;
}

- (id)initWithGame:(NSString *)path
{
    self = [super init];
    if (self) {
        rootViewController = self;
        self.path = path;
        extern int load_game_wrapper(const char *);
        if (!load_game_wrapper(getCString(self.path))) {
            NSLog(@"loaded %@", self.path);
            extern int loaded;
            loaded = 1;
            audio_open();
        } else {
            NSLog(@"unable to load %@", self.path);
        }
    }
    return self;
}

- (void)loadView
{
    struct CGRect rect = [[UIScreen mainScreen] applicationFrame];
    rect.origin.x = rect.origin.y = 0.0f;
    self.view = inputView = [[TouchInputView alloc] initWithFrame:rect];
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.view.backgroundColor = [UIColor clearColor];
/*    self.view.opaque = YES;
    self.view.clearsContextBeforeDrawing = NO;
    self.view.userInteractionEnabled = YES;
    self.view.multipleTouchEnabled = YES;
    self.view.exclusiveTouch = NO;*/
}

- (void)viewDidUnload
{
    [inputView release];
    inputView = nil;
    [super viewDidUnload];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    UILabel *(^emoji)(NSString *str) = ^(NSString *str) {
        UILabel *l = [[[UILabel alloc] initWithFrame:CGRectZero] autorelease];
        l.backgroundColor = [UIColor clearColor];
        l.text = str;
        l.font = emojiFontOfSize(17.0);
        l.numberOfLines = 1;
        l.textAlignment = UITextAlignmentCenter;
        [l sizeToFit];
        l.frame = CGRectMake(self.view.frame.size.width-l.frame.size.width, 0.0, l.frame.size.width, l.frame.size.height);
        l.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleBottomMargin;
        return l;
    };
    UILabel *(^toilet)(void) = ^{
        return emoji(@"\ue05a");
    };
    self.cornerLabel = emoji(@"\ue12f \ue138");
    [self.view addSubview:self.cornerLabel];
    screenView = [[ScreenView alloc] initWithFrame:CGRectZero];
    screenView.frame = get_screen_rect(self.interfaceOrientation);
    [self.view addSubview:screenView];
/*    self.debugLabel = [[[UILabel alloc] initWithFrame:self.view.frame] autorelease];
    self.debugLabel.backgroundColor = [UIColor clearColor];
    self.debugLabel.font = [UIFont systemFontOfSize:12.0];
    self.debugLabel.textColor = [UIColor whiteColor];
    self.debugLabel.numberOfLines = 1;
    self.debugLabel.userInteractionEnabled = NO;
    [self setDebugText:@"DEBUG"];
    [self.view addSubview:self.debugLabel];*/
}

- (void)viewDidDisappear:(BOOL)animated
{
/*    [self.debugLabel removeFromSuperview];
    self.debugLabel = nil;*/
    [screenView removeFromSuperview];
    [screenView release];
    screenView = nil;
    [self.cornerLabel removeFromSuperview];
    self.cornerLabel = nil;
    [super viewDidDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [UIView animateWithDuration:duration delay:0.0 options:0 animations:^{ screenView.frame = get_screen_rect(toInterfaceOrientation); } completion:nil];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    screenView.frame = get_screen_rect(self.interfaceOrientation);
}

- (void)handleReset
{
    void audio_close(void);
    audio_close();
    extern int audio_do_not_initialize;
    audio_do_not_initialize = 1;
    UIAlertView *av = [[[UIAlertView alloc] initWithTitle:@"Reset" message:@"Are you sure?" delegate:self cancelButtonTitle:@"No" otherButtonTitles:@"Yes", nil] autorelease];
    [av show];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 1) {
        extern void nst_reset(void);
/*        nst_reset();*/
    }
    extern int audio_do_not_initialize;
    audio_do_not_initialize = 0;
    void audio_open(void);
    audio_open();
}

- (void)changeScreenViewToSize:(CGSize)size
{
    if (!screenView)
        return;
    [screenView removeFromSuperview];
    [screenView release];
    extern int screen_width, screen_height;
    screen_width = size.width;
    screen_height = size.height;
    screenView = [[ScreenView alloc] initWithFrame:get_screen_rect(self.interfaceOrientation)];
    [self.view addSubview:screenView];
}

- (void)setDebugText:(NSString *)text
{
    self.debugLabel.text = text;
    [self.debugLabel sizeToFit];
}

@end
