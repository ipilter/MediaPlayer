#pragma once

#include <QtCore/qglobal.h>
#include <vector>

using VTime = qint64;
using Sequence = std::pair<VTime, VTime>;
using Sequences = std::vector<Sequence>;