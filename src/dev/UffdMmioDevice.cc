#include "dev/UffdMmioDevice.hh"
#include "debug/UffdMMIO.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "mem/uffd_region_tracker.hh"


namespace gem5
{

UffdMmioDevice::UffdMmioDevice(const UffdMmioDeviceParams &p)
    : BasicPioDevice(dynamic_cast<const BasicPioDeviceParams &>(p), p.pio_size)
{}

Tick
UffdMmioDevice::write(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;
    uint32_t value32;

    // iowrite32 was used
    if (pkt->getSize() == 4) {
        value32 = pkt->getLE<uint32_t>();
        DPRINTF(UffdMMIO, "Write32 to offset 0x%x: 0x%x\n", offset, value32);

        switch (offset) {
            case 0x0:
                startLow = value32;
                gotStartLow = true;
                break;
            case 0x4:
                startHigh = static_cast<uint64_t>(value32) << 32;
                gotStartHigh = true;
                break;
            case 0x8:
                endLow = value32;
                gotEndLow = true;
                break;
            case 0xc:
                endHigh = static_cast<uint64_t>(value32) << 32;
                gotEndHigh = true;
                break;
            default:
                break;
        }

        if (gotStartLow && gotStartHigh && gotEndLow && gotEndHigh) {
            uint64_t startAddr = startLow | startHigh;
            uint64_t endAddr = endLow | endHigh;
            DPRINTF(UffdMMIO, "Complete region: 0x%lx - 0x%lx\n", startAddr, endAddr);
            UffdRegionTracker::get().addRegion(startAddr, endAddr);

            gotStartLow = gotStartHigh = gotEndLow = gotEndHigh = false;
        }
    // iowrite 64 was used
    } else if (pkt->getSize() == 8) {
        uint64_t value64 = pkt->getLE<uint64_t>();
        DPRINTF(UffdMMIO, "Write64 to offset 0x%x: 0x%lx\n", offset, value64);
        if (offset == 0x0) {
            startLow = value64 & 0xffffffff;
            startHigh = value64 & 0xffffffff00000000;
            gotStartLow = gotStartHigh = true;
        } else if (offset == 0x8) {
            endLow = value64 & 0xffffffff;
            endHigh = value64 & 0xffffffff00000000;
            gotEndLow = gotEndHigh = true;
        }
    }

    pkt->makeResponse();
    return pioDelay;
}

Tick
UffdMmioDevice::read(PacketPtr pkt)
{
    // not really expected to read from the device here -> return 0
    DPRINTF(UffdMMIO, "Read from offset 0x%x\n", pkt->getAddr() - pioAddr);
    pkt->setLE<uint64_t>(0);
    pkt->makeResponse();
    return pioDelay;
}

} // namespace gem5
