#pragma once
#include <csics/geo/Concepts.hpp>

#include <csics/geo/Ellipsoids.hpp>
#include <csics/geo/Ops.hpp>

namespace csics::geo {

template <typename T, Ellipsoid E = WGS84>
class Geodetic {
   public:
       using value_type = T;
       using ellipsoid_v = E;
    Geodetic(T latitude, T longitude, T altitude)
        : latitude_(latitude), longitude_(longitude), altitude_(altitude) {}

    constexpr const T latitude() const { return latitude_; }
    constexpr const T longitude() const { return longitude_; }
    constexpr const T altitude() const { return altitude_; }

    template <typename C, typename Conv = ToGeocentric>
        requires GeocentricCoordinate<C, E>
    Geodetic(const C& geocentric) {
        *this = Conv::template apply<E, C, Geodetic<T, E>>(geocentric);
    }
    

   private:
    T latitude_;
    T longitude_;
    T altitude_;
};

template <typename T, Ellipsoid E = WGS84>
using LatLongAlt = Geodetic<T, E>;


template <typename T, Ellipsoid E = WGS84>
class Geocentric {
   public:
       using value_type = T;
       using ellipsoid_v = E;
    Geocentric(T x, T y, T z) : x_(x), y_(y), z_(z) {}

    constexpr const T x() const { return x_; }
    constexpr const T y() const { return y_; }
    constexpr const T z() const { return z_; }

    template<typename C, typename Conv = ToGeodetic>
        requires GeodeticCoordinate<C, E>
    Geocentric(const C& geodetic) {
         *this = Conv::template apply<E, C, Geocentric<T, E>>(geodetic);
    }

   private:
    T x_;
    T y_;
    T z_;
};

template<typename T, Ellipsoid E = WGS84>
using UTM = Geocentric<T, E>;

static_assert(GeocentricCoordinate<Geocentric<double, WGS84>, WGS84>,
              "Geocentric does not satisfy GeocentricCoordinate concept");

static_assert(GeodeticCoordinate<LatLongAlt<double, WGS84>, WGS84>,
              "LatLongAlt does not satisfy GeodeticCoordinate concept");

};  // namespace csics::geo
