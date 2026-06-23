// Verifies that rendering an offline context in slices (many wajuce_context_render
// calls) is sample-for-sample identical to one full-length render. This is the
// contract the Dart `startRenderingChunked` (and the WAV-export progress UI)
// relies on: the engine carries graph state across consecutive render calls, so
// slicing only adds progress checkpoints, never changes the audio.
#include "../../../src/wajuce.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

// Builds a small song-like graph (a few scheduled osc -> biquad -> gain voices)
// into a fresh context and returns its id.
int buildGraph(int sampleRate, int channels) {
  const int ctx = wajuce_context_create(sampleRate, 512, 0, channels);
  const int dest = wajuce_context_get_destination_id(ctx);
  const int master = wajuce_create_gain(ctx);
  wajuce_param_set(master, "gain", 0.3f);
  wajuce_connect(ctx, master, dest, 0, 0);
  for (int v = 0; v < 24; ++v) {
    const int osc = wajuce_create_oscillator(ctx);
    const int filt = wajuce_create_biquad_filter(ctx);
    const int gain = wajuce_create_gain(ctx);
    wajuce_param_set(osc, "frequency", 180.0f + v * 25.0f);
    wajuce_param_set(filt, "frequency", 1500.0f);
    wajuce_param_set(gain, "gain", 0.5f);
    wajuce_connect(ctx, osc, filt, 0, 0);
    wajuce_connect(ctx, filt, gain, 0, 0);
    wajuce_connect(ctx, gain, master, 0, 0);
    const double start = v * 0.1;
    wajuce_osc_start(osc, start);
    wajuce_osc_stop(osc, start + 0.35);
  }
  return ctx;
}

} // namespace

int main() {
  constexpr int sampleRate = 44100;
  constexpr int channels = 2;
  constexpr int frames = sampleRate * 3; // 3 s
  constexpr int total = frames * channels;

  // One-shot render.
  const int ctxA = buildGraph(sampleRate, channels);
  std::vector<float> full(static_cast<size_t>(total), 0.0f);
  wajuce_context_render(ctxA, full.data(), frames, channels);
  wajuce_context_destroy(ctxA);

  // Sliced render of an identical graph. Slice is a multiple of bufferSize (512)
  // so block boundaries land where the one-shot's internal blocks do.
  const int ctxB = buildGraph(sampleRate, channels);
  std::vector<float> sliced(static_cast<size_t>(total), 0.0f);
  const int slice = 512 * 8;
  std::vector<float> tmp(static_cast<size_t>(slice) * channels, 0.0f);
  int done = 0;
  while (done < frames) {
    const int n = std::min(slice, frames - done);
    wajuce_context_render(ctxB, tmp.data(), n, channels);
    // Planar: channel c occupies [c*n, c*n+n) in tmp, [c*frames+done, ...) in out.
    for (int c = 0; c < channels; ++c) {
      for (int i = 0; i < n; ++i) {
        sliced[static_cast<size_t>(c) * frames + done + i] =
            tmp[static_cast<size_t>(c) * n + i];
      }
    }
    done += n;
  }
  wajuce_context_destroy(ctxB);

  double maxDiff = 0.0;
  double energy = 0.0;
  for (int i = 0; i < total; ++i) {
    maxDiff = std::max(maxDiff, std::abs(static_cast<double>(full[i]) - sliced[i]));
    energy += static_cast<double>(full[i]) * full[i];
  }
  std::printf("one-shot energy=%.3f, max sample diff (sliced vs full)=%.3e\n",
              energy, maxDiff);
  if (energy <= 0.0) {
    std::fprintf(stderr, "FAIL: rendered silence\n");
    return 1;
  }
  if (maxDiff > 1.0e-6) {
    std::fprintf(stderr, "FAIL: sliced render differs from one-shot\n");
    return 1;
  }
  std::puts("PASS: offline_render_chunk_equiv");
  return 0;
}
