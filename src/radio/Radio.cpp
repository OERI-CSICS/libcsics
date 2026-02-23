#include <csics/radio/Common.hpp>

namespace csics::radio {

RadioDeviceArgs::RadioDeviceArgs()
    : device_type(DeviceType::DEFAULT), args(std::monostate{}) {}

#ifdef CSICS_USE_UHD
UsrpArgs::operator RadioDeviceArgs() const {
    RadioDeviceArgs args;
    args.device_type = DeviceType::USRP;
    args.args = *this;
    return args;
}
#endif
}  // namespace csics::radio
