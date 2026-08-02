// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "wajuce",
  platforms: [
    .iOS("15.0")
  ],
  products: [
    // Built as a dynamic library so Xcode produces (and embeds) wajuce.framework.
    // The Dart side resolves the FFI symbols with
    // DynamicLibrary.open('wajuce.framework/wajuce'), exactly as under CocoaPods.
    .library(name: "wajuce", type: .dynamic, targets: ["wajuce"])
  ],
  dependencies: [
    .package(name: "FlutterFramework", path: "../FlutterFramework")
  ],
  targets: [
    .target(
      name: "wajuce",
      dependencies: [
        .product(name: "FlutterFramework", package: "FlutterFramework")
      ],
      cxxSettings: [
        .define("WAJUCE_USE_RTAUDIO", to: "0"),
        .define("WAJUCE_USE_RTMIDI", to: "0"),
        // Output-only: keep all microphone symbols out of the binary so apps that
        // never capture audio don't need NSMicrophoneUsageDescription (ITMS-90683).
        .define("WAJUCE_ENABLE_AUDIO_INPUT", to: "0"),
        // The iOS build needs no header search paths into native/engine: the
        // engine is reached by relative #include and its own includes resolve
        // relative to it. iPlug2's headers are only picked up behind
        // __has_include and contribute nothing to this target. Swift Package
        // Manager rejects header search paths outside the package root anyway.
        .unsafeFlags(["-Wno-everything", "-fno-objc-arc"])
      ],
      linkerSettings: [
        .linkedFramework("AudioToolbox"),
        .linkedFramework("AVFoundation"),
        .linkedFramework("CoreFoundation"),
        .linkedFramework("Foundation")
      ]
    )
  ],
  cxxLanguageStandard: .cxx17
)
