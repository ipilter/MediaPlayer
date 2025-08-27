#include "CacheData.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

void CacheData::set(const QString& folderPath, const QDateTime& timestamp, const QStringList& files) {
    mData[folderPath] = CacheEntry{timestamp, files};
}

const CacheEntry* CacheData::get(const QString& folderPath) const {
    auto it = mData.find(folderPath);
    if (it != mData.end())
        return &it.value();
    return nullptr;
}

bool CacheData::readFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return false;

    mData.clear();
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString folder = it.key();
        QJsonObject obj = it.value().toObject();
        QDateTime ts = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        QStringList files;
        for (const auto& f : obj["files"].toArray())
            files << f.toString();
        mData[folder] = CacheEntry{ts, files};
    }
    return true;
}

bool CacheData::writeToFile(const QString& filePath) const {
    QJsonObject root;

    int idx = 0;
    for (auto it = mData.begin(); it!=mData.end(); ++it)
    {
      const auto& key = it.key();
      const auto& value = it.value();
      const auto& ref = value;

      QJsonObject obj;
        obj["timestamp"] = ref.timestamp.toString(Qt::ISODate);
        QJsonArray arr;
        for (const auto& f : ref.files)
            arr.append(f);
        obj["files"] = arr;
        root[key] = obj;
    }
    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson());
    file.close();
    return true;
}
