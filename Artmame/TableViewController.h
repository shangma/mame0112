//
//  TableViewController.h
//  Artnestopia
//
//  Created by arthur on 22/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TableViewController : UITableViewController <UITableViewDataSource, UITableViewDelegate>
{
    NSMutableArray *arr;
}
- (void)reloadDocsPath;
@end
