import 'dart:async';
import 'dart:math' as math;
import 'dart:typed_data';

import 'audio_buffer.dart';
import 'backend/backend.dart' as backend;
import 'context.dart';

/// Offline rendering context for non-real-time audio processing.
/// Mirrors Web Audio API OfflineAudioContext.
///
/// Useful for testing, offline rendering, and audio export.
///
/// ```dart
/// final offline = WAOfflineContext(
///   numberOfChannels: 2,
///   length: 44100,  // 1 second
///   sampleRate: 44100,
/// );
///
/// final osc = offline.createOscillator();
/// osc.frequency.value = 440;
/// osc.connect(offline.destination);
/// osc.start();
///
/// final renderedBuffer = await offline.startRendering();
/// ```
class WAOfflineContext extends WAContext {
  final int _numberOfChannels;
  final int _length;
  final double _offlineSampleRate;

  /// Creates a new OfflineAudioContext.
  WAOfflineContext({
    required int numberOfChannels,
    required int length,
    required double sampleRate,
  })  : _numberOfChannels = numberOfChannels,
        _length = length,
        _offlineSampleRate = sampleRate,
        super(sampleRate: sampleRate.toInt(), bufferSize: 512);

  /// Number of channels in the output.
  int get numberOfChannels => _numberOfChannels;

  /// Number of sample frames to render.
  int get length => _length;

  /// The sample rate specified at construction.
  double get offlineSampleRate => _offlineSampleRate;

  /// Start rendering and return the resulting AudioBuffer.
  Future<WABuffer> startRendering() async {
    final renderedChannels =
        backend.contextRender(contextId, _length, _numberOfChannels);
    return WABuffer(
      numberOfChannels: _numberOfChannels,
      length: _length,
      sampleRate: _offlineSampleRate,
      channels: renderedChannels,
    );
  }

  /// Like [startRendering] but renders in [chunkFrames]-sized slices, invoking
  /// [onProgress] with a 0..1 fraction after each, and `await`-ing the event
  /// loop between slices so a long render never blocks the caller's thread.
  ///
  /// This relies on the **native** engine keeping graph state (oscillator phase,
  /// filter/delay history, the playback clock) across consecutive `render`
  /// calls, so N slices produce exactly the same samples as one full render.
  /// On web `contextRender` is a one-shot stub, so prefer [startRendering]
  /// there; this is intended for the native offline-export path.
  Future<WABuffer> startRenderingChunked({
    int chunkFrames = 0,
    void Function(double progress)? onProgress,
  }) async {
    // Slice on a whole number of render blocks: the engine's output depends on
    // where block boundaries land, so an unaligned slice would not match a
    // one-shot render. Round the requested (or default ~0.1 s) chunk up to a
    // multiple of bufferSize.
    final block = math.max(1, bufferSize);
    final wanted = chunkFrames > 0 ? chunkFrames : _offlineSampleRate ~/ 10;
    final chunk = math.max(1, (wanted / block).ceil()) * block;
    final channels = List<Float32List>.generate(
        _numberOfChannels, (_) => Float32List(_length));
    var done = 0;
    while (done < _length) {
      final n = math.min(chunk, _length - done);
      final slice = backend.contextRender(contextId, n, _numberOfChannels);
      for (var c = 0; c < _numberOfChannels && c < slice.length; c++) {
        channels[c].setRange(done, done + n, slice[c]);
      }
      done += n;
      onProgress?.call(done / _length);
      await Future<void>.delayed(Duration.zero);
    }
    return WABuffer(
      numberOfChannels: _numberOfChannels,
      length: _length,
      sampleRate: _offlineSampleRate,
      channels: channels,
    );
  }
}
