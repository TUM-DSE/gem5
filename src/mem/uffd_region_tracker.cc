#include "mem/uffd_region_tracker.hh"
#include <algorithm>
#include "debug/Faults.hh"
#include "base/debug.hh"
#include "arch/x86/faults.hh"

namespace gem5
{

namespace UffdStats {
    thread_local size_t lastCheckCount = 0;
}

UffdRegionTracker &
UffdRegionTracker::get()
{
    static UffdRegionTracker instance;
    return instance;
}

void
UffdRegionTracker::addRegion(Addr start, Addr end)
{
    if (start >= end)
        return;

    auto it = regions.lower_bound(start);

    if (it != regions.begin()) {
        --it;
    }

    while (it != regions.end() && it->first <= end) {
        if (it->second < start) {
            ++it;
            continue;
        }
        start = std::min(start, it->first);
        end   = std::max(end, it->second);
        it = regions.erase(it);
    }

    regions[start] = end;
}

/*void
UffdRegionTracker::addRegion(Addr start, Addr end)
{
    DPRINTF(Faults, "[UffdRegionTracker] Registering region from 0x%" PRIx64 " to 0x%" PRIx64 "\n", start, end);
    if (start >= end)
        return;

    Addr new_start = start;
    Addr new_end = end;

    size_t i = 0;
    for (; i < regions.size(); ++i) {
        if (regions[i].end >= start) {
            break;
        }
    }

    while (i < regions.size() && regions[i].start <= new_end) {
        if (regions[i].start < new_start)
            new_start = regions[i].start;
        if (regions[i].end > new_end)
            new_end = regions[i].end;

        regions.erase(regions.begin() + i);
    }

    regions.insert(regions.begin() + i, Region{new_start, new_end});
}*/

void
UffdRegionTracker::removeRegion(Addr start, Addr end)
{
    DPRINTF(Faults, "[UffdRegionTracker] Unregistering region from 0x%" PRIx64 " to 0x%" PRIx64 "\n", start, end);
    if (start >= end)
        return;

    auto it = regions.lower_bound(start);

    if (it != regions.begin())
        --it;

    while (it != regions.end() && it->first < end) {
        Addr region_start = it->first;
        Addr region_end   = it->second;

        if (region_end <= start) {
            ++it;
            continue;
        }

        if (region_start >= end) {
            break;
        }

        if (start <= region_start && end >= region_end) {
            it = regions.erase(it);
            continue;
        }

        if (region_start < start && region_end > end) {
            it->second = start;
            regions[end] = region_end;
            break;
        }

        if (region_start < start && region_end > start) {
            it->second = start;
            ++it;
            continue;
        }

        if (region_start < end && region_end > end) {
            Addr new_start = end;
            Addr old_end = it->second;
            regions.erase(it);
            regions[new_start] = old_end;
            break;
        }

        it = regions.erase(it);
    }
}

/*void
UffdRegionTracker::removeRegion(Addr start, Addr end)
{
    DPRINTF(Faults, "[UffdRegionTracker] Unregistering region from 0x%" PRIx64 " to 0x%" PRIx64 "\n", start, end);
    if (start >= end)
        return;

    size_t i = 0;
    while (i < regions.size()) {
        if (regions[i].end <= start) {
            ++i;
            continue;
        }

        if (regions[i].start >= end) {
            break;
        }

        // section in region -> region needs to be split into two
        if (regions[i].start < start && regions[i].end > end) {
            Addr old_end = regions[i].end;
            regions[i].end = start;
            Region new_region;
            new_region.start = end;
            new_region.end = old_end;
            regions.insert(regions.begin() + i + 1, new_region);
            break;
        }

        // overlap at start
        if (regions[i].start < start && regions[i].end > start) {
            regions[i].end = start;
            ++i;
            continue;
        }

        // overlap at end
        if (regions[i].start < end && regions[i].end > end) {
            regions[i].start = end;
            break;
        }

        regions.erase(regions.begin() + i);
    }
}*/

UffdRegionTracker::BackingResult
UffdRegionTracker::isBacked(Addr addr) const
{
    size_t checked = 0;

    auto it = regions.upper_bound(addr);
    checked++;

    if (it != regions.begin()) {
        --it;
        checked++;
        if (it->first <= addr && addr < it->second) {
            return {true, checked};
        }
    }

    return {false, checked};
}

/*UffdRegionTracker::BackingResult
UffdRegionTracker::isBacked(Addr addr) const
{
    size_t checked = 0;
    for (size_t i = 0; i < regions.size(); ++i) {
        checked++;
        if (regions[i].start <= addr && addr < regions[i].end) {
            return {true, checked};
        }
    }
    return {false, checked};
}*/

} // namespace gem5
