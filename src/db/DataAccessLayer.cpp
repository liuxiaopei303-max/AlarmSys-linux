// DataAccessLayer.cpp

#include "DataAccessLayer.h"
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariantMap>
#include <QHostInfo>
#include <QtConcurrent>
DataAccessLayer& DataAccessLayer::getInstance()
{
    static DataAccessLayer instance;
    return instance;
}

bool DataAccessLayer::initDatabase()
{
    DbInfo dbInfo = loadDbParams();
    DbType dbType = DbType_PostgreSQL;
    bool init = m_dbManager.initialize(dbType, dbInfo, 50);
    loadDictionaries();
    loadSensorInfo();

    return init;
}

DbInfo DataAccessLayer::loadDbParams()
{
    DbInfo dbInfo;
    dbInfo.connMode = 0;
    // 生成唯一的连接名
    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString hostName = QHostInfo::localHostName();
    dbInfo.connName = QString("qt_sql_%1_%2").arg(hostName).arg(uniqueId);

    QString dbPath = QCoreApplication::applicationDirPath() + "/db.ini";
    QSettings settings(dbPath, QSettings::IniFormat);

    settings.beginGroup("Database");
    dbInfo.dbName = settings.value("dbName", "postgres").toString();
    dbInfo.hostName = settings.value("hostName", "127.0.0.1").toString();
    dbInfo.hostPort = settings.value("hostPort", 5432).toInt();
    dbInfo.userName = settings.value("userName", "postgres").toString();
    dbInfo.userPwd = settings.value("userPwd", "123456").toString();
    settings.endGroup();
    return dbInfo;
}

DataAccessLayer::DataAccessLayer(QObject* parent)
    : QObject(parent), m_dbManager(DatabaseManager::getInstance())
{
    m_targetCategories.clear();
    m_modalityTypes.clear();
    m_collectionMethods.clear();
    m_behaviorTypes.clear();
    m_sensorInfo.clear();
    m_dictPrecipitation.clear();
    m_dictSeaState.clear();
    m_dictVisible.clear();
    m_dictWeatherTypes.clear();
    m_dictRouteFlight.clear();
    m_dictRouteFlightID.clear();
}

DataAccessLayer::~DataAccessLayer() {}


QFuture<QList<TargetInfo>> DataAccessLayer::getAllTargetInfoAsync()
{
    return QtConcurrent::run([this]() {
        return getAllTargetInfo();
        });
}

QList<TargetInfo> DataAccessLayer::getAllTargetInfo()
{
    QString query = "SELECT target_id, target_type, target_name FROM target_info";
    QSqlQuery result = m_dbManager.executeQuery(query);

    QList<TargetInfo> targetList;
    while (result.next()) {
        TargetInfo info;
        info.targetId = result.value("target_id").toLongLong();
        info.targetType = result.value("target_type").toInt();
        info.targetName = result.value("target_name").toString();
        targetList.append(info);
    }
    return targetList;
}

bool DataAccessLayer::addTargetInfo(const TargetInfo& info)
{
    QString query = "INSERT INTO target_info (target_id, target_type, target_name) VALUES (?, ?, ?)";
    return m_dbManager.executeNonQuery(query, { info.targetId, info.targetType, info.targetName });
}

QFuture<bool> DataAccessLayer::addTargetInfoAsync(const TargetInfo& info)
{
    return m_dbManager.executeNonQueryAsync("INSERT INTO target_info (target_id, target_type, target_name) VALUES (?, ?, ?)",
        { info.targetId, info.targetType, info.targetName });
}

TargetInfo DataAccessLayer::getTargetInfo(qint64 targetId)
{
    TargetInfo info;
    QString query = "SELECT target_id, target_type, target_name FROM target_info WHERE target_id = ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { targetId });

    if (result.next()) {
        info.targetId = result.value("target_id").toLongLong();
        info.targetType = result.value("target_type").toInt();
        info.targetName = result.value("target_name").toString();
    }

    return info;
}

QFuture<TargetInfo> DataAccessLayer::getTargetInfoAsync(qint64 targetId)
{
    return QtConcurrent::run([this, targetId]() {
        return getTargetInfo(targetId);
        });
}

bool DataAccessLayer::updateTargetInfo(const TargetInfo& info)
{
    QString query = "UPDATE target_info SET target_type = ?, target_name = ? WHERE target_id = ?";
    return m_dbManager.executeNonQuery(query, { info.targetType, info.targetName, info.targetId });
}

QFuture<bool> DataAccessLayer::updateTargetInfoAsync(const TargetInfo& info)
{
    return m_dbManager.executeNonQueryAsync("UPDATE target_info SET target_type = ?, target_name = ? WHERE target_id = ?",
        { info.targetType, info.targetName, info.targetId });
}

bool DataAccessLayer::deleteTarget(qint64 targetId)
{
    QSqlDatabase db = m_dbManager.beginTransaction();
    try {
        // 删除目标关联信息
        if (!m_dbManager.executeNonQuery("DELETE FROM target_association WHERE target_id = ?", { targetId })) {
            throw std::runtime_error("Failed to delete target associations");
        }

        // 获取所有的采集记录
        QList<QUuid> collectionIds;
        QString collectionQuery = "SELECT collection_id FROM target_collection WHERE target_id = ?";
        QSqlQuery collectionResult = m_dbManager.executeQuery(collectionQuery, { targetId });
        while (collectionResult.next()) {
            collectionIds.append(collectionResult.value(0).toUuid());
        }

        // 删除目标的所有采集数据
        QStringList collectionTables = { "collection_radar_echo", "collection_video", "collection_sonar", "collection_ais", "collection_track", "collection_adsb" };
        for (const QString& table : collectionTables) {
            QString deleteQuery = QString("DELETE FROM %1 WHERE collection_id = ?").arg(table);
            for (const QUuid& collectionId : collectionIds) {
                if (!m_dbManager.executeNonQuery(deleteQuery, { collectionId })) {
                    throw std::runtime_error(QString("Failed to delete records from %1").arg(table).toStdString());
                }
            }
        }

        // 删除目标采集信息
        if (!m_dbManager.executeNonQuery("DELETE FROM target_collection WHERE target_id = ?", { targetId })) {
            throw std::runtime_error("Failed to delete target collections");
        }

        // 删除目标基本信息
        if (!m_dbManager.executeNonQuery("DELETE FROM target_info WHERE target_id = ?", { targetId })) {
            throw std::runtime_error("Failed to delete target info");
        }

        m_dbManager.commitTransaction(db);
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Error deleting target:" << e.what();
        m_dbManager.rollbackTransaction(db);
        return false;
    }
}

QFuture<bool> DataAccessLayer::deleteTargetAsync(qint64 targetId)
{
    return QtConcurrent::run([this, targetId]() {
        return deleteTarget(targetId);
        });
}


bool DataAccessLayer::mergeTargets(const QList<qint64>& targetIdsToMerge, const TargetInfo& newTargetInfo)
{
    QSqlDatabase db = m_dbManager.beginTransaction();
    if (!db.isValid()) {
        qWarning() << "Failed to begin transaction for merging targets";
        return false;
    }

    try {
        // 创建新目标
        QVariantMap newTargetData;
        newTargetData["target_id"] = newTargetInfo.targetId;
        newTargetData["target_type"] = newTargetInfo.targetType;
        newTargetData["target_name"] = newTargetInfo.targetName;

        QString insertQuery = m_dbManager.buildInsertSql("target_info", newTargetData);
        if (!m_dbManager.executeNonQuery(insertQuery, newTargetData.values())) {
            throw std::runtime_error("Failed to add new target");
        }

        // 准备 IN 子句的占位符
        QStringList placeholders;
        for (int i = 0; i < targetIdsToMerge.size(); ++i) {
            placeholders << "?";
        }
        QString placeholderString = placeholders.join(',');

        // 更新目标关联信息
        QString updateAssociationQuery = QString("UPDATE target_association SET target_id = ? WHERE target_id IN (%1)").arg(placeholderString);
        QVariantList associationParams;
        associationParams << QVariant(newTargetInfo.targetId);
        for (const qint64& id : targetIdsToMerge) {
            associationParams << QVariant(id);
        }

        if (!m_dbManager.executeNonQuery(updateAssociationQuery, associationParams)) {
            throw std::runtime_error("Failed to update target associations");
        }

        // 更新目标采集信息
        QString updateCollectionQuery = QString("UPDATE target_collection SET target_id = ? WHERE target_id IN (%1)").arg(placeholderString);
        QVariantList collectionParams;
        collectionParams << QVariant(newTargetInfo.targetId);
        for (const qint64& id : targetIdsToMerge) {
            collectionParams << QVariant(id);
        }

        if (!m_dbManager.executeNonQuery(updateCollectionQuery, collectionParams)) {
            throw std::runtime_error("Failed to update target collections");
        }

        // 删除旧的目标基本信息
        QString deleteOldTargetsQuery = QString("DELETE FROM target_info WHERE target_id IN (%1)").arg(placeholderString);
        QVariantList deleteParams;
        for (const qint64& id : targetIdsToMerge) {
            deleteParams << QVariant(id);
        }

        if (!m_dbManager.executeNonQuery(deleteOldTargetsQuery, deleteParams)) {
            throw std::runtime_error("Failed to delete old targets");
        }

        if (!m_dbManager.commitTransaction(db)) {
            throw std::runtime_error("Failed to commit transaction");
        }
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Error merging targets:" << e.what();
        m_dbManager.rollbackTransaction(db);
        return false;
    }
}


qint64 DataAccessLayer::generateUniqueTargetId()
{
    QUuid uuid = QUuid::createUuid();
    QByteArray hash = QCryptographicHash::hash(uuid.toByteArray(), QCryptographicHash::Md5);

    // 取哈希值的前8个字节（64位）
    qint64 id = 0;
    for (int i = 0; i < 8; ++i)
    {
        id = (id << 8) | (hash[i] & 0xFF);
    }

    // 确保 ID 是正数
    return id & 0x7FFFFFFFFFFFFFFFLL;
}



bool DataAccessLayer::addTargetAssociation(const TargetAssociation& association)
{
    QString query = "INSERT INTO target_association (target_id, target_name, identifier, track_id, fused_track_id, association_time) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    return m_dbManager.executeNonQuery(query, { association.targetId, association.targetName, association.identifier,
                                               association.trackId, association.fusedTrackId, association.associationTime });
}

QFuture<bool> DataAccessLayer::addTargetAssociationAsync(const TargetAssociation& association)
{
    return m_dbManager.executeNonQueryAsync("INSERT INTO target_association (target_id, target_name, identifier, track_id, fused_track_id, association_time) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        { association.targetId, association.targetName, association.identifier,
         association.trackId, association.fusedTrackId, association.associationTime });
}

QList<TargetAssociation> DataAccessLayer::getTargetAssociations(qint64 targetId)
{
    QString query = "SELECT * FROM target_association WHERE target_id = ? ORDER BY association_time DESC";
    QSqlQuery result = m_dbManager.executeQuery(query, { targetId });

    QList<TargetAssociation> associations;
    while (result.next()) {
        TargetAssociation association;
        association.associationId = result.value("association_id").toLongLong();
        association.targetId = result.value("target_id").toLongLong();
        association.targetName = result.value("target_name").toString();
        association.identifier = result.value("identifier").toString();
        association.trackId = result.value("track_id").toLongLong();
        association.fusedTrackId = result.value("fused_track_id").toLongLong();
        association.associationTime = result.value("association_time").toDateTime();
        associations.append(association);
    }
    return associations;
}

QFuture<QList<TargetAssociation>> DataAccessLayer::getTargetAssociationsAsync(qint64 targetId)
{
    return QtConcurrent::run([this, targetId]() {
        return getTargetAssociations(targetId);
        });
}

TargetAssociation DataAccessLayer::getLatestTargetAssociation(qint64 targetId)
{
    QString query = "SELECT * FROM target_association WHERE target_id = ? ORDER BY association_time DESC LIMIT 1";
    QSqlQuery result = m_dbManager.executeQuery(query, { targetId });

    TargetAssociation association;
    if (result.next()) {
        association.associationId = result.value("association_id").toLongLong();
        association.targetId = result.value("target_id").toLongLong();
        association.targetName = result.value("target_name").toString();
        association.identifier = result.value("identifier").toString();
        association.trackId = result.value("track_id").toLongLong();
        association.fusedTrackId = result.value("fused_track_id").toLongLong();
        association.associationTime = result.value("association_time").toDateTime();
    }
    return association;
}

QFuture<TargetAssociation> DataAccessLayer::getLatestTargetAssociationAsync(qint64 targetId)
{
    return QtConcurrent::run([this, targetId]() {
        return getLatestTargetAssociation(targetId);
        });
}

QMap<qint64, TargetAssociation> DataAccessLayer::getAllLatestTargetAssociations()
{
    QString query = "SELECT DISTINCT ON (target_id) * FROM target_association ORDER BY target_id, association_time DESC";
    QSqlQuery result = m_dbManager.executeQuery(query, QVariantList());

    QMap<qint64, TargetAssociation> latestAssociations;
    while (result.next()) {
        TargetAssociation association;
        association.associationId = result.value("association_id").toLongLong();
        association.targetId = result.value("target_id").toLongLong();
        association.targetName = result.value("target_name").toString();
        association.identifier = result.value("identifier").toString();
        association.trackId = result.value("track_id").toLongLong();
        association.fusedTrackId = result.value("fused_track_id").toLongLong();
        association.associationTime = result.value("association_time").toDateTime();
        latestAssociations[association.targetId] = association;
    }
    return latestAssociations;
}

QFuture<QMap<qint64, TargetAssociation>> DataAccessLayer::getAllLatestTargetAssociationsAsync()
{
    return QtConcurrent::run([this]() {
        return getAllLatestTargetAssociations();
        });
}

// 3. 目标采集信息接口
bool DataAccessLayer::addTargetCollection(const TargetCollection& collection)
{
    QString query = "INSERT INTO target_collection (collection_id, target_id, modality_id, collection_time, collection_method, "
        "longitude, latitude, sensor_id, relative_bearing, relative_distance, behavior_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    bool success = m_dbManager.executeNonQuery(query, { collection.collectionId, collection.targetId, collection.modalityId,
                                               collection.collectionTime, collection.collectionMethod, collection.longitude,
                                               collection.latitude, collection.sensorId, collection.relativeBearing,
                                               collection.relativeDistance, collection.behaviorId });
    return success;
}

QFuture<bool> DataAccessLayer::addTargetCollectionAsync(const TargetCollection& collection)
{
    return m_dbManager.executeNonQueryAsync("INSERT INTO target_collection (collection_id, target_id, modality_id, collection_time, collection_method, "
        "longitude, latitude, sensor_id, relative_bearing, relative_distance, behavior_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        { collection.collectionId, collection.targetId, collection.modalityId,
         collection.collectionTime, collection.collectionMethod, collection.longitude,
         collection.latitude, collection.sensorId, collection.relativeBearing,
         collection.relativeDistance, collection.behaviorId });
}

