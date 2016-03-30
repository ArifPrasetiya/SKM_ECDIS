#ifndef PORTRAYALPARAMETERSDLG_H
#define PORTRAYALPARAMETERSDLG_H

#include <set>
#include <QDialog>
#include <QTreeWidget>
#include <visualizationlayer/inc/visman/scene_manager_interface.h>

namespace Ui { class PortrayalParametersDlg; }

typedef std::set<std::wstring> VisibleViewingGroupContainer;

class PortrayalParametersDlg : public QDialog
{
  Q_OBJECT
  
public:
  explicit PortrayalParametersDlg(const sdk::vis::ISceneManagerSP& scene_manager,
    QWidget *parent = 0);
  ~PortrayalParametersDlg();
  
private slots:
  void OnOK();
  void OnApply();

  void OnSetViewingGroup();
  void OnSaveViewingGroup();
  void OnRestoreViewingGroup();
  void OnDeleteViewingGroup();

protected:
  // Initializes the dialog instance
  void Initialize();
  // Applies new settings to portrayal parameters
  void ApplySettings();

  // Adds display group to display group tab
  bool AddDisplayGroupPages(const std::string& root_name);

  // Collects currently visible viewing groups
  void CollectCurrentlyVisibleViewingGroups(
    sdk::vis::ISceneDisplayGroupSP& display_group,
    VisibleViewingGroupContainer& visible_viewing_groups);

private:
  // UI
  Ui::PortrayalParametersDlg *ui;

  // Scene manager
  sdk::vis::ISceneManagerSP m_scene_manager;
};

class ViewingGroupTree : public QTreeWidget
{
  Q_OBJECT
public:
  ViewingGroupTree(QWidget* parent);
  ~ViewingGroupTree();

  void AddViewingGroup(const sdk::vis::ISceneDisplayGroupSP& viewing_group);

  void CollectResult(const sdk::vis::ISceneDisplayGroupsManagerSP& viewing_group_manager);

  void CollectVisibleViewingGroups(VisibleViewingGroupContainer& visible_viewing_groups);

  void SetVisibleViewingGroups(const VisibleViewingGroupContainer& visible_viewing_groups);

private slots:
  void onItemChanged(QTreeWidgetItem*, int);

protected:
  QTreeWidgetItem* AddViewingGroup(const sdk::vis::ISceneDisplayGroupSP& viewing_group,
    QTreeWidgetItem* parent);

  void SetCheckedChildren(QTreeWidgetItem* parent, bool checked);

  void CollectResult(
    const sdk::vis::ISceneDisplayGroupsManagerSP& viewing_group_manager,
    QTreeWidgetItem* parent);

  void CollectVisibleViewingGroups(VisibleViewingGroupContainer& visible_viewing_groups,
    QTreeWidgetItem* parent);

  void SetVisibleViewingGroups(const VisibleViewingGroupContainer& visible_viewing_groups,
    QTreeWidgetItem* parent);
};

#endif // PORTRAYALPARAMETERSDLG_H
