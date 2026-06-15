#ifndef GEM5_MEM_UPF_STATS_HH
#define GEM5_MEM_UPF_STATS_HH

#include <cstddef>

namespace gem5
{
namespace UpfStats {
  extern thread_local size_t lastCheckCount;
}

constexpr int UPF_TRAP_COST = 3;

} // namespace gem5

#endif
