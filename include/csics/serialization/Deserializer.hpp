
#pragma once

#include "csics/serialization/Concepts.hpp"

namespace csics::serialization {

struct De {
   public:
    template <Deserializer D, typename T>
    constexpr auto operator()(D& d, T& t) const {
        return apply(d, t);
    }

   private:
    template <Deserializer D, typename T>
        requires StructSerializable<std::remove_cvref_t<T>, D>
    static constexpr auto apply(D& d, T& t) {
        auto fields = get_fields<T>();
        return std::apply(
            [&](auto&&... field) {
                expected<void, typename D::error_type> res{};
                bool success = true;

                (
                    [&] {
                        if (!success) {
                            return;
                        }
                        res = apply(d, t.*(field.ptr()));
                        if (!res.has_value()) {
                            success = false;
                            return;
                        }
                        t.*(field.ptr()) = std::move(res.value());
                    }(),
                    ...);
            },
            fields);
    };

    template <Deserializer D, typename T>
        requires PrimitiveDeserializable<std::remove_cvref_t<T>, D>
    static constexpr expected<T, typename D::error_type> apply(D& d, T& t) {
        auto res = d.template read<std::remove_cvref_t<T>>();
        if (!res.has_value()) {
            return res.error();
        }
        t = std::move(res.value());
        return res;
    };

    template <Deserializer D, typename T>
        requires MapSerializable<std::remove_cvref_t<T>, D>
    static constexpr auto apply(D& d, T& map) {
        static_assert(DeserializeMap<T, D>,
                      "Deserializer does not support maps");
        return d.read_map(map);
    };

    template <Deserializer D, typename T>
        requires IterableSerializable<std::remove_cvref_t<T>, D>
    static constexpr auto apply(D& d, T& iterable) {
        static_assert(DeserializeIterable<T, D>,
                      "Deserializer does not support iterables");
        return d.read_iterable(iterable);
    };

    template <Deserializer D, typename T>
        requires DirectDeserializable<std::remove_cvref_t<T>, D>
    static constexpr auto apply(D& d, T&) {
        std::cerr << "Applying direct deserialization for type "
                  << typeid(T).name() << std::endl;
        return deserialize_direct(d, detail::type_tag<T>{});
    }
};

constexpr De deserialize{};

};  // namespace csics::serialization
