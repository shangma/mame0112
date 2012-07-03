//
//  TableViewController.m
//  Artnestopia
//
//  Created by arthur on 22/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "TableViewController.h"
#import "RootViewController.h"
#import "Helper.h"

@implementation TableViewController

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void)dealloc
{
    NSLog(@"TableViewController dealloc");
    [arr release];
    arr = nil;
    [super dealloc];
}

- (id)init
{
    self = [super initWithStyle:UITableViewStyleGrouped];
    if (self) {
        arr = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)viewDidLoad
{
    [self reloadDocsPath];
    if (!arr.count) {
        UILabel *l;
        l = [[[UILabel alloc] initWithFrame:CGRectMake(0.0, 0.0, self.view.frame.size.width, self.view.frame.size.height)] autorelease];
        l.numberOfLines = 0;
        l.textAlignment = UITextAlignmentCenter;
        l.font = emojiFontOfSize(12.0);
        l.text = @"Place rom files with extension .zip in Documents, kill then restart";
        l.backgroundColor = [UIColor clearColor];
        [l sizeToFit];
        self.tableView.tableHeaderView = l;
    }
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self reloadDocsPath];
    [self.tableView reloadData];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)row
{
    return [arr count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 72.0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSString *identifier = @"cell1";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:identifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier] autorelease];
    }
    cell.textLabel.text = getDisplayNameForPath([arr objectAtIndex:indexPath.row]);
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSString *game = [arr objectAtIndex:indexPath.row];
/*    int usePad = NO;
    int useZapper = NO;
    if (containsString(game, @"Duck Hunt") || containsString(game, @"Gumshoe") || containsString(game, @"Hogans Alley")) {
        useZapper = YES;
    } else {
        usePad = YES;
    }*/
    RootViewController *vc = [[[RootViewController alloc] initWithGame:game] autorelease];
    [self.navigationController pushViewController:vc animated:YES];
}

- (void)reloadDocsPath
{
    NSLog(@"reloadDocsPath");
    [arr removeAllObjects];
    NSArray *docsarr = readContentsOfPath(getDocsPath());
    for (NSString *elt in docsarr) {
        if ([@"zip" caseInsensitiveCompare:[elt pathExtension]] != NSOrderedSame) {
            continue;
        }
        [arr addObject:[[elt lastPathComponent] stringByDeletingPathExtension]];
    }
    sortFileArrayAlphabetically(arr);
}

@end
