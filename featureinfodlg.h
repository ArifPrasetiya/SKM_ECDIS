#ifndef FEATUREINFODLG_H
#define FEATUREINFODLG_H

#include <map>

#include <QDialog>
#include <QTreeWidgetItem>

#include <datalayer/inc/geodatabase/gdb_dataset.h>
#include <datalayer/inc/geodatabase/gdb_workspace.h>
#include "mark_unmark_feature_interface.h"

namespace Ui { class FeatureInfoDlg; }

class FeatureInfoDlg : public QDialog
{
  Q_OBJECT
  
public:
  explicit FeatureInfoDlg(MarkUnmarkFeature* mark_unmark_feature, QWidget *parent = 0);
  ~FeatureInfoDlg();
  
  // Fills up features info window with data
  void FillUpFeaturesInfo(const sdk::gdb::IEnumFeatureSP& features);

protected:
  bool AddFeatureToTree(const sdk::gdb::IFeatureSP& feature,
    QTreeWidgetItem* parent, QTreeWidgetItem** feature_item,
    uint& tree_item_id);

private slots:
  void OnHighlightFeature();
  void OnClearHighlight();

private:
  // UI
  Ui::FeatureInfoDlg *ui;

  // Collected features
  typedef std::map<uint, sdk::gdb::ObjectID> FeatureContainer;
  FeatureContainer m_features;

  // Mark/unmark feature object interface
  MarkUnmarkFeature* m_mark_unmark_feature;
};

#endif // FEATUREINFODLG_H
