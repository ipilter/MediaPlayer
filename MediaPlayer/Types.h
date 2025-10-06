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
  SequenceState() = default;
  SequenceState(const SequenceState& other)
  {
    mState = other.mState;
    mSelected = other.mSelected;
    mIsEditing = other.mIsEditing;
    mFilePath = other.mFilePath;
    mProcessTimer = other.mProcessTimer;
  }

  OperationState mState = OperationState::Ready;
  bool mSelected = false;
  bool mIsEditing = false;
  QString mFilePath; // just the file name at the time of being cut. user must check if the file really exists!
  VTime mProcessTimer = VTime(0); // current processing time if in processing state
};

using SequenceMap = std::map<Sequence, SequenceState>;
using SequenceEntry = SequenceMap::value_type;
using SequenceVector = std::vector<Sequence>;

struct Placement
{
  QPoint mPosition;
  QSize mSize;
};
