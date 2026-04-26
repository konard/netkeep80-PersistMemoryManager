#pragma once
#include <cstddef>
#include <cstdint>
namespace pmm {
/*
## pmm-recoverymode
*/
enum class RecoveryMode : std::uint8_t {
Verify = 0, Repair = 1, };
/*
## pmm-violationtype
*/
enum class ViolationType : std::uint8_t {
None = 0, BlockStateInconsistent, PrevOffsetMismatch, CounterMismatch, FreeTreeStale, ForestRegistryMissing, ForestDomainMissing, ForestDomainFlagsMissing, HeaderCorruption, };
/*
## pmm-diagnosticaction
*/
enum class DiagnosticAction : std::uint8_t {
NoAction = 0, Repaired, Rebuilt, Aborted, };
/*
## pmm-diagnosticentry
*/
struct DiagnosticEntry {
ViolationType type = ViolationType::None;
DiagnosticAction action = DiagnosticAction::NoAction;
std::uint64_t block_index = 0;
std::uint64_t expected = 0;
std::uint64_t actual = 0;
};
inline constexpr std::size_t kMaxDiagnosticEntries = 64;
/*
## pmm-verifyresult
*/
struct VerifyResult {
RecoveryMode mode = RecoveryMode::Verify;
bool ok = true;
std::size_t violation_count = 0;
DiagnosticEntry entries[kMaxDiagnosticEntries] = {
};
std::size_t entry_count = 0;
void add(ViolationType type, DiagnosticAction action, std::uint64_t block_index = 0, std::uint64_t expected = 0, std::uint64_t actual = 0) noexcept {
ok = false;
violation_count++;
if (entry_count < kMaxDiagnosticEntries) {
entries[entry_count].type = type;
entries[entry_count].action = action;
entries[entry_count].block_index = block_index;
entries[entry_count].expected = expected;
entries[entry_count].actual = actual;
entry_count++;
}
}
};
}
