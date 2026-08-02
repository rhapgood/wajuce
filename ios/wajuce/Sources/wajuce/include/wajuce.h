// wajuce native runtime — public headers for the Swift package target.
//
// wajuce is an FFI plugin: Dart reaches the runtime through the C ABI declared
// in src/wajuce.h and resolved at run time by dart:ffi, so nothing has to import
// this module from Objective-C or Swift. Swift Package Manager nonetheless
// requires every C-family target to have a public headers directory inside the
// package, so this header exists to provide one.

#ifndef WAJUCE_SWIFTPM_UMBRELLA_H
#define WAJUCE_SWIFTPM_UMBRELLA_H

#endif  // WAJUCE_SWIFTPM_UMBRELLA_H
