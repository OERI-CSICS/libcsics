
#include <chrono>
#include <random>
namespace csics {

template <typename T>
concept RandomEngine = requires(T t) {
    typename T::result_type;
    { t() } -> std::convertible_to<typename T::result_type>;
    { t.seed(0) } -> std::same_as<void>;
};

template <std::floating_point T>
struct floating_point_mantissa;

template <>
struct floating_point_mantissa<float> {
    static constexpr std::uint32_t bits = 23;
};

template <>
struct floating_point_mantissa<double> {
    static constexpr std::uint64_t bits = 52;
};

template <RandomEngine Engine = std::mt19937_64>
class Random {
   public:
    using result_type = typename Engine::result_type;

    Random() : engine_() { seed(); }

    explicit Random(Engine engine) : engine_(std::move(engine)) {
        engine_.seed(1);
    }

    void seed(result_type seed) { engine_.seed(seed); }
    void seed() {
        auto now = std::chrono::high_resolution_clock::now()
                       .time_since_epoch()
                       .count();
        engine_.seed((static_cast<result_type>(now)));
    }

    template <typename T>
        requires std::is_integral_v<T>
    T uniform() {
        result_type rand = engine_();
        return static_cast<T>(
            rand %
            (static_cast<result_type>(std::numeric_limits<T>::max()) - 1));
    }

    template <typename T>
        requires std::is_floating_point_v<T>
    T uniform() {
        constexpr result_type bit_shift =
            sizeof(result_type) * 8 - floating_point_mantissa<T>::bits;
        constexpr result_type scale =
            static_cast<T>(1.0) /
            (static_cast<result_type>(1) << floating_point_mantissa<T>::bits);
        result_type rand = engine_() >> bit_shift;
        return static_cast<T>(rand) * scale;
    }

   private:
    Engine engine_;
};

Random() -> Random<std::mt19937_64>;
};  // namespace csics
