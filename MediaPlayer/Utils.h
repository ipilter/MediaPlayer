#pragma once

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QRegularExpression>

namespace
{
inline QString newName(const QString& baseName, const size_t number, const size_t width, const QString& suffix)
{
  return baseName + "_" + QString("%1").arg(number, static_cast<int>(width), 10, QChar('0')) + "." + suffix;
}
}

inline QString getNewFileName(QString filePath)
{
  const size_t width = 2; // Number width as string in suffix

  while (QFile::exists(filePath))
  {
    const QFileInfo fileInfo(filePath);
    const QRegularExpression re("_(\\d+)$");

    QRegularExpressionMatch match = re.match(fileInfo.completeBaseName());
    if (match.hasMatch())
    {
      // Extract the number, increment it, and construct the new file name
      QString baseName = fileInfo.completeBaseName();
      baseName.chop(match.captured(1).length() + 1); // Remove the underscore and number
      //const QString baseName = fileInfo.completeBaseName().left(fileInfo.completeBaseName().length() - match.captured(1).length() + 1);

      const size_t number = match.captured(1).toLongLong() + 1u;
      filePath = fileInfo.absolutePath() + "/" + newName(baseName, number, width, fileInfo.suffix());
    }
    else
    {
      // If no underscore and number, simply append "_1"
      filePath = fileInfo.absolutePath() + "/" + newName(fileInfo.completeBaseName(), 1u, width, fileInfo.suffix());
    }
  }

  return filePath;
}