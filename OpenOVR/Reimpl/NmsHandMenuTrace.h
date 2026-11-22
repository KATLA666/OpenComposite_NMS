#pragma once

// Temporary, opt-in hand-menu diagnostics. Fixed storage; at most 32 lines/s.
// Poll logs describe returned OpenVR data. RAW/BIND logs are read-only probes.
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <type_traits>
#ifdef _WIN32
#include <windows.h>
#endif

namespace NmsHandMenuTrace {
inline constexpr std::array<const char*, 12> targets{
    "/actions/frontend_right/in/menu_altselect", "/actions/frontend_left/in/menu_altselect",
    "/actions/frontend_right/in/select", "/actions/frontend_left/in/select",
    "/actions/frontend_right/in/back", "/actions/frontend_left/in/back",
    "/actions/onfootcontrols_right/in/alt_fire", "/actions/onfootcontrols_left/in/alt_fire",
    "/actions/onfootcontrols_right/in/fire", "/actions/onfootcontrols_left/in/fire",
    "/actions/onfootquickmenu_right/in/select", "/actions/onfootquickmenu_left/in/select"
};
enum class Api : size_t { Sync, Digital, Legacy, Origins, OriginInfo, BindingInfo, Pose, Analog, SkeletalAction, Bones, SkeletalSummary, EstimatedBones, EstimatedSummary, FingerState, WaitPoses, LastPoses, SinglePose, SystemPoses, ComponentBinding, Count };
inline bool Enabled()
{
    static const bool enabled = [] {
#ifdef _WIN32
        char value[2]{};
        return GetEnvironmentVariableA("OPENCOMPOSITE_NMS_MENU_TRACE", value, sizeof(value)) == 1 && value[0] == '1';
#else
        const char* value = std::getenv("OPENCOMPOSITE_NMS_MENU_TRACE");
        return value && std::strcmp(value, "1") == 0;
#endif
    }();
    return enabled;
}
inline std::array<std::atomic<uint64_t>, static_cast<size_t>(Api::Count)> counters{};
inline std::array<std::atomic<uint64_t>, targets.size()> pollCounts{};
inline std::atomic<uint64_t> dropped{0}, lastProbe{0};
inline void Hit(Api api)
{
    if (Enabled()) counters[static_cast<size_t>(api)].fetch_add(1, std::memory_order_relaxed);
}
inline uint64_t Millis()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline uint64_t Mix(uint64_t hash, uint64_t value) { return (hash ^ value) * UINT64_C(1099511628211); }
template <class T> inline uint64_t HandleBits(T value)
{
    // OpenXR handles are opaque pointers on 64-bit targets, integers elsewhere.
    if constexpr (std::is_pointer_v<T>) return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value));
    else return static_cast<uint64_t>(value);
}
inline uint64_t Hash(const char* value)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (; *value; ++value) hash = Mix(hash, static_cast<unsigned char>(*value));
    return hash;
}
inline uint32_t Bits(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}
inline int Target(const char* name)
{
    for (size_t i = 0; i < targets.size(); ++i)
        if (std::strcmp(name, targets[i]) == 0) return static_cast<int>(i);
    return -1;
}
inline bool GateAt(size_t channel, uint64_t signature, uint64_t now, uint64_t heartbeat = 2000)
{
    struct Entry { uint64_t signature=0, ms=0; bool seen=false; };
    static std::array<Entry, 272> entries{};
    static std::mutex mutex;
    static uint64_t window=0;
    static unsigned lines=0;
    if (!Enabled() || channel >= entries.size()) return false;
    std::lock_guard<std::mutex> lock(mutex);
    auto& entry=entries[channel];
    if (entry.seen && entry.signature == signature && now-entry.ms < heartbeat) return false;
    if (now-window >= 1000) { window=now; lines=0; }
    if (lines >= 32) { dropped.fetch_add(1, std::memory_order_relaxed); return false; }
    ++lines;
    entry={signature,now,true};
    return true;
}
inline bool Gate(size_t channel, uint64_t signature, uint64_t heartbeat = 2000)
{
    return Enabled() && GateAt(channel, signature, Millis(), heartbeat);
}
inline void Print(const char* format, ...)
{
    // Callers must acquire a Gate permit before formatting or printing.
    char body[1800], line[1900];
    va_list args;
    va_start(args, format);
    std::vsnprintf(body, sizeof(body), format, args);
    va_end(args);
    std::snprintf(line, sizeof(line), "[NMS_MENU] %s\n", body);
#ifdef _WIN32
    OutputDebugStringA(line);
#else
    std::fputs(line, stderr);
#endif
}

