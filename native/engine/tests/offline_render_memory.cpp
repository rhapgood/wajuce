// Reproduces the WAV-export OOM crash scenario: a dense, song-like graph
// (hundreds of oscillator -> filter -> gain voices, all created up front, each
// scheduled at its own absolute start/stop time) rendered as ONE long offline
// buffer. Before render() was blocked, every node allocated a buffer of
// channels * fullLength floats -> gigabytes -> the OS reaped the process
// (iOS high-watermark OOM at ~3.3 GB; silent kill on Android). With blocked
// rendering, peak memory stays bounded. This harness fails loudly (timeout /
// kill) if the regression returns, and otherwise prints peak RSS so the bound
// is visible.
#include "../../../src/wajuce.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#if defined(__linux__) || defined(__ANDROID__)
#include <cstdio>
static long peakRssKb() {
  FILE *f = std::fopen("/proc/self/status", "r");
  if (!f) return -1;
  char line[256];
  long kb = -1;
  while (std::fgets(line, sizeof(line), f)) {
    if (std::sscanf(line, "VmHWM: %ld kB", &kb) == 1) break;
  }
  std::fclose(f);
  return kb;
}
#else
static long peakRssKb() { return -1; }
#endif

int main() {
  constexpr int sampleRate = 44100;
  constexpr int channels = 2;
  // A multi-loop song's worth of audio. The pre-fix code gave every node a
  // (frames * channels) float buffer all at once: with 150 nodes that is
  // 150 * 2 * 20 * 44100 * 4 bytes * 2 (current+previous) ~= 4 GB and the OS
  // reaped the process. Blocked rendering keeps peak RSS to tens of MB.
  constexpr int frames = sampleRate * 20;
  // A dense song: many simultaneous voices, each osc -> biquad -> gain.
  constexpr int voices = 150;

  const int ctx = wajuce_context_create(sampleRate, 512, 0, channels);
  const int dest = wajuce_context_get_destination_id(ctx);
  const int master = wajuce_create_gain(ctx);
  wajuce_param_set(master, "gain", 0.2f);
  wajuce_connect(ctx, master, dest, 0, 0);

  for (int v = 0; v < voices; ++v) {
    const int osc = wajuce_create_oscillator(ctx);
    const int filt = wajuce_create_biquad_filter(ctx);
    const int gain = wajuce_create_gain(ctx);
    wajuce_param_set(osc, "frequency", 110.0f + (v % 48) * 12.0f);
    wajuce_param_set(filt, "frequency", 1200.0f);
    wajuce_param_set(gain, "gain", 0.3f);
    wajuce_connect(ctx, osc, filt, 0, 0);
    wajuce_connect(ctx, filt, gain, 0, 0);
    wajuce_connect(ctx, gain, master, 0, 0);
    // Spread the voices across the whole timeline, each a short note.
    const double seconds = frames / static_cast<double>(sampleRate);
    const double start = (v / static_cast<double>(voices)) * (seconds - 1.0);
    wajuce_osc_start(osc, start);
    wajuce_osc_stop(osc, start + 0.4);
  }

  std::printf("rendering %d frames (%.0f s), %d voices...\n", frames,
              frames / static_cast<double>(sampleRate), voices);
  std::vector<float> out(static_cast<size_t>(frames) * channels, 0.0f);
  const int32_t got = wajuce_context_render(ctx, out.data(), frames, channels);

  // Sanity: it rendered the whole length and produced non-trivial signal.
  double energy = 0.0;
  for (float s : out) energy += static_cast<double>(s) * s;

  const long rss = peakRssKb();
  std::printf("rendered %d frames; signal energy=%.1f; peak RSS=%ld kB (%.1f MB)\n",
              got, energy, rss, rss / 1024.0);

  wajuce_context_destroy(ctx);

  if (got != frames) {
    std::fprintf(stderr, "FAIL: render returned %d, expected %d\n", got, frames);
    return 1;
  }
  if (energy <= 0.0) {
    std::fprintf(stderr, "FAIL: rendered silence\n");
    return 1;
  }
  std::puts("PASS: offline_render_memory");
  return 0;
}
