
#import <UIKit/UIKit.h>

int main(int argc, char** argv)
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  int r = UIApplicationMain(argc, argv, nil, @"GdtAppDelegate");
  [pool release];
  return r;
}
