#include <QSettings>
#include <QMessageBox>

#include "portrayalparametersdlg.h"
#include "ui_portrayalparametersdlg.h"

#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_any_handler.h>
#include <base/inc/base_library/base_types_functions.h>
#include <visualizationlayer/inc/visman/portrayal_parameters_interface.h>

#include "utils.h"

// Prepare viewing groups
const char* kDisplayGroupPages[] = 
{
  sdk::vis::kDisplayGroupView_Default_Base,
  sdk::vis::kDisplayGroupView_Default_Standard,
  sdk::vis::kDisplayGroupView_Default_Other,
  sdk::vis::kDisplayGroupView_Default_Texts
};

PortrayalParametersDlg::PortrayalParametersDlg(
  const sdk::vis::ISceneManagerSP& scene_manager, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::PortrayalParametersDlg),
    m_scene_manager(scene_manager)
{
  ui->setupUi(this);

  Initialize();
}

PortrayalParametersDlg::~PortrayalParametersDlg()
{
  delete ui;
}

void PortrayalParametersDlg::OnOK()
{
  ApplySettings();

  QDialog::accept();
}

void PortrayalParametersDlg::OnApply()
{
  ApplySettings();
}

void PortrayalParametersDlg::OnSetViewingGroup()
{
  int sel = ui->myViewingGroups->currentIndex();
  if (-1 == sel)
  {
    QMessageBox::warning(this, tr("Warning"), tr("Please select a viewing group."));
    return;
  }
  QString text = ui->myViewingGroups->itemText(sel);
  if (text.isEmpty())
  {
    QMessageBox::warning(this, tr("Warning"), tr("Please select a viewing group."));
    return;
  }

  QSettings my_viewing_groups(QString::fromStdWString(GetViewingGroupsPath()),
    QSettings::IniFormat);

  my_viewing_groups.beginGroup(text);
  QStringList my_viewing_groups_list = my_viewing_groups.childKeys();
  VisibleViewingGroupContainer visible_viewing_groups;

  for (QStringList::iterator it = my_viewing_groups_list.begin();
    it != my_viewing_groups_list.end(); ++it)
    visible_viewing_groups.insert(it->toStdWString());
  my_viewing_groups.endGroup();

  int count = ui->tabWidget->count();
  for (int i = 0; i < count; ++i)
  {
    ViewingGroupTree* tree = reinterpret_cast<ViewingGroupTree*>(ui->tabWidget->widget(i));
    if (tree)
      tree->SetVisibleViewingGroups(visible_viewing_groups);
  }
}

void PortrayalParametersDlg::OnSaveViewingGroup()
{
  QString text = ui->myViewingGroups->currentText();
  if (text.isEmpty())
  {
    QMessageBox::critical(this, tr("Warning"),
      tr("Please enter a name of the viewing group."));
    return;
  }

  QSettings my_viewing_groups(QString::fromStdWString(GetViewingGroupsPath()),
    QSettings::IniFormat);

  my_viewing_groups.remove(text);
  VisibleViewingGroupContainer visible_viewing_groups;
  int count = ui->tabWidget->count();
  for (int i = 0; i < count; ++i)
  {
    ViewingGroupTree* tree = reinterpret_cast<ViewingGroupTree*>(ui->tabWidget->widget(i));
    if (tree)
      tree->CollectVisibleViewingGroups(visible_viewing_groups);
  }

  my_viewing_groups.beginGroup(text);
  for (VisibleViewingGroupContainer::iterator it =
    visible_viewing_groups.begin(); it != visible_viewing_groups.end(); ++it)
    my_viewing_groups.setValue(QString::fromStdWString(*it), "1");
  my_viewing_groups.endGroup();

  if (-1 == ui->myViewingGroups->findText(text))
    ui->myViewingGroups->addItem(text);
}