// Separate channel ranges keep the original twelve action probes compatible.
// 160..227: additional digital actions; 240..263: six analog actions.
inline constexpr std::array<const char*, 17> extraDigitalTargets{
    "/actions/onfootcontrols/in/vr_snapleft", "/actions/onfootcontrols/in/vr_snapright",
    "/actions/onfootcontrols/in/vr_snaparound", "/actions/onfootcontrols/in/vr_turnleft",
    "/actions/onfootcontrols/in/vr_turnright", "/actions/frontend/in/vr_turnleft",
    "/actions/frontend/in/vr_turnright", "/actions/onfootcontrols_right/in/vr_turnleft",
    "/actions/onfootcontrols_right/in/vr_turnright", "/actions/onfootcontrols_left/in/vr_turnleft",
    "/actions/onfootcontrols_left/in/vr_turnright", "/actions/frontend_right/in/exit",
    "/actions/frontend_right/in/menu_canceldelete", "/actions/onfootcontrols_right/in/back",
    "/actions/frontend_left/in/exit", "/actions/frontend_left/in/menu_canceldelete",
    "/actions/onfootcontrols_left/in/back"
};
inline constexpr std::array<const char*, 6> analogTargets{
    "/actions/onfootcontrols/in/camera", "/actions/onfootcontrols/in/move",
    "/actions/frontend/in/camera", "/actions/frontend/in/cursor",
    "/actions/frontend/in/move", "/actions/frontend/in/char_cam"
};
inline int ExtraDigitalTarget(const char* name)
{
    for (size_t i = 0; i < extraDigitalTargets.size(); ++i)
        if (std::strcmp(name, extraDigitalTargets[i]) == 0) return static_cast<int>(i);
    return -1;
}
inline int AnalogTarget(const char* name)
{
    for (size_t i = 0; i < analogTargets.size(); ++i)
        if (std::strcmp(name, analogTargets[i]) == 0) return static_cast<int>(i);
    return -1;
}
struct AnalogSample {
    bool queried = false, active = false, changed = false;
    float x = 0, y = 0, deadzone = 0, maxzone = 1;
};
inline int AnalogBucket(float value)
{
    if (!std::isfinite(value)) return 99;
    if (value < -1.05f) return -10;
    if (value > 1.05f) return 10;
    return static_cast<int>(value * 8.0f);
}
template<class Data> inline void AnalogReturned(int target, size_t lane, uint64_t sync,
    const char* action, const char* restrictPath, const char* origin, const Data& out, const AnalogSample (&raw)[2])
{
    uint64_t signature = uint64_t(out.bActive);
    signature = Mix(signature, static_cast<uint64_t>(AnalogBucket(out.x)));
    signature = Mix(signature, static_cast<uint64_t>(AnalogBucket(out.y)));
    signature = Mix(signature, static_cast<uint64_t>(AnalogBucket(out.deltaX)));
    signature = Mix(signature, static_cast<uint64_t>(AnalogBucket(out.deltaY)));
    signature = Mix(signature, Hash(origin));
    for (const auto& hand : raw) {
        signature = Mix(signature, uint64_t(hand.queried) | (uint64_t(hand.active) << 1));
        signature = Mix(signature, static_cast<uint64_t>(AnalogBucket(hand.x)));
        signature = Mix(signature, static_cast<uint64_t>(AnalogBucket(hand.y)));
    }
    if (!Gate(240 + size_t(target) * 4 + lane, signature)) return;
    Print("ANALOG act=%s sync=%llu restrict=%s A=%d value=[%.4f %.4f] delta=[%.4f %.4f] origin=%s updateTime=%.6f L[queried=%d A=%d C=%d raw=%.4f/%.4f dz=%.3f max=%.3f] R[queried=%d A=%d C=%d raw=%.4f/%.4f dz=%.3f max=%.3f]",
        action, static_cast<unsigned long long>(sync), restrictPath, int(out.bActive),
        double(out.x), double(out.y), double(out.deltaX), double(out.deltaY), origin, double(out.fUpdateTime),
        int(raw[0].queried), int(raw[0].active), int(raw[0].changed), double(raw[0].x), double(raw[0].y), double(raw[0].deadzone), double(raw[0].maxzone),
        int(raw[1].queried), int(raw[1].active), int(raw[1].changed), double(raw[1].x), double(raw[1].y), double(raw[1].deadzone), double(raw[1].maxzone));
}

