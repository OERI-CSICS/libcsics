#pragma once
namespace csics::geo {
    struct WGS84 {
        static constexpr double a_() {return 6378137.0f; }
        static constexpr double f_() { return 1.0 / 298.257223563; }
        double a = WGS84::a_();
        double f = WGS84::f_();

        WGS84() = default;
    };
};
