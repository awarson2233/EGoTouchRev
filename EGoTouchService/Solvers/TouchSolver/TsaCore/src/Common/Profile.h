#pragma once
// 分段计时：构建时打开 EGO_TSACORE_PROFILE（定义 TSACORE_PROFILE）后，主流程的每个步骤记累计耗时与次数，
// 宿主经 TsaCore_ProfileEntries 取走（回放 --profile 打印）。默认不编入，TSA_STEP 只是原样调用。

#include <cstdint>

#if defined(TSACORE_PROFILE)
#include <chrono>

namespace Solvers::Tsa::Core::Profile {

void Add(const char* name, uint64_t ns) noexcept;

class Scope {
public:
    explicit Scope(const char* name) noexcept : name_(name), start_(std::chrono::steady_clock::now()) {}
    ~Scope() {
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
#else
#define TSA_PROFILE_SCOPE(name) ((void)0)
#endif

/// 主流程的一个步骤；打开计时时以调用表达式为名记一段。
#define TSA_STEP(call) \
    do {                           \
        TSA_PROFILE_SCOPE(#call);  \
        call;                      \
    } while (false)
