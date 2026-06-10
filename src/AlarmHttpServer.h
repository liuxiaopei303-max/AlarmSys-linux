#pragma once

#include <QObject>

class QTcpServer;
class CustomConfig;

/** 无 UI 的告警 HTTP 接口，行为对齐原 AlarmSettingWidget::initHttpServer */
class AlarmHttpServer : public QObject {
    Q_OBJECT
public:
    explicit AlarmHttpServer(CustomConfig* cfg, QObject* parent = nullptr);
    ~AlarmHttpServer() override;

    bool start(quint16 port);

private:
    CustomConfig* m_cfg = nullptr;
    QTcpServer* m_server = nullptr;
};