void PortrayalParametersDlg::OnRestoreViewingGroup()
{
  sdk::vis::ISceneDisplayGroupsManagerSP display_groups_manager;
  if (SDK_FAILED(m_scene_manager->GetDisplayGroupsManager(display_groups_manager)))
    return;

  VisibleViewingGroupContainer visible_viewing_groups;
  for (size_t i = 0; i < SDK_ARRAY_LENGTH(kDisplayGroupPages); ++i)
  {
    sdk::vis::ISceneDisplayGroupSP display_group;
    if (SDK_OK(display_groups_manager->FindDisplayGroup(sdk::ScopedString("default"),
      sdk::ScopedString(kDisplayGroupPages[i]), display_group))) {
      std::string root(kDisplayGroupPages[i]);
      visible_viewing_groups.insert(std::wstring(root.begin(), root.end()));
      CollectCurrentlyVisibleViewingGroups(display_group, visible_viewing_groups);
    }
  }

  int count = ui->tabWidget->count();
  for (int i = 0; i < count; ++i)
  {
    ViewingGroupTree* tree = reinterpret_cast<ViewingGroupTree*>(ui->tabWidget->widget(i));
    if (tree)
      tree->SetVisibleViewingGroups(visible_viewing_groups);
  }
}

void PortrayalParametersDlg::OnDeleteViewingGroup()
{
  int sel = ui->myViewingGroups->currentIndex();
  if (-1 == sel)
  {
    QMessageBox::warning(this, tr("Warning"), tr("Please select a viewing group."));
    return;
  }
  QString text = ui->myViewingGroups->itemText(sel);
  if (text.isEmpty())
  {
    QMessageBox::warning(this, tr("Warning"), tr("Please select a viewing group."));
    return;
  }

  if (QMessageBox::Yes != QMessageBox::question(this, tr("Warning"),
    tr("Are you sure you want to delete ") + text + "?", QMessageBox::Yes, QMessageBox::No))
    return;

  QSettings my_viewing_groups(QString::fromStdWString(GetViewingGroupsPath()),
    QSettings::IniFormat);

  my_viewing_groups.remove(text);
  ui->myViewingGroups->removeItem(sel);
  ui->myViewingGroups->setCurrentIndex(0);
}

void PortrayalParametersDlg::Initialize()
{
  if (!m_scene_manager)
    return;

  sdk::vis::IPortrayalManagerSP portrayal_manager;
  if (SDK_FAILED(m_scene_manager->GetPortrayalManager(portrayal_manager)) || !portrayal_manager)
    return;

  sdk::vis::IPortrayalParametersSP params;
  if (SDK_FAILED(portrayal_manager->GetPortrayalParameters(params)) || !params)
    return;

  // Getting each of portrayal parameters

  // Shallow contour
  sdk::ScopedAny v;
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_ShallowContour, v)))
    return;
  v.ChangeType(kSDKAnyType_Double);
  ui->shallowContour->setText(QString::number(ANY_DOUBLE(&v)));
  v.Clear();

  // Safety contour
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_SafetyContour, v)))
    return;
  v.ChangeType(kSDKAnyType_Double);
  ui->safetyContour->setText(QString::number(ANY_DOUBLE(&v)));
  v.Clear();

  // Deep contour
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_DeepContour, v)))
    return;
  v.ChangeType(kSDKAnyType_Double);
  ui->deepContour->setText(QString::number(ANY_DOUBLE(&v)));
  v.Clear();

  // Safety depth
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_SafetyDepth, v)))
    return;
  v.ChangeType(kSDKAnyType_Double);
  ui->safetyDepth->setText(QString::number(ANY_DOUBLE(&v)));
  v.Clear();

  // Colour mode
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_ColourMode, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->colorMode->setChecked(ANY_UI32(&v) == sdk::vis::kPP_ColourMode_4Colour ? true : false);
  v.Clear();

  // Symbolized boundaries
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_SymbolisedBoundaries, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->symbolizedBoundaries->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Paper chart symbols
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_PaperChartSymbols, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->paperChartSymbols->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Real light sector length
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_RealLengthLightSectorLegs, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->realSectorLegLength->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Shallow pattern
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_ShallowPattern, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->shallowPattern->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Dangers in shallow waters
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_DangerInShallowWaters, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->dangersInShallowWaters->setChecked(
    sdk::vis::kPP_DangerInShallowWaters_Display == ANY_UI32(&v) ? true : false);
  v.Clear();

  // Use periodic attributes
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_UsePeriodicAttributes, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->usePeriodicAttributes->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Use SCAMIN/SCAMAX attributes
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_UseScaminScamaxAttributes, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->useSCAMINSCAMAX->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Antialiasing mode
  if (SDK_FAILED(params->GetParameter(sdk::vis::kPP_AntiAliasingMode, v)))
    return;
  v.ChangeType(kSDKAnyType_Uint32);
  ui->antialiasing->setChecked(ANY_UI32(&v) ? true : false);
  v.Clear();

  // Viewing groups
  for (size_t c = 0; c < SDK_ARRAY_LENGTH(kDisplayGroupPages); ++c)
  {
    if (!AddDisplayGroupPages(kDisplayGroupPages[c]))
      return;
  }

  QSettings my_viewing_groups(QString::fromStdWString(GetViewingGroupsPath()), QSettings::IniFormat);
  QStringList my_viewing_groups_list = my_viewing_groups.childGroups();

  for (QStringList::iterator it = my_viewing_groups_list.begin();
    it != my_viewing_groups_list.end(); ++it)
    ui->myViewingGroups->addItem(*it);

  return;
}

