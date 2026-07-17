// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/core/event.hpp
//
// A minimal, header-only observer/signal utility used to notify library
// consumers of asynchronous occurrences (hook detected, module loaded, ...)
// without coupling emitters to concrete listener types.
//
// Deliberately simpler than a full signal/slot library: RMG only needs
// synchronous, same-thread-or-documented-thread delivery with basic
// connect/disconnect semantics.
// ==============================================================================

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

namespace rmg::core {

/// @brief Opaque handle returned by Signal::connect(), used to later
///        disconnect a specific listener.
class Connection {
public:
    Connection() = default;
    explicit Connection(std::size_t id) : id_(id), valid_(true) {}

    [[nodiscard]] bool isValid() const noexcept { return valid_; }
    [[nodiscard]] std::size_t id() const noexcept { return id_; }

private:
    std::size_t id_ = 0;
    bool valid_ = false;
};

/// @brief A thread-safe, multi-listener signal.
///
/// @tparam Args The argument types passed to listeners when the signal is
///              emitted. Listeners are stored as `std::function<void(Args...)>`.
///
/// Emission order matches connection order. Emission takes a copy of the
/// listener list under the lock, then invokes callbacks without holding the
/// lock, so a listener is free to call connect()/disconnect() on the same
/// signal without deadlocking (at the cost of that listener not observing
/// changes made during the in-flight emit).
template <typename... Args> class Signal {
public:
    using Slot = std::function<void(Args...)>;

    /// @brief Registers @p slot to be invoked on every future emit() call.
    /// @return A Connection handle that can be passed to disconnect().
    Connection connect(Slot slot) {
        std::lock_guard lock(mutex_);
        const std::size_t id = nextId_++;
        listeners_.push_back(Listener{id, std::move(slot)});
        return Connection(id);
    }

    /// @brief Removes a previously connected listener. No-op if @p connection
    ///        is invalid or already disconnected.
    void disconnect(const Connection& connection) {
        if (!connection.isValid()) {
            return;
        }
        std::lock_guard lock(mutex_);
        std::erase_if(listeners_,
                      [&](const Listener& listener) { return listener.id == connection.id(); });
    }

    /// @brief Removes every connected listener.
    void disconnectAll() {
        std::lock_guard lock(mutex_);
        listeners_.clear();
    }

    /// @brief Invokes every currently connected listener with @p args, in
    ///        connection order.
    void emit(Args... args) const {
        std::vector<Slot> snapshot;
        {
            std::lock_guard lock(mutex_);
            snapshot.reserve(listeners_.size());
            for (const Listener& listener : listeners_) {
                snapshot.push_back(listener.slot);
            }
        }
        for (const Slot& slot : snapshot) {
            slot(args...);
        }
    }

    /// @brief Returns the number of currently connected listeners.
    [[nodiscard]] std::size_t listenerCount() const noexcept {
        std::lock_guard lock(mutex_);
        return listeners_.size();
    }

private:
    struct Listener {
        std::size_t id;
        Slot slot;
    };

    mutable std::mutex mutex_;
    std::vector<Listener> listeners_;
    std::size_t nextId_ = 1;
};

} // namespace rmg::core