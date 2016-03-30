#ifndef DATABASEUPDATEHISTORYDLG_H
#define DATABASEUPDATEHISTORYDLG_H

#include <QDialog>
#include <QTreeWidgetItem>

#include <datalayer/inc/geodatabase/gdb_workspace.h>
#include "mark_unmark_feature_interface.h"

namespace Ui { class DatabaseUpdateHistoryDlg; }

class DatabaseUpdateHistoryDlg : public QDialog
{
  Q_OBJECT
  
public:
  explicit DatabaseUpdateHistoryDlg(MarkUnmarkFeature* mark_unmark_feature,
    const sdk::gdb::IWorkspaceFactorySP& wks_factory,
    QWidget *parent = 0);
  ~DatabaseUpdateHistoryDlg();
  
  // Fills up the update history
  void FillUpUpdateHistory();

private slots:
  void on_tree_itemExpanded(QTreeWidgetItem *item);
  void OnHighlightFeature();
  void OnClearHighlight();

private:
  // UI
  Ui::DatabaseUpdateHistoryDlg *ui;

  // Workspace factory
  const sdk::gdb::IWorkspaceFactorySP m_wks_factory;

  // Mark/unmark feature object interface
  MarkUnmarkFeature*                  m_mark_unmark_feature;
};

#endif // DATABASEUPDATEHISTORYDLG_H
