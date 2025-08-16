#ifndef GEM5_MEM_UFFD_REGION_TRACKER_HH
#define GEM5_MEM_UFFD_REGION_TRACKER_HH

#include <vector>
#include <map>
#include "base/types.hh"

namespace gem5
{
namespace UffdStats {
  extern thread_local size_t lastCheckCount;
}

constexpr int UFFD_REGION_CHECK_COST = 3;

class UffdRegionTracker
{
  public:
    struct BackingResult {
        bool is_backed;
        size_t regions_checked;
    };

    static UffdRegionTracker &get();

    void addRegion(Addr start, Addr end);

    void removeRegion(Addr start, Addr end);

    BackingResult isBacked(Addr addr) const;

  private:
    struct Region {
        Addr start;
        Addr end;
    };

    //std::vector<Region> regions;
    std::map<Addr, Addr> regions;
};

} // namespace gem5

#endif