QList<TargetCollection> DataAccessLayer::getTargetCollections(qint64 targetId)
{
    QString query = "SELECT * FROM target_collection WHERE target_id = ? ORDER BY collection_time DESC";
    QSqlQuery result = m_dbManager.executeQuery(query, { targetId });

    QList<TargetCollection> collections;
    while (result.next()) {
        TargetCollection collection;
        collection.collectionId = result.value("collection_id").toUuid();
        collection.targetId = result.value("target_id").toLongLong();
        collection.modalityId = result.value("modality_id").toInt();
        collection.collectionTime = result.value("collection_time").toDateTime();
        collection.collectionMethod = result.value("collection_method").toInt();
        collection.longitude = result.value("longitude").toDouble();
        collection.latitude = result.value("latitude").toDouble();
        collection.sensorId = result.value("sensor_id").toInt();
        collection.relativeBearing = result.value("relative_bearing").toDouble();
        collection.relativeDistance = result.value("relative_distance").toDouble();
        collection.behaviorId = result.value("behavior_id").toInt();
        collections.append(collection);
    }
    return collections;
}

QFuture<QList<TargetCollection>> DataAccessLayer::getTargetCollectionsAsync(qint64 targetId)
{
    return QtConcurrent::run([this, targetId]() {
        return getTargetCollections(targetId);
        });
}

bool DataAccessLayer::addSensorInfo(const SensorInfo& sensor)
{
    QString query = "INSERT INTO sensor_info (sensor_id, sensor_name) VALUES (?, ?)";
    return m_dbManager.executeNonQuery(query, { sensor.sensorId, sensor.sensorName });
}

QFuture<bool> DataAccessLayer::addSensorInfoAsync(const SensorInfo& sensor)
{
    return m_dbManager.executeNonQueryAsync("INSERT INTO sensor_info (sensor_id, sensor_name) VALUES (?, ?)",
        { sensor.sensorId, sensor.sensorName });
}

SensorInfo DataAccessLayer::getSensorInfo(int sensorId)
{
    SensorInfo sensor;
    QString query = "SELECT sensor_id, sensor_name FROM sensor_info WHERE sensor_id = ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { sensorId });

    if (result.next()) {
        sensor.sensorId = result.value("sensor_id").toInt();
        sensor.sensorName = result.value("sensor_name").toString();
    }

    return sensor;
}

int DataAccessLayer::getCameraRootPos(int camera_id)
{
    QString query = "SELECT root_pos FROM camera_root WHERE camera_id = ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { camera_id });
    int root_pos = 0;
    if (result.next()) {
        root_pos = result.value("root_pos").toInt();
    }
    return root_pos;
}



QFuture<SensorInfo> DataAccessLayer::getSensorInfoAsync(int sensorId)
{
    return QtConcurrent::run([this, sensorId]() {
        return getSensorInfo(sensorId);
        });
}

bool DataAccessLayer::updateSensorInfo(const SensorInfo& sensor)
{
    QString query = "UPDATE sensor_info SET sensor_name = ? WHERE sensor_id = ?";
    return m_dbManager.executeNonQuery(query, { sensor.sensorName, sensor.sensorId });
}

QFuture<bool> DataAccessLayer::updateSensorInfoAsync(const SensorInfo& sensor)
{
    return m_dbManager.executeNonQueryAsync("UPDATE sensor_info SET sensor_name = ? WHERE sensor_id = ?",
        { sensor.sensorName, sensor.sensorId });
}

bool DataAccessLayer::updateCameraRoot(int camera_id, int root_pos)
{
    return m_dbManager.executeNonQueryAsync("UPDATE camera_root SET root_pos = ? WHERE camera_id = ?",
        { root_pos, camera_id });
}


QMap<int, QString> DataAccessLayer::getAllSensorInfo()
{
    QMap<int, QString> sensorMap;
    QString query = "SELECT sensor_id, sensor_name FROM sensor_info";
    QSqlQuery result = m_dbManager.executeQuery(query);

    while (result.next()) {
        int sensorId = result.value("sensor_id").toInt();
        QString sensorName = result.value("sensor_name").toString();
        sensorMap[sensorId] = sensorName;
    }

    return sensorMap;
}

QMap<int, int> DataAccessLayer::getAllCameraRootPos()
{
    QMap<int, int> rootMap;
    QString query = "SELECT camera_id, root_pos FROM camera_root";
    QSqlQuery result = m_dbManager.executeQuery(query);

    while (result.next()) {
        int camera_id = result.value("camera_id").toInt();
        int root_pos = result.value("root_pos").toInt();
        rootMap[camera_id] = root_pos;
    }

    return rootMap;

}

QFuture<QMap<int, QString>> DataAccessLayer::getAllSensorInfoAsync()
{
    return QtConcurrent::run([this]() {
        return getAllSensorInfo();
        });
}


QFuture<int> DataAccessLayer::addAISRecordsAsync(const QList<AISRecord>& records)
{
    QList<QVariantMap> dataList;
    for (const AISRecord& record : records) {
        QVariantMap data;
        data["mmsi"] = record.mmsi;
        data["record_time"] = record.recordTime;
        data["sensor_id"] = record.sensorId;
        data["name"] = record.name;
        data["longitude"] = record.longitude;
        data["latitude"] = record.latitude;
        data["speed"] = record.speed;
        data["course"] = record.course;
        data["heading"] = record.heading;
        data["imo"] = record.imo;
        data["call_sign"] = record.callSign;
        data["ship_type"] = record.shipType;
        data["file_name"] = record.fileName;
        data["message_offset"] = record.messageOffset;
        data["message_size"] = record.messageSize;
        data["wave_byte"] = record.waveByte;
        dataList.append(data);
    }
    return m_dbManager.executeBatchInsertAsync("record_ais", dataList);
}

QFuture<int> DataAccessLayer::addADSBRecordsAsync(const QList<ADSBRecord>& records)
{
    QList<QVariantMap> dataList;
    for (const ADSBRecord& record : records) {
        QVariantMap data;
        data["icao"] = record.icao;
        data["record_time"] = record.recordTime;
        data["sensor_id"] = record.sensorId;
        data["longitude"] = record.longitude;
        data["latitude"] = record.latitude;
        data["altitude"] = record.altitude;
        dataList.append(data);
    }
    return m_dbManager.executeBatchInsertAsync("record_adsb", dataList);
}

QFuture<int> DataAccessLayer::addTrackRecordsAsync(const QList<TrackRecord>& records)
{
    QList<QVariantMap> dataList;
    for (const TrackRecord& record : records) {
        QVariantMap data;
        data["track_id"] = record.trackId;
        data["record_time"] = record.recordTime;
        data["sensor_id"] = record.sensorId;
        data["name"] = record.name;
        data["type"] = record.type;
        data["mmsi"] = record.mmsi;
        data["distance"] = record.distance;
        data["azimuth"] = record.azimuth;
        data["speed"] = record.speed;
        data["course"] = record.course;
        data["size"] = record.size;
        data["longitude"] = record.longitude;
        data["latitude"] = record.latitude;
        data["cpa"] = record.cpa;
        data["tcpa"] = record.tcpa;
        dataList.append(data);
    }
    return m_dbManager.executeBatchInsertAsync("record_radar_track", dataList);
}

QFuture<int> DataAccessLayer::addFusedTrackRecordsAsync(const QList<FusedTrackRecord>& records)
{
    QList<QVariantMap> dataList;
    for (const FusedTrackRecord& record : records) {
        QVariantMap data;
        data["fused_track_id"] = record.fusedTrackId;
        data["record_time"] = record.recordTime;
        data["sensor_id"] = record.sensorId;
        data["name"] = record.name;
        data["fusion_type"] = record.fusionType;
        for (int i = 0; i < 8; ++i) {
            QString key = QString("original_track_id%1").arg(i + 1);
            data[key] = (i < record.originalTrackIds.size()) ? record.originalTrackIds[i] : QVariant();
        }
        data["distance"] = record.distance;
        data["azimuth"] = record.azimuth;
        data["speed"] = record.speed;
        data["course"] = record.course;
        data["size"] = record.size;
        data["longitude"] = record.longitude;
        data["latitude"] = record.latitude;
        data["cpa"] = record.cpa;
        data["tcpa"] = record.tcpa;
        dataList.append(data);
    }
    return m_dbManager.executeBatchInsertAsync("record_fused_track", dataList);
}
// 6. 雷达回波和光电采集接口
QFuture<bool> DataAccessLayer::addRadarEchoCollectionsAsync(const QList<RadarEchoCollection>& collections)
{
    return QtConcurrent::run([this, collections]() {
        return addRadarEchoCollections(collections);
        });
}

bool DataAccessLayer::addRadarEchoCollections(const QList<RadarEchoCollection>& collections)
{
    bool allSuccess = true;
    for (const RadarEchoCollection& collection : collections) {
        QString query = "INSERT INTO collection_radar_echo (collection_id, sequence_number, sensor_id, collection_time, distance, azimuth, image_path, image_blob) "
            "VALUES (:collection_id, :sequence_number, :sensor_id, :collection_time, :distance, :azimuth, :image_path, :image_blob)";

        QVariantList params;
        params << collection.collectionId << collection.sequenceNumber << collection.sensorId
            << collection.collectionTime << collection.distance << collection.azimuth
            << collection.imagePath << collection.imageBlob;

        if (!m_dbManager.executeNonQuery(query, params)) {
            allSuccess = false;
            qWarning() << "Failed to insert radar echo collection:" << collection.collectionId;
        }
    }
    return allSuccess;
}

QFuture<bool> DataAccessLayer::addVideoCollectionsAsync(const QList<VideoCollection>& collections)
{
    return QtConcurrent::run([this, collections]() {
        return addVideoCollections(collections);
        });
}

bool DataAccessLayer::addVideoCollections(const QList<VideoCollection>& collections)
{
    bool allSuccess = true;
    for (const VideoCollection& collection : collections) {
        QString query = "INSERT INTO collection_video (collection_id, sequence_number, sensor_id, camera_type, collection_time, p, t, z, image_path, image_blob) "
            "VALUES (:collection_id, :sequence_number, :sensor_id, :camera_type, :collection_time, :p, :t, :z, :image_path, :image_blob)";

        QVariantList params;
        params << collection.collectionId << collection.sequenceNumber << collection.sensorId
            << collection.cameraType << collection.collectionTime << collection.p << collection.t << collection.z
            << collection.imagePath << collection.imageBlob;

        if (!m_dbManager.executeNonQuery(query, params)) {
            allSuccess = false;
            qWarning() << "Failed to insert video collection:" << collection.collectionId;
        }
    }
    return allSuccess;
}

QFuture<bool> DataAccessLayer::addPicDictAsync(const QList<PicCollection>& collections)
{
    return QtConcurrent::run([this, collections]() {
        return addPicDict(collections);
        });
}
bool DataAccessLayer::addPicDict(const QList<PicCollection>& collections)
{
    bool allSuccess = true;
    for (const PicCollection& collection : collections) {
        QString query = "INSERT INTO dict_pic (dict_id, sensor_id, camera_type, catch_time, weather_id, sea_state_id, visible_id, p, t, z, catch_rect, catch_blob) "
            "VALUES (:dict_id, :sensor_id, :camera_type, :catch_time, :weather_id, :sea_state_id, :visible_id, :p, :t, :z, :catch_rect, :catch_blob)";

        QVariantList params;
        params << collection.collectionId << collection.sensorId
            << collection.cameraType << collection.collectionTime << collection.weatherId << collection.seaStateId << collection.visibleId
            << collection.p << collection.t << collection.z
            << collection.rectInfo << collection.imageBlob;

        if (!m_dbManager.executeNonQuery(query, params)) {
            allSuccess = false;
            qWarning() << "Failed to insert video collection:" << collection.collectionId;
        }
    }
    return allSuccess;
}

QFuture<bool> DataAccessLayer::addSonarCollectionsAsync(const QList<SonarCollection>& collections)
{
    return QtConcurrent::run([this, collections]() {
        return addSonarCollections(collections);
        });
}

bool DataAccessLayer::addSonarCollections(const QList<SonarCollection>& collections)
{
    bool allSuccess = true;
    for (const SonarCollection& collection : collections) {
        QString query = "INSERT INTO collection_sonar (collection_id, sequence_number, sensor_id, collection_time, image_path, image_blob) "
            "VALUES (:collection_id, :sequence_number, :sensor_id, :collection_time, :image_path, :image_blob)";

        QVariantList params;
        params << collection.collectionId << collection.sequenceNumber << collection.sensorId
            << collection.collectionTime << collection.imagePath << collection.imageBlob;

        if (!m_dbManager.executeNonQuery(query, params)) {
            allSuccess = false;
            qWarning() << "Failed to insert sonar collection:" << collection.collectionId;
        }
    }
    return allSuccess;
}

// 7. 获取采集记录接口
QList<RadarEchoCollection> DataAccessLayer::getRadarEchoCollections(qint64 targetId)
{
    QList<RadarEchoCollection> collections;
    QString query = "SELECT c.*, tc.sensor_id "
        "FROM collection_radar_echo c "
        "JOIN target_collection tc ON c.collection_id = tc.collection_id "
        "WHERE tc.target_id = :target_id "
        "ORDER BY c.collection_time ASC";
    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    while (result.next()) {
        RadarEchoCollection collection;
        collection.collectionId = result.value("collection_id").toUuid();
        collection.sequenceNumber = result.value("sequence_number").toInt();
        collection.sensorId = result.value("sensor_id").toInt();
        collection.sensorName = getSensorName(collection.sensorId);
        collection.collectionTime = result.value("collection_time").toDateTime();
        collection.distance = result.value("distance").toDouble();
        collection.azimuth = result.value("azimuth").toDouble();
        collection.imagePath = result.value("image_path").toString();
        collection.imageBlob = result.value("image_blob").toByteArray();
        collections.append(collection);
    }

    return collections;
}

QList<VideoCollection> DataAccessLayer::getVisibleLightVideoCollections(qint64 targetId)
{
    return getVideoCollectionsByCameraType(targetId, 1);
}

QList<VideoCollection> DataAccessLayer::getInfraredVideoCollections(qint64 targetId)
{
    return getVideoCollectionsByCameraType(targetId, 0); // Assuming 0 is for infrared
}

QList<VideoCollection> DataAccessLayer::getVideoCollectionsByCameraType(qint64 targetId, int cameraType)
{
    QList<VideoCollection> collections;
    QString query = "SELECT c.*, tc.sensor_id "
        "FROM collection_video c "
        "JOIN target_collection tc ON c.collection_id = tc.collection_id "
        "WHERE tc.target_id = :target_id AND c.camera_type = :camera_type "
        "ORDER BY c.collection_time ASC";
    QVariantList params;
    params << targetId << cameraType;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    while (result.next()) {
        VideoCollection collection;
        collection.collectionId = result.value("collection_id").toUuid();
        collection.sequenceNumber = result.value("sequence_number").toInt();
        collection.sensorId = result.value("sensor_id").toInt();
        collection.sensorName = getSensorName(collection.sensorId);
        collection.collectionTime = result.value("collection_time").toDateTime();
        collection.cameraType = result.value("camera_type").toInt();
        collection.p = result.value("p").toDouble();
        collection.t = result.value("t").toDouble();
        collection.z = result.value("z").toDouble();
        collection.imagePath = result.value("image_path").toString();
        collection.imageBlob = result.value("image_blob").toByteArray();
        collections.append(collection);
    }

    return collections;
}

