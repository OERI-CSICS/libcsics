#include <csics/geo/Ops.hpp>
#include <GeographicLib/Geocentric.hpp>

namespace csics::geo {
    void ToGeodetic::geocentric_to_geodetic(const double x, const double y, const double z, const double a, const double f,
        double& latitude, double& longitude, double& altitude) {
        GeographicLib::Geocentric geocentric(a, f);
        geocentric.Reverse(x, y, z, latitude, longitude, altitude);
    }

    void ToGeocentric::geodetic_to_geocentric(const double latitude, const double longitude, const double altitude,
        const double a, const double f, double& x, double& y, double& z) {
        GeographicLib::Geocentric geocentric(a, f);
        geocentric.Forward(latitude, longitude, altitude, x, y, z);
    }
};
