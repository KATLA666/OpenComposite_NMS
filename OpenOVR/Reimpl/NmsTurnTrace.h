#pragma once

// Temporary, opt-in NMS turn sensitivity diagnostics for Dbgview.exe.
// Enable with: $env:OPENCOMPOSITE_NMS_TURN_TRACE = "1"
// Filter DebugView for: [NMS_TURN]
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#ifdef _WIN32
#include <windows.h>
#endif

namespace NmsTurnTrace {
struct AnalogSample {
	bool queried = false, active = false, changed = false;
	float x = 0.0f, y = 0.0f, deadzone = 0.0f, maxzone = 1.0f;
};

inline bool Enabled()
{
	static const bool enabled = [] {
#ifdef _WIN32
		char value[2]{};
		return GetEnvironmentVariableA("OPENCOMPOSITE_NMS_TURN_TRACE", value, sizeof(value)) == 1 && value[0] == '1';
#else
		const char* value = std::getenv("OPENCOMPOSITE_NMS_TURN_TRACE");
		return value && std::strcmp(value, "1") == 0;
#endif
	}();
	return enabled;
}

inline uint64_t Millis()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
	    std::chrono::steady_clock::now().time_since_epoch()).count());
}

inline uint64_t Mix(uint64_t hash, uint64_t value)
{
	return (hash ^ value) * UINT64_C(1099511628211);
}

inline uint64_t Hash(const char* value)
{
	uint64_t hash = UINT64_C(14695981039346656037);
	for (; value && *value; ++value)
		hash = Mix(hash, static_cast<unsigned char>(*value));
	return hash;
}

inline int AnalogBucket(float value)
{
	if (!std::isfinite(value)) return 99;
	if (value < -1.05f) return -10;
	if (value > 1.05f) return 10;
	return static_cast<int>(value * 8.0f);
}

inline void Print(const char* format, ...)
{
	char body[1800], line[1900];
	va_list args;
	va_start(args, format);
	std::vsnprintf(body, sizeof(body), format, args);
	va_end(args);
	std::snprintf(line, sizeof(line), "[NMS_TURN] %s\n", body);
#ifdef _WIN32
	OutputDebugStringA(line);
#else
	std::fputs(line, stderr);
#endif
}

inline void BootOnce()
{
	if (!Enabled()) return;
	static std::atomic<bool> booted{ false };
	if (!booted.exchange(true, std::memory_order_relaxed)) {
		Print("BOOT version=2 build=%s_%s filter=[NMS_TURN]", __DATE__, __TIME__);
	}
}

inline bool ContainsICase(const char* haystack, const char* needle)
{
	if (!haystack || !needle || !*needle) return false;
	const size_t nlen = std::strlen(needle);
	for (const char* p = haystack; *p; ++p) {
		size_t i = 0;
		while (i < nlen && p[i]) {
			char a = p[i];
			char b = needle[i];
			if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
			if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
			if (a != b) break;
			++i;
		}
		if (i == nlen) return true;
	}
	return false;
}

inline bool IsTurnName(const char* name)
{
	return ContainsICase(name, "turn")
	    || ContainsICase(name, "snap")
	    || ContainsICase(name, "camera")
	    || ContainsICase(name, "char_cam")
	    || ContainsICase(name, "steer");
}

inline bool Gate(const std::string& key, uint64_t signature, uint64_t minIntervalMs = 50, uint64_t heartbeatMs = 2000)
{
	if (!Enabled()) return false;
	struct Entry {
		uint64_t signature = 0;
		uint64_t ms = 0;
		bool seen = false;
	};
	static std::unordered_map<std::string, Entry> entries;
	static std::mutex mutex;
	const uint64_t now = Millis();
	std::lock_guard<std::mutex> lock(mutex);
	auto& entry = entries[key];
	if (entry.seen) {
		if (entry.signature == signature) {
			if (heartbeatMs == UINT64_MAX || now - entry.ms < heartbeatMs) return false;
		} else {
			if (now - entry.ms < minIntervalMs) return false;
		}
	}
	entry = { signature, now, true };
	return true;
}

inline std::atomic<uint64_t> syncCount{ 0 }, analogCount{ 0 }, digitalCount{ 0 }, legacyCount{ 0 };

inline void ReportRateIfDue(const char* profileLeft, const char* profileRight)
{
	if (!Enabled()) return;
	static std::mutex mutex;
	static uint64_t lastMs = 0;
	static uint64_t lastSync = 0, lastAnalog = 0, lastDigital = 0, lastLegacy = 0;
	const uint64_t now = Millis();
	std::lock_guard<std::mutex> lock(mutex);
	if (lastMs == 0) {
		lastMs = now;
		lastSync = syncCount.load(std::memory_order_relaxed);
		lastAnalog = analogCount.load(std::memory_order_relaxed);
		lastDigital = digitalCount.load(std::memory_order_relaxed);
		lastLegacy = legacyCount.load(std::memory_order_relaxed);
		return;
	}
	if (now - lastMs < 1000) return;
	const double dt = static_cast<double>(now - lastMs) / 1000.0;
	const uint64_t curSync = syncCount.load(std::memory_order_relaxed);
	const uint64_t curAnalog = analogCount.load(std::memory_order_relaxed);
	const uint64_t curDigital = digitalCount.load(std::memory_order_relaxed);
	const uint64_t curLegacy = legacyCount.load(std::memory_order_relaxed);
	const uint64_t dSync = curSync - lastSync;
	const uint64_t dAnalog = curAnalog - lastAnalog;
	const uint64_t dDigital = curDigital - lastDigital;
	const uint64_t dLegacy = curLegacy - lastLegacy;
	lastMs = now;
	lastSync = curSync;
	lastAnalog = curAnalog;
	lastDigital = curDigital;
	lastLegacy = curLegacy;
	Print("RATE dt=%.2f syncs=%llu syncHz=%.1f analog=%llu digital=%llu legacy=%llu profiles=[L=%s | R=%s]",
	    dt, static_cast<unsigned long long>(dSync), dt > 0.0 ? static_cast<double>(dSync) / dt : 0.0,
	    static_cast<unsigned long long>(dAnalog), static_cast<unsigned long long>(dDigital),
	    static_cast<unsigned long long>(dLegacy), profileLeft, profileRight);
}
}
