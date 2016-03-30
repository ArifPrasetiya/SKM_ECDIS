#include <sstream>

#include <QMessageBox>

#include "databaseupdatehistorydlg.h"
#include "ui_databaseupdatehistorydlg.h"

#include <base/inc/sdk_any_handler.h>
#include <base/inc/sdk_string_handler.h>
#include <base/inc/sdk_results_enum.h>
#include <base/inc/interfaces/sql/sql_interface.h>
#include <datalayer/inc/geodatabase/gdb_update_history.h>
#include <datalayer/inc/senc/senc_exchange_set.h>

#include <QDebug>

using namespace SDK_NAMESPACE;
using namespace SDK_GDB_NAMESPACE;

uint kEmptyObjectReference = -1;
uint kNoObjectReference    = -2;

DatabaseUpdateHistoryDlg::DatabaseUpdateHistoryDlg(MarkUnmarkFeature* mark_unmark_feature,
  const IWorkspaceFactorySP& wks_factory, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::DatabaseUpdateHistoryDlg),
    m_wks_factory(wks_factory),
    m_mark_unmark_feature(mark_unmark_feature)
{
  ui->setupUi(this);
}

DatabaseUpdateHistoryDlg::~DatabaseUpdateHistoryDlg()
{
  delete ui;
}

void DatabaseUpdateHistoryDlg::FillUpUpdateHistory()
{
  while (ui->tree->topLevelItemCount())
  {
    qDeleteAll(ui->tree->topLevelItem(0)->takeChildren());
    delete ui->tree->takeTopLevelItem(0);
  }

  while (ui->list->count())
  {
      delete ui->list->item(0);
  }

  if (!m_wks_factory)
    return;
  IWorkspaceFactoryUtilSP wks_util = 
    m_wks_factory.GetInterface<IWorkspaceFactoryUtil>();
  if (!wks_util)
    return;

  // Getting all of opened workspaces
  IWorkspaceCollectionSP wks_collection;
  if (SDK_FAILED(m_wks_factory->GetWorkspaces(&wks_collection)))
    return;

  // Filling up the update history for each of geodatabase workspaces
  SDKUInt32 wks_count = 0;
  if (SDK_FAILED(wks_collection->GetWorkspaceCount(wks_count)))
    return;

  for (SDKUInt32 c = 0; c < wks_count; ++c)
  {
    // Adding workspace name to tree
    ScopedString wks_name;
    if (SDK_FAILED(wks_collection->GetWorkspaceName(c, wks_name)))
      continue;

    // Getting workspace
    IWorkspaceSP wks;
    if (SDK_FAILED(wks_collection->GetWorkspace(wks_name, &wks)))
      continue;

//    WorkspaceID id;
//    if (SDK_FAILED(wks->GetWorkspaceID(id)))
//          continue;
//    qDebug()<< id;

    QTreeWidgetItem* root_item = new QTreeWidgetItem(ui->tree);
    root_item->setText(0, QString::fromStdWString(WideFromSDKString(wks_name)));
    root_item->setData(0, Qt::UserRole, kEmptyObjectReference);
    ui->tree->addTopLevelItem(root_item);

    QListWidgetItem* root_item1 = new QListWidgetItem(ui->list);
    root_item1->setText(QString::fromStdWString(WideFromSDKString(wks_name)));
    root_item1->setData(Qt::UserRole, kEmptyObjectReference);
    ui->list->addItem(root_item1);

//    // Getting workspace update history
//    IWorkspaceUpdateHistorySP wks_uph;
//    if (SDK_FAILED(wks->GetWorkspaceUpdateHistory(&wks_uph)))
//      continue;

//    // Making new query parameters for update history
//    IQueryParametersSP query_params;
//    if (SDK_FAILED(wks_util->CreateQueryParameters(&query_params)))
//      continue;
//    query_params->SetParameter(kUPH_QUERY_AGEN_ONLY, ScopedAny(true));

//    // Getting update history recordset with given parameters
//    sql::IRowSetSP wks_uph_rs;
//    if (SDK_FAILED(wks_uph->GetWorkspaceUpdateHistory(query_params,
//      &wks_uph_rs)))
//      continue;

//    // Iterating each of records from recordset and adding info to tree
//    SDKUInt64 row = 0;
//    while(SDK_OK(wks_uph_rs->Move(row++)))
//    {
//      ScopedAny agen;
//      if (SDK_FAILED(wks_uph_rs->GetValueByColumnName(
//        senc::kUPHLogTable_AGEN, agen)) || !ANY_IS_STR(&agen))
//        continue;

//      // Adding new child item data
//      QTreeWidgetItem* item = new QTreeWidgetItem();
//      item->setText(0, QString::fromStdWString(agen.GetAsWString()));
//      item->setData(0, Qt::UserRole, kEmptyObjectReference);
//      root_item->addChild(item);

//      // Adding fake item to allow to expand given node
//      QTreeWidgetItem* fake_item = new QTreeWidgetItem();
//      fake_item->setText(0, "");
//      fake_item->setData(0, Qt::UserRole, kEmptyObjectReference);
//      item->addChild(fake_item);
//    }
  }
}

