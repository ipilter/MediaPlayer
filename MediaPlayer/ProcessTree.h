#pragma once

#include "Runnable.h"

#include <QProcess> // TODO use cpp and use predefine

#include <functional>

class ProcessTreeNode : public QObject
{
  Q_OBJECT

  using ProcessPtr = std::unique_ptr<QProcess>;

public:
  using Ptr = std::unique_ptr<ProcessTreeNode>;
  using Callback = std::function<void(const QString&)>;

public:
  ProcessTreeNode(Runnable::Ptr runnable
                  , Callback onStart = nullptr
                  , Callback onFinish = nullptr)
    : mRunnable(std::move(runnable))
    , mOnStart(onStart)
    , mOnFinish(onFinish)
    , mProcess(std::make_unique<QProcess>(this))
  {
    connect(mProcess.get(), &QProcess::started,
      this, &ProcessTreeNode::onProcessStarted);
    connect(mProcess.get(), &QProcess::finished,
      this, &ProcessTreeNode::onProcessFinished);
  }

  ~ProcessTreeNode()
  {
    mProcess->kill();
    mProcess->waitForFinished();
  }

  static Ptr create(Runnable::Ptr runnable
                    , Callback onStart = nullptr
                    , Callback onFinish = nullptr)
  {
    return std::make_unique<ProcessTreeNode>(
      std::move(runnable), onStart, onFinish);
  }

  void start()
  {
    mProcess->start(mRunnable->command(), mRunnable->arguments());
  }

  Ptr& addChild(Runnable::Ptr runnable
                , Callback onStart = nullptr
                , Callback onFinish = nullptr)
  {
    mChildren.push_back(ProcessTreeNode::create(
      std::move(runnable), onStart, onFinish));
    return mChildren.back();
  }

private slots:
  void onProcessStarted()
  {
    mRunnable->onStarted();

    if (mOnStart)
    {
      mOnStart(QString("started %1").arg(mRunnable->name()));
    }
  }

  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
  {
    mRunnable->onFinished();

    if (mOnFinish)
    {
      mOnFinish(QString("finished %1 with exit code and status: %2, %3")
        .arg(mRunnable->name()).arg(exitCode).arg(exitStatus));
    }

    // start children, if any
    for(auto& child : mChildren)
    {
      child->start();
    }
  }

private:
  Runnable::Ptr mRunnable;

  Callback mOnStart;
  Callback mOnFinish;

  ProcessPtr mProcess;
  std::vector<Ptr> mChildren;
};
