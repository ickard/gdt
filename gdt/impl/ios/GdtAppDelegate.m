/*
 * GdtAppDelegate.m
 *
 * Copyright (c) 2011 Rickard Edström
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#import <UIKit/UIKit.h>
#import "GdtAppDelegate.h"
#import "GdtView.h"
#include "gdt.h"

static GdtAppDelegate* _instance = nil;
static accelerometerhandler_t cb_accelerometer = NULL;

void gdt_set_callback_accelerometer(accelerometerhandler_t on_accelerometer_event) {
	cb_accelerometer = on_accelerometer_event;

	UIAccelerometer* a = [UIAccelerometer sharedAccelerometer];
	a.delegate = _instance;
}

@implementation GdtAppDelegate

-(void)applicationDidBecomeActive:(UIApplication*)_
{
	gdt_hook_active();
}

-(void)applicationWillResignActive:(UIApplication*)_
{
	gdt_hook_inactive();
}

-(void)applicationWillEnterForeground:(UIApplication*)_
{
	[view visible:YES];
}

-(void)applicationDidEnterBackground:(UIApplication*)_
{
	[view visible:NO];
	gdt_hook_save_state();
}

-(void)accelerometer:(UIAccelerometer*)_ didAccelerate:(UIAcceleration*)a {
	static accelerometer_data_t v;
	if (cb_accelerometer) {
		v.x = a.x;
		v.y = a.y;
		v.z = a.z;
		v.time = a.timestamp;
		cb_accelerometer(&v);
	}
}

-(void)applicationDidFinishLaunching :(UIApplication*) _
{
	_instance = self;

	CGRect bounds = [[UIScreen mainScreen] bounds];
	window = [[UIWindow alloc] initWithFrame:bounds];
	view = [[GdtView alloc] initWithFrame: bounds];
		
	[window addSubview:view];
	[window makeKeyAndVisible];
}

-(void)dealloc
{
	[view release];
	[window release];
	[super dealloc];
}

@end
