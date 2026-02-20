

#pragma once
#include <csics/geo/Concepts.hpp>

namespace csics::geo {
    class ToGeodetic {
        public:
            template <typename E, typename C>
            requires Ellipsoid<E> && GeocentricCoordinate<C, E>
            constexpr GeodeticCoordinate<C::value_type, E> auto operator()(const C& geodetic) const {
               return apply<E, C, GeodeticCoordinate<typename C::value_type, E>>(geodetic); 
            }
            template <typename E, typename C, typename Ret>
            requires Ellipsoid<E> && GeocentricCoordinate<C, E> && GeodeticCoordinate<Ret, E>
            static Ret apply(const C& geocentric) {
                double latitude, longitude, altitude;
                geocentric_to_geodetic(geocentric.x(), geocentric.y(), geocentric.z(), E::a, E::f, latitude, longitude, altitude);
                return Ret(latitude, longitude, altitude);
            }
        private:
            static void geocentric_to_geodetic(const double x, const double y, const double z, const double a, const double f,
                double& latitude, double& longitude, double& altitude);

    };

    constexpr ToGeodetic to_geodetic{};

    class ToGeocentric {
        public:
            template <typename E, typename C>
            requires Ellipsoid<E> && GeodeticCoordinate<C, E>
            constexpr GeocentricCoordinate<C::value_type, E> auto operator()(const C& geodetic) const {
               return apply<E, C, GeocentricCoordinate<typename C::value_type, E>>(geodetic); 
            }
            template <typename E, typename C, typename Ret>
            requires Ellipsoid<E> && GeodeticCoordinate<C, E> && GeocentricCoordinate<Ret, E>
            static Ret apply(const C& geodetic) {
                double x, y, z;
                geodetic_to_geocentric(geodetic.latitude(), geodetic.longitude(), geodetic.altitude(), E::a, E::f, x, y, z);
                return Ret(x, y, z);
            }
        private:
            static void geodetic_to_geocentric(const double latitude, const double longitude, const double altitude,
                const double a, const double f, double& x, double& y, double& z);
    };

    constexpr ToGeocentric to_geocentric{};
};
