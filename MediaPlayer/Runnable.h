#pragma once

#include <QString>
#include <QStringList>
#include <QObject>

#include <memory>

class Runnable : public QObject
{
  Q_OBJECT

public:
  using Ptr = std::unique_ptr<Runnable>;

signals:
  void logMessage(QString);

public:
  Runnable(const QString& name, const QString& command, const QStringList& arguments);
  virtual ~Runnable() = default;

  static Runnable::Ptr create(const QString& name, const QString& command, const QStringList& arguments);

  virtual void onStarted();
  virtual void onFinished();

  const QString& name() const;
  const QString& command() const;
  const QStringList& arguments() const;

protected:
  QString mName;
  QString mCommand;
  QStringList mArguments;
};
