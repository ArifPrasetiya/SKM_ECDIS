// MarkUnmarkFeature.h : Interface to mark/unmark feature object over chart
//
#ifndef MARK_UNMARK_FEATURE_INTERFACE_H
#define MARK_UNMARK_FEATURE_INTERFACE_H
#pragma once

// Interface to mark/unmark feature on chart
class MarkUnmarkFeature
{
public:
  // Marks feature specified by the ObjectID.
  virtual bool MarkFeature(sdk::gdb::ObjectID& feature_id) = 0;
  // Removes the feature object marking.
  virtual bool UnmarkFeature() = 0;
};
#endif // MARK_UNMARK_FEATURE_INTERFACE_H