QList<SonarCollection> DataAccessLayer::getSonarCollections(qint64 targetId)
{
    QList<SonarCollection> collections;
    QString query = "SELECT c.*, tc.sensor_id "
        "FROM collection_sonar c "
        "JOIN target_collection tc ON c.collection_id = tc.collection_id "
        "WHERE tc.target_id = :target_id "
        "ORDER BY c.collection_time ASC";
    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    while (result.next()) {
        SonarCollection collection;
        collection.collectionId = result.value("collection_id").toUuid();
        collection.sequenceNumber = result.value("sequence_number").toInt();
        collection.sensorId = result.value("sensor_id").toInt();
        collection.sensorName = getSensorName(collection.sensorId);
        collection.collectionTime = result.value("collection_time").toDateTime();
        collection.imagePath = result.value("image_path").toString();
        collection.imageBlob = result.value("image_blob").toByteArray();
        collections.append(collection);
    }

    return collections;
}

QList<AISCollectionRecord> DataAccessLayer::getAISCollections(qint64 targetId)
{
    QList<AISCollectionRecord> collections;
    QString query = "SELECT c.* "
        "FROM collection_ais c "
        "JOIN target_collection tc ON c.collection_id = tc.collection_id "
        "WHERE tc.target_id = :target_id "
        "ORDER BY c.record_time ASC";
    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    while (result.next()) {
        AISCollectionRecord collection;
        collection.collectionId = result.value("collection_id").toUuid();
        collection.sequenceNumber = result.value("sequence_number").toInt();
        collection.sensorId = result.value("sensor_id").toInt();
        collection.sensorName = getSensorName(collection.sensorId);
        collection.mmsi = result.value("mmsi").toInt();
        collection.recordTime = result.value("record_time").toDateTime();
        collection.name = result.value("name").toString();
        collection.longitude = result.value("longitude").toDouble();
        collection.latitude = result.value("latitude").toDouble();
        collection.speed = result.value("speed").toDouble();
        collection.course = result.value("course").toDouble();
        collection.heading = result.value("heading").toInt();
        collection.imo = result.value("imo").toInt();
        collection.callSign = result.value("call_sign").toString();
        collection.shipType = result.value("ship_type").toInt();
        collection.fileName = result.value("file_name").toString();
        collection.messageOffset = result.value("message_offset").toInt();
        collection.messageSize = result.value("message_size").toInt();
        collection.waveByte = result.value("wave_byte").toByteArray();
        collections.append(collection);
    }

    return collections;
}

QList<TrackCollectionRecord> DataAccessLayer::getTrackCollections(qint64 targetId)
{
    QMap<QUuid, TrackCollectionRecord> collectionMap;
    QString query = "SELECT c.* "
        "FROM collection_track c "
        "JOIN target_collection tc ON c.collection_id = tc.collection_id "
        "WHERE tc.target_id = :target_id "
        "ORDER BY c.record_time ASC";
    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    while (result.next()) {
        QUuid collectionId = result.value("collection_id").toUuid();

        if (!collectionMap.contains(collectionId)) {
            TrackCollectionRecord collection;
            collection.collectionId = collectionId;
            collection.sensorId = result.value("sensor_id").toInt();
            collection.sensorName = getSensorName(collection.sensorId);
            collection.collectionTime = result.value("record_time").toDateTime();
            collection.isFused = false;
            collectionMap[collectionId] = collection;
        }

        TrackInfo track;
        track.trackId = result.value("track_id").toLongLong();
        track.sequenceNumber = result.value("sequence_number").toInt();
        track.recordTime = result.value("record_time").toDateTime();
        track.distance = result.value("distance").toDouble();
        track.azimuth = result.value("azimuth").toDouble();
        track.speed = result.value("speed").toDouble();
        track.course = result.value("course").toDouble();
        track.size = result.value("size").toDouble();
        track.longitude = result.value("longitude").toDouble();
        track.latitude = result.value("latitude").toDouble();
        track.cpa = result.value("cpa").toDouble();
        track.tcpa = result.value("tcpa").toDouble();

        collectionMap[collectionId].tracks.append(track);

        if (result.value("is_fused").toBool()) {
            collectionMap[collectionId].isFused = true;
        }
    }

    return collectionMap.values();
}

QList<ADSBCollectionRecord> DataAccessLayer::getADSBCollections(qint64 targetId)
{
    QList<ADSBCollectionRecord> collections;
    QString query = "SELECT c.* "
        "FROM collection_adsb c "
        "JOIN target_collection tc ON c.collection_id = tc.collection_id "
        "WHERE tc.target_id = :target_id "
        "ORDER BY c.record_time ASC";
    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    while (result.next()) {
        ADSBCollectionRecord collection;
        collection.collectionId = result.value("collection_id").toUuid();
        collection.sequenceNumber = result.value("sequence_number").toInt();
        collection.sensorId = result.value("sensor_id").toInt();
        collection.sensorName = getSensorName(collection.sensorId);
        collection.icao = result.value("icao").toString();
        collection.recordTime = result.value("record_time").toDateTime();
        collection.longitude = result.value("longitude").toDouble();
        collection.latitude = result.value("latitude").toDouble();
        collection.altitude = result.value("altitude").toDouble();
        collections.append(collection);
    }

    return collections;
}

QList<ShipFileInfo> DataAccessLayer::getShipFileCollections(qint64 targetId)
{
    QList<ShipFileInfo> collections;
    QString query = "SELECT* FROM mmsi_info WHERE mmsi = :target_id";
    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);
    while (result.next()) {
        ShipFileInfo collection;
        collection.CallSign = result.value("呼号").toString();
        collection.mmsi = targetId;
        collection.targetType = result.value("类型").toString();
        collection.shipLength = result.value("船长").toString();
        collection.shipWidth = result.value("船宽").toString();
        collection.shipDraft = result.value("吃水").toString();
        collection.Condition = result.value("状态").toString();
        collection.Destination = result.value("目的地").toString();
        collection.ArrTime = result.value("预到时间").toString();
        QString dateTimeString = result.value("更新时间").toString();
        QString format = "yyyy-MM-dd HH:mm:ss";
        collection.collectionTime = QDateTime::fromString(dateTimeString, format);
        collections.append(collection);
    }

    //查询图片
    QString query2 = "SELECT image_bytes FROM mmsi_pics WHERE mmsi = :target_id";
    QSqlQuery result2 = m_dbManager.executeQuery(query2, params);
    while (result2.next()) {
        QByteArray imageBytes = result2.value("image_bytes").toByteArray();
        if (!collections.isEmpty())
        {
            for (int i = 0; i < collections.size(); i++)
            {
                collections.at(i).imageBlob.append(imageBytes);
            }
        }
    }
    return collections;
}



// 8. 删除采集记录接口
bool DataAccessLayer::deleteRadarEchoRecord(const QUuid& collectionId, int sequenceNumber)
{
    QString query = "DELETE FROM collection_radar_echo WHERE collection_id = :collection_id AND sequence_number = :sequence_number";
    QVariantList params;
    params << collectionId << sequenceNumber;
    return m_dbManager.executeNonQuery(query, params);
}

bool DataAccessLayer::deleteVideoRecord(const QUuid& collectionId, int sequenceNumber, int cameraType)
{
    QString query = "DELETE FROM collection_video WHERE collection_id = :collection_id AND sequence_number = :sequence_number AND camera_type = :camera_type";
    QVariantList params;
    params << collectionId << sequenceNumber << cameraType;
    return m_dbManager.executeNonQuery(query, params);
}

bool DataAccessLayer::deleteSonarRecord(const QUuid& collectionId, int sequenceNumber)
{
    QString query = "DELETE FROM collection_sonar WHERE collection_id = :collection_id AND sequence_number = :sequence_number";
    QVariantList params;
    params << collectionId << sequenceNumber;
    return m_dbManager.executeNonQuery(query, params);
}

bool DataAccessLayer::deleteAISRecord(const QUuid& collectionId, int sequenceNumber)
{
    QString query = "DELETE FROM collection_ais WHERE collection_id = :collection_id AND sequence_number = :sequence_number";
    QVariantList params;
    params << collectionId << sequenceNumber;
    return m_dbManager.executeNonQuery(query, params);
}

bool DataAccessLayer::deleteTrackRecords(const QUuid& collectionId)
{
    QString query = "DELETE FROM collection_track WHERE collection_id = :collection_id";
    QVariantList params;
    params << collectionId;
    return m_dbManager.executeNonQuery(query, params);
}

bool DataAccessLayer::deleteADSBRecord(const QUuid& collectionId, int sequenceNumber)
{
    QString query = "DELETE FROM collection_adsb WHERE collection_id = :collection_id AND sequence_number = :sequence_number";
    QVariantList params;
    params << collectionId << sequenceNumber;
    return m_dbManager.executeNonQuery(query, params);
}

// 9. 获取目标采集统计信息
CaptureTargetItem DataAccessLayer::getTargetCaptureInfo(qint64 targetId)
{
    CaptureTargetItem item;

    QString query = "SELECT ti.target_id, ti.target_name, ti.target_type, "
        "tcs.radar_echo_collections, tcs.visible_light_collections, "
        "tcs.infrared_collections, tcs.sonar_collections, tcs.ais_collections, "
        "tcs.track_collections, tcs.adsb_collections, "
        "MAX(tc.collection_time) as last_collection_time "
        "FROM target_info ti "
        "LEFT JOIN target_collection_stats tcs ON ti.target_id = tcs.target_id "
        "LEFT JOIN target_collection tc ON ti.target_id = tc.target_id "
        "WHERE ti.target_id = :target_id "
        "GROUP BY ti.target_id, tcs.total_collections, tcs.radar_echo_collections, "
        "tcs.visible_light_collections, tcs.infrared_collections, tcs.sonar_collections, "
        "tcs.ais_collections, tcs.track_collections, tcs.adsb_collections";



    QVariantList params;
    params << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);

    if (result.next()) {
        item.id = result.value("target_id").toLongLong();
        item.name = result.value("target_name").toString();
        int dbTargetType = result.value("target_type").toInt();
        item.type = static_cast<TargetType>(dbTargetType);
        item.typeName = m_targetCategories[dbTargetType];
        item.captureTime = result.value("last_collection_time").toDateTime();
        item.isRealTime = false;  // 初始设置为 false

        //// 设置捕获的模态
        //if (result.value("radar_echo_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::RadarEcho);
        //if (result.value("visible_light_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::VisibleLightVideo);
        //if (result.value("infrared_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::InfraredVideo);
        //if (result.value("ais_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::AIS);
        //if (result.value("sonar_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::Sonar);
        //if (result.value("track_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::Track);
        //if (result.value("adsb_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::ADSB);
        // NaviRadar 暂时没有数据，所以这里不添加
    }

    QString query_file = "SELECT* FROM mmsi_info WHERE mmsi = :target_id";
    QSqlQuery result_file = m_dbManager.executeQuery(query_file, params);
    if (result_file.next()) {
        // item.capturedModals.insert(ModalType::ShipFile);  // capturedModals成员已移除
    }
    return item;
}

QMap<qint64, CaptureTargetItem> DataAccessLayer::getAllTargetCaptureInfo()
{
    QMap<qint64, CaptureTargetItem> allTargetInfo;

    QString query = "SELECT ti.target_id, ti.target_name, ti.target_type, "
        "tcs.radar_echo_collections, tcs.visible_light_collections, "
        "tcs.infrared_collections, tcs.sonar_collections, tcs.ais_collections, "
        "tcs.track_collections, tcs.adsb_collections, "
        "MAX(tc.collection_time) as last_collection_time "
        "FROM target_info ti "
        "LEFT JOIN target_collection_stats tcs ON ti.target_id = tcs.target_id "
        "LEFT JOIN target_collection tc ON ti.target_id = tc.target_id "
        "GROUP BY ti.target_id, ti.target_name, ti.target_type, tcs.total_collections, "
        "tcs.radar_echo_collections, tcs.visible_light_collections, tcs.infrared_collections, "
        "tcs.sonar_collections, tcs.ais_collections, tcs.track_collections, tcs.adsb_collections";

    QSqlQuery result = m_dbManager.executeQuery(query, QVariantList());

    while (result.next()) {
        CaptureTargetItem item;
        item.id = result.value("target_id").toLongLong();
        item.name = result.value("target_name").toString();
        int dbTargetType = result.value("target_type").toInt();
        item.type = static_cast<TargetType>(dbTargetType);
        item.typeName = m_targetCategories[dbTargetType];
        item.captureTime = result.value("last_collection_time").toDateTime();
        item.isRealTime = false;  // 初始设置为 false

        //// 设置捕获的模态
        //if (result.value("radar_echo_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::RadarEcho);
        //if (result.value("visible_light_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::VisibleLightVideo);
        //if (result.value("infrared_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::InfraredVideo);
        //if (result.value("ais_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::AIS);
        //if (result.value("sonar_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::Sonar);
        //if (result.value("track_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::Track);
        //if (result.value("adsb_collections").toInt() > 0)
        //    item.capturedModals.insert(ModalType::ADSB);
        // NaviRadar 暂时没有数据，所以这里不添加

        QString query_file = "SELECT* FROM mmsi_info WHERE mmsi = :target_id";
        QVariantList params;
        params << item.id;
        QSqlQuery result_file = m_dbManager.executeQuery(query_file, params);
        if (result_file.next()) {
            // item.capturedModals.insert(ModalType::ShipFile);  // capturedModals成员已移除
        }

        allTargetInfo[item.id] = item;
    }

    return allTargetInfo;
}

// 10. 获取目标最近一次记录的航迹信息
TargetRealtimeInfo DataAccessLayer::getTargetRealtimeInfo(qint64 targetId)
{
    TargetRealtimeInfo info;
    info.targetId = targetId;

    QString query =
        "WITH latest_track AS ( "
        "    SELECT ct.* "
        "    FROM collection_track ct "
        "    JOIN target_collection tc ON ct.collection_id = tc.collection_id "
        "    WHERE tc.target_id = :target_id "
        "    ORDER BY ct.record_time DESC "
        "    LIMIT 1 "
        "), "
        "latest_ais AS ( "
        "    SELECT ca.* "
        "    FROM collection_ais ca "
        "    JOIN target_collection tc ON ca.collection_id = tc.collection_id "
        "    WHERE tc.target_id = :target_id "
        "    ORDER BY ca.record_time DESC "
        "    LIMIT 1 "
        ") "
        "SELECT COALESCE(lt.longitude, la.longitude) AS longitude, "
        "       COALESCE(lt.latitude, la.latitude) AS latitude, "
        "       COALESCE(lt.distance, 0) AS distance, "
        "       COALESCE(lt.azimuth, 0) AS direction, "
        "       COALESCE(lt.speed, la.speed) AS speed, "
        "       COALESCE(lt.course, la.course) AS course, "
        "       COALESCE(lt.record_time, la.record_time) AS record_time "
        "FROM latest_track lt "
        "FULL OUTER JOIN latest_ais la ON 1=1 "
        "ORDER BY COALESCE(lt.record_time, la.record_time) DESC "
        "LIMIT 1";

    QVariantList params;
    params << targetId << targetId;
    QSqlQuery result = m_dbManager.executeQuery(query, params, 0);

    if (result.next()) {
        info.longitude = result.value("longitude").toDouble();
        info.latitude = result.value("latitude").toDouble();
        info.strLon = convertToDegreeMinute(info.longitude);
        info.strLat = convertToDegreeMinute(info.latitude);
        info.distanceNm = result.value("distance").toDouble() * 0.000539957; // 转换为海里
        info.directionDeg = result.value("direction").toDouble();
        info.speed = result.value("speed").toDouble();
        info.course = result.value("course").toDouble();
        info.recordTime = result.value("record_time").toDateTime();
    }
    return info;
}
QMap<int, QString> DataAccessLayer::getTargetCategories()
{
    QMutexLocker locker(&m_dictionariesMutex);
    return m_targetCategories;
}

