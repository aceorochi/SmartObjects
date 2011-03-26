#import <Foundation/Foundation.h>
#import "view.hpp"

namespace js
{
  // Using `js::defaults`, the following:
  //
  //     js::defaults[@"dog"] = @"tucker";
  //     id dog = js::defaults[@"dog"];
  //
  // is equivalent to this much more verbose code:
  //
  //     NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  //     [defaults setValue:@"tucker" forKey:@"dog"];
  //     id dog = [defaults valueForKey:@"dog"];
  //
  static view<> defaults([NSUserDefaults standardUserDefaults]);
}