#pragma once

// Temporary NMS cockpit diagnostic capture v2. No extra OpenVR/OpenXR queries.
// All counters include every entry; ordinary identical lines get a 1 s heartbeat.
// DIG, SYNC and INTERNAL lines are emitted for every poll/sync to show ordering.
// Pose numbers are sampled at <=20 Hz per source/device/context; flag changes
// bypass sampling. s = action syncSerial, f = InternalUpdate syncSerialDigital.
// ctx is an instrumented OC call chain, NOT a native stack trace. "unknown"
// means an external caller OR an internal caller not instrumented here.
#include "generated/interfaces/vrtypes.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

namespace nms_trace {
#define NMS_TRACE_APIS(X) \
    X(UpdateActionState) X(InternalUpdate) X(GetDigitalActionData) X(GetAnalogActionData) \
    X(GetLegacyControllerState) X(GetActionOrigins) X(GetOriginTrackedDeviceInfo) X(GetActionBindingInfo) \
    X(GetPoseActionData) X(GetPoseActionDataRelativeToNow) X(GetPoseActionDataForNextFrame) \
    X(GetSkeletalActionData) X(GetSkeletalBoneData) X(GetSkeletalSummaryData) \
    X(WaitGetPoses) X(GetLastPoses) X(GetSinglePoseRendering) X(GetLastPoseForTrackedDeviceIndex) \
    X(GetControllerState) X(GetControllerStateWithPose) X(GetDeviceToAbsoluteTrackingPose) \
    X(SetTrackingSpace) X(GetTrackingSpace) X(GetSeatedZeroPoseToStandingAbsoluteTrackingPose) \
    X(GetRawZeroPoseToStandingAbsoluteTrackingPose) X(ResetSeatedZeroPose) X(ApplyTransform) \
    X(GetTrackedDeviceIndexForControllerRole) X(GetControllerRoleForTrackedDeviceIndex) \
    X(IsInputAvailable) X(IsInputFocusCapturedByAnotherProcess) X(IsSteamVRDrawingControllers) \
    X(ShouldApplicationPause) X(ShouldApplicationReduceRenderingWork) \
    X(OnPostFrame) X(CheckControllerEvents) X(PollNextEvent) X(PollNextEventWithPose) \
    X(PollNextEventWithPoseAndOverlays) X(GetLastPosePredictionIDs) X(GetPosesForFrame) X(OldSystemWrapper)
enum class Api {
#define NMS_TRACE_ENUM(n) n,
    NMS_TRACE_APIS(NMS_TRACE_ENUM)
#undef NMS_TRACE_ENUM
    Count
};
inline constexpr const char* names[] = {
#define NMS_TRACE_NAME(n) #n,
    NMS_TRACE_APIS(NMS_TRACE_NAME)
#undef NMS_TRACE_NAME
};
#undef NMS_TRACE_APIS
inline std::array<std::atomic<uint64_t>, static_cast<size_t>(Api::Count)> counters{};
inline std::atomic<uint64_t> sync{0}, frame{0}, callSerial{0};
inline thread_local std::string context = "unknown";
inline thread_local uint64_t callId = 0;
inline std::mutex mutex;
inline uint64_t sequence = 0;
struct Entry {
    std::string body;
    uint64_t ms = 0, calls = 0;
};
inline std::map<std::string, Entry> entries;

inline uint64_t Millis()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline long long Qpc()
{
#ifdef _WIN32
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return value.QuadPart;
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}
inline unsigned long ThreadId()
{
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return 0; // Only for the standalone logger test; capture is a Windows build.
#endif
}
inline std::string Format(const char* format, ...)
{
    char body[3072];
    va_list args;
    va_start(args, format);
    vsnprintf(body, sizeof(body), format, args);
    va_end(args);
    return body;
}
inline void Publish(uint64_t s, uint64_t f)
{
    sync.store(s, std::memory_order_relaxed);
    frame.store(f, std::memory_order_relaxed);
}
inline void Hit(Api api) { ++counters[static_cast<size_t>(api)]; }

inline void Emit(const std::string& key, const std::string& body, uint64_t s, uint64_t f, bool force = false)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& e = entries[key + "|" + context];
    ++e.calls;
    const uint64_t now = Millis();
    if (!force && e.calls > 1 && e.body == body && now - e.ms < 1000)
        return;
    e.body = body;
    e.ms = now;
    const std::string line = Format(
        "[NMS_TRACE] seq=%llu qpc=%lld tid=%lu s=%llu f=%llu call=%llu calls=%llu key=%s ctx=%s ",
        static_cast<unsigned long long>(++sequence), Qpc(), ThreadId(),
        static_cast<unsigned long long>(s), static_cast<unsigned long long>(f),
        static_cast<unsigned long long>(callId), static_cast<unsigned long long>(e.calls),
        key.c_str(), context.c_str()) + body + "\n";
#ifdef _WIN32
    OutputDebugStringA(line.c_str());
#else
    fputs(line.c_str(), stderr);
#endif
}
inline void State(const std::string& key, const std::string& body, bool force = false)
{
    Emit(key, body, sync.load(std::memory_order_relaxed), frame.load(std::memory_order_relaxed), force);
}

