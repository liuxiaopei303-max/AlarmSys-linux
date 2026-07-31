#pragma once

#include <QString>

struct TargetTypeFusionInput {
    QString camType;
    QString trackType;
    QString llmType;
};

struct TargetTypeFusionResult {
    bool shouldUpdate = false;
    QString finalType;
    /** track / llm（当前研判忽略 cam） */
    QString finalSource;
};

/** 暂不考虑 cam：有 llm 以 llm 为准，否则用 track */
TargetTypeFusionResult fuseTargetTypes(const TargetTypeFusionInput& input);

/** 研判结果与 track_target_type 不等价时，需 gRPC 通知外部融合 */
bool shouldPushTargetTypeGrpc(const QString& trackTargetType, const QString& judgedFinalType);
