#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QtGlobal>

// Time is elapsed monotonic milliseconds; sampleSeconds identifies a NEW track update,
// rather than the same cached track revisited by a fast alarm polling loop.
class AngleDurationGate
{
public:
    bool passed(const QString& key, int seconds, bool anglesPassed,
                qint64 sampleSeconds, qint64 nowMs)
    {
        if (seconds <= 0) { m_states.remove(key); return anglesPassed; }
        if (!anglesPassed || sampleSeconds <= 0) { m_states.remove(key); return false; }
        auto it = m_states.find(key);
        if (it == m_states.end() || nowMs < it->lastSeenMs
            || nowMs - it->lastSeenMs > 10000
            || sampleSeconds < it->sampleSeconds
            || sampleSeconds - it->sampleSeconds > 10) {
            m_states.insert(key, {nowMs, nowMs, sampleSeconds, false});
            return false;
        }
        if (sampleSeconds == it->sampleSeconds) return it->completed;
        it->lastSeenMs = nowMs;
        it->sampleSeconds = sampleSeconds;
        it->completed = nowMs - it->startedMs >= static_cast<qint64>(seconds) * 1000;
        return it->completed;
    }

    void clear() { m_states.clear(); }
    void forget(const QString& key) { m_states.remove(key); }
    void retainOnly(const QSet<QString>& keys)
    {
        for (auto it = m_states.begin(); it != m_states.end();) {
            if (!keys.contains(it.key())) it = m_states.erase(it);
            else ++it;
        }
    }
    void prune(qint64 nowMs)
    {
        for (auto it = m_states.begin(); it != m_states.end();) {
            if (nowMs < it->lastSeenMs || nowMs - it->lastSeenMs > 10000)
                it = m_states.erase(it);
            else ++it;
        }
    }

private:
    struct State { qint64 startedMs; qint64 lastSeenMs; qint64 sampleSeconds; bool completed; };
    QHash<QString, State> m_states;
};