void PortrayalParametersDlg::ApplySettings()
{
  if (!m_scene_manager)
    return;

  sdk::vis::IPortrayalManagerSP portrayal_manager;
  if (SDK_FAILED(m_scene_manager->GetPortrayalManager(portrayal_manager)) || !portrayal_manager)
    return;

  sdk::vis::IPortrayalParametersSP params;
  if (SDK_FAILED(portrayal_manager->GetPortrayalParameters(params)) || !params)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Shallow contour
  params->SetParameter(sdk::vis::kPP_ShallowContour,
    sdk::ScopedAny(ui->shallowContour->text().toDouble()));

  // Safety contour
  params->SetParameter(sdk::vis::kPP_SafetyContour,
    sdk::ScopedAny(ui->safetyContour->text().toDouble()));

  // Deep contour
  params->SetParameter(sdk::vis::kPP_DeepContour,
    sdk::ScopedAny(ui->deepContour->text().toDouble()));

  // Safety depth
  params->SetParameter(sdk::vis::kPP_SafetyDepth,
    sdk::ScopedAny(ui->safetyDepth->text().toDouble()));

  // Colour mode
  params->SetParameter(sdk::vis::kPP_ColourMode, sdk::ScopedAny(
    static_cast<SDKUInt32>(ui->colorMode->checkState() == Qt::Checked ?
    sdk::vis::kPP_ColourMode_4Colour : sdk::vis::kPP_ColourMode_2Colour)));

  // Symbolized boundaries
  params->SetParameter(sdk::vis::kPP_SymbolisedBoundaries,
    sdk::ScopedAny(ui->symbolizedBoundaries->checkState() == Qt::Checked ?
      true : false));

  // Paper chart symbols
  params->SetParameter(sdk::vis::kPP_PaperChartSymbols,
    sdk::ScopedAny(ui->paperChartSymbols->checkState() == Qt::Checked ?
      true : false));

  // Real light sector length
  params->SetParameter(sdk::vis::kPP_RealLengthLightSectorLegs,
    sdk::ScopedAny(static_cast<SDKUInt32>(
      ui->realSectorLegLength->checkState() == Qt::Checked ? 1 : 0)));

  // Shallow pattern
  params->SetParameter(sdk::vis::kPP_ShallowPattern,
    sdk::ScopedAny(static_cast<SDKUInt32>(
      ui->shallowPattern->checkState() == Qt::Checked ? 1 : 0)));

  // Dangers in shallow waters
  params->SetParameter(sdk::vis::kPP_DangerInShallowWaters,
    sdk::ScopedAny(static_cast<SDKUInt32>(
    ui->dangersInShallowWaters->checkState() == Qt::Checked ?
      sdk::vis::kPP_DangerInShallowWaters_Display :
        sdk::vis::kPP_DangerInShallowWaters_Not_Display)));

  // Use periodic attributes
  params->SetParameter(sdk::vis::kPP_UsePeriodicAttributes,
    sdk::ScopedAny(static_cast<SDKUInt32>(
      ui->usePeriodicAttributes->checkState() == Qt::Checked ? 1 : 0)));

  // Use SCAMIN/SCAMAX attributes
  params->SetParameter(sdk::vis::kPP_UseScaminScamaxAttributes,
    sdk::ScopedAny(static_cast<SDKUInt32>(
      ui->useSCAMINSCAMAX->checkState() == Qt::Checked ? 1 : 0)));

  // Antialiasing mode
  params->SetParameter(sdk::vis::kPP_AntiAliasingMode,
    sdk::ScopedAny(static_cast<SDKUInt32>(
      ui->antialiasing->checkState() == Qt::Checked ? 1 : 0)));

  // Viewing groups
  sdk::vis::ISceneDisplayGroupsManagerSP display_group_manager;
  m_scene_manager->GetDisplayGroupsManager(display_group_manager);
  if (display_group_manager)
  {
    int count = ui->tabWidget->count();
    for (int i = 0; i < count; ++i)
    {
      ViewingGroupTree* tree = reinterpret_cast<ViewingGroupTree*>(ui->tabWidget->widget(i));
      if (tree)
         tree->CollectResult(display_group_manager);
    }
  }

  // Applying new portrayal parameters
  portrayal_manager->SetPortrayalParameters(params);

  // Rendering scene
  sdk::vis::ISceneControlSP scene_control;
  if (SDK_OK(m_scene_manager->GetSceneControl(scene_control)) && scene_control)
    scene_control->UpdateScene(sdk::vis::kUpdateSceneFlags_RenderingAndDisplay);
  if (nativeParentWidget())
    nativeParentWidget()->update();

  QApplication::restoreOverrideCursor();
}

