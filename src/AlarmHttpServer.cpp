#include "AlarmHttpServer.h"

#include "customconfig.h"

#include <QHostAddress>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

AlarmHttpServer::AlarmHttpServer(CustomConfig* cfg, QObject* parent)
    : QObject(parent)
    , m_cfg(cfg)
{
}

AlarmHttpServer::~AlarmHttpServer()
{
    delete m_server;
}

bool AlarmHttpServer::start(quint16 port)
{
    delete m_server;
    m_server = new QTcpServer(this);
    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "Alarm HTTP 启动失败:" << m_server->errorString();
        return false;
    }
    qInfo() << "Alarm HTTP 监听端口" << m_server->serverPort();

    connect(m_server, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket* clientSocket = m_server->nextPendingConnection();

        auto sendHttpResponse = [](QTcpSocket* socket, int statusCode, const QString& statusText, const QByteArray& body) {
            QByteArray response;
            response.append(QStringLiteral("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
            response.append("Content-Type: application/json; charset=utf-8\r\n");
            response.append("Access-Control-Allow-Origin: *\r\n");
            response.append("Access-Control-Allow-Methods: GET, PUT, POST, OPTIONS\r\n");
            response.append("Access-Control-Allow-Headers: Content-Type, Authorization\r\n");
            response.append("Access-Control-Max-Age: 86400\r\n");
            response.append(QStringLiteral("Content-Length: %1\r\n").arg(body.size()).toUtf8());
            response.append("Connection: close\r\n");
            response.append("\r\n");
            response.append(body);
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        };

        auto updateRule = [this](const QString& scope) {
            if (!m_cfg) {
                return;
            }
            m_cfg->reloadAlarmConfigFromDb(scope);
        };

        auto parseReloadScope = [](const QByteArray& requestData, const QString& method) -> QString {
            if (method != QLatin1String("POST") && method != QLatin1String("PUT")) {
                return QStringLiteral("all");
            }
            QByteArray bodyBytes;
            const int bodyStart = requestData.indexOf("\r\n\r\n");
            if (bodyStart >= 0) {
                bodyBytes = requestData.mid(bodyStart + 4).trimmed();
            }
            if (bodyBytes.isEmpty()) {
                return QStringLiteral("all");
            }
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(bodyBytes, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                return QStringLiteral("all");
            }
            const QJsonObject root = doc.object();
            const QJsonValue scopeVal = root.value(QStringLiteral("scope"));
            if (scopeVal.isString() && !scopeVal.toString().trimmed().isEmpty()) {
                return scopeVal.toString().trimmed();
            }
            return QStringLiteral("all");
        };

        connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket, updateRule, parseReloadScope, sendHttpResponse]() {
            const QByteArray requestData = clientSocket->readAll();
            const QString requestString = QString::fromUtf8(requestData);
            const QStringList requestLines = requestString.split("\r\n");
            if (requestLines.isEmpty()) {
                return;
            }
            const QStringList requestLineParts = requestLines[0].split(' ');
            if (requestLineParts.size() < 3) {
                return;
            }
            const QString method = requestLineParts[0];
            const QString path = requestLineParts[1];
            const QString pathOnly = path.contains('?') ? path.section('?', 0, 0) : path;

            if (method == QLatin1String("OPTIONS")) {
                sendHttpResponse(clientSocket, 200, QStringLiteral("OK"), "{}");
                return;
            }

            if (pathOnly == QLatin1String("/api/alarm_filter") || pathOnly == QLatin1String("api/alarm_filter")) {
                if (method != QLatin1String("POST")) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("Method Not Allowed, use POST");
                    sendHttpResponse(clientSocket, 405, QStringLiteral("Method Not Allowed"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }

                QByteArray bodyBytes;
                const int bodyStart = requestData.indexOf("\r\n\r\n");
                if (bodyStart >= 0) {
                    bodyBytes = requestData.mid(bodyStart + 4);
                }
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(bodyBytes, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("invalid JSON body");
                    sendHttpResponse(clientSocket, 400, QStringLiteral("Bad Request"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }

                const QJsonObject root = doc.object();
                const QJsonObject spec = root.value(QStringLiteral("specification")).toObject();
                const QString specAtType = spec.value(QStringLiteral("@type")).toString();
                if (specAtType != QLatin1String("type.casia.tasks.v1.filterTargetOrForce")) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("invalid specification.@type");
                    sendHttpResponse(clientSocket, 400, QStringLiteral("Bad Request"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }

                bool okType = false;
                bool okTarget = false;
                int type = -1;
                qint64 targetId = -1;
                const QJsonValue typeVal = spec.value(QStringLiteral("type"));
                const QJsonValue idVal = spec.value(QStringLiteral("id"));
                if (typeVal.isDouble()) {
                    type = typeVal.toInt();
                    okType = true;
                } else if (typeVal.isString()) {
                    type = typeVal.toString().toInt(&okType);
                }
                if (idVal.isDouble()) {
                    targetId = static_cast<qint64>(idVal.toDouble());
                    okTarget = targetId > 0;
                } else if (idVal.isString()) {
                    targetId = idVal.toString().trimmed().toLongLong(&okTarget);
                    okTarget = okTarget && targetId > 0;
                }

                if (okType && okTarget && type >= 0 && type <= 1 && targetId > 0) {
                    if (m_cfg) {
                        m_cfg->addAlarmFilter(type, targetId);
                    }
                    qInfo() << "alarm_filter POST type=" << type << "target_id=" << targetId;
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = 0;
                    resp[QStringLiteral("message")] = QStringLiteral("ok");
                    sendHttpResponse(clientSocket, 200, QStringLiteral("OK"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                } else {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("invalid or missing specification.type(0/1) or specification.id");
                    sendHttpResponse(clientSocket, 400, QStringLiteral("Bad Request"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                }
                return;
            }

            if (pathOnly == QLatin1String("/api/alarms/update") || pathOnly == QLatin1String("api/alarms/update")) {
                const QString scope = parseReloadScope(requestData, method);
                updateRule(scope);
                QJsonObject resp;
                resp[QStringLiteral("code")] = 0;
                resp[QStringLiteral("scope")] = scope;
                resp[QStringLiteral("message")] = QStringLiteral("ok");
                sendHttpResponse(clientSocket, 200, QStringLiteral("OK"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                return;
            }

            if (pathOnly == QLatin1String("/api/alarm_event/push") || pathOnly == QLatin1String("api/alarm_event/push")) {
                if (method == QLatin1String("GET")) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = 0;
                    resp[QStringLiteral("full")] = m_cfg && m_cfg->alarmEventPushFullEnabled() ? 1 : 0;
                    resp[QStringLiteral("message")] = m_cfg && m_cfg->alarmEventPushFullEnabled()
                        ? QStringLiteral("full SendAllAlarmEventMsg")
                        : QStringLiteral("dds_light_only");
                    sendHttpResponse(clientSocket, 200, QStringLiteral("OK"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }
                if (method == QLatin1String("PUT")) {
                    const QUrl url(QStringLiteral("http://dummy") + path);
                    const QUrlQuery query(url);
                    const QString fullStr = query.queryItemValue(QStringLiteral("full"));
                    if (fullStr != QLatin1String("0") && fullStr != QLatin1String("1")) {
                        QJsonObject resp;
                        resp[QStringLiteral("code")] = -1;
                        resp[QStringLiteral("message")] = QStringLiteral("missing or invalid full (use 0 or 1)");
                        sendHttpResponse(clientSocket, 400, QStringLiteral("Bad Request"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    } else if (m_cfg) {
                        m_cfg->setAlarmEventPushFullEnabled(fullStr == QLatin1String("1"));
                        QJsonObject resp;
                        resp[QStringLiteral("code")] = 0;
                        resp[QStringLiteral("full")] = (fullStr == QLatin1String("1")) ? 1 : 0;
                        resp[QStringLiteral("message")] = QStringLiteral("ok");
                        sendHttpResponse(clientSocket, 200, QStringLiteral("OK"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    }
                    return;
                }
                QJsonObject resp;
                resp[QStringLiteral("code")] = -1;
                resp[QStringLiteral("message")] = QStringLiteral("Method Not Allowed, use GET or PUT");
                sendHttpResponse(clientSocket, 405, QStringLiteral("Method Not Allowed"), QJsonDocument(resp).toJson(QJsonDocument::Compact));
                return;
            }

            if (pathOnly == QLatin1String("/api/alarm_confirm") || pathOnly == QLatin1String("api/alarm_confirm")) {
                if (method != QLatin1String("POST")) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("Method Not Allowed, use POST");
                    sendHttpResponse(clientSocket, 405, QStringLiteral("Method Not Allowed"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }

                QByteArray bodyBytes;
                const int bodyStart = requestData.indexOf("\r\n\r\n");
                if (bodyStart >= 0) {
                    bodyBytes = requestData.mid(bodyStart + 4);
                }
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(bodyBytes, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("invalid JSON body");
                    sendHttpResponse(clientSocket, 400, QStringLiteral("Bad Request"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }

                const QJsonObject root = doc.object();
                QJsonValue uniqueVal = root.value(QStringLiteral("uniqueId"));
                if (uniqueVal.isUndefined())
                    uniqueVal = root.value(QStringLiteral("unique_id"));
                if (uniqueVal.isUndefined())
                    uniqueVal = root.value(QStringLiteral("uniqueID"));

                bool okUnique = false;
                qint64 uniqueId = 0;
                if (uniqueVal.isDouble()) {
                    uniqueId = static_cast<qint64>(uniqueVal.toDouble());
                    okUnique = uniqueId > 0;
                } else if (uniqueVal.isString()) {
                    uniqueId = uniqueVal.toString().trimmed().toLongLong(&okUnique);
                    okUnique = okUnique && uniqueId > 0;
                }

                if (!okUnique) {
                    QJsonObject resp;
                    resp[QStringLiteral("code")] = -1;
                    resp[QStringLiteral("message")] = QStringLiteral("missing or invalid uniqueId");
                    sendHttpResponse(clientSocket, 400, QStringLiteral("Bad Request"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                    return;
                }

                QString msg;
                const bool ok = m_cfg && m_cfg->confirmAlarmByUniqueId(uniqueId, &msg);
                QJsonObject resp;
                resp[QStringLiteral("code")] = ok ? 0 : -1;
                resp[QStringLiteral("message")] = msg;
                if (ok) {
                    qInfo() << "alarm_confirm POST uniqueId=" << uniqueId << msg;
                    sendHttpResponse(clientSocket, 200, QStringLiteral("OK"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                } else {
                    const int httpCode = msg.contains(QStringLiteral("track not found"))
                        ? 404
                        : 400;
                    sendHttpResponse(clientSocket, httpCode,
                        httpCode == 404 ? QStringLiteral("Not Found") : QStringLiteral("Bad Request"),
                        QJsonDocument(resp).toJson(QJsonDocument::Compact));
                }
                return;
            }

            sendHttpResponse(clientSocket, 404, QStringLiteral("Not Found"), "{}");
        });

        connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
    });

    return true;
}
