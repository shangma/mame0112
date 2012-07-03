//
//  TouchInputView.m
//  Artsnes9x
//
//  Created by arthur on 28/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "TouchInputView.h"
#import "RootViewController.h"
#import "HelpView.h"
#import "Helper.h"
#import "ACPoint.h"

#import "glue.h"

extern int inputcode[];
extern RootViewController *rootViewController;

@implementation TouchInputView

@synthesize helpView = _helpView;

- (void)dealloc
{
    NSLog(@"TouchInputView dealloc");
    self.helpView = nil;
    [currentTouches release];
    currentTouches = nil;
    [super dealloc];
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        NSLog(@"TouchInputView init");
        currentTouches = [[NSMutableDictionary alloc] init];
        self.userInteractionEnabled = YES;
        self.multipleTouchEnabled = YES;
        self.exclusiveTouch = NO;  
        
    }
    return self;
}

void set_joystick_center()
{
    NSLog(@"set_joystick_center");
    inputcode[IOS_JOYCODE_1_LEFT] = 0;
    inputcode[IOS_JOYCODE_1_RIGHT] = 0;
    inputcode[IOS_JOYCODE_1_UP] = 0;
    inputcode[IOS_JOYCODE_1_DOWN] = 0;
}

void set_joystick_up()
{
    NSLog(@"set_joystick_up");
    inputcode[IOS_JOYCODE_1_LEFT] = 0;
    inputcode[IOS_JOYCODE_1_RIGHT] = 0;
    inputcode[IOS_JOYCODE_1_UP] = 5;
    inputcode[IOS_JOYCODE_1_DOWN] = 0;
}

void set_joystick_down()
{
    NSLog(@"set_joystick_down");
    inputcode[IOS_JOYCODE_1_LEFT] = 0;
    inputcode[IOS_JOYCODE_1_RIGHT] = 0;
    inputcode[IOS_JOYCODE_1_UP] = 0;
    inputcode[IOS_JOYCODE_1_DOWN] = 5;
}

void set_joystick_left()
{
    NSLog(@"set_joystick_left");
    inputcode[IOS_JOYCODE_1_LEFT] = 5;
    inputcode[IOS_JOYCODE_1_RIGHT] = 0;
    inputcode[IOS_JOYCODE_1_UP] = 0;
    inputcode[IOS_JOYCODE_1_DOWN] = 0;
}

void set_joystick_right()
{
    NSLog(@"set_joystick_right");
    inputcode[IOS_JOYCODE_1_LEFT] = 0;
    inputcode[IOS_JOYCODE_1_RIGHT] = 5;
    inputcode[IOS_JOYCODE_1_UP] = 0;
    inputcode[IOS_JOYCODE_1_DOWN] = 0;
}

