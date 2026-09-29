#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>

// Optional CPU-side timings. No GPU timestamp claims, growing history, or per-frame output.
// OOVR_LOG and the Windows API declarations are supplied before this header.
namespace BackendPerf {
enum class Stage : size_t { Wait, Begin, Views, Copy, Layers, End, Post, Pump, Count };
enum class Counter : size_t { TimingQueries, NoRender, Discarded, Sessions, Profiles, EndErrors, Count };

inline bool EnvironmentFlag(const char* name)
{
#ifdef _WIN32
	char value[2]{};
	return GetEnvironmentVariableA(name, value, sizeof(value)) == 1 && value[0] == '1';
#else
	const char* value = std::getenv(name);
	return value && value[0] == '1' && value[1] == '\0';
#endif
}

inline bool Enabled()
{
	static const bool enabled = EnvironmentFlag("OPENCOMPOSITE_PERF_TRACE");
	return enabled;
}

inline int64_t NowNs()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(
	    std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct Sample {
	std::atomic<uint64_t> count{ 0 };
	std::atomic<uint64_t> totalNs{ 0 };
	std::atomic<uint64_t> maxNs{ 0 };
};

struct Statistics {
	std::array<Sample, static_cast<size_t>(Stage::Count)> stages{};
	std::array<std::atomic<uint64_t>, static_cast<size_t>(Counter::Count)> counters{};
	std::atomic<int64_t> lastReportNs{ NowNs() };
	std::atomic<int64_t> displayPeriodNs{ 0 };
	std::atomic<size_t> overlayCount{ 0 };
	std::atomic<int> graphicsAPI{ -1 };
};

inline Statistics& Data()
{
	static Statistics data;
	return data;
}

inline void Log(const char* format, ...)
{
	char line[1024];
	va_list args;
	va_start(args, format);
	std::vsnprintf(line, sizeof(line), format, args);
	va_end(args);
#ifdef _WIN32
	OutputDebugStringA(line);
#else
	OOVR_LOG(line);
#endif
}

inline void Count(Counter counter)
{
	if (Enabled())
		Data().counters[static_cast<size_t>(counter)].fetch_add(1, std::memory_order_relaxed);
}

inline void SetDisplayPeriod(int64_t periodNs)
{
	if (Enabled())
		Data().displayPeriodNs.store(periodNs, std::memory_order_relaxed);
}

inline void SetOverlayCount(size_t count)
{
	if (Enabled())
		Data().overlayCount.store(count, std::memory_order_relaxed);
}

inline void SetGraphicsAPI(int api)
{
	if (Enabled())
		Data().graphicsAPI.store(api, std::memory_order_relaxed);
}

class Scope {
	Stage stage;
	bool enabled;
	int64_t startNs = 0;

public:
	explicit Scope(Stage stage)
	    : stage(stage), enabled(Enabled())
	{
		if (enabled) {
			Data(); // Start the reporting window before the measured operation.
			startNs = NowNs();
		}
	}
	Scope(const Scope&) = delete;
	Scope& operator=(const Scope&) = delete;
	~Scope()
	{
		if (!enabled)
			return;
		const uint64_t elapsed = static_cast<uint64_t>(NowNs() - startNs);
		Sample& sample = Data().stages[static_cast<size_t>(stage)];
		sample.totalNs.fetch_add(elapsed, std::memory_order_relaxed);
		sample.count.fetch_add(1, std::memory_order_relaxed);
		uint64_t previous = sample.maxNs.load(std::memory_order_relaxed);
		while (elapsed > previous && !sample.maxNs.compare_exchange_weak(previous, elapsed, std::memory_order_relaxed)) {
		}
	}
};

template <typename F>
decltype(auto) Measure(Stage stage, F&& operation)
{
	Scope scope(stage);
	return std::forward<F>(operation)();
}

inline void MaybeReport()
{
	if (!Enabled())
		return;
	Statistics& data = Data();
	const int64_t now = NowNs();
	int64_t previous = data.lastReportNs.load(std::memory_order_relaxed);
	if (now - previous < INT64_C(5000000000))
		return;
	// Only one reporter per window, even if different caller threads reach this point.
	if (!data.lastReportNs.compare_exchange_strong(previous, now, std::memory_order_relaxed))
		return;

	const double seconds = static_cast<double>(now - previous) / 1e9;
	std::array<double, static_cast<size_t>(Stage::Count)> average{}, maximum{};
	std::array<uint64_t, static_cast<size_t>(Stage::Count)> calls{};
	for (size_t i = 0; i < calls.size(); ++i) {
		calls[i] = data.stages[i].count.exchange(0, std::memory_order_relaxed);
		const uint64_t total = data.stages[i].totalNs.exchange(0, std::memory_order_relaxed);
		maximum[i] = static_cast<double>(data.stages[i].maxNs.exchange(0, std::memory_order_relaxed)) / 1e6;
		average[i] = calls[i] ? static_cast<double>(total) / (1e6 * calls[i]) : 0.0;
	}
	std::array<uint64_t, static_cast<size_t>(Counter::Count)> counters{};
	for (size_t i = 0; i < counters.size(); ++i)
		counters[i] = data.counters[i].exchange(0, std::memory_order_relaxed);

	// avg/max are CPU milliseconds per call; copy is per eye, not per frame.
	// endHz counts xrEndFrame calls, including empty/failed submissions, not displayed FPS.
	Log("[OC_PERF] dt=%.2f api=%d endHz=%.2f periodMs=%.3f wait=%.3f/%.3f begin=%.3f/%.3f views=%.3f/%.3f copy=%.3f/%.3f\n",
	    seconds, data.graphicsAPI.load(std::memory_order_relaxed), calls[5] / seconds,
	    static_cast<double>(data.displayPeriodNs.load(std::memory_order_relaxed)) / 1e6,
	    average[0], maximum[0], average[1], maximum[1], average[2], maximum[2], average[3], maximum[3]);
	Log("[OC_PERF] layers=%.3f/%.3f end=%.3f/%.3f post=%.3f/%.3f pump=%.3f/%.3f timingCalls=%llu noRender=%llu discarded=%llu sessions=%llu profiles=%llu endErrors=%llu overlays=%llu\n",
	    average[4], maximum[4], average[5], maximum[5], average[6], maximum[6], average[7], maximum[7],
	    static_cast<unsigned long long>(counters[0]), static_cast<unsigned long long>(counters[1]),
	    static_cast<unsigned long long>(counters[2]), static_cast<unsigned long long>(counters[3]),
	    static_cast<unsigned long long>(counters[4]), static_cast<unsigned long long>(counters[5]),
	    static_cast<unsigned long long>(data.overlayCount.load(std::memory_order_relaxed)));
}
} // namespace BackendPerf
