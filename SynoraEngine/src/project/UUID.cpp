#include "SynoraEngine/project/UUID.h"

static std::random_device s_Device{};
static std::mt19937_64 s_RandEngine{s_Device()};
static std::uniform_int_distribution<uint64_t> s_Dist{};

namespace SYN {
UUID generateUUID() { return s_Dist(s_RandEngine); }
} // namespace SYN
