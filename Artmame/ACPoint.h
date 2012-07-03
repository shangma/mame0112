//
//  ACPoint.h
//  Solenotes
//
//  Created by arthur on 31/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ACPoint : NSObject
@property (nonatomic) CGPoint point;
@property (nonatomic) CGPoint lastPoint;
@property (nonatomic) NSTimeInterval timestamp;
@property (nonatomic) BOOL movement;
@end