void set_joystick_button()
{
    NSLog(@"set_joystick_button");
    inputcode[IOS_JOYCODE_1_BUTTON1] = 5;
    inputcode[IOS_JOYCODE_1_BUTTON2] = 5;
    inputcode[IOS_JOYCODE_1_BUTTON3] = 5;
    inputcode[IOS_JOYCODE_1_BUTTON4] = 5;
    inputcode[IOS_JOYCODE_1_BUTTON5] = 5;
    inputcode[IOS_JOYCODE_1_BUTTON6] = 5;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
//    [super touchesBegan:touches withEvent:event];
    for (UITouch *t in touches) {
        CGPoint p = [t locationInView:self];
        NSValue *key = [NSValue valueWithPointer:t];
        if ([currentTouches objectForKey:key])
            continue;
        if (currentTouches.count > 0) {
            set_joystick_button();
            continue;
        }
        if (p.y < rootViewController.cornerLabel.frame.size.height) {
            if (p.x > self.frame.size.width - (rootViewController.cornerLabel.frame.size.width / 2)) {
                NSLog(@"touchesBegan: COIN1");
                inputcode[IOS_KEYCODE_COIN1] = 5;
                continue;
            } else if (p.x > self.frame.size.width - rootViewController.cornerLabel.frame.size.width) {
                NSLog(@"touchesBegan: START1");
                inputcode[IOS_KEYCODE_START1] = 5;
                continue;
            }
        }
        ACPoint *point = [[[ACPoint alloc] init] autorelease];
        point.point = point.lastPoint = p;
        point.timestamp = t.timestamp;
        [currentTouches setObject:point forKey:key];
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    int (^func)(CGPoint p1, CGPoint p2) = ^(CGPoint p1, CGPoint p2) {
        CGFloat deltaX = p2.x - p1.x;
        CGFloat deltaY = p2.y - p1.y;
        if ((deltaX == 0.0) && (deltaY == 0.0))
            return 0;
        if (fabs(deltaX) > fabs(deltaY)) {
            if (deltaX > 0.0)
                return 4;
            else
                return 3;
        } else if (fabs(deltaY) > fabs(deltaX)) {
            if (deltaY > 0.0)
                return 2;
            else
                return 1;
        }
        return 5;
    };
    CGPoint currentPoint;
    CGFloat deltaX = 0.0;
    CGFloat deltaY = 0.0;
    for (UITouch *t in touches) {
        NSValue *key = [NSValue valueWithPointer:t];
        ACPoint *initialPoint = [currentTouches objectForKey:key];
        if (!initialPoint) {
            NSLog(@"initialPoint not found");
            continue;
        }
        currentPoint = [t locationInView:self];
        if (func(initialPoint.point, currentPoint) != func(initialPoint.lastPoint, currentPoint)) {
            initialPoint.point = initialPoint.lastPoint;
        }
        deltaX = currentPoint.x - initialPoint.point.x;
        deltaY = currentPoint.y - initialPoint.point.y;
//        NSLog(@"deltaX %e deltaY %e", deltaX, deltaY);
        if ((deltaY > 10.0) && (fabs(deltaX) < (fabs(deltaY) / 1.5))) {
//            NSLog(@"recognized deltaY > 10.0");
            set_joystick_down();
            initialPoint.movement = YES;
        } else if ((deltaY < -10.0) && (fabs(deltaX) < (fabs(deltaY) / 1.5))) {
//            NSLog(@"recognized deltaY < -10.0");
            set_joystick_up();
            initialPoint.movement = YES;
        } else if ((deltaX > 10.0) && (fabs(deltaY) < (fabs(deltaX) / 1.5))) {
//            NSLog(@"recognized deltaX > 10.0");
            set_joystick_right();
            initialPoint.movement = YES;
            if (initialPoint.point.x < self.frame.size.width * 0.05) {
                extern RootViewController *rootViewController;
                [rootViewController.navigationController popViewControllerAnimated:YES];
            }
        } else if ((deltaX < -10.0) && (fabs(deltaY) < (fabs(deltaX) / 1.5))) {
//            NSLog(@"recognized deltaX < 10.0");
            set_joystick_left();
            initialPoint.movement = YES;
        }  
        initialPoint.lastPoint = currentPoint;
    }
}

- (void)handleTouchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    for(UITouch *t in touches) {
        CGPoint currentPoint = [t locationInView:self];
        NSValue *key = [NSValue valueWithPointer:t];
        ACPoint *point = [currentTouches objectForKey:key];
        if (point) {
            NSTimeInterval deltaTime = [[NSProcessInfo processInfo] systemUptime] - point.timestamp;
            if (!point.movement && (deltaTime < 0.5)) {
                set_joystick_button();
            }
            [currentTouches removeObjectForKey:key];
        }
    }
    if (currentTouches.count == 0) {
        set_joystick_center();
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
//    [super touchesCancelled:touches withEvent:event];
    [self handleTouchesEnded:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
//    [super touchesEnded:touches withEvent:event];
    [self handleTouchesEnded:touches withEvent:event];
}

@end
