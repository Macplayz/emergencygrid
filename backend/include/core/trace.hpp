#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "json.hpp"

namespace emergencygrid {

struct TraceEvent {
    uint32_t step{0};
    std::string algorithm{"Dijkstra"};
    std::string operation;
    std::string entityId;
    std::string from;
    std::string to;
    std::optional<double> previousValue;
    std::optional<double> newValue;
    std::optional<double> cost;
    nlohmann::json metadata = nlohmann::json::object();
    std::string explanation;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["step"] = step;
        j["algorithm"] = algorithm;
        j["operation"] = operation;
        if (!entityId.empty()) j["entityId"] = entityId;
        if (!from.empty()) j["from"] = from;
        if (!to.empty()) j["to"] = to;
        if (previousValue.has_value()) j["previousValue"] = *previousValue;
        if (newValue.has_value()) j["newValue"] = *newValue;
        if (cost.has_value()) j["cost"] = *cost;
        if (!metadata.empty()) j["metadata"] = metadata;
        j["explanation"] = explanation;
        return j;
    }
};

class TraceCollector {
public:
    explicit TraceCollector(bool enabled = false) : enabled_(enabled) {}

    bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    void addEvent(TraceEvent event) {
        if (!enabled_) return;
        event.step = static_cast<uint32_t>(events_.size() + 1);
        events_.push_back(std::move(event));
    }

    const std::vector<TraceEvent>& getEvents() const noexcept { return events_; }

    nlohmann::json toJsonArray() const {
        nlohmann::json arr = nlohmann::json::array();
        if (!enabled_) return arr;
        arr.get_ptr<nlohmann::json::array_t*>()->reserve(events_.size());
        for (const auto& ev : events_) {
            arr.push_back(ev.toJson());
        }
        return arr;
    }

    void clear() noexcept {
        events_.clear();
    }

    size_t size() const noexcept {
        return events_.size();
    }

private:
    bool enabled_{false};
    std::vector<TraceEvent> events_;
};

} // namespace emergencygrid
