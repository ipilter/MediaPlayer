#pragma once

#include "Utils.h"

#include <QString>
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
  Runnable(const QString& name, const QString& command, const QStringList& arguments)
    : mName(name)
    , mCommand(command)
    , mArguments(arguments)
  { }

  virtual ~Runnable() = default;

  static Runnable::Ptr create(const QString& name, const QString& command, const QStringList& arguments)
  {
    return std::make_unique<Runnable>(name, command, arguments);
  }

  virtual void onStarted() {};
  virtual void onFinished() {};

  const QString& name() const
  {
    return mName;
  }

  const QString& command() const
  {
    return mCommand;
  }

  const QStringList& arguments() const
  {
    return mArguments;
  }

protected:
  QString mName;
  QString mCommand;
  QStringList mArguments;
};
