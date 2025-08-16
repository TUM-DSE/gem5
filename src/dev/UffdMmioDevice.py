from m5.params import *
from m5.objects.Device import BasicPioDevice

class UffdMmioDevice(BasicPioDevice):
    type = 'UffdMmioDevice'
    cxx_class = 'gem5::UffdMmioDevice'
    cxx_header = "dev/UffdMmioDevice.hh"

    pio_size = Param.Addr(0x1000, "Size of PIO region")
