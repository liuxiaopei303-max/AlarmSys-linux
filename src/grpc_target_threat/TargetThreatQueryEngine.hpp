#pragma once

#include "target_threat_query.pb.h"

#include <QString>
#include <QtGlobal>

class CustomConfig;

namespace alarmsys {
namespace grpc_target_threat {

/** 按 target_id 读取实时目标，区内算最高区域威胁、区外算默认类型/速度，并附带当前告警。 */
class TargetThreatQueryEngine
{
public:
    explicit TargetThreatQueryEngine(CustomConfig* config);

    void query(
        qint64 targetId,
        const QString& targetIdText,
        ::alarmsys::grpc::target_threat::v1::GetTargetThreatResponse* response) const;

private:
    CustomConfig* m_config = nullptr;
};

} // namespace grpc_target_threat
} // namespace alarmsys