bool PortrayalParametersDlg::AddDisplayGroupPages(const std::string& root_name)
{
  sdk::vis::ISceneDisplayGroupsManagerSP display_groups_manager;
  if (SDK_FAILED(m_scene_manager->GetDisplayGroupsManager(display_groups_manager)))
  {
    QMessageBox::critical(this, tr("Warning"), tr("Failed to get the display groups manager."));
    return false;
  }

  sdk::vis::ISceneDisplayGroupSP display_group;
  if (SDK_FAILED(display_groups_manager->FindDisplayGroup(sdk::ScopedString("default"),
    sdk::ScopedString(root_name), display_group)))
  {
    QMessageBox::critical(this, tr("Warning"), tr("Failed to get a display group."));
    return false;
  }

  sdk::ScopedAny descr;
  if (SDK_FAILED(display_group->GetProperty(
    sdk::vis::kDisplayGroupProperty_ShortDescription, descr)) || !ANY_IS_STR(&descr))
  {
    descr.Clear();
    if (SDK_FAILED(display_group->GetProperty(
      sdk::vis::kDisplayGroupProperty_FullDescription, descr)) || !ANY_IS_STR(&descr))
    {
      descr.Clear();
      if (SDK_FAILED(display_group->GetProperty(
        sdk::vis::kDisplayGroupProperty_Name, descr)) || !ANY_IS_STR(&descr))
      {
        QMessageBox::critical(this, tr("Warning"), tr("Failed to get a display group description."));
        return false;
      }
    }
  }

  // Adding new viewing group
  ViewingGroupTree* viewing_group_tree = new ViewingGroupTree(ui->tabWidget);
  ui->tabWidget->addTab(viewing_group_tree,
    QString::fromStdWString(sdk::WideFromSDKString(*ANY_STR(&descr))));
  viewing_group_tree->AddViewingGroup(display_group);
  return true;
}