QMap<int, QString> DataAccessLayer::getModalityTypes()
{
    QMutexLocker locker(&m_dictionariesMutex);
    return m_modalityTypes;
}

QMap<int, QString> DataAccessLayer::getCollectionMethods()
{
    QMutexLocker locker(&m_dictionariesMutex);
    return m_collectionMethods;
}

QMap<int, QString> DataAccessLayer::getBehaviorTypes()
{
    QMutexLocker locker(&m_dictionariesMutex);
    return m_behaviorTypes;
}

QStringList DataAccessLayer::getBehaviorTypes2()
{
    QMutexLocker locker(&m_dictionariesMutex);
    QStringList list;
    for (int i = 0; i < m_behaviorTypes2.size(); i++)
        list.append(m_behaviorTypes2.at(i));
    return list;
}

QStringList DataAccessLayer::getDictTargetTypes()
{
    QMutexLocker locker(&m_dictionariesMutex);
    QStringList list;
    for (int i = 0; i < m_dictTargetTypes.size(); i++)
        list.append(m_dictTargetTypes.at(i));
    return list;
}

QMap<QString, int> DataAccessLayer::getDictSeaStateTypes()
{
    QMutexLocker locker(&m_dictionariesMutex);
    return m_dictSeaState;
}

QMap<QString, int> DataAccessLayer::getDictWeatherTypes() {
    QMutexLocker locker(&m_dictionariesMutex);
    return m_dictWeatherTypes;
}

QMap<QString, int> DataAccessLayer::getDictVisible() {
    QMutexLocker locker(&m_dictionariesMutex);
    return m_dictVisible;
}

QMap<QString, int> DataAccessLayer::getDictPrecipitation() {
    QMutexLocker locker(&m_dictionariesMutex);
    return m_dictPrecipitation;
}

QMap<QString, QString> DataAccessLayer::getDictRouteFlight() {
    QMutexLocker locker(&m_dictionariesMutex);
    return m_dictRouteFlight;
}

QMap<QString, int> DataAccessLayer::getDictRouteFlightID() {
    QMutexLocker locker(&m_dictionariesMutex);
    return m_dictRouteFlightID;
}

QString DataAccessLayer::getSensorName(const int& sensorId)
{
    QMutexLocker locker(&m_sensorInfoMutex);
    return m_sensorInfo[sensorId];
}

QString DataAccessLayer::convertToDegreeMinute(double coordinate)
{
    bool isNegative = coordinate < 0;
    coordinate = std::abs(coordinate);

    int degrees = static_cast<int>(coordinate);
    double minutes = (coordinate - degrees) * 60.0;

    int minutesInt = static_cast<int>(minutes);
    int minutesFrac = qRound((minutes - minutesInt) * 1000);

    QString result = QString("%1%2°%3.%4′")
        .arg(isNegative ? "-" : "")
        .arg(degrees)
        .arg(minutesInt, 2, 10, QChar('0'))
        .arg(minutesFrac, 3, 10, QChar('0'));

    return result;
}


void DataAccessLayer::loadDictionaries()
{
    auto loadDictionary = [this](const QString& query, QMap<int, QString>& map) {
        QSqlQuery result = m_dbManager.executeQuery(query);
        while (result.next()) {
            map[result.value(0).toInt()] = result.value(1).toString();
        }
    };
    auto loadDictionaryVec = [this](const QString& query, QVector<QString>& vec) {
        QSqlQuery result = m_dbManager.executeQuery(query);
        while (result.next()) {
            vec.push_back(result.value(1).toString());
        }
    };
    auto loadDictionaryMap = [this](const QString& query, QMap<QString, int>& map) {
        QSqlQuery result = m_dbManager.executeQuery(query);
        while (result.next()) {
            map[result.value(1).toString()] = result.value(0).toInt();
        }
    };
    auto loadDictionaryMap2 = [this](const QString& query, QMap<QString, QString>& map) {
        QSqlQuery result = m_dbManager.executeQuery(query);
        while (result.next()) {
            map[result.value(0).toString()] = result.value(1).toString();
        }
    };

    loadDictionary("SELECT category_id, category_name FROM dict_target_category", m_targetCategories);
    loadDictionary("SELECT modality_id, modality_name FROM dict_modality_type", m_modalityTypes);
    loadDictionary("SELECT method_id, method_name FROM dict_collection_method", m_collectionMethods);
    loadDictionary("SELECT id, description FROM dict_behavior_types", m_behaviorTypes);

    loadDictionaryVec("SELECT id, description FROM dict_behavior_types2", m_behaviorTypes2);
    loadDictionaryVec("SELECT id, description FROM dict_type", m_dictTargetTypes);

    loadDictionaryMap("SELECT id, description FROM dict_sea_state", m_dictSeaState);
    loadDictionaryMap("SELECT id, description FROM dict_weather", m_dictWeatherTypes);
    loadDictionaryMap("SELECT id, description FROM dict_visible", m_dictVisible);
    loadDictionaryMap("SELECT id, description FROM dict_precipitation", m_dictPrecipitation);

    loadDictionaryMap2("SELECT area_name, area_points FROM area_table WHERE group_id = 0", m_dictRouteFlight);//目前只要group_id为0的数据
    loadDictionaryMap("SELECT area_id, area_name FROM area_table WHERE group_id = 0", m_dictRouteFlightID);//目前只要group_id为0的数据

}


QFuture<void> DataAccessLayer::loadDictionariesAsync()
{
    return QtConcurrent::run([this]() {
        loadDictionaries();
        });
}


void DataAccessLayer::loadSensorInfo()
{
    m_sensorInfo = getAllSensorInfo();
}

QFuture<void> DataAccessLayer::loadSensorInfoAsync()
{
    return QtConcurrent::run([this]() {
        loadSensorInfo();
        });
}

bool DataAccessLayer::addCollectionTaskHistoryInfo(const CollectionTaskHistoryInfo& info)
{
    QString query = "INSERT INTO collection_history ("\
        "task_name, collector, collection_time, collection_duration, collection_status"\
        ") VALUES(?, ?, ?, ?, ?)";
    return m_dbManager.executeNonQuery(query, { info.taskName, info.collector, info.collectionTime, info.collectionDuration, info.collectionStatus });
}

QList<CollectionTaskHistoryInfo> DataAccessLayer::getAllCollectionTaskHistoryInfo()
{
    QString query = "SELECT task_name, collector, collection_time, collection_duration, collection_status FROM collection_history";
    QSqlQuery result = m_dbManager.executeQuery(query);

    QList<CollectionTaskHistoryInfo> historyList;
    while (result.next()) {
        CollectionTaskHistoryInfo info;
        info.taskName = result.value("task_name").toString();
        info.collector = result.value("collector").toString();
        QVariant timeVariant = result.value("collection_time");
        if (timeVariant.canConvert<QDateTime>()) {
            QDateTime dateTime = timeVariant.toDateTime();
            info.collectionTime = dateTime.toString("yyyy-MM-dd hh:mm:ss");
        }
        else {
            info.collectionTime = timeVariant.toString();
        }
        info.collectionDuration = result.value("collection_duration").toInt();
        info.collectionStatus = result.value("collection_status").toString();
        historyList.append(info);
    }
    return historyList;
}

ShipFileInfo DataAccessLayer::getShipFileInfo(QString mmsi)
{
    /*QString query = "SELECT * FROM mmsi_info WHERE mmsi= ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { mmsi });
    ShipFileInfo info;
    try
    {
        while (result.next()) {
            info.mmsi = result.value("mmsi").isNull() ? 0 : result.value("mmsi").toInt();
            info.CallSign = result.value("呼号").isNull() ? "" : result.value("呼号").toString();
            info.targetType = result.value("类型").isNull() ? "" : result.value("类型").toString();
            info.Condition = result.value("状态").isNull() ? "" : result.value("状态").toString();
            info.shipLength = result.value("船长").isNull() ? "" : result.value("船长").toString();
            info.shipLength = result.value("船宽").isNull() ? "" : result.value("船宽").toString();
            info.Destination = result.value("目的地").isNull() ? "" : result.value("目的地").toString();
            info.ArrTime = result.value("预到时间").isNull() ? "" : result.value("预到时间").toString();
            info.Updatetime = result.value("更新时间").isNull() ? "" : result.value("更新时间").toString();
            info.shipDraft = result.value("吃水").isNull() ? "" : result.value("吃水").toString();
            info.SearchTime = result.value("update_time").isNull() ? 0 : result.value("update_time").toInt();
        }
    }
    catch (const std::exception& e) {
        qWarning() << QString("Exception in executeQuery: %1").arg(e.what());
    }
    catch (...) {
        qWarning() << "Unknown exception in executeQuery";
    }

    return info;*/

    ShipFileInfo info;

    // 获取连接
    QSqlDatabase db = m_dbManager.m_connectionPool.getConnection();
    if (!db.isValid()) {
        return info;
    }

    try {
        // 手动创建查询并执行
        QSqlQuery query(db);
        query.prepare("SELECT * FROM mmsi_info WHERE mmsi = ?");
        query.bindValue(0, mmsi);

        if (query.exec()) {
            // 直接执行获取数据，避免返回查询对象
            if (query.next()) {
                info.mmsi = query.value("mmsi").toInt();
                info.CallSign = query.value("呼号").toString();
                info.targetType = query.value("类型").toString();
                info.Condition = query.value("状态").toString();
                info.shipLength = query.value("船长").toString();
                info.shipWidth = query.value("船宽").toString();  // 修正错误
                info.Destination = query.value("目的地").toString();
                info.ArrTime = query.value("预到时间").toString();
                info.Updatetime = query.value("更新时间").toString();
                info.shipDraft = query.value("吃水").toString();
                info.SearchTime = query.value("update_time").toInt();
            }
        }
    }
    catch (const std::exception& e) {
        qWarning() << "查询船舶信息发生异常:" << e.what();
    }
    catch (...) {
        qWarning() << "查询船舶信息发生未知异常";
    }

    // 确保连接被释放
    m_dbManager.m_connectionPool.releaseConnection(db);

    return info;
}
QList<AlarmSettingInfo> DataAccessLayer::getAlarmSetting()
{
    QString query = "SELECT * FROM alarm_setting";
    QSqlQuery result = m_dbManager.executeQuery(query);
    QList<AlarmSettingInfo> info;
    while (result.next()) {
        AlarmSettingInfo p_alarmsetting;
        p_alarmsetting.alramName = result.value("name").toString();
        p_alarmsetting.alarmState = result.value("alarmstate").toInt();
        p_alarmsetting.area_name = result.value("area_name").toString();
        p_alarmsetting.filterState = result.value("filterstate").toInt();
        p_alarmsetting.area_function = result.value("area_function").toInt();
        p_alarmsetting.track_type = result.value("track_type").toInt();
        p_alarmsetting.speed_condition = result.value("speed_condition").toInt();
        p_alarmsetting.speed = result.value("speed").toInt();
        p_alarmsetting.area_judge = result.value("area_judge").toInt();
        p_alarmsetting.direction = result.value("direction").toInt();
        p_alarmsetting.group_id = result.value("group_id").toInt();
        p_alarmsetting.area_id = result.value("area_id").toInt();
        p_alarmsetting.ID = result.value("condition_id").toInt();
        info.append(p_alarmsetting);
    }
    return info;
}

QMap<QString, AlarmData> DataAccessLayer::getAlarmData()
{
    QString query = "SELECT * FROM alarm_data ORDER BY \"time\"";
    QSqlQuery result = m_dbManager.executeQuery(query);
    QMap<QString, AlarmData> info;
    while (result.next()) {
        AlarmData p_alarmdata;
        p_alarmdata.alarm_id = result.value("alarm_id").toString();
        p_alarmdata.condition_id = result.value("condition_id").toString();
        p_alarmdata.mode_id = result.value("mode_id").toInt();
        p_alarmdata.track_id = result.value("track_id").toInt();
        p_alarmdata.targettype = result.value("targettype").toInt();
        p_alarmdata.targetbehavior = result.value("targetbehavior").toInt();
        p_alarmdata.targetdist = result.value("targetdist").toDouble();
        p_alarmdata.targetlon = result.value("targetlon").toDouble();
        p_alarmdata.targetlat = result.value("targetlat").toDouble();
        p_alarmdata.targetspeed = result.value("targetspeed").toDouble();
        p_alarmdata.targetdir = result.value("targetdir").toDouble();
        QVariant var = result.value("time");
        QDateTime dateTime = var.toDateTime();
        p_alarmdata.time = dateTime.toString("yyyy-MM-dd hh:mm:ss");
        QVariant varOrig = result.value("origintime");
        if (!varOrig.isNull()) {
            QDateTime ot = varOrig.toDateTime();
            p_alarmdata.origintime = ot.toString("yyyy-MM-dd hh:mm:ss.zzz");
        }
        p_alarmdata.alarm_status = result.value("alarm_status").toInt();
        p_alarmdata.task_status = result.value("task_status").toInt();
        p_alarmdata.unique_id = result.value("unique_id").toLongLong();
        p_alarmdata.threatScore = result.value("threat_score").toInt();
        info[p_alarmdata.alarm_id] = p_alarmdata;
    }
    return info;
}

QMap<QString, AlarmData> DataAccessLayer::getAlarmData(const QDateTime& starttime, int level)
{
    QString query;
    QSqlQuery result;
    int tlevel = level;

    if (tlevel == 0)
    {
        // 显示所有级别
        query = "SELECT * FROM alarm_data WHERE \"time\" > ? ORDER BY \"time\"";
        result = m_dbManager.executeQuery(query, { starttime });
    }
    else if (tlevel <= 7) // 1, 2, 3, 4, 5, 6, 7 都是有效的组合值
    {
        // 构建 IN 子句的级别列表
        QStringList levelList;
        if (tlevel & 1) levelList << "1"; // 低级
        if (tlevel & 2) levelList << "2"; // 中级
        if (tlevel & 4) levelList << "3"; // 高级

        QString levelStr = levelList.join(",");
        query = QString("SELECT * FROM alarm_data WHERE \"time\" > ? AND condition_id IN (SELECT condition_id FROM alarm_setting WHERE alarm_level IN (%1)) ORDER BY \"time\"").arg(levelStr);
        result = m_dbManager.executeQuery(query, { starttime });
    }
    else
    {
        // 兼容旧的单一级别筛选方式
        query = "SELECT * FROM alarm_data WHERE \"time\" > ? AND condition_id IN (SELECT condition_id FROM alarm_setting WHERE alarm_level = ?) ORDER BY \"time\"";
        result = m_dbManager.executeQuery(query, { starttime, tlevel });
    }

    QMap<QString, AlarmData> info;
    while (result.next()) {
        AlarmData p_alarmdata;
        p_alarmdata.alarm_id = result.value("alarm_id").toString();
        p_alarmdata.condition_id = result.value("condition_id").toString();
        p_alarmdata.mode_id = result.value("mode_id").toInt();
        p_alarmdata.track_id = result.value("track_id").toInt();
        p_alarmdata.targettype = result.value("targettype").toInt();
        p_alarmdata.targetbehavior = result.value("targetbehavior").toInt();
        p_alarmdata.targetdist = result.value("targetdist").toDouble();
        p_alarmdata.targetlon = result.value("targetlon").toDouble();
        p_alarmdata.targetlat = result.value("targetlat").toDouble();
        p_alarmdata.targetspeed = result.value("targetspeed").toDouble();
        p_alarmdata.targetdir = result.value("targetdir").toDouble();
        QVariant var = result.value("time");
        QDateTime dateTime = var.toDateTime();
        p_alarmdata.time = dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
        QVariant var1 = result.value("origintime");
        QDateTime dateTime1 = var1.toDateTime();
        p_alarmdata.origintime = dateTime1.toString("yyyy-MM-dd hh:mm:ss.zzz");
        p_alarmdata.alarm_status = result.value("alarm_status").toInt();
        p_alarmdata.task_status = result.value("task_status").toInt();
        p_alarmdata.unique_id = result.value("unique_id").toLongLong();
        p_alarmdata.threatScore = result.value("threat_score").toInt();
        info[p_alarmdata.alarm_id] = p_alarmdata;
    }
    return info;
}