class Scope {
    std::string previous, key, result = "returned; result-not-recorded";
    uint64_t previousId;
    bool verbose;
public:
    Scope(Api api, const std::string& detail = "", bool logEntry = true)
        : previous(context), key(std::string(names[static_cast<size_t>(api)]) + ":" + detail),
          previousId(callId), verbose(logEntry)
    {
        Hit(api);
        context += "/" + std::string(names[static_cast<size_t>(api)]);
        callId = ++callSerial;
        if (verbose) State("ENTER:" + key, "entered");
    }
    ~Scope()
    {
        if (verbose) State("EXIT:" + key, result);
        context = previous;
        callId = previousId;
    }
    void Result(const std::string& value) { result = value; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};

template<class T> inline T Return(Scope& scope, T value)
{
    scope.Result(Format("result=%lld", static_cast<long long>(value)));
    return value;
}

inline void Coverage(bool force = false)
{
    static std::atomic<uint64_t> last{0};
    const uint64_t now = Millis();
    uint64_t old = last.load();
    if (!force && now - old < 1000) return;
    if (!last.compare_exchange_strong(old, now) && !force) return;
    std::string body = "cumulative OC entries (includes nested/internal calls):";
    for (size_t i = 0; i < static_cast<size_t>(Api::Count); ++i)
        body += " " + std::string(names[i]) + "=" + std::to_string(counters[i].load());
    State("COVERAGE", body, true);
}
inline void Boot(const char* unit, const char* date, const char* time)
{
    long long frequency = 1000000000;
#ifdef _WIN32
    LARGE_INTEGER value{};
    QueryPerformanceFrequency(&value);
    frequency = value.QuadPart;
#endif
    State(std::string("BOOT:") + unit, Format("version=2 build=%s_%s qpcHz=%lld poseIntervalMs=50 origin:0=seated,1=standing,2=raw", date, time, frequency), true);
    Coverage(true);
}

// This gate does not suppress validity/connection/tracking-result/finite changes.
inline bool Sample(const std::string& key, int status, uint64_t interval = 50)
{
    struct Gate { uint64_t ms = 0; int status = 0; bool seen = false; };
    static std::map<std::string, Gate> gates;
    std::lock_guard<std::mutex> lock(mutex);
    auto& g = gates[key + "|" + context];
    const uint64_t now = Millis();
    if (g.seen && status == g.status && now - g.ms < interval) return false;
    g = {now, status, true};
    return true;
}
inline bool Finite(const vr::HmdMatrix34_t& m)
{
    for (const auto& row : m.m)
        for (float value : row)
            if (!std::isfinite(value)) return false;
    return true;
}
inline std::string MatrixText(const vr::HmdMatrix34_t& m)
{
    return Format("M=[%.6f,%.6f,%.6f,%.6f;%.6f,%.6f,%.6f,%.6f;%.6f,%.6f,%.6f,%.6f]",
        m.m[0][0],m.m[0][1],m.m[0][2],m.m[0][3], m.m[1][0],m.m[1][1],m.m[1][2],m.m[1][3],
        m.m[2][0],m.m[2][1],m.m[2][2],m.m[2][3]);
}
inline void Matrix(const std::string& key, const vr::HmdMatrix34_t* m)
{
    if (!m) { State(key, "null matrix"); return; }
    const bool finite = Finite(*m);
    if (Sample(key, int(finite))) State(key, Format("finite=%d ", int(finite)) + MatrixText(*m));
}
inline void Pose(const std::string& source, uint32_t idx, int origin, const vr::TrackedDevicePose_t* p)
{
    const std::string key = "POSE:" + source + ":idx=" + std::to_string(idx) + ":origin=" + std::to_string(origin);
    if (!p) { State(key, "null buffer"); return; }
    const bool connected = p->bDeviceIsConnected, valid = p->bPoseIsValid;
    const int result = connected ? int(p->eTrackingResult) : -1;
    bool finite = true;
    if (valid) {
        finite = Finite(p->mDeviceToAbsoluteTracking);
        for (int i = 0; i < 3; ++i)
            finite = finite && std::isfinite(p->vVelocity.v[i]) && std::isfinite(p->vAngularVelocity.v[i]);
    }
    const int status = result * 8 + int(connected) + 2 * int(valid) + 4 * int(finite);
    if (!Sample(key, status, connected ? 50 : UINT64_MAX)) return;
    std::string body = Format("connected=%d valid=%d tracking=%d finite=%s", int(connected), int(valid), result,
        valid ? (finite ? "1" : "0") : "unread");
    if (valid) body += " " + MatrixText(p->mDeviceToAbsoluteTracking) + Format(
        " v=(%.6f,%.6f,%.6f) w=(%.6f,%.6f,%.6f)",
        p->vVelocity.v[0], p->vVelocity.v[1], p->vVelocity.v[2],
        p->vAngularVelocity.v[0], p->vAngularVelocity.v[1], p->vAngularVelocity.v[2]);
    State(key, body);
}
inline void PoseArray(const char* source, int origin, const vr::TrackedDevicePose_t* poses, uint32_t count)
{
    State(std::string("ARRAY:") + source, Format("origin=%d buffer=%d count=%u captureLimit=%u", origin, int(poses != nullptr), count, vr::k_unMaxTrackedDeviceCount));
    if (!poses) return;
    for (uint32_t i = 0; i < count && i < vr::k_unMaxTrackedDeviceCount; ++i) Pose(source, i, origin, poses + i);
}
inline void ControllerState(const char* source, uint32_t idx, const vr::VRControllerState_t* s)
{
    if (!s) return;
    State(std::string("STATE:") + source + ":" + std::to_string(idx), Format(
        "pressed=%016llx touched=%016llx axes=[%.4f,%.4f;%.4f,%.4f;%.4f,%.4f;%.4f,%.4f;%.4f,%.4f]",
        static_cast<unsigned long long>(s->ulButtonPressed), static_cast<unsigned long long>(s->ulButtonTouched),
        s->rAxis[0].x,s->rAxis[0].y,s->rAxis[1].x,s->rAxis[1].y,s->rAxis[2].x,s->rAxis[2].y,
        s->rAxis[3].x,s->rAxis[3].y,s->rAxis[4].x,s->rAxis[4].y));
}
} // namespace nms_trace
