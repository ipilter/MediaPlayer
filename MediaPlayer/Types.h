#pragma once

#include <QtCore/qglobal.h>
#include <vector>

using Time = qint64;
using Sequence = std::pair<Time, Time>;
using Sequences = std::vector<Sequence>;