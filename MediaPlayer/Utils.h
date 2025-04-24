#pragma once

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QRegularExpression>

inline QString prettifyFileName(QString fileName)
{
  fileName.replace(QRegularExpression("[^A-Za-z]"), ".");
  fileName.replace(QRegularExpression("\\.{2,}"), ".");
  return fileName.toLower();
}

inline QString uniqueFileName(const QString& fileName)
{
  size_t wIdx = 0;
  QString wFileName = fileName;
  while (QFile::exists(wFileName))
  {
    wFileName = fileName + QString(".%1").arg(++wIdx);
  }

  return wFileName;
}
