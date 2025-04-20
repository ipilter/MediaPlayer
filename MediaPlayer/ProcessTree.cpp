#include "ProcessTree.h"

#include <QProcess>
#include <QDebug>

#include <functional>

ProcessTree::ProcessTree(Runnable::Ptr runnable, Callback onStart, Callback onFinish)
  : mRunnable(std::move(runnable))
  , mOnStart(onStart)
  , mOnFinish(onFinish)
  , mProcess(std::make_unique<QProcess>(this))
{
  connect(mProcess.get(), &QProcess::started, this, &ProcessTree::onProcessStarted);
  connect(mProcess.get(), &QProcess::finished, this, &ProcessTree::onProcessFinished);
}

ProcessTree::~ProcessTree()
{
  mProcess->kill();
  mProcess->waitForFinished();
}

ProcessTree::Ptr ProcessTree::create(Runnable::Ptr runnable, Callback onStart, Callback onFinish)
{
  return std::make_unique<ProcessTree>(std::move(runnable), onStart, onFinish);
}

void ProcessTree::start()
{
  mProcess->start(mRunnable->command(), mRunnable->arguments());
}

ProcessTree::Ptr& ProcessTree::addChild(Runnable::Ptr runnable, Callback onStart, Callback onFinish)
{
  mChildren.push_back(ProcessTree::create(std::move(runnable), onStart, onFinish));
  return mChildren.back();
}

void ProcessTree::onProcessStarted()
{
  mRunnable->onStarted();

  if (mOnStart)
  {
    mOnStart(QString("started %1").arg(mRunnable->name()));
  }

  QString args = mRunnable->arguments().join(" ");
  qDebug() << "Process started:" << mRunnable->command() << args;
}

void ProcessTree::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  mRunnable->onFinished();

  if (mOnFinish)
  {
    mOnFinish(QString("finished %1 with exit code and status: %2, %3")
      .arg(mRunnable->name()).arg(exitCode).arg(exitStatus));
  }

  // start children, if any
  for (auto& child : mChildren)
  {
    child->start();
  }
}
