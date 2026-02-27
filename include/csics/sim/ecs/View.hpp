
#pragma once

#include <tuple>

#include "csics/sim/ecs/Entity.hpp"
#include "csics/sim/ecs/SparseSet.hpp"
namespace csics::sim::ecs {
template <typename... Cs>
    requires(sizeof...(Cs) > 0)
class View {
   public:
    View(SparseSet<Cs>&... sets) : sets_(sets...), const_sets_(sets...) {}

   private:
    std::tuple<SparseSet<Cs>&...> sets_;
    std::tuple<const SparseSet<Cs>&...> const_sets_;

   public:
    template <bool Const>
    class BaseIterator {
       public:
        template <typename T>
        using MaybeConst = std::conditional_t<Const, const T, T>;
        using difference_type = std::ptrdiff_t;
        using value_type = std::tuple<Entity, Cs...>;
        using reference = std::tuple<MaybeConst<Entity>&,
                                     MaybeConst<Cs>&...>;

        BaseIterator(MaybeConst<std::tuple<MaybeConst<SparseSet<Cs>>&...>>& sets,
                     const Entity* entity_iter, const Entity* const entity_end)
            : sets_(sets), entity_iter_(entity_iter), entity_end_(entity_end) {}

        void operator++() {
            static auto contains = []<typename... Ss>(
                                       const std::tuple<Ss&...>& sets,
                                       const Entity& e) {
                return (std::get<Ss>(sets).contains(e) && ...);
            };

            do {
                entity_iter_++;
            } while (entity_iter_ != entity_end_ &&
                     !contains(sets_, *entity_iter_));
        }

        auto operator++(int) {
            auto copy = *this;
            ++*this;
            return copy;
        }

        reference operator*() const {
            return std::forward_as_tuple(
                *entity_iter_,
                *std::get<MaybeConst<SparseSet<Cs>>>(sets_).at(*entity_iter_)...);
        }

        bool operator==(const BaseIterator& o) const {
            return entity_iter_ == o.entity_iter_;
        }

        bool operator!=(const BaseIterator& o) const { return !(*this == o); }

       private:
        MaybeConst<std::tuple<MaybeConst<SparseSet<Cs>>&...>>& sets_;
        const Entity* entity_iter_;
        const Entity* const entity_end_;
    };

    using Iterator = BaseIterator<false>;
    using ConstIterator = BaseIterator<true>;

    Iterator begin() {
        auto entities = find_smallest();
        return Iterator(sets_, entities->begin(), entities->end());
    }

    Iterator end() {
        auto entities = find_smallest();
        return Iterator(sets_, entities->end(), entities->end());
    }

    ConstIterator begin() const {
        auto entities = find_smallest();
        return ConstIterator(const_sets_, entities->begin(), entities->end());
    }

    ConstIterator end() const {
        auto entities = find_smallest();
        return ConstIterator(const_sets_, entities->end(), entities->end());
    }

    auto cbegin() const { return begin(); }
    auto cend() const { return end(); }

    const Buffer<Entity>* find_smallest() const {
        std::size_t smallest = std::numeric_limits<std::size_t>::max();
        const Buffer<Entity>* entities = std::get<0>(sets_).entities();
        auto cmp = [&](const auto& a) {
            entities = (a.size() < smallest)
                           ? (smallest = a.size(), &a.entities())
                           : entities;
        };

        std::apply([&](auto&... args) { (cmp(args), ...); }, sets_);
        return entities;
    }
};

};  // namespace csics::sim::ecs
