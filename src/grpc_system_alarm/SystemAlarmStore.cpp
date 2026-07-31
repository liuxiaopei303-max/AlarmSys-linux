#include "SystemAlarmStore.hpp"

namespace alarmsys {
namespace grpc_system_alarm {

namespace {

bool isValidKind(::alarmsys::grpc::system_alarm::AlarmKind kind)
{
    using K = ::alarmsys::grpc::system_alarm::AlarmKind;
    return kind == K::EQUIPMENT || kind == K::DATA_COMM || kind == K::TASK
        || kind == K::ENVIRONMENT || kind == K::COMPREHENSIVE || kind == K::SYSTEM_AUTH;
}

}  // namespace

SystemAlarmStore& SystemAlarmStore::instance()
{
    static SystemAlarmStore store;
    return store;
}

bool SystemAlarmStore::report(const ::alarmsys::grpc::system_alarm::SystemAlarmItem& item, std::string* err)
{
    if (item.alarm_id().empty()) {
        if (err) {
            *err = "invalid_param: alarm_id empty";
        }
        return false;
    }
    if (item.system_id().empty()) {
        if (err) {
            *err = "invalid_param: system_id empty";
        }
        return false;
    }
    if (item.description().empty()) {
        if (err) {
            *err = "invalid_param: description empty";
        }
        return false;
    }
    if (item.timestamp_ms() <= 0) {
        if (err) {
            *err = "invalid_param: timestamp_ms";
        }
        return false;
    }
    if (item.raised_time_ms() <= 0) {
        if (err) {
            *err = "invalid_param: raised_time_ms";
        }
        return false;
    }
    if (!isValidKind(item.alarm_kind())) {
        if (err) {
            *err = "invalid_param: alarm_kind";
        }
        return false;
    }

    ::alarmsys::grpc::system_alarm::SystemAlarmItem stored = item;
    stored.set_canceled_time_ms(0);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_active[stored.alarm_id()] = stored;
    if (err) {
        *err = "ok";
    }
    return true;
}

bool SystemAlarmStore::cancel(const std::string& alarm_id,
                              const std::string& system_id,
                              int64_t canceled_time_ms,
                              std::string* err)
{
    if (alarm_id.empty()) {
        if (err) {
            *err = "invalid_param: alarm_id empty";
        }
        return false;
    }
    if (system_id.empty()) {
        if (err) {
            *err = "invalid_param: system_id empty";
        }
        return false;
    }
    if (canceled_time_ms <= 0) {
        if (err) {
            *err = "invalid_param: canceled_time_ms";
        }
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_active.find(alarm_id);
    if (it == m_active.end()) {
        if (err) {
            *err = "not_found";
        }
        return false;
    }

    // system_id 作审计，不强制必须与上报方一致
    (void)system_id;
    m_active.erase(it);
    if (err) {
        *err = "ok";
    }
    return true;
}

std::vector<::alarmsys::grpc::system_alarm::SystemAlarmItem> SystemAlarmStore::listActive(
    const std::string& system_id_filter,
    ::alarmsys::grpc::system_alarm::AlarmKind kind_filter,
    bool level_filter_set,
    ::alarmsys::grpc::system_alarm::AlarmLevel level_filter) const
{
    std::vector<::alarmsys::grpc::system_alarm::SystemAlarmItem> out;
    std::lock_guard<std::mutex> lock(m_mutex);
    out.reserve(m_active.size());
    for (const auto& kv : m_active) {
        const auto& a = kv.second;
        if (!system_id_filter.empty() && a.system_id() != system_id_filter) {
            continue;
        }
        if (kind_filter != ::alarmsys::grpc::system_alarm::ALARM_KIND_UNSPECIFIED
            && a.alarm_kind() != kind_filter) {
            continue;
        }
        if (level_filter_set && a.level() != level_filter) {
            continue;
        }
        out.push_back(a);
    }
    return out;
}

size_t SystemAlarmStore::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active.size();
}

}  // namespace grpc_system_alarm
}  // namespace alarmsys
