// Message tags reserve 32 bits for a runtime string pointer (CustomTagProcessor::setArgString).
// On 64-bit hosts the pointer goes into a table and the tag holds a handle (low byte never zero,
// matching the game's "first byte is non-zero" validity check).
#pragma once
#include <cstdint>

namespace pc::msgarg {
uint32_t store(const wchar_t* str);
const wchar_t* load(const void* param);
}  // namespace pc::msgarg
