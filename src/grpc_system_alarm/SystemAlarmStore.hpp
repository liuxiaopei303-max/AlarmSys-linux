#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "system_alarm.pb.h"

namespace alarmsys {
namespace grpc_system_alarm {

/** 系统告警内存池：仅保存当前未取消告警，按 alarm_id 覆盖。 */
class SystemAlarmStore {
public:
    static SystemAlarmStore& instance();

    /** 上报/更新；同 alarm_id 覆盖。返回 false 表示参数非法。 */
    bool report(const ::alarmsys::grpc::system_alarm::SystemAlarmItem& item, std::string* err);

    /** 取消：从活跃池移除。返回 false 表示 not_found 或参数非法。 */
    bool cancel(const std::string& alarm_id,
                const std::string& system_id,
                int64_t canceled_time_ms,
                std::string* err);

    std::vector<::alarmsys::grpc::system_alarm::SystemAlarmItem> listActive(
        const std::string& system_id_filter,
        ::alarmsys::grpc::system_alarm::AlarmKind kind_filter,
        bool level_filter_set,
        ::alarmsys::grpc::system_alarm::AlarmLevel level_filter) const;

    size_t size() const;

private:
    SystemAlarmStore() = default;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ::alarmsys::grpc::system_alarm::SystemAlarmItem> m_active;
};

}  // namespace grpc_system_alarm
}  // namespace alarmsys
