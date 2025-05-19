#pragma once

#include "Types.h"

#include <QSlider>

class Slider : public QSlider
{
  Q_OBJECT

public:
  explicit Slider(Qt::Orientation wOrientation, QWidget* wParent = nullptr);

  void setSequences(const SequenceMap& wSequences);

signals:
  void sequenceSelected(const Sequence* wSequence);
  void sequenceDoubleClicked(const Sequence* wSequence);

protected:
  void paintEvent(QPaintEvent* wEvent) override;
  void mousePressEvent(QMouseEvent* wEvent) override;
  void mouseDoubleClickEvent(QMouseEvent* wEvent) override;

private:
  const QColor& sequenceColor(const SequenceEntry& wSequenceEntry) const;
  const QRect sequenceRect(const SequenceEntry& wSequenceEntry) const;

  SequenceMap mSequences;

  struct SequenceLess
  {
    bool operator()(const SequenceEntry* lhs, const SequenceEntry* rhs) const
    {
      const VTime lhsDuration = lhs->first.second - lhs->first.first;
      const VTime rhsDuration = rhs->first.second - rhs->first.first;

      if (lhsDuration != rhsDuration)
      {
        return lhsDuration < rhsDuration;
      }

      if (lhs->first.first.ms() != rhs->first.first.ms())
      {
        return lhs->first.first.ms() < rhs->first.first.ms();
      }

      return lhs->first.second.ms() < rhs->first.second.ms();
    }
  };

  using OrderedSequenceEntries = std::set<SequenceEntry*, SequenceLess>;

  const int mSequenceRectBottom = 2;
  const int mSequenceRectTop = 10;
  const int mSequenceRectMinLength = 4; // NOTE: even values, see wClickArea in paintEvent
};