void DatabaseUpdateHistoryDlg::on_tree_itemExpanded(QTreeWidgetItem *item)
{
  try
  {
    QTreeWidgetItem* datasets_item = item->child(0);
    if (!datasets_item)
      return;

    std::wstring text = datasets_item->text(0).toStdWString();
    if (!text.empty())
      return; // Data already filled up, nothing to do

    // It's an agency record, it should be expanded
    // Filling up a list of datasets from update history
    QTreeWidgetItem* agen_item = item;
    std::wstring agen = agen_item->text(0).toStdWString();

    // Getting workspace name
    QTreeWidgetItem* wks_item = agen_item->parent();
    if (!wks_item)
      throw "Failed to get parent item.";

    std::wstring wks_name = wks_item->text(0).toStdWString();

    // Getting required workspace
    if (!m_wks_factory)
      throw "Workspace factory instance is NULL";
    IWorkspaceFactoryUtilSP wks_util = 
      m_wks_factory.GetInterface<IWorkspaceFactoryUtil>();
    if (!wks_util)
      throw "Failed to get IWorkspaceFactoryUtil";
    
    IWorkspaceCollectionSP wks_collection;
    if (SDK_FAILED(m_wks_factory->GetWorkspaces(&wks_collection)))
      throw "Failed to get workspaces from workspace factory";
    IWorkspaceSP wks;
    if (SDK_FAILED(wks_collection->GetWorkspace(ScopedString(wks_name), &wks)))
      throw "Failed to get workspace by name";

    // Getting workspace feature catalog
    IClassCatalogSP feature_catalog;
    if (SDK_FAILED(wks->GetFeatureCatalog(kPRSP_S101,
      &feature_catalog)))
      throw "Failed to get a feature catalog.";

    // Getting workspace update history
    IWorkspaceUpdateHistorySP wks_uph;
    if (SDK_FAILED(wks->GetWorkspaceUpdateHistory(&wks_uph)))
      throw "Failed to get workspace update history.";

    // Making new query parameters for update history
    IQueryParametersSP parameters;
    if (SDK_FAILED(wks_util->CreateQueryParameters(&parameters)))
      throw "Failed to create query parameters.";
    parameters->SetParameter(kUPH_QUERY_AGEN_LIST, ScopedAny(agen));

    // Getting update history recordset for given agency
    sql::IRowSetSP wks_uph_rs;
    if (SDK_FAILED(wks_uph->GetWorkspaceUpdateHistory(parameters, &wks_uph_rs)))
      throw "Failed to get workspace update history.";

    if (SDK_OK(wks_uph_rs->MoveFirst()))
    {
      do
      {
        // Dataset name
        ScopedAny dsnm;
        if (SDK_FAILED(wks_uph_rs->GetValueByColumnName(senc::kUPHLogTable_DSNM,
          dsnm)))
          throw "Failed to get DSNM from the workspace update history.";
        if (!ANY_IS_STR(&dsnm))
          throw "Failed to get DSNM from the workspace update history.";

        // Adding dataset name to tree
        QTreeWidgetItem* dsnm_item = new QTreeWidgetItem();
        dsnm_item->setText(0, QString::fromStdWString(dsnm.GetAsWString()));
        dsnm_item->setData(0, Qt::UserRole, kEmptyObjectReference);
        agen_item->addChild(dsnm_item);

        // Getting dataset update history
        sql::IRowSetSP upd_rs;
        std::string dsnm_str = dsnm.GetAsString();

        if (SDK_FAILED(wks_uph->GetDatasetUpdateHistory(dsnm_str.c_str(), 
          &upd_rs)))
          throw "Failed to get Update history records.";

        if (SDK_OK(upd_rs->MoveFirst()))
        {
          do
          {
            ScopedAny updn;
            if (SDK_FAILED(upd_rs->GetValue(kUPH_UPDN, updn)))
              throw "Failed to get UPDN.";
            updn.ChangeType(kSDKAnyType_Uint32);

            std::wostringstream s;
            s << "UPDN " << ANY_UI32(&updn);

            QTreeWidgetItem* updn_item = new QTreeWidgetItem();
            updn_item->setText(0, QString::fromStdWString(s.str()));
            updn_item->setData(0, Qt::UserRole, kEmptyObjectReference);
            dsnm_item->addChild(updn_item);

            // Parsing each of update entries
            sql::IRowSetSP rec_rs;
            if (SDK_FAILED(wks_uph->GetDatasetUpdateRecords(dsnm_str.c_str(), 
              ANY_UI32(&updn), &rec_rs)))
              throw "Failed to get UPDN records.";

            if (SDK_OK(rec_rs->MoveFirst()))
            {
              do
              {
                std::wostringstream s;

                // RCID
                ScopedAny rcid;
                if (SDK_FAILED(rec_rs->GetValue(kUPH_REC_RCID, rcid)))
                  throw "Failed to get RCID.";
                rcid.ChangeType(kSDKAnyType_Uint32);
                s << "RCID=" << ANY_UI32(&rcid) << ",";

                // RIND
                ScopedAny rind;
                if (SDK_FAILED(rec_rs->GetValue(kUPH_REC_RIND, rind)))
                  throw "Failed to get RIND.";
                rind.ChangeType(kSDKAnyType_Int32);
                s << "RIND=" << ANY_I32(&rind) << ",";

                // RUIN
                ScopedAny ruin;
                if (SDK_FAILED(rec_rs->GetValue(kUPH_REC_RUIN, ruin)))
                  throw "Failed to get RUIN.";
                ruin.ChangeType(kSDKAnyType_Uint32);
                size_t c = 0;
                for (; c < SDK_ARRAY_LENGTH(senc::kRUINMapTable); ++c)
                {
                  if (senc::kRUINMapTable[c].ruin == ANY_UI32(&ruin))
                  {
                    s << L"RUIN=" << senc::kRUINMapTable[c].ruin_name << L",";
                    break;
                  }
                }
                if (c >= SDK_ARRAY_LENGTH(senc::kRUINMapTable))
                  throw "Unknown RUIN.";

                // OBJL
                ScopedAny objl;
                if (SDK_FAILED(rec_rs->GetValue(kUPH_REC_OBJL, objl)))
                  throw "Failed to get OBJL.";
                objl.ChangeType(kSDKAnyType_Uint32);
                s  << L"OBJL=" << ANY_UI32(&objl);

                // Feature class name
                IClassSP feature_class;
                if (SDK_OK(feature_catalog->FindClassByCode(
                  kClassType_FeatureClass, ANY_UI32(&objl),
                  &feature_class)))
                {
                  ScopedAny class_name;
                  feature_class->GetProperty(kClassProperty_Name, class_name);
                  if (ANY_IS_STR(&class_name))
                    s << L" - " << WideFromSDKString(*ANY_STR(&class_name));
                }

                // Adding collected information to tree
                uint rindex = static_cast<uint>(ANY_UI32(&rind));
                QTreeWidgetItem* rec_item = new QTreeWidgetItem();
                rec_item->setText(0, QString::fromStdWString(s.str()));
                rec_item->setData(0, Qt::UserRole, 
                  ((rindex != (uint)-1) ? rindex : kNoObjectReference));
                updn_item->addChild(rec_item);
              }
              while(SDK_OK(rec_rs->MoveNext()));
            }
          }
          while(SDK_OK(upd_rs->MoveNext()));
        }
      }
      while(SDK_OK(wks_uph_rs->MoveNext()));

      delete datasets_item;
    }
  }
  catch(const char* err_mes)
  {
    QMessageBox(QMessageBox::Critical, tr("Warning"), tr(err_mes));
  }
  catch(...)
  {
    QMessageBox(QMessageBox::Critical, tr("Warning"), 
      tr("Unexpected exception."));
  }
}

