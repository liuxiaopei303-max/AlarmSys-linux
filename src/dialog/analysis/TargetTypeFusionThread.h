#pragma once

#include <QHash>
#include <QThread>

class TargetTypeFusionThread : public QThread
{
    Q_OBJECT
public:
    explicit TargetTypeFusionThread(QObject* parent = nullptr);
    void stop();

protected:
    void run() override;

private:
    void processOnce();
    QSet<qint64> collectActiveUniqueIds() const;

    bool m_running = false;
    int m_cycleCount = 0;
    QHash<qint64, QString> m_lastGrpcPushedType;
};
