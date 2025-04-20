#pragma once

#include "Runnable.h"
#include "Utils.h"

#include <QString>
#include <QObject>

#include <memory>

Runnable::Runnable(const QString& name, const QString& command, const QStringList& arguments)
  : mName(name)
  , mCommand(command)
  , mArguments(arguments)
{
}

Runnable::Ptr Runnable::create(const QString& name, const QString& command, const QStringList& arguments)
{
  return std::make_unique<Runnable>(name, command, arguments);
}

void Runnable::onStarted()
{
}

void Runnable::onFinished()
{
}

const QString& Runnable::name() const
{
  return mName;
}

const QString& Runnable::command() const
{
  return mCommand;
}

const QStringList& Runnable::arguments() const
{
  return mArguments;
}