QMap<QString, AlarmRule> DataAccessLayer::getAlarmRule()
{
    QString query = "SELECT * FROM alarm_setting";
    QSqlQuery result = m_dbManager.executeQuery(query);
    QMap<QString, AlarmRule> info;
    while (result.next()) {
        AlarmRule p_alarmrule;
        p_alarmrule.name = result.value("name").toString();
        p_alarmrule.alarmstate = result.value("alarmstate").toInt();
        p_alarmrule.area_name = result.value("area_name").toString();
        p_alarmrule.filterstate = result.value("filterstate").toInt();
        p_alarmrule.area_function = result.value("area_function").toInt();
        p_alarmrule.area_type = result.value("area_type").toInt();
        p_alarmrule.track_type = result.value("track_type").toInt();
        p_alarmrule.speed_condition = result.value("speed_condition").toInt();
        p_alarmrule.speed = result.value("speed").toInt();
        p_alarmrule.area_judge = result.value("area_judge").toInt();
        p_alarmrule.direction = result.value("direction").toInt();
        p_alarmrule.group_id = result.value("group_id").toInt();
        p_alarmrule.area_id = result.value("area_id").toInt();
        p_alarmrule.condition_id = result.value("condition_id").toString();
        p_alarmrule.alarm_level = result.value("alarm_level").toInt();
        p_alarmrule.distance_condition = result.value("distance_condition").toInt();
        p_alarmrule.distance = result.value("distance").toInt();
        p_alarmrule.area_st = result.value("area_st").toString();
        p_alarmrule.area_et = result.value("area_et").toString();
        p_alarmrule.area_cycle = result.value("area_cycle").toInt();
        p_alarmrule.targetattr_st = result.value("targetattr_st").toString();
        p_alarmrule.targetattr_et = result.value("targetattr_et").toString();
        p_alarmrule.targetattr_cycle = result.value("targetattr_cycle").toInt();
        p_alarmrule.targetattr_side = result.value("targetattr_side").toInt();
        p_alarmrule.targetattr_ifmil = result.value("targetattr_ifmil").toInt();
        p_alarmrule.targetattr_type = result.value("targetattr_type").toInt();
        p_alarmrule.targetbh_st = result.value("targetbh_st").toString();
        p_alarmrule.targetbh_et = result.value("targetbh_et").toString();
        p_alarmrule.targetbh_cycle = result.value("targetbh_cycle").toInt();
        p_alarmrule.targetbh_type = result.value("targetbh_type").toInt();
        p_alarmrule.height_min = result.value("height_min").toInt();
        p_alarmrule.height_max = result.value("height_max").toInt();
        p_alarmrule.course_min = result.value("course_min").toInt();
        p_alarmrule.course_max = result.value("course_max").toInt();
        p_alarmrule.blacklist_judge = result.value("blacklist_judge").toInt();
        p_alarmrule.whitelist_judge = result.value("whitelist_judge").toInt();
        p_alarmrule.affiliation_judge = result.value("affiliation_judge").toInt();
        p_alarmrule.corp_judge = result.value("corp_judge").toInt();
        p_alarmrule.ais_type = result.value("corp_judge").toInt();
        p_alarmrule.ais_behavior = result.value("ais_behavior").toInt();
        p_alarmrule.has_ais = result.value("has_ais").toInt();
        p_alarmrule.dist_to_protect_area = result.value("dist_to_protect_area").toInt();
        p_alarmrule.uav_detect_type = result.value("uav_detect_type").toInt();
        p_alarmrule.camera_detect_type = result.value("camera_detect_type").toInt();
        p_alarmrule.threat_level1 = result.value("threat_level1").toInt();
        p_alarmrule.threat_level2 = result.value("threat_level2").toInt();
        p_alarmrule.entry_time = result.value("entry_time_value").toInt();
        info[p_alarmrule.condition_id] = p_alarmrule;
    }
    return info;
}

QMap<QString, QList<AlarmIdentificationRuleSub>> DataAccessLayer::getAlarmIdentificationRulesSub()
{
    QMap<QString, QList<AlarmIdentificationRuleSub>> map;
    const QString schemeId = getActiveSchemeId();
    QString query =
        "SELECT s.id, s.rule_id, s.criteria_id, s.criteria_type, s.criteria_order, "
        "s.detection_rules, s.logic_operator, s.enabled, s.created_at, s.updated_at "
        "FROM alarm_identification_rules_sub s "
        "WHERE EXISTS ("
        "  SELECT 1 FROM area_identification_rules a "
        "  WHERE a.rule_id = s.rule_id AND a.scheme_id = ? AND COALESCE(a.enabled, true) = true"
        ") "
        "ORDER BY s.rule_id, s.criteria_order, s.id";
    QSqlQuery result = m_dbManager.executeQuery(query, { schemeId });
    while (result.next()) {
        AlarmIdentificationRuleSub row;
        row.id = result.value("id").toInt();
        row.rule_id = result.value("rule_id").toString();
        row.criteria_id = result.value("criteria_id").toString();
        row.criteria_type = result.value("criteria_type").toInt();
        row.criteria_order = result.value("criteria_order").toInt();
        QVariant det = result.value("detection_rules");
        if (!det.isNull())
            row.detection_rules_json = det.type() == QVariant::ByteArray
                ? QString::fromUtf8(det.toByteArray())
                : det.toString();
        row.logic_operator = result.value("logic_operator").toString();
        if (row.logic_operator.isEmpty())
            row.logic_operator = QStringLiteral("AND");
        QVariant en = result.value("enabled");
        row.enabled = en.isNull() ? true : en.toBool();
        QVariant ct = result.value("created_at");
        if (!ct.isNull())
            row.created_at = ct.toDateTime();
        QVariant ut = result.value("updated_at");
        if (!ut.isNull())
            row.updated_at = ut.toDateTime();
        map[row.rule_id].append(row);
    }
    return map;
}

bool DataAccessLayer::hasMinioMultiMetadataForUniqueId(qint64 uniqueId, const QString& fileNameLikePattern)
{
    if (uniqueId <= 0)
        return false;
    const QString query =
        "SELECT id FROM minio_multi_metadata "
        "WHERE \"unique_id\" = ? "
        "AND \"file_name\" LIKE ? "
        "ORDER BY uploaded_at DESC "
        "LIMIT 1";
    QSqlQuery result = m_dbManager.executeQuery(query, { QVariant::fromValue(uniqueId), fileNameLikePattern });
    return result.next();
}

QMap<QString, QStringList> DataAccessLayer::getIdentificationRuleIdsByAreaForActiveScheme()
{
    QMap<QString, QStringList> map;
    const QString schemeId = getActiveSchemeId();
    const QString query =
        "SELECT group_id, area_id, rule_id FROM area_identification_rules "
        "WHERE scheme_id = ? AND COALESCE(enabled, true) = true "
        "ORDER BY group_id, area_id, rule_id";
    QSqlQuery result = m_dbManager.executeQuery(query, { schemeId });
    while (result.next()) {
        const int g = result.value(QStringLiteral("group_id")).toInt();
        const int a = result.value(QStringLiteral("area_id")).toInt();
        const QString rid = result.value(QStringLiteral("rule_id")).toString();
        const QString k = QString::number(g) + QLatin1Char('_') + QString::number(a);
        QStringList& lst = map[k];
        if (!lst.contains(rid))
            lst.append(rid);
    }
    return map;
}

QMap<QString, QStringList> DataAccessLayer::getVerificationRuleIdsByAreaForActiveScheme()
{
    QMap<QString, QStringList> map;
    const QString schemeId = getActiveSchemeId();
    const QString query =
        "SELECT group_id, area_id, rule_id FROM area_verification_rules "
        "WHERE scheme_id = ? AND COALESCE(enabled, true) = true "
        "ORDER BY group_id, area_id, rule_id";
    QSqlQuery result = m_dbManager.executeQuery(query, { schemeId });
    while (result.next()) {
        const int g = result.value(QStringLiteral("group_id")).toInt();
        const int a = result.value(QStringLiteral("area_id")).toInt();
        const QString rid = result.value(QStringLiteral("rule_id")).toString();
        const QString k = QString::number(g) + QLatin1Char('_') + QString::number(a);
        QStringList& lst = map[k];
        if (!lst.contains(rid))
            lst.append(rid);
    }
    return map;
}

QMap<QString, QList<AreaHandleRuleBinding>> DataAccessLayer::getAreaHandleRulesByAreaForActiveScheme()
{
    QMap<QString, QList<AreaHandleRuleBinding>> map;
    const QString schemeId = getActiveSchemeId();
    const QString query =
        "SELECT group_id, area_id, handle_scheme_id, handle_scheme_name, priority, enabled "
        "FROM area_handle_rules "
        "WHERE scheme_id = ? AND COALESCE(enabled, true) = true "
        "ORDER BY group_id, area_id, priority ASC, handle_scheme_id";
    QSqlQuery result = m_dbManager.executeQuery(query, { schemeId });
    while (result.next()) {
        AreaHandleRuleBinding row;
        row.handleSchemeId = result.value(QStringLiteral("handle_scheme_id")).toString();
        row.handleSchemeName = result.value(QStringLiteral("handle_scheme_name")).toString();
        row.priority = result.value(QStringLiteral("priority")).toInt();
        QVariant en = result.value(QStringLiteral("enabled"));
        row.enabled = en.isNull() ? true : en.toBool();
        const int g = result.value(QStringLiteral("group_id")).toInt();
        const int a = result.value(QStringLiteral("area_id")).toInt();
        const QString k = QString::number(g) + QLatin1Char('_') + QString::number(a);
        map[k].append(row);
    }
    return map;
}

QMap<int, TargetInfoFilter> DataAccessLayer::getTargetInfoFilter()
{
    QString query = "SELECT * FROM filter_target_table";
    QSqlQuery result = m_dbManager.executeQuery(query);
    QMap<int, TargetInfoFilter> info;
    while (result.next()) {
        TargetInfoFilter p_targetinfofilter;
        p_targetinfofilter.reid = result.value("reid").toInt();
        p_targetinfofilter.black_white_attr = result.value("black_white_attr").toInt();
        p_targetinfofilter.affiliation_attr = result.value("affiliation_attr").toInt();
        p_targetinfofilter.corp_attr = result.value("corp_attr").toInt();
        info[p_targetinfofilter.reid] = p_targetinfofilter;
    }
    return info;
}
QString DataAccessLayer::getActiveSchemeId()
{
    QString query = "SELECT scheme_id FROM alarm_master_schemes WHERE enabled = true LIMIT 1";
    QSqlQuery result = m_dbManager.executeQuery(query);
    if (result.next()) {
        return result.value("scheme_id").toString();
    }
    qDebug() << "未找到活跃的告警方案，使用默认scheme_id";
    return "default";
}

QMap<QString, QPair<int, int>> DataAccessLayer::getActiveSchemeProtectAreas()
{
    QMap<QString, QPair<int, int>> protectAreas;

    // 获取当前活跃的方案ID
    QString activeSchemeId = getActiveSchemeId();

    QString query = "SELECT group_id, area_id, protect_group_id, protect_area_id FROM alarm_scheme_areas WHERE scheme_id = ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { activeSchemeId });

    while (result.next()) {
        int groupId = result.value("group_id").toInt();
        int areaId = result.value("area_id").toInt();
        int protectGroupId = result.value("protect_group_id").toInt();
        int protectAreaId = result.value("protect_area_id").toInt();

        // 使用 "groupId_areaId" 作为键
        QString areaKey = QString("%1_%2").arg(groupId).arg(areaId);
        protectAreas[areaKey] = QPair<int, int>(protectGroupId, protectAreaId);
    }

    return protectAreas;
}