void PortrayalParametersDlg::CollectCurrentlyVisibleViewingGroups(
  sdk::vis::ISceneDisplayGroupSP& display_group,
  VisibleViewingGroupContainer& visible_viewing_groups)
{
  if (!display_group)
    return;

  SDKUInt32 children_count = 0;
  display_group->GetChildrenCount(children_count);
  for (SDKUInt32 i = 0; i < children_count; ++i)
  {
    sdk::vis::ISceneDisplayGroupSP child_display_group;
    if (SDK_OK(display_group->GetChildGroup(i, child_display_group)))
    {
      sdk::ScopedAny is_visible;
      if (SDK_OK(child_display_group->GetProperty(
        sdk::vis::kDisplayGroupProperty_IsVisible, is_visible))
        || SDK_FAILED(is_visible.ChangeType(kSDKAnyType_Bool)))
      {
        if (ANY_BOOL(&is_visible))
        {
          sdk::ScopedAny name;
          if (SDK_OK(child_display_group->GetProperty(
            sdk::vis::kDisplayGroupProperty_Name, name)
            || !ANY_IS_STR(&name)))
            visible_viewing_groups.insert(sdk::WideFromSDKString(*ANY_STR(&name)));
        }
      }

      CollectCurrentlyVisibleViewingGroups(child_display_group, visible_viewing_groups);
    }
  }
}

///
ViewingGroupTree::ViewingGroupTree(QWidget* parent)
  : QTreeWidget(parent)
{
  setColumnCount(1);
  setHeaderHidden(true);
  setFont(QApplication::font());
}

ViewingGroupTree::~ViewingGroupTree()
{
}

void ViewingGroupTree::AddViewingGroup(const sdk::vis::ISceneDisplayGroupSP& viewing_group)
{
  disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    this, SLOT(onItemChanged(QTreeWidgetItem*, int)));

  // DeleteAllItems
  while (topLevelItemCount())
  {
    qDeleteAll(topLevelItem(0)->takeChildren());
    delete takeTopLevelItem(0);
  }

  QTreeWidgetItem* item = AddViewingGroup(viewing_group, NULL);
  item->setExpanded(true);

  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
}

QTreeWidgetItem* ViewingGroupTree::AddViewingGroup(
  const sdk::vis::ISceneDisplayGroupSP& viewing_group, QTreeWidgetItem* parent)
{
  sdk::ScopedAny name;
  if (SDK_FAILED(viewing_group->GetProperty(sdk::vis::kDisplayGroupProperty_Name, name)
    || !ANY_IS_STR(&name)))
    return NULL;

  sdk::ScopedAny descr;
  if (SDK_FAILED(viewing_group->GetProperty(
    sdk::vis::kDisplayGroupProperty_FullDescription, descr) || !ANY_IS_STR(&descr)))
    return NULL;

  sdk::ScopedAny is_visible;
  if (SDK_FAILED(viewing_group->GetProperty(
    sdk::vis::kDisplayGroupProperty_IsVisible, is_visible))
    || SDK_FAILED(is_visible.ChangeType(kSDKAnyType_Bool)))
    return NULL;

  std::wstring item_name = sdk::WideFromSDKString(*ANY_STR(&descr));
  if (item_name.empty())
    item_name = sdk::WideFromSDKString(*ANY_STR(&name));

  QTreeWidgetItem* item;
  if (parent)
    item = new QTreeWidgetItem();
  else
    item = new QTreeWidgetItem(this);
  item->setText(0, QString::fromStdWString(item_name));
  item->setData(0, Qt::UserRole, QString::fromStdWString(sdk::WideFromSDKString(*ANY_STR(&name))));
  item->setFlags(Qt::ItemIsUserCheckable);
  item->setCheckState(0, ANY_BOOL(&is_visible) ? Qt::Checked : Qt::Unchecked);
  item->setDisabled(false);
  if (parent)
    parent->addChild(item);
  else
    addTopLevelItem(item);

  SDKUInt32 count = 0;
  viewing_group->GetChildrenCount(count);

  if (count)
  {
    for (SDKUInt32 i = 0; i < count; ++i)
    {
      sdk::vis::ISceneDisplayGroupSP viewing_group_child;
      if (SDK_OK(viewing_group->GetChildGroup(i, viewing_group_child)))
        AddViewingGroup(viewing_group_child, item);
    }
  }

  return item;
}

