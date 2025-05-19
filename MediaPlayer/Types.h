#pragma once

#include "VTime.h"

#include <QPoint>
#include <QSize>

#include <map>
#include <vector>

using Sequence = std::pair<VTime, VTime>;

enum class OperationState
{
  Ready,
  Processing,
  Succeeded,
  Failed
};

struct SequenceState
{
  OperationState mState = OperationState::Ready;
  bool mSelected = false;
  bool mIsEditing = false;
  QString mFilePath; // just the file name at the time of being cut. user must check if the file really exists!
};

using SequenceMap = std::map<Sequence, SequenceState>;
using SequenceEntry = SequenceMap::value_type;
using SequenceVector = std::vector<Sequence>;

struct Placement
{
  QPoint mPosition;
  QSize mSize;
};