QList<ThreatAssessmentParams> DataAccessLayer::getThreatAssessmentParams()
{
    QString activeSchemeId = getActiveSchemeId();
    QString query = "SELECT * FROM area_params WHERE scheme_id = ? ORDER BY group_id, area_id";
    QSqlQuery result = m_dbManager.executeQuery(query, { activeSchemeId });
    QList<ThreatAssessmentParams> info;
    while (result.next()) {
        ThreatAssessmentParams params;
        params.groupId = result.value("group_id").toInt();
        params.areaId = result.value("area_id").toInt();
        params.seaSpeedboatScore = result.value("sea_speedboat_score").toDouble();
        params.seaWarshipScore = result.value("sea_warship_score").toDouble();
        params.seaMotorboatScore = result.value("sea_motorboat_score").toDouble();
        params.seaFishingBoatScore = result.value("sea_fishing_boat_score").toDouble();
        params.seaShipScore = result.value("sea_ship_score").toDouble();
        params.seaCargoShipScore = result.value("sea_cargo_ship_score").toDouble();
        params.seaUnknownScore = result.value("sea_unknown_score").toDouble();
        params.seaBuoyScore = result.value("sea_buoy_score").toDouble();
        params.airDroneScore = result.value("air_drone_score").toDouble();
        params.airDroneSwarmScore = result.value("air_drone_swarm_score").toDouble();
        params.airCompoundWingScore = result.value("air_compound_wing_score").toDouble();
        params.airRotorcraftScore = result.value("air_rotorcraft_score").toDouble();
        params.airAircraftScore = result.value("air_aircraft_score").toDouble();
        params.airBirdScore = result.value("air_bird_score").toDouble();
        params.airBirdFlockScore = result.value("air_bird_flock_score").toDouble();
        params.intentionAttackScore = result.value("intention_attack_score").toDouble();
        params.intentionReconnaissanceScore = result.value("intention_reconnaissance_score").toDouble();
        params.intentionPatrolScore = result.value("intention_patrol_score").toDouble();
        params.intentionTrainingScore = result.value("intention_training_score").toDouble();
        params.intentionTransitOperationScore = result.value("intention_transit_operation_score").toDouble();
        params.intentionTransitReturnScore = result.value("intention_transit_return_score").toDouble();
        params.intentionCargoRouteScore = result.value("intention_cargo_route_score").toDouble();
        params.intentionOtherScore = result.value("intention_other_score").toDouble();
        params.typeWeight = result.value("type_weight").toDouble();
        params.angleWeight = result.value("angle_weight").toDouble();
        params.speedWeight = result.value("speed_weight").toDouble();
        params.distanceWeight = result.value("distance_weight").toDouble();
        params.speedThresholdLow = result.value("speed_threshold_low").toDouble();
        params.speedThresholdHigh = result.value("speed_threshold_high").toDouble();
        params.maxEffectiveDistance = result.value("max_effective_distance").toDouble();
        params.highThreatThreshold = result.value("high_threat_threshold").toDouble();
        params.mediumThreatThreshold = result.value("medium_threat_threshold").toDouble();
        QVariant createdAtVar = result.value("created_at");
        QVariant updatedAtVar = result.value("updated_at");
        if (!createdAtVar.isNull()) {
            params.createdAt = createdAtVar.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        }
        if (!updatedAtVar.isNull()) {
            params.updatedAt = updatedAtVar.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        }
        info.append(params);
    }
    return info;
}
ThreatAssessmentParams DataAccessLayer::getThreatAssessmentParams(int groupId, int areaId, const QString& schemeId)
{
    QString query = "SELECT * FROM area_params WHERE group_id = ? AND area_id = ? AND scheme_id = ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { groupId, areaId, schemeId });
    ThreatAssessmentParams params;
    if (result.next()) {
        params.groupId = result.value("group_id").toInt();
        params.areaId = result.value("area_id").toInt();
        params.seaSpeedboatScore = result.value("sea_speedboat_score").toDouble();
        params.seaWarshipScore = result.value("sea_warship_score").toDouble();
        params.seaMotorboatScore = result.value("sea_motorboat_score").toDouble();
        params.seaFishingBoatScore = result.value("sea_fishing_boat_score").toDouble();
        params.seaShipScore = result.value("sea_ship_score").toDouble();
        params.seaCargoShipScore = result.value("sea_cargo_ship_score").toDouble();
        params.seaUnknownScore = result.value("sea_unknown_score").toDouble();
        params.seaBuoyScore = result.value("sea_buoy_score").toDouble();
        params.airDroneScore = result.value("air_drone_score").toDouble();
        params.airDroneSwarmScore = result.value("air_drone_swarm_score").toDouble();
        params.airCompoundWingScore = result.value("air_compound_wing_score").toDouble();
        params.airRotorcraftScore = result.value("air_rotorcraft_score").toDouble();
        params.airAircraftScore = result.value("air_aircraft_score").toDouble();
        params.airBirdScore = result.value("air_bird_score").toDouble();
        params.airBirdFlockScore = result.value("air_bird_flock_score").toDouble();
        params.intentionAttackScore = result.value("intention_attack_score").toDouble();
        params.intentionReconnaissanceScore = result.value("intention_reconnaissance_score").toDouble();
        params.intentionPatrolScore = result.value("intention_patrol_score").toDouble();
        params.intentionTrainingScore = result.value("intention_training_score").toDouble();
        params.intentionTransitOperationScore = result.value("intention_transit_operation_score").toDouble();
        params.intentionTransitReturnScore = result.value("intention_transit_return_score").toDouble();
        params.intentionCargoRouteScore = result.value("intention_cargo_route_score").toDouble();
        params.intentionOtherScore = result.value("intention_other_score").toDouble();
        params.typeWeight = result.value("type_weight").toDouble();
        params.angleWeight = result.value("angle_weight").toDouble();
        params.speedWeight = result.value("speed_weight").toDouble();
        params.distanceWeight = result.value("distance_weight").toDouble();
        params.speedThresholdLow = result.value("speed_threshold_low").toDouble();
        params.speedThresholdHigh = result.value("speed_threshold_high").toDouble();
        params.maxEffectiveDistance = result.value("max_effective_distance").toDouble();
        params.highThreatThreshold = result.value("high_threat_threshold").toDouble();
        params.mediumThreatThreshold = result.value("medium_threat_threshold").toDouble();
        QVariant createdAtVar = result.value("created_at");
        if (!createdAtVar.isNull()) {
            params.createdAt = createdAtVar.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        }
        QVariant updatedAtVar = result.value("updated_at");
        if (!updatedAtVar.isNull()) {
            params.updatedAt = updatedAtVar.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
        }
    }
    else {
        params = ThreatAssessmentParams(groupId, areaId);
        qDebug() << "未找到区域" << groupId << "-" << areaId << "的威胁度参数(方案ID:" << schemeId << ")，返回默认值";
    }
    return params;
}
ThreatAssessmentParams DataAccessLayer::getThreatAssessmentParams(int groupId, int areaId)
{
    QString activeSchemeId = getActiveSchemeId();
    return getThreatAssessmentParams(groupId, areaId, activeSchemeId);
}
bool DataAccessLayer::saveThreatAssessmentParams(const ThreatAssessmentParams& params)
{
    QString checkQuery = "SELECT COUNT(*) FROM area_params WHERE group_id = ? AND area_id = ?";
    QSqlQuery checkResult = m_dbManager.executeQuery(checkQuery, { params.groupId, params.areaId });
    bool recordExists = false;
    if (checkResult.next()) {
        int count = checkResult.value(0).toInt();
        recordExists = (count > 0);
    }
    QString query;
    QVariantList queryParams;
    if (recordExists) {
        query = "UPDATE area_params SET "
            "sea_speedboat_score = ?, sea_warship_score = ?, sea_motorboat_score = ?, "
            "sea_fishing_boat_score = ?, sea_ship_score = ?, sea_cargo_ship_score = ?, "
            "sea_unknown_score = ?, sea_buoy_score = ?, "
            "air_drone_score = ?, air_drone_swarm_score = ?, air_compound_wing_score = ?, "
            "air_rotorcraft_score = ?, air_aircraft_score = ?, air_bird_score = ?, "
            "air_bird_flock_score = ?, "
            "intention_attack_score = ?, intention_reconnaissance_score = ?, "
            "intention_patrol_score = ?, intention_training_score = ?, "
            "intention_transit_operation_score = ?, intention_transit_return_score = ?, "
            "intention_cargo_route_score = ?, intention_other_score = ?, "
            "type_weight = ?, angle_weight = ?, speed_weight = ?, distance_weight = ?, "
            "speed_threshold_low = ?, speed_threshold_high = ?, max_effective_distance = ?, "
            "high_threat_threshold = ?, medium_threat_threshold = ?, "
            "updated_at = CURRENT_TIMESTAMP "
            "WHERE group_id = ? AND area_id = ?";
        queryParams << params.seaSpeedboatScore << params.seaWarshipScore << params.seaMotorboatScore
            << params.seaFishingBoatScore << params.seaShipScore << params.seaCargoShipScore
            << params.seaUnknownScore << params.seaBuoyScore
            << params.airDroneScore << params.airDroneSwarmScore << params.airCompoundWingScore
            << params.airRotorcraftScore << params.airAircraftScore << params.airBirdScore
            << params.airBirdFlockScore
            << params.intentionAttackScore << params.intentionReconnaissanceScore
            << params.intentionPatrolScore << params.intentionTrainingScore
            << params.intentionTransitOperationScore << params.intentionTransitReturnScore
            << params.intentionCargoRouteScore << params.intentionOtherScore
            << params.typeWeight << params.angleWeight << params.speedWeight << params.distanceWeight
            << params.speedThresholdLow << params.speedThresholdHigh << params.maxEffectiveDistance
            << params.highThreatThreshold << params.mediumThreatThreshold
            << params.groupId << params.areaId;
    }
    else {
        query = "INSERT INTO area_params ("
            "group_id, area_id, "
            "sea_speedboat_score, sea_warship_score, sea_motorboat_score, "
            "sea_fishing_boat_score, sea_ship_score, sea_cargo_ship_score, "
            "sea_unknown_score, sea_buoy_score, "
            "air_drone_score, air_drone_swarm_score, air_compound_wing_score, "
            "air_rotorcraft_score, air_aircraft_score, air_bird_score, "
            "air_bird_flock_score, "
            "intention_attack_score, intention_reconnaissance_score, "
            "intention_patrol_score, intention_training_score, "
            "intention_transit_operation_score, intention_transit_return_score, "
            "intention_cargo_route_score, intention_other_score, "
            "type_weight, angle_weight, speed_weight, distance_weight, "
            "speed_threshold_low, speed_threshold_high, max_effective_distance, "
            "high_threat_threshold, medium_threat_threshold) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        queryParams << params.groupId << params.areaId
            << params.seaSpeedboatScore << params.seaWarshipScore << params.seaMotorboatScore
            << params.seaFishingBoatScore << params.seaShipScore << params.seaCargoShipScore
            << params.seaUnknownScore << params.seaBuoyScore
            << params.airDroneScore << params.airDroneSwarmScore << params.airCompoundWingScore
            << params.airRotorcraftScore << params.airAircraftScore << params.airBirdScore
            << params.airBirdFlockScore
            << params.intentionAttackScore << params.intentionReconnaissanceScore
            << params.intentionPatrolScore << params.intentionTrainingScore
            << params.intentionTransitOperationScore << params.intentionTransitReturnScore
            << params.intentionCargoRouteScore << params.intentionOtherScore
            << params.typeWeight << params.angleWeight << params.speedWeight << params.distanceWeight
            << params.speedThresholdLow << params.speedThresholdHigh << params.maxEffectiveDistance
            << params.highThreatThreshold << params.mediumThreatThreshold;
    }
    bool success = m_dbManager.executeNonQuery(query, queryParams);
    if (success) {
        qDebug() << "威胁度参数保存成功 - 区域:" << params.groupId << "-" << params.areaId;
    }
    else {
        qWarning() << "威胁度参数保存失败 - 区域:" << params.groupId << "-" << params.areaId;
    }
    return success;
}
QString DataAccessLayer::getTargetType(int id)
{
    QString query = "SELECT final_target_type FROM cognitive_results_comprehensive WHERE reid = ?";
    QSqlQuery result = m_dbManager.executeQuery(query, { id });
    while (result.next()) {
        return result.value("final_target_type").toString();
    }
    return "";
}
DataAccessLayer::DetectionTypeResult DataAccessLayer::getDetectionTypesByReId(qint64 reId)
{
    DetectionTypeResult result;
    QString query = "SELECT final_target_type, llm_target_type, cam_target_type, uav_target_type, final_threat_level "
        "FROM cognitive_results_comprehensive WHERE unique_id = ? "
        "ORDER BY observed_at DESC LIMIT 1"; // 获取最新的一条记录
    QSqlQuery sqlResult = m_dbManager.executeQuery(query, { reId });
    if (sqlResult.next()) {
        result.finalTargetType = sqlResult.value("final_target_type").toString();
        result.llmTargetType = sqlResult.value("llm_target_type").toString();
        result.camTargetType = sqlResult.value("cam_target_type").toString();
        result.uavTargetType = sqlResult.value("uav_target_type").toString();
        result.finalThreatLevel = sqlResult.value("final_threat_level").toInt();
        result.found = true;
    }
    return result;
}

