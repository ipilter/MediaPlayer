#pragma once

#include "VTime.h"

#include <map>
#include <vector>

using Sequence = std::pair<VTime, VTime>;

enum class SequenceState
{
  Ready,
  Processing,
  Succeeded,
  Failed
};

using SequenceMap = std::map<Sequence, SequenceState>;
using SequenceEntry = SequenceMap::value_type;
using SequenceVector = std::vector<Sequence>;