inline bool BeginProbe()
{
    if (!Enabled()) return false;
    const uint64_t now=Millis();
    uint64_t old=lastProbe.load(std::memory_order_relaxed);
    if (now-old < 2000 || !lastProbe.compare_exchange_strong(old,now,std::memory_order_relaxed)) return false;
    if (Gate(0,0,UINT64_MAX))
        Print("BOOT version=3 edgeClock=input_sync build=%s_%s cap=32_lines_per_second probeMs=2000", __DATE__, __TIME__);
    return true;
}

// One Gate permit is required before calling this formatter.
template<class Pose> inline void PosePermitted(const char* api, int hand, uint32_t index, int origin, const Pose* pose)
{
    if (!pose) {
        Print("POSE api=%s hand=%d device=%u origin=%d output=<not-requested>", api, hand, index, origin);
        return;
    }
    const auto& m=pose->mDeviceToAbsoluteTracking.m;
    Print("POSE api=%s hand=%d device=%u origin=%d valid=%d connected=%d tracking=%d pos=[%.4f %.4f %.4f] minusZ=[%.4f %.4f %.4f] up=[%.4f %.4f %.4f]",
        api,hand,index,origin,int(pose->bPoseIsValid),int(pose->bDeviceIsConnected),int(pose->eTrackingResult),
        double(m[0][3]),double(m[1][3]),double(m[2][3]),-double(m[0][2]),-double(m[1][2]),-double(m[2][2]),
        double(m[0][1]),double(m[1][1]),double(m[2][1]));
}
inline void Coverage()
{
    if (!Gate(136,0)) return;
    auto count=[](Api api) { return static_cast<unsigned long long>(counters[static_cast<size_t>(api)].load(std::memory_order_relaxed)); };
    Print("COVERAGE analog=%llu skeletalAction=%llu bones=%llu skeletalSummary=%llu estimatedBones=%llu estimatedSummary=%llu fingerState=%llu waitPoses=%llu lastPoses=%llu singlePose=%llu systemPoses=%llu componentBinding=%llu",
        count(Api::Analog),count(Api::SkeletalAction),count(Api::Bones),count(Api::SkeletalSummary),
        count(Api::EstimatedBones),count(Api::EstimatedSummary),count(Api::FingerState),count(Api::WaitPoses),
        count(Api::LastPoses),count(Api::SinglePose),count(Api::SystemPoses),count(Api::ComponentBinding));
}

inline void Summary(uint64_t sync, uint64_t digital, uint64_t present)
{
    if (!Gate(9,0)) return;
    Print("SUMMARY sync=%llu digital=%llu edgeClock=input_sync targets=0x%llx OCcalls: sync=%llu digital=%llu legacy=%llu origins=%llu originInfo=%llu bindingInfo=%llu pose=%llu dropped=%llu",
        static_cast<unsigned long long>(sync),static_cast<unsigned long long>(digital),static_cast<unsigned long long>(present),
        static_cast<unsigned long long>(counters[0].load()),static_cast<unsigned long long>(counters[1].load()),
        static_cast<unsigned long long>(counters[2].load()),static_cast<unsigned long long>(counters[3].load()),
        static_cast<unsigned long long>(counters[4].load()),static_cast<unsigned long long>(counters[5].load()),
        static_cast<unsigned long long>(counters[6].load()),static_cast<unsigned long long>(dropped.load()));
}
}
