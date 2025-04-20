#pragma once

#include "Runnable.h"

#include <QProcess>

class ProcessTree : public QObject
{
  Q_OBJECT

  using ProcessPtr = std::unique_ptr<QProcess>;

public:
  using Ptr = std::unique_ptr<ProcessTree>;
  using Callback = std::function<void(const QString&)>;

public:
  ProcessTree(Runnable::Ptr runnable
              , Callback onStart = nullptr
              , Callback onFinish = nullptr);
  ~ProcessTree();

  static Ptr create(Runnable::Ptr runnable
                    , Callback onStart = nullptr
                    , Callback onFinish = nullptr);

  void start();

  Ptr& addChild(Runnable::Ptr runnable
                , Callback onStart = nullptr
                , Callback onFinish = nullptr);

private slots:
  void onProcessStarted();
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  Runnable::Ptr mRunnable;

  Callback mOnStart;
  Callback mOnFinish;

  ProcessPtr mProcess;
  std::vector<Ptr> mChildren;
};
