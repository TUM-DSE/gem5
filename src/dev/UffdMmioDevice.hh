#ifndef __DEV_UFFD_MMIO_DEVICE_HH__
#define __DEV_UFFD_MMIO_DEVICE_HH__

#include "dev/io_device.hh"
#include "params/UffdMmioDevice.hh"
#include <cstdint>

namespace gem5
{

class UffdMmioDevice : public BasicPioDevice
{
  public:
    UffdMmioDevice(const UffdMmioDeviceParams &p);

    Tick write(PacketPtr pkt) override;
    Tick read(PacketPtr pkt) override;

  private:
    uint64_t startLow = 0;
    uint64_t startHigh = 0;
    uint64_t endLow = 0;
    uint64_t endHigh = 0;

    bool gotStartLow = false;
    bool gotStartHigh = false;
    bool gotEndLow = false;
    bool gotEndHigh = false;
};

} // namespace gem5

#endif // __DEV_UFFD_MMIO_DEVICE_HH__
