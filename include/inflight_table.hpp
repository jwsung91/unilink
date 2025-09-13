#pragma once
#include <unordered_map>
#include <future>
#include <atomic>
#include <cstdint>
#include <functional>
#include "common.hpp"

class InflightTable {
public:
    struct Pending {
        std::promise<Msg> prom;
        Clock::time_point deadline;
    };

    uint32_t next_seq() { return ++seq_; }

    void emplace(uint32_t s, Pending p) { table_.emplace(s, std::move(p)); }

    bool fulfill(uint32_t s, Msg m) {
        auto it = table_.find(s);
        if (it == table_.end()) return false;
        it->second.prom.set_value(std::move(m));
        table_.erase(it);
        return true;
    }

    template <class OnTimeout>
    void sweep(OnTimeout on_timeout) {
        auto now = Clock::now();
        for (auto it = table_.begin(); it != table_.end(); ) {
            if (now >= it->second.deadline) {
                try {
                    it->second.prom.set_exception(std::make_exception_ptr(std::runtime_error("request timeout")));
                } catch (...) {}
                uint32_t s = it->first;
                it = table_.erase(it);
                on_timeout(s);
            } else ++it;
        }
    }

    void clear_exception(const std::exception_ptr& eptr) {
        for (auto& kv : table_) {
            try { kv.second.prom.set_exception(eptr); } catch (...) {}
        }
        table_.clear();
    }

    bool empty() const { return table_.empty(); }

private:
    std::unordered_map<uint32_t, Pending> table_;
    std::atomic_uint32_t seq_{0};
};
