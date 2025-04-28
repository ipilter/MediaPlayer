#pragma once

#include "VTime.h"

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
};

using SequenceMap = std::map<Sequence, SequenceState>;
using SequenceEntry = SequenceMap::value_type;
using SequenceVector = std::vector<Sequence>;
