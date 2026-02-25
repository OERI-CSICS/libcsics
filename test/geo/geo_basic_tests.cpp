#include <gtest/gtest.h>

#include <csics/csics.hpp>

TEST(CSICSGeoTests, GeodeticToGeocentric) {
    csics::geo::LatLongAlt<double> geodetic(45.0, 45.0, 1000.0);
    csics::geo::Geocentric<double> geocentric(geodetic);

    EXPECT_NEAR(geocentric.x(), 3194919.145, 1e-3);
    EXPECT_NEAR(geocentric.y(), 3194919.145, 1e-3);
    EXPECT_NEAR(geocentric.z(), 4488055.516, 1e-3);
}

TEST(CSICSGeoTests, GeocentricToGeodetic) {
    csics::geo::Geocentric<double> geocentric(3194919.145, 3194919.145, 4488055.516);
    csics::geo::LatLongAlt<double> geodetic(geocentric);

    EXPECT_NEAR(geodetic.latitude(), 45.0, 1e-6);
    EXPECT_NEAR(geodetic.longitude(), 45.0, 1e-6);
    EXPECT_NEAR(geodetic.altitude(), 1000.0, 1e-3);
}


