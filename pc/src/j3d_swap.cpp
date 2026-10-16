#include "pc/j3d.h"

#include <cstring>
#include <mutex>

namespace pc::j3d {

thread_local bool gConvert = false;

namespace {
std::mutex sLock;
constexpr uint8_t kMarker[4] = {'p', 'c', 'L', 'E'};
}  // namespace

bool beginConvert(const void* file) {
    if (file == nullptr) {
        return false;
    }
    std::lock_guard<std::mutex> lk(sLock);
    uint8_t* tail = static_cast<uint8_t*>(const_cast<void*>(file)) + 0x1C;  // last word of the header padding
    if (std::memcmp(tail, kMarker, 4) == 0) {
        return false;
    }
    std::memcpy(tail, kMarker, 4);
    return true;
}

}  // namespace pc::j3d
