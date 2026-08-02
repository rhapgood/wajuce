#
# wajuce iOS podspec
#
Pod::Spec.new do |s|
  s.name             = 'wajuce'
  s.version          = '0.0.1'
  s.summary          = 'Web Audio API for Flutter, powered by an iPlug2-backed native engine.'
  s.description      = <<-DESC
Cross-platform Web Audio API implementation for Flutter.
                       DESC
  s.homepage         = 'https://github.com/acidsound/wajuce'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'AcidApps' => 'dev@acidapps.io' }

  s.source           = { :path => '.' }
  # Shared with the Swift Package Manager target in ios/wajuce/Package.swift.
  s.source_files     = 'wajuce/Sources/wajuce/WajuceIPlug.mm'
  s.dependency 'Flutter'

  s.platform = :ios, '15.0'
  s.requires_arc = false
  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'GCC_PREPROCESSOR_DEFINITIONS' => [
      'WAJUCE_USE_RTAUDIO=0',
      'WAJUCE_USE_RTMIDI=0',
      # Output-only: keep all microphone symbols out of the binary so apps that
      # never capture audio don't need NSMicrophoneUsageDescription (ITMS-90683).
      'WAJUCE_ENABLE_AUDIO_INPUT=0',
    ].join(' '),
    'HEADER_SEARCH_PATHS' => [
      '"$(PODS_TARGET_SRCROOT)/../native/engine/Source"',
      '"$(PODS_TARGET_SRCROOT)/../native/engine/vendor/iPlug2/IPlug"',
      '"$(PODS_TARGET_SRCROOT)/../native/engine/vendor/iPlug2/WDL"',
    ].join(' '),
    'OTHER_CPLUSPLUSFLAGS' => '-Wno-everything',
  }

  s.frameworks = 'AudioToolbox', 'AVFoundation', 'CoreFoundation', 'Foundation'
end
