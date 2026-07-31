#include "TargetTypeFusionLogic.h"

#include <QStringList>

namespace {

QString normalizeToken(QString value)
{
    return value.trimmed().toLower();
}

QString coarseFamily(const QString& type)
{
    const QString t = normalizeToken(type);
    if (t.isEmpty()) {
        return QString();
    }
    if (t == QLatin1String("buoy") || t.contains(QStringLiteral("浮标"))) {
        return QStringLiteral("buoy");
    }
    if (t == QLatin1String("uav") || t == QLatin1String("drone")
        || t.contains(QStringLiteral("无人机"))) {
        return QStringLiteral("uav");
    }
    if (t == QLatin1String("bird") || t.contains(QStringLiteral("鸟"))) {
        return QStringLiteral("bird");
    }
    static const QStringList shipSubtypes = {
        QStringLiteral("ship"),
        QStringLiteral("yacht"),
        QStringLiteral("cargo"),
        QStringLiteral("fishing"),
        QStringLiteral("speedboat"),
        QStringLiteral("cargoship"),
        QStringLiteral("fishingboat"),
        QStringLiteral("warship"),
        QStringLiteral("motorboat"),
    };
    for (const QString& shipType : shipSubtypes) {
        if (t == shipType || t.contains(QStringLiteral("船"))
            || t.contains(QStringLiteral("货轮"))
            || t.contains(QStringLiteral("游艇"))
            || t.contains(QStringLiteral("渔船"))) {
            return QStringLiteral("ship");
        }
    }
    if (t == QLatin1String("other") || t.contains(QStringLiteral("其他"))
        || t.contains(QStringLiteral("未知"))) {
        return QStringLiteral("other");
    }
    return QStringLiteral("other");
}

bool isShipSubtype(const QString& type)
{
    const QString t = normalizeToken(type);
    if (t.isEmpty() || t == QLatin1String("ship")) {
        return false;
    }
    return coarseFamily(t) == QLatin1String("ship");
}

bool coarseEquals(const QString& a, const QString& b)
{
    const QString ca = coarseFamily(a);
    const QString cb = coarseFamily(b);
    return !ca.isEmpty() && ca == cb;
}

TargetTypeFusionResult makeResult(const QString& finalType, const QString& source)
{
    TargetTypeFusionResult result;
    result.shouldUpdate = !finalType.isEmpty();
    result.finalType = normalizeToken(finalType);
    result.finalSource = source;
    return result;
}

} // namespace

TargetTypeFusionResult fuseTargetTypes(const TargetTypeFusionInput& input)
{
    // 暂不考虑 cam：只看 llm / track；有 llm 一律以 llm 为准
    Q_UNUSED(input.camType);
    const QString track = normalizeToken(input.trackType);
    const QString llm = normalizeToken(input.llmType);

    if (!llm.isEmpty()) {
        return makeResult(llm, QStringLiteral("llm"));
    }
    if (!track.isEmpty()) {
        return makeResult(track, QStringLiteral("track"));
    }
    return TargetTypeFusionResult();
}

bool shouldPushTargetTypeGrpc(const QString& trackTargetType, const QString& judgedFinalType)
{
    const QString judged = normalizeToken(judgedFinalType);
    if (judged.isEmpty()) {
        return false;
    }

    const QString track = normalizeToken(trackTargetType);
    if (track.isEmpty()) {
        return true;
    }
    if (track == judged) {
        return false;
    }
    if (coarseEquals(track, judged)) {
        if (isShipSubtype(track) || isShipSubtype(judged)) {
            return track != judged;
        }
        return false;
    }
    return true;
}