void ViewingGroupTree::CollectResult(
  const sdk::vis::ISceneDisplayGroupsManagerSP& viewing_group_manager)
{
  if (!viewing_group_manager)
    return;

  return CollectResult(viewing_group_manager, topLevelItem(0));
}

void ViewingGroupTree::CollectResult(
  const sdk::vis::ISceneDisplayGroupsManagerSP& viewing_group_manager,
  QTreeWidgetItem* parent)
{
  if (!parent)
    return;

  sdk::vis::ISceneDisplayGroupSP viewing_group;
  if (SDK_OK(viewing_group_manager->FindDisplayGroup(sdk::ScopedString("default"),
    sdk::ScopedString(parent->data(0, Qt::UserRole).toString().toStdWString()),
    viewing_group)))
    viewing_group->SetProperty(sdk::vis::kDisplayGroupProperty_IsVisible,
      sdk::ScopedAny(parent->checkState(0) == Qt::Checked));

  int count = parent->childCount();
  for (int i = 0; i < count; ++i)
    CollectResult(viewing_group_manager, parent->child(i));
}

void ViewingGroupTree::CollectVisibleViewingGroups(
  VisibleViewingGroupContainer& visible_viewing_groups)
{
  return CollectVisibleViewingGroups(visible_viewing_groups, topLevelItem(0));
}

void ViewingGroupTree::CollectVisibleViewingGroups(
  VisibleViewingGroupContainer& visible_viewing_groups, QTreeWidgetItem* parent)
{
  if (!parent)
    return;

  if (parent->checkState(0) == Qt::Checked)
    visible_viewing_groups.insert(parent->data(0, Qt::UserRole).toString().toStdWString());

  int count = parent->childCount();
  for (int i = 0; i < count; ++i)
    CollectVisibleViewingGroups(visible_viewing_groups, parent->child(i));
}

void ViewingGroupTree::SetVisibleViewingGroups(
  const VisibleViewingGroupContainer& visible_viewing_groups)
{
  disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
             this, SLOT(onItemChanged(QTreeWidgetItem*, int)));

  SetVisibleViewingGroups(visible_viewing_groups, topLevelItem(0));

  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
          this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
}

void ViewingGroupTree::SetVisibleViewingGroups(
  const VisibleViewingGroupContainer& visible_viewing_groups,
  QTreeWidgetItem* parent)
{
  if (!parent)
    return;

  VisibleViewingGroupContainer::const_iterator it = visible_viewing_groups.find(
    parent->data(0, Qt::UserRole).toString().toStdWString());
  parent->setCheckState(0, (it != visible_viewing_groups.end()) ? Qt::Checked : Qt::Unchecked);

  int count = parent->childCount();
  for (int i = 0; i < count; ++i)
    SetVisibleViewingGroups(visible_viewing_groups, parent->child(i));
}

void ViewingGroupTree::onItemChanged(QTreeWidgetItem* item, int column)
{
  disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
             this, SLOT(onItemChanged(QTreeWidgetItem*, int)));

  SetCheckedChildren(item, item->checkState(0) == Qt::Checked);

  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
          this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
}

void ViewingGroupTree::SetCheckedChildren(QTreeWidgetItem* parent, bool checked)
{
  if (!parent)
    return;

  int count = parent->childCount();
  for (int i = 0; i < count; ++i)
  {
    QTreeWidgetItem* item = parent->child(i);
    if (item)
    {
      item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
      SetCheckedChildren(item, checked);
    }
  }
}