bool DataAccessLayer::updateAlarmStatus(QString id, int status)
{
    QString query = "UPDATE alarm_data SET alarm_status = ? WHERE alarm_id = ?";
    return m_dbManager.executeNonQuery(query, { status, id });
}
bool DataAccessLayer::updateAlarmData(QString id, AlarmData data)
{
    QString query = QStringLiteral(
        "UPDATE alarm_data SET targetdist = ?, targetlat = ?, targetlon = ?, targetspeed = ?, "
        "targetdir = ?, \"time\" = ?, origintime = ?, alarm_status = ?, threat_score = ?, unique_id = ? "
        "WHERE alarm_id = ?");
    QDateTime tTime = QDateTime::fromString(data.time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    QDateTime tOrigin = QDateTime::fromString(data.origintime, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    const QVariant vTime = tTime.isValid() ? QVariant(tTime) : QVariant(data.time);
    const QVariant vOrigin = tOrigin.isValid() ? QVariant(tOrigin) : QVariant(data.origintime);
    return m_dbManager.executeNonQuery(query, {
        data.targetdist, data.targetlat, data.targetlon, data.targetspeed, data.targetdir,
        vTime, vOrigin, data.alarm_status, data.threatScore,
        QVariant::fromValue(static_cast<qint64>(data.unique_id)), id });
}

void DataAccessLayer::saveSystemEvaluationThreatOnce(const AlarmData& data)
{
    const QString q = QStringLiteral(
        "INSERT INTO system_evaluation_data (alarm_id, unique_id, threat_time, alarm_time) "
        "VALUES (?, ?, ?, NULL) "
        "ON CONFLICT (alarm_id) DO NOTHING");
    const QVariantList p = {
        data.alarm_id,
        QVariant::fromValue(static_cast<qint64>(data.unique_id)),
        QVariant::fromValue(data.threat_time_ms),
    };
    if (!m_dbManager.executeNonQuery(q, p)) {
        qWarning() << "Failed to insert system_evaluation_data (threat) alarm_id=" << data.alarm_id;
    }
}

bool DataAccessLayer::setSystemEvaluationAlarmTimeOnce(const QString& alarmId, qint64 uniqueId, qint64 threatTimeMs, qint64 alarmTimeMs)
{
    const qint64 threatMs = threatTimeMs > 0 ? threatTimeMs : alarmTimeMs;
    const QString q = QStringLiteral(
        "INSERT INTO system_evaluation_data (alarm_id, unique_id, threat_time, alarm_time) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT (alarm_id) DO UPDATE SET "
        "alarm_time = EXCLUDED.alarm_time "
        "WHERE system_evaluation_data.alarm_time IS NULL");
    return m_dbManager.executeNonQuery(q, {
        alarmId,
        QVariant::fromValue(uniqueId),
        QVariant::fromValue(threatMs),
        QVariant::fromValue(alarmTimeMs),
    });
}

void DataAccessLayer::addAlarmData(AlarmData data)
{
    // 评估表与 alarm_data 解耦：即使 alarm_data 插入失败也先写 threat_time
    saveSystemEvaluationThreatOnce(data);

    const QString query = QStringLiteral(
        "INSERT INTO alarm_data (alarm_id, condition_id, mode_id, track_id, targettype, targetbehavior, "
        "targetdist, targetlon, targetlat, targetspeed, targetdir, \"time\", alarm_status, origintime, "
        "unique_id, threat_score) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    QDateTime tTime = QDateTime::fromString(data.time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    QDateTime tOrigin = QDateTime::fromString(data.origintime, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    const QVariant vTime = tTime.isValid() ? QVariant(tTime) : QVariant(data.time);
    const QVariant vOrigin = tOrigin.isValid() ? QVariant(tOrigin) : QVariant(data.origintime);

    const QVariantList params = {
        data.alarm_id,
        data.condition_id,
        data.mode_id,
        data.track_id,
        data.targettype,
        data.targetbehavior,
        data.targetdist,
        data.targetlon,
        data.targetlat,
        data.targetspeed,
        data.targetdir,
        vTime,
        data.alarm_status,
        vOrigin,
        QVariant::fromValue(static_cast<qint64>(data.unique_id)),
        data.threatScore,
    };

    if (!m_dbManager.executeNonQuery(query, params)) {
        qWarning() << "Failed to insert alarm_data  Table:" << data.alarm_id;
    }
}

QList<AlarmAreaInfo> DataAccessLayer::getAlarmAreaInfo()
{
    QString query = "SELECT * FROM area_table";
    QSqlQuery result = m_dbManager.executeQuery(query);
    QList<AlarmAreaInfo> info;
    while (result.next()) {
        AlarmAreaInfo p_alarmareainfo;
        p_alarmareainfo.area_name = result.value("area_name").toString();
        p_alarmareainfo.area_type = result.value("area_type").toInt();
        p_alarmareainfo.area_id = result.value("area_id").toInt();
        p_alarmareainfo.group_id = result.value("group_id").toInt();
        p_alarmareainfo.warning_type = result.value("waring_type").toInt();
        info.append(p_alarmareainfo);
    }
    return info;
}

QList<UavSpeakerText> DataAccessLayer::ReadAllUavTextMsg()
{
    QString query = "SELECT * FROM uav_speaker_file order by createtime";
    QSqlQuery result = m_dbManager.executeQuery(query);
    QList<UavSpeakerText> info;
    while (result.next()) {
        UavSpeakerText text;
        text.id = result.value("text_id").toString();
        text.mode = result.value("mode_id").toInt();
        text.name = result.value("text_name").toString();
        text.content = result.value("content").toString();
        text.time = result.value("createtime").toString();
        text.md5 = result.value("md5").toString();
        text.file_id = result.value("file_id").toString();
        info.append(text);
    }
    return info;
}

bool DataAccessLayer::insertUavTextMsg(UavSpeakerText text)
{
    QString query = "insert into uav_speaker_file (text_id, text_name, mode_id, content, createtime, md5, file_id) values (?,?,?,?,?,?,?)";
    return m_dbManager.executeNonQuery(query, { text.id, text.name, text.mode, text.content, text.time,text.md5,text.file_id });

}

bool DataAccessLayer::updateUavTextMsg(UavSpeakerText text)
{
    QString query = "UPDATE uav_speaker_file SET text_name = ? ,content = ? where text_id = ?";
    return m_dbManager.executeNonQuery(query, { text.name, text.content, text.id });
}

bool DataAccessLayer::deleteUavTextMsg(UavSpeakerText text)
{
    QString query = "delete from uav_speaker_file where text_id = ?";
    return m_dbManager.executeNonQuery(query, { text.id });
    return false;
}

WeatherInfoFromDB DataAccessLayer::getWeatherInfo()
{
    WeatherInfoFromDB info;
    QList<QString> dateList;
    QList<QString> descriptionList;
    QList<QString> infoList;
    QDate currentDate = QDate::currentDate();
    for (int i = 0; i < 7; ++i) {
        QDate nextDate = currentDate.addDays(i);
        QString formattedDate = QString("%1%2%3").arg(nextDate.year()).arg(nextDate.month(), 2, 10, QChar('0')).arg(nextDate.day(), 2, 10, QChar('0'));
        dateList.append(formattedDate);
        QString query = ("SELECT * FROM weather_info WHERE date = ?");
        QSqlQuery result = m_dbManager.executeQuery(query, { formattedDate });

        if (!result.exec()) {
            qDebug() << result.lastError().text();
        }
        QString description;
        QString info;
        while (result.next()) {
            description = result.value("description").toString();
            info = result.value("info").toString();
        }
        descriptionList.append(description);
        infoList.append(info);
    }
    info.descriptionList = descriptionList;
    info.infoList = infoList;
    return info;
}

QVector<QByteArray> DataAccessLayer::getShipPicture(unsigned int mmsi)
{
    //QVector<QByteArray> info;
    //QByteArray byteArray;
    //QString query = ("SELECT image_bytes FROM mmsi_pics WHERE mmsi = ?");
    //QSqlQuery result = m_dbManager.executeQuery(query, { mmsi });
    //while (result.next()) {
    //    QByteArray imageBytes = result.value("image_bytes").isNull() ? byteArray : result.value("image_bytes").toByteArray();
    //    info.append(imageBytes);
    //}
    //return info;

    QVector<QByteArray> info;
    QByteArray byteArray;

    // 获取连接
    QSqlDatabase db = m_dbManager.m_connectionPool.getConnection();
    if (!db.isValid()) {
        return info;
    }

    try {
        // 手动创建查询并执行
        QSqlQuery query(db);
        query.prepare("SELECT image_bytes FROM mmsi_pics WHERE mmsi = ?");
        query.bindValue(0, mmsi);

        if (query.exec()) {
            // 直接执行获取数据，避免返回查询对象
            if (query.next()) {
                QByteArray imageBytes = query.value("image_bytes").isNull() ? byteArray : query.value("image_bytes").toByteArray();
                info.append(imageBytes);
            }
        }
    }
    catch (const std::exception& e) {
        qWarning() << "查询船舶信息发生异常:" << e.what();
    }
    catch (...) {
        qWarning() << "查询船舶信息发生未知异常";
    }

    // 确保连接被释放
    m_dbManager.m_connectionPool.releaseConnection(db);

    return info;
}

void DataAccessLayer::deleteAlarmSetting() {
    QString delete_query = "DELETE FROM alarm_setting";
    m_dbManager.executeNonQuery(delete_query);
}

void DataAccessLayer::addAlarmSetting(AlarmRule collection)
{
    QString query;
    query = "INSERT INTO alarm_setting (name, alarmstate, area_name, filterstate, area_function, area_type, track_type, speed_condition, speed, area_judge, direction, group_id, area_id, condition_id, alarm_level, distance_condition, distance, area_st, area_et, area_cycle, targetattr_st, targetattr_et, targetattr_cycle, targetattr_side, targetattr_ifmil, targetattr_type, targetbh_st, targetbh_et, targetbh_cycle, targetbh_type, height_min, height_max, course_min, course_max, blacklist_judge, whitelist_judge, affiliation_judge, corp_judge) "
        "VALUES (:name, :alarmstate, :area_name, :filterstate, :area_function, :area_type, :track_type, :speed_condition, :speed, :area_judge, :direction, :group_id, :area_id, :condition_id, :alarm_level, :distance_condition, :distance, :area_st, :area_et, :area_cycle, :targetattr_st, :targetattr_et, :targetattr_cycle, :targetattr_side, :targetattr_ifmil, :targetattr_type, :targetbh_st, :targetbh_et, :targetbh_cycle, :targetbh_type, :height_min, :height_max, :course_min, :course_max, :blacklist_judge, :whitelist_judge, :affiliation_judge, :corp_judge)";
    QVariantList params;
    params << collection.name << collection.alarmstate << collection.area_name << collection.filterstate << collection.area_function << collection.area_type << collection.track_type <<
        collection.speed_condition << collection.speed << collection.area_judge << collection.direction << collection.group_id << collection.area_id << collection.condition_id << collection.alarm_level << collection.distance_condition << collection.distance << collection.area_st << collection.area_et << collection.area_cycle << collection.targetattr_st << collection.targetattr_et << collection.targetattr_cycle << collection.targetattr_side << collection.targetattr_ifmil << collection.targetattr_type << collection.targetbh_st << collection.targetbh_et << collection.targetbh_cycle << collection.targetbh_type << collection.height_min << collection.height_max << collection.course_min << collection.course_max << collection.blacklist_judge << collection.whitelist_judge << collection.affiliation_judge << collection.corp_judge;
    if (!m_dbManager.executeNonQuery(query, params)) {

        qWarning() << "Failed to insert area Table:" << collection.name;
    }
}

void DataAccessLayer::deleteAlarmSetting(QString contidition)
{
    QString delete_query = "DELETE FROM alarm_setting where condition_id = ?";
    m_dbManager.executeNonQuery(delete_query, { contidition });
}

void DataAccessLayer::updateAlarmSetting(AlarmRule collection)
{
    QString query;
    query = "UPDATE alarm_setting set name = :name, alarmstate = :alarmstate, area_name = :area_name, filterstate = :filterstate, area_function = :area_function, area_type = :area_type, track_type = :track_type, speed_condition = :speed_condition,"
        "speed = :speed, area_judge = :area_judge, direction = :direction, group_id = :group_id, area_id = :area_id, alarm_level = :alarm_level, distance_condition = :distance_condition, distance = :distance, area_st = :area_st, area_et = :area_et, area_cycle = :area_cycle, targetattr_st = :targetattr_st, targetattr_et = :targetattr_et, targetattr_cycle = :targetattr_cycle, targetattr_side = :targetattr_side, targetattr_ifmil = :targetattr_ifmil, targetattr_type = :targetattr_type, targetbh_st = :targetbh_st, targetbh_et = :targetbh_et, targetbh_cycle = :targetbh_cycle, targetbh_type = :targetbh_type, height_min = :height_min, height_max = :height_max, course_min = :course_min, course_max = :course_max, blacklist_judge = :blacklist_judge, whitelist_judge = :whitelist_judge, affiliation_judge = :affiliation_judge, corp_judge = :corp_judge where condition_id = :condition_id";
    QVariantList params;
    params << collection.name << collection.alarmstate << collection.area_name << collection.filterstate << collection.area_function << collection.area_type << collection.track_type <<
        collection.speed_condition << collection.speed << collection.area_judge << collection.direction << collection.group_id << collection.area_id << collection.alarm_level << collection.distance_condition << collection.distance << collection.area_st << collection.area_et << collection.area_cycle << collection.targetattr_st << collection.targetattr_et << collection.targetattr_cycle << collection.targetattr_side << collection.targetattr_ifmil << collection.targetattr_type << collection.targetbh_st << collection.targetbh_et << collection.targetbh_cycle << collection.targetbh_type << collection.height_min << collection.height_max << collection.course_min << collection.course_max << collection.blacklist_judge << collection.whitelist_judge << collection.affiliation_judge << collection.corp_judge << collection.condition_id;
    m_dbManager.executeNonQuery(query, params);
}


void DataAccessLayer::saveAlarmSetting(QList<AlarmRule> info)
{
    QString query = "DELETE FROM alarm_setting";
    QSqlQuery result = m_dbManager.executeQuery(query);
    for (const AlarmRule& collection : info) {
        QString query;
        query = "INSERT INTO alarm_setting (name, alarmstate, area_name, filterstate, area_function, area_type, track_type, speed_condition, speed, area_judge, direction, group_id, area_id, condition_id, alarm_level, distance_condition, distance, area_st, area_et, area_cycle, targetattr_st, targetattr_et, targetattr_cycle, targetattr_side, targetattr_ifmil, targetattr_type, targetbh_st, targetbh_et, targetbh_cycle, targetbh_type, height_min, height_max, course_min, course_max, blacklist_judge, whitelist_judge, affiliation_judge, corp_judge) "
            "VALUES (:name, :alarmstate, :area_name, :filterstate, :area_function, :area_type, :track_type, :speed_condition, :speed, :area_judge, :direction, :group_id, :area_id, :condition_id, :alarm_level, :distance_condition, :distance, :area_st, :area_et, :area_cycle, :targetattr_st, :targetattr_et, :targetattr_cycle, :targetattr_side, :targetattr_ifmil, :targetattr_type, :targetbh_st, :targetbh_et, :targetbh_cycle, :targetbh_type, :height_min, :height_max, :course_min, :course_max, :blacklist_judge, :whitelist_judge, :affiliation_judge, :corp_judge)";
        QVariantList params;
        params << collection.name << collection.alarmstate << collection.area_name << collection.filterstate << collection.area_function << collection.area_type << collection.track_type <<
            collection.speed_condition << collection.speed << collection.area_judge << collection.direction << collection.group_id << collection.area_id << collection.condition_id << collection.alarm_level << collection.distance_condition << collection.distance << collection.area_st << collection.area_et << collection.area_cycle << collection.targetattr_st << collection.targetattr_et << collection.targetattr_cycle << collection.targetattr_side << collection.targetattr_ifmil << collection.targetattr_type << collection.targetbh_st << collection.targetbh_et << collection.targetbh_cycle << collection.targetbh_type << collection.height_min << collection.height_max << collection.course_min << collection.course_max << collection.blacklist_judge << collection.whitelist_judge << collection.affiliation_judge << collection.corp_judge;
        if (!m_dbManager.executeNonQuery(query, params)) {

            qWarning() << "Failed to insert area Table:" << collection.name;
        }

    }
}


AreaInfo DataAccessLayer::getAlarmArea(int groupID, int areaID)
{
    QString query = "SELECT *FROM area_table WHERE group_id=? AND area_id=?";
    QSqlQuery result = m_dbManager.executeQuery(query, { groupID,areaID });
    AreaInfo info;
    while (result.next()) {

        info.groupID = result.value("group_id").toInt();
        info.areaID = result.value("area_id").toInt();
        info.groupName = result.value("group_name").toString();
        info.areaName = result.value("area_name").toString();
        info.areaType = result.value("area_type").toInt();
        info.stratPoint = result.value("start_point").toString();
        info.endPoint = result.value("end_point").toString();
        info.areaRect = result.value("area_rect").toString();
        info.areaPoints = result.value("area_points").toString();
        info.lineWidth = result.value("line_width").toInt();
        info.lineColor = result.value("line_color").toString();
        info.checkState = result.value("check_state").toInt();
        info.waringType = result.value("waring_type").toInt();
        info.waringTime = result.value("waring_time").toInt();
        info.protectGroupID = result.value("protect_group_id").toInt();
        info.protectAreaID = result.value("protect_area_id").toInt();

    }
    return info;
}
QList<AreaInfo>DataAccessLayer::getGroupArea(int groupID)
{
    QString query = QString("SELECT *FROM area_table order by area_id");
    QSqlQuery result = m_dbManager.executeQuery(query, { groupID });

    QList<AreaInfo> targetList;
    while (result.next()) {
        AreaInfo info;
        info.groupID = result.value("group_id").toInt();
        info.areaID = result.value("area_id").toInt();
        info.groupName = result.value("group_name").toString();
        info.areaName = result.value("area_name").toString();
        info.areaType = result.value("area_type").toInt();
        info.stratPoint = result.value("start_point").toString();
        info.endPoint = result.value("end_point").toString();
        info.areaRect = result.value("area_rect").toString();
        info.areaPoints = result.value("area_points").toString();
        info.lineWidth = result.value("line_width").toInt();
        info.lineColor = result.value("line_color").toString();
        info.checkState = result.value("check_state").toInt();
        info.waringType = result.value("waring_type").toInt();
        info.waringTime = result.value("waring_time").toInt();
        targetList.append(info);
    }
    return targetList;
}

QList<AreaInfo>DataAccessLayer::getAreaInfo()
{
    QString query = QString("SELECT *FROM area_table order by area_id");

    QSqlQuery result = m_dbManager.executeQuery(query);

    QList<AreaInfo> targetList;
    while (result.next()) {
        AreaInfo info;
        info.groupID = result.value("group_id").toInt();
        info.areaID = result.value("area_id").toInt();
        info.groupName = result.value("group_name").toString();
        info.areaName = result.value("area_name").toString();
        info.areaType = result.value("area_type").toInt();
        info.stratPoint = result.value("start_point").toString();
        info.endPoint = result.value("end_point").toString();
        info.areaRect = result.value("area_rect").toString();
        info.areaPoints = result.value("area_points").toString();
        info.lineWidth = result.value("line_width").toInt();
        info.lineColor = result.value("line_color").toString();
        info.checkState = result.value("check_state").toInt();
        info.waringType = result.value("waring_type").toInt();
        info.waringTime = result.value("waring_time").toInt();
        info.protectGroupID = result.value("protect_group_id").toInt();
        info.protectAreaID = result.value("protect_area_id").toInt();
        targetList.append(info);
    }
    return targetList;
}

QList<AlarmPicInfo> DataAccessLayer::getAlarmPicCollections(QString alarmId)
{

    QList<AlarmPicInfo> collections;
    QString query = "SELECT* FROM alarm_pic_data WHERE alarm_id = :alarmId";
    QVariantList params;
    params << alarmId;
    QSqlQuery result = m_dbManager.executeQuery(query, params);
    while (result.next()) {
        AlarmPicInfo collection;
        collection.alarm_id = result.value("alarm_id").toString();
        collection.pic_time = result.value("alarm_time").toDateTime();
        collection.imageBlob = result.value("alarm_pic").toByteArray();
        collections.append(collection);
    }
    return collections;
}

QFuture<bool> DataAccessLayer::addAlarmPicAsync(const QList<AlarmPicInfo>& collections)
{
    return QtConcurrent::run([this, collections]() {
        return addAlarmPic(collections);
        });
}

bool DataAccessLayer::addAlarmPic(const QList<AlarmPicInfo>& collections)
{
    bool allSuccess = true;
    for (const AlarmPicInfo& collection : collections) {
        QString query = "INSERT INTO alarm_pic_data (alarm_id, alarm_time, alarm_pic) "
            "VALUES (:alarm_id, :alarm_time, :alarm_pic)";

        QVariantList params;
        params << collection.alarm_id << collection.pic_time
            << collection.imageBlob;

        if (!m_dbManager.executeNonQuery(query, params)) {
            allSuccess = false;
            qWarning() << "Failed to insert alarm pic:" << collection.alarm_id;
        }
    }
    return allSuccess;
}

DataAccessLayer::TodayWeatherInfo DataAccessLayer::getTodayWeatherInfo()
{
    TodayWeatherInfo info;

    QDate currentDate = QDate::currentDate();
    QString date = QString("%1%2%3")
                              .arg(currentDate.year())
                              .arg(currentDate.month(), 2, 10, QChar('0'))
                              .arg(currentDate.day(), 2, 10, QChar('0'));
                              
    QString query = QString("SELECT description, info FROM weather_info WHERE date = :date");
    QVariantList params;
    params << date;
    QSqlQuery result = m_dbManager.executeQuery(query, params);
    
    while (result.next()) {
        info.description = result.value("description").toString();
        info.info = result.value("info").toString();
    } 

    return info;
}

QString DataAccessLayer::sourceEnumToString(SourceEnum source)
{
    switch (source) {
        case SOURCE_AIS:
            return "ais";
        case SOURCE_LLM:
            return "llm";
        case SOURCE_FUSION_ENGINE:
            return "fusion_engine";
        case SOURCE_RADAR:
            return "radar";
        case SOURCE_MANUAL:
            return "manual";
        case SOURCE_ARCHIVE:
            return "archive";
        case SOURCE_UNKNOWN:
        default:
            return "llm";
    }
}

QString DataAccessLayer::affiliationEnumToString(AffiliationEnum affiliation)
{
    switch (affiliation) {
        case AFFILIATION_ENEMY:
            return "enemy";
        case AFFILIATION_OWNERSHIP:
            return "ownership";
        case AFFILIATION_FRIENDLY:
            return "friendly";
        case AFFILIATION_NEUTRAL:
            return "neutral";
        case AFFILIATION_UNKNOWN:
        default:
            return "unknown";
    }
}

SourceEnum DataAccessLayer::stringToSourceEnum(const QString& source)
{
    if (source == "ais") return SOURCE_AIS;
    if (source == "llm") return SOURCE_LLM;
    if (source == "fusion_engine") return SOURCE_FUSION_ENGINE;
    if (source == "radar") return SOURCE_RADAR;
    if (source == "manual") return SOURCE_MANUAL;
    if (source == "archive") return SOURCE_ARCHIVE;
    return SOURCE_UNKNOWN;
}

AffiliationEnum DataAccessLayer::stringToAffiliationEnum(const QString& affiliation)
{
    if (affiliation == "enemy") return AFFILIATION_ENEMY;
    if (affiliation == "ownership") return AFFILIATION_OWNERSHIP;
    if (affiliation == "friendly") return AFFILIATION_FRIENDLY;
    if (affiliation == "neutral") return AFFILIATION_NEUTRAL;
    return AFFILIATION_UNKNOWN;
}

bool DataAccessLayer::addCognitiveResult(const CognitiveResultsComprehensive& result)
{
    QString query = "INSERT INTO cognitive_results_comprehensive ("
        "track_id, unique_id, observed_at, "
        "final_target_type, final_type_source, final_target_status, final_status_source, "
        "final_affiliation, final_affiliation_source, final_threat_level, final_intent, "
        "fused_location, fused_altitude_m, fused_speed_knots, fused_course_deg, "
        "ais_mmsi, ais_imo, ais_callsign, ais_target_type_raw, ais_nav_status_code, "
        "ais_length_m, ais_width_m, "
        "archive_target_type, archive_length_m, archive_width_m, archive_destination, archive_status, "
        "llm_target_type, llm_target_type_confidence, llm_intent_raw, llm_detection_confidence, "
        "created_at,time_target_type,time_target_confidence,reid) "
        "VALUES (?, ?, ?, ?, ?::dict_source_enum, ?, ?::dict_source_enum, "
        "?::dict_affiliation_enum, ?::dict_source_enum, ?, ?, "
        "ST_SetSRID(ST_MakePoint(?, ?), 4326), ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?)";

    QVariantList params; 
    params << result.trackId << result.uniqueId << result.observedAt
        << result.finalTargetType << sourceEnumToString(result.finalTypeSource)
        << result.finalTargetStatus << sourceEnumToString(result.finalStatusSource)
        << affiliationEnumToString(result.finalAffiliation) << sourceEnumToString(result.finalAffiliationSource)
        << result.finalThreatLevel << result.finalIntent
        << result.fusedLongitude << result.fusedLatitude << result.fusedAltitudeM
        << result.fusedSpeedKnots << result.fusedCourseDeg
        << result.aisMmsi << result.aisImo << result.aisCallsign
        << result.aisTargetTypeRaw << result.aisNavStatusCode
        << result.aisLengthM << result.aisWidthM
        << result.archiveTargetType << result.archiveLengthM << result.archiveWidthM
        << result.archiveDestination << result.archiveStatus
        << result.llmTargetType << result.llmTargetTypeConfidence
        << result.llmIntentRaw << result.llmDetectionConfidence
        << result.createdAt << result.timeTargetType 
        << result.timeTargetConfidence << result.reId;  // 转换为 double

    return m_dbManager.executeNonQueryAsync(query, params);
}

bool DataAccessLayer::updateCognitiveResult(const CognitiveResultsComprehensive& result)
{
    QString query = "UPDATE cognitive_results_comprehensive SET observed_at = ?,final_target_type = ?,time_target_type = ?, final_threat_level = ?, time_target_confidence = ?, reid = ? WHERE unique_id = ?";

    QVariantList params;
    
    if(result.finalTargetType == "ship" || result.finalTargetType == "buoy")
    {
        // 对于船舶和浮标类型，只更新ship或buoy类型的记录
        query = "UPDATE cognitive_results_comprehensive SET observed_at = ?, final_target_type = ?, time_target_type = ?, final_threat_level = ?, time_target_confidence = ?, reid = ? WHERE unique_id = ? AND (final_target_type = 'ship' OR final_target_type = 'buoy')";
        params << result.observedAt
               << result.finalTargetType  // 修正：应该是finalTargetType而不是timeTargetType
               << result.timeTargetType
               << result.finalThreatLevel
               << result.timeTargetConfidence
               << result.reId
               << result.uniqueId;
    }
    else if(result.finalTargetType == "uav" || result.finalTargetType == "missile")
    {
        // 对于无人机和导弹类型，只更新uav或missile类型的记录
        query = "UPDATE cognitive_results_comprehensive SET observed_at = ?, final_target_type = ?, time_target_type = ?, final_threat_level = ?, time_target_confidence = ?, reid = ? WHERE unique_id = ? AND (final_target_type = 'uav' OR final_target_type = 'missile')";
        params << result.observedAt
               << result.finalTargetType  // 修正：应该是finalTargetType而不是timeTargetType
               << result.timeTargetType
               << result.finalThreatLevel
               << result.timeTargetConfidence
               << result.reId
               << result.uniqueId;
    }
    else
    {
        // 对于其他类型，精确匹配目标类型
        query = "UPDATE cognitive_results_comprehensive SET observed_at = ?, final_target_type = ?, time_target_type = ?, final_threat_level = ?, time_target_confidence = ?, reid = ? WHERE unique_id = ? AND final_target_type = ?";
        params << result.observedAt
               << result.finalTargetType  // 修正：应该是finalTargetType而不是timeTargetType
               << result.timeTargetType
               << result.finalThreatLevel
               << result.timeTargetConfidence
               << result.reId
               << result.uniqueId
               << result.finalTargetType;
    }

    return m_dbManager.executeNonQueryAsync(query, params);
}

bool DataAccessLayer::saveCognitiveResult(CognitiveResultsComprehensive result)
{
    //// 检查表的约束情况，如果没有合适的唯一约束，回退到传统方式
    //QString checkConstraintQuery = "SELECT 1 FROM information_schema.table_constraints "
    //                              "WHERE table_name = 'cognitive_results_comprehensive' "
    //                              "AND constraint_type IN ('PRIMARY KEY', 'UNIQUE') "
    //                              "AND constraint_name LIKE '%unique_id%'";
    //
    //QSqlQuery constraintResult = m_dbManager.executeQuery(checkConstraintQuery);
    //bool hasUniqueConstraint = constraintResult.next();
    bool hasUniqueConstraint = true;
    
    if (hasUniqueConstraint) {
        // 如果存在unique_id约束，使用UPSERT
        QString query = "INSERT INTO cognitive_results_comprehensive ("
            "track_id, unique_id, observed_at, "
            "final_target_type, final_type_source, final_target_status, final_status_source, "
            "final_affiliation, final_affiliation_source, final_threat_level, final_intent, "
            "fused_location, fused_altitude_m, fused_speed_knots, fused_course_deg, "
            "ais_mmsi, ais_imo, ais_callsign, ais_target_type_raw, ais_nav_status_code, "
            "ais_length_m, ais_width_m, "
            "archive_target_type, archive_length_m, archive_width_m, archive_destination, archive_status, "
            "llm_target_type, llm_target_type_confidence, llm_intent_raw, llm_detection_confidence, "
            "created_at, time_target_type, time_target_confidence, reid) "
            "VALUES (?, ?, ?, ?, ?::dict_source_enum, ?, ?::dict_source_enum, "
            "?::dict_affiliation_enum, ?::dict_source_enum, ?, ?, "
            "ST_SetSRID(ST_MakePoint(?, ?), 4326), ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (unique_id) "
            "DO UPDATE SET "
            "track_id = EXCLUDED.track_id, "
            "observed_at = EXCLUDED.observed_at, "
            "final_target_type = EXCLUDED.final_target_type, "
            "final_threat_level = EXCLUDED.final_threat_level, "
            "time_target_type = EXCLUDED.time_target_type, "
            "time_target_confidence = EXCLUDED.time_target_confidence, "
            "reid = EXCLUDED.reid, "
            "fused_location = EXCLUDED.fused_location, "
            "fused_speed_knots = EXCLUDED.fused_speed_knots, "
            "fused_course_deg = EXCLUDED.fused_course_deg";
        
        QVariantList params;
        params << result.trackId << result.uniqueId << result.observedAt
            << result.finalTargetType << sourceEnumToString(result.finalTypeSource)
            << result.finalTargetStatus << sourceEnumToString(result.finalStatusSource)
            << affiliationEnumToString(result.finalAffiliation) << sourceEnumToString(result.finalAffiliationSource)
            << result.finalThreatLevel << result.finalIntent
            << result.fusedLongitude << result.fusedLatitude << result.fusedAltitudeM
            << result.fusedSpeedKnots << result.fusedCourseDeg
            << result.aisMmsi << result.aisImo << result.aisCallsign
            << result.aisTargetTypeRaw << result.aisNavStatusCode
            << result.aisLengthM << result.aisWidthM
            << result.archiveTargetType << result.archiveLengthM << result.archiveWidthM
            << result.archiveDestination << result.archiveStatus
            << result.llmTargetType << result.llmTargetTypeConfidence
            << result.llmIntentRaw << result.llmDetectionConfidence
            << result.createdAt << result.timeTargetType 
            << result.timeTargetConfidence << result.reId;

        bool success = m_dbManager.executeNonQueryAsync(query, params);
        qDebug() << "认知结果UPSERT成功 - unique_id:" << result.uniqueId << "target_type:" << result.finalTargetType;
        return success;
    } else {
        // 如果没有约束，使用传统的查询+更新/插入方式（但优化查询逻辑）
        qDebug() << "数据库表缺少unique_id约束，使用传统UPSERT方式";
        
        // 先尝试更新，根据目标类型进行分组更新
        QString updateQuery;
        QVariantList updateParams;
        
        if(result.finalTargetType == "ship" || result.finalTargetType == "buoy") {
            updateQuery = "UPDATE cognitive_results_comprehensive SET "
                         "track_id = ?, observed_at = ?, final_target_type = ?, final_threat_level = ?, "
                         "time_target_type = ?, time_target_confidence = ?, reid = ?, "
                         "fused_location = ST_SetSRID(ST_MakePoint(?, ?), 4326), "
                         "fused_speed_knots = ?, fused_course_deg = ? "
                         "WHERE unique_id = ? AND final_target_type IN ('ship', 'buoy')";
            updateParams << result.trackId << result.observedAt << result.finalTargetType 
                        << result.finalThreatLevel << result.timeTargetType << result.timeTargetConfidence 
                        << result.reId << result.fusedLongitude << result.fusedLatitude 
                        << result.fusedSpeedKnots << result.fusedCourseDeg << result.uniqueId;
        } else if(result.finalTargetType == "uav" || result.finalTargetType == "missile") {
            updateQuery = "UPDATE cognitive_results_comprehensive SET "
                         "track_id = ?, observed_at = ?, final_target_type = ?, final_threat_level = ?, "
                         "time_target_type = ?, time_target_confidence = ?, reid = ?, "
                         "fused_location = ST_SetSRID(ST_MakePoint(?, ?), 4326), "
                         "fused_speed_knots = ?, fused_course_deg = ? "
                         "WHERE unique_id = ? AND final_target_type IN ('uav', 'missile')";
            updateParams << result.trackId << result.observedAt << result.finalTargetType 
                        << result.finalThreatLevel << result.timeTargetType << result.timeTargetConfidence 
                        << result.reId << result.fusedLongitude << result.fusedLatitude 
                        << result.fusedSpeedKnots << result.fusedCourseDeg << result.uniqueId;
        } else {
            updateQuery = "UPDATE cognitive_results_comprehensive SET "
                         "track_id = ?, observed_at = ?, final_threat_level = ?, "
                         "time_target_type = ?, time_target_confidence = ?, reid = ?, "
                         "fused_location = ST_SetSRID(ST_MakePoint(?, ?), 4326), "
                         "fused_speed_knots = ?, fused_course_deg = ? "
                         "WHERE unique_id = ? AND final_target_type = ?";
            updateParams << result.trackId << result.observedAt << result.finalThreatLevel 
                        << result.timeTargetType << result.timeTargetConfidence << result.reId 
                        << result.fusedLongitude << result.fusedLatitude << result.fusedSpeedKnots 
                        << result.fusedCourseDeg << result.uniqueId << result.finalTargetType;
        }
        
        // 执行更新
        QSqlQuery updateResult = m_dbManager.executeQuery(updateQuery, updateParams);
        int rowsAffected = updateResult.numRowsAffected();
        
        if (rowsAffected > 0) {
            qDebug() << "认知结果更新成功 - unique_id:" << result.uniqueId << "target_type:" << result.finalTargetType;
            return true;
        } else {
            // 如果更新没有影响行，说明记录不存在，执行插入
            qDebug() << "记录不存在，执行插入 - unique_id:" << result.uniqueId << "target_type:" << result.finalTargetType;
            return addCognitiveResult(result);
        }
    }
}