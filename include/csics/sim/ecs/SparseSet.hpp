#pragma once

#include "Entity.hpp"
#include "csics/Buffer.hpp"
namespace csics::sim::ecs {

template <typename T>
concept Component = true;  // maybe constraints later, but for now we can just
                           // allow any type as a component

template <Component C, std::size_t N = 1000>
class SparseSet {
   public:
    using value_type = C;
    SparseSet()
        : dense_(), sparse_(N, std::numeric_limits<std::size_t>::max()) {}

    SparseSet(const SparseSet&) = default;
    SparseSet(SparseSet&&) = default;
    SparseSet& operator=(const SparseSet&) = default;
    SparseSet& operator=(SparseSet&&) = default;

    operator SparseSet<const C>&() {
        return reinterpret_cast<SparseSet<const C>&>(*this);
    }

    template <typename... Args>
    void emplace(Entity entity, Args&&... args) {
        insert(entity, C(std::forward<Args>(args)...));
    }

    template <typename Cc = C>
    void insert(Entity entity, Cc&& component) {
        CSICS_RUNTIME_ASSERT(entity.id < N,
                             "insert: Entity ID exceeds maximum capacity "
                             "(entity.id: " +
                                 std::to_string(entity.id) + ")");
        if (sparse_[entity.id] == std::numeric_limits<std::size_t>::max()) {
            sparse_[entity.id] = dense_.size();
            dense_.push_back(component);
            entities_.push_back(entity);
        } else {
            dense_[sparse_[entity.id]] = std::forward<Cc>(component);
        }
    }

    const C* at(Entity entity) const {
        CSICS_RUNTIME_ASSERT(entity.id < N,
                             "at: Entity ID exceeds maximum capacity "
                             "(entity.id: " +
                                 std::to_string(entity.id) + ")");
        if (sparse_[entity.id] != std::numeric_limits<std::size_t>::max()) {
            if (entities_[sparse_[entity.id]].generation != entity.generation) {
                return nullptr;
            }
            return &dense_[sparse_[entity.id]];
        } else {
            return nullptr;
        }
    }

    C* at(Entity entity) {
        CSICS_RUNTIME_ASSERT(entity.id < N,
                             "at: Entity ID exceeds maximum capacity "
                             "(entity.id: " +
                                 std::to_string(entity.id) + ")");
        if (sparse_[entity.id] != std::numeric_limits<std::size_t>::max()) {
            if (entities_[sparse_[entity.id]].generation != entity.generation) {
                return nullptr;
            }
            return &dense_[sparse_[entity.id]];
        } else {
            return nullptr;
        }
    }

    bool contains(Entity entity) const {
        CSICS_RUNTIME_ASSERT(entity.id < N,
                             "contains: Entity ID exceeds maximum capacity "
                             "(entity.id: " +
                                 std::to_string(entity.id) + ")");
        return sparse_[entity.id] != std::numeric_limits<std::size_t>::max();
    }

    constexpr size_t size() const { return dense_.size(); }
    constexpr bool empty() const { return dense_.empty(); }

    void remove(Entity entity) {
        if (entity.id >= N ||
            sparse_[entity.id] == std::numeric_limits<std::size_t>::max() ||
            dense_.empty()) {
            return;
        }
        dense_[sparse_[entity.id]] =
            dense_[dense_.size() -
                   1];  // Move the last component to the removed spot
        auto last_entity =
            entities_[entities_.size() -
                      1];  // Move the last entity to the removed spot
        sparse_[last_entity.id] =
            sparse_[entity.id];  // Update the sparse index for the moved
                                 // entity
        entities_[sparse_[entity.id]] = last_entity;  // Update the entity list
        sparse_[entity.id] =
            std::numeric_limits<std::size_t>::max();  // Mark the removed entity
                                                      // as not present
        dense_.pop_back();
        entities_.pop_back();
    }

    Buffer<Entity>& entities() { return entities_; }
    const Buffer<Entity>& entities() const { return entities_; }

    BasicBufferView<C> components() { return dense_; }
    BasicBufferView<const C> components() const { return dense_; }

    auto begin() { return dense_.begin(); }
    auto end() { return dense_.end(); }
    auto begin() const { return dense_.begin(); }
    auto end() const { return dense_.end(); }

   private:
    Buffer<C> dense_;
    Buffer<size_t> sparse_;
    Buffer<Entity> entities_;
};

};  // namespace csics::sim::ecs
