#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QStringList>

// Structure to hold cache entry data
struct CacheEntry {
    QDateTime timestamp;
    QStringList files;
};

// CacheData: maps folder path to {timestamp, list of contained files}
class CacheData {
public:
    // Add or update cache entry for a folder
    void set(const QString& folderPath, const QDateTime& timestamp, const QStringList& files);

    // Retrieve cache entry for a folder, returns nullptr if not found
    const CacheEntry* get(const QString& folderPath) const;

    // Read cache data from file
    bool readFromFile(const QString& filePath);

    // Write cache data to file
    bool writeToFile(const QString& filePath) const;

    // Expose the map for iteration if needed
    const QMap<QString, CacheEntry>& entries() const { return mData; }

private:
    QMap<QString, CacheEntry> mData;
};
