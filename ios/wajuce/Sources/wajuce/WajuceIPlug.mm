#import <Foundation/Foundation.h>

// Neither CocoaPods nor Swift Package Manager reliably adds source files from
// outside this directory to the target, so the native runtime is included here
// instead. That exports the Dart FFI C ABI symbols from the framework. Quoted
// includes resolve relative to the including file, so the engine's own relative
// includes keep working from its real location.
#include "../../../../native/engine/Source/WAIPlugEngine.cpp"