void DatabaseUpdateHistoryDlg::OnHighlightFeature()
{
  if (!m_mark_unmark_feature)
    return;
  if (!m_wks_factory)
    return;

  do
  {
    QList<QTreeWidgetItem*> selected_items = ui->tree->selectedItems();
    if (!selected_items.count())
      break;

    QTreeWidgetItem* item = selected_items.at(0);
    uint item_data = item->data(0, Qt::UserRole).toUInt();
    if (item_data == kNoObjectReference)
    {
      QMessageBox::warning(this, tr("Warning"), 
        tr("There is no object reference for this update\nProbably it was deleted in the one of the following updates"));
      return;
    }
    if (item_data == kEmptyObjectReference)
      break;
    QTreeWidgetItem* dsnm_item = item->parent()->parent();
    if (!dsnm_item)
      break;
    std::wstring dsnm = dsnm_item->text(0).toStdWString();

    for (QTreeWidgetItem* parent_item = item; 
      NULL != (parent_item = item->parent()); item = parent_item);

    std::wstring wks_name = item->text(0).toStdWString();
    IWorkspaceSP wks;
    IWorkspaceCollectionSP wks_collection;
    if (SDK_SUCCEEDED(m_wks_factory->GetWorkspaces(&wks_collection)))
      wks_collection->GetWorkspace(ScopedString(wks_name), &wks);

    if (!wks)
    {
      QMessageBox::critical(this, tr("Warning"), 
        tr("Failed to get the workspace."));
      return;
    }
    IDatasetSP dataset;
    if (SDK_FAILED(wks->GetDatasetByName(ScopedString(dsnm), &dataset)))
    {
      QMessageBox::critical(this, tr("Warning"), 
        tr("Failed to get the dataset."));
      return;
    }
    DatasetID did;
    dataset->GetDatasetID(did);

    ObjectID oid;
    oid.did = did;
    oid.oid = ObjectID_OID(senc::SECI_FEATS, item_data);

    m_mark_unmark_feature->MarkFeature(oid);
    return;
  }
  while(false);

  QMessageBox::warning(this, tr("Warning"), 
    tr("Select an update record item."));
}

void DatabaseUpdateHistoryDlg::OnClearHighlight()
{
  if (!m_mark_unmark_feature)
    return;

  // Unmarking feature
  m_mark_unmark_feature->UnmarkFeature();
}
