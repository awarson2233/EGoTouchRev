#pragma once
// 分段计时：构建时打开 EGO_TSACORE_PROFILE（定义 TSACORE_PROFILE）后，主流程的每个步骤记累计耗时与次数，
// 宿主经 Core::ProfileEntries 取走（回放 --profile 打印）。默认不编入，TSA_STEP 只是原样调用。
// 每段计时本身有几十 ns 的开销，一帧几十段叠加起来会抬高整帧耗时：宿主可经 Core::ProfileSteps 关掉分步，
// 只留 TSA_PROFILE_SCOPE 的整帧计时。

#include <cstdint>

#if defined(TSACORE_PROFILE)
#include <chrono>

namespace Solvers::Tsa::Core::Profile {

void Add(const char* name, uint64_t ns) noexcept;
extern bool g_steps;  // 分步计时是否打开

class Scope {
public:
    explicit Scope(const char* name, bool on = true) noexcept : name_(on ? name : nullptr) {
        if (name_ != nullptr) start_ = std::chrono::steady_clock::now();
    }
    ~Scope() {
        if (name_ == nullptr) return;
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_);
        Add(name_, static_cast<uint64_t>(ns.count()));
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    const char* name_;
    std::chrono::steady_clock::time_point start_;
};

}  // namespace Solvers::Tsa::Core::Profile

#define TSA_PROFILE_SCOPE(name) ::Solvers::Tsa::Core::Profile::Scope tsaProfileScope_{name}
#define TSA_PROFILE_STEP_SCOPE(name) \
    ::Solvers::Tsa::Core::Profile::Scope tsaProfileScope_ { name, ::Solvers::Tsa::Core::Profile::g_steps }
#else
#define TSA_PROFILE_SCOPE(name) ((void)0)
#define TSA_PROFILE_STEP_SCOPE(name) ((void)0)
#endif

/// 主流程的一个步骤；打开分步计时时以调用表达式为名记一段。
#define TSA_STEP(call) \
    do {                                \
        TSA_PROFILE_STEP_SCOPE(#call);  \
        call;                           \
    } while (false)
