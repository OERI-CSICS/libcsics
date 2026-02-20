#pragma once
namespace csics::geo {
    struct WGS84 {
        static constexpr double a = 6378137.0f;
        static constexpr double f = 1.0f / 298.257223563f;
    };
};
