#include "featureinfodlg.h"
#include "ui_featureinfodlg.h"

#include <vector>
#include <sstream>

#include <QMessageBox>

#include <base/inc/sdk_any_handler.h>
#include <base/inc/sdk_string_handler.h>
#include <base/inc/sdk_results_enum.h>
#include <datalayer/inc/geodatabase/gdb_workspace.h>
#include <datalayer/inc/geodatabase/gdb_const.h>
#include <datalayer/inc/geodatabase/gdb_relationship.h>

FeatureInfoDlg::FeatureInfoDlg(MarkUnmarkFeature* mark_unmark_feature, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::FeatureInfoDlg),
    m_features(),
    m_mark_unmark_feature(mark_unmark_feature)
{
  ui->setupUi(this);
}

FeatureInfoDlg::~FeatureInfoDlg()
{
  delete ui;
}

void FeatureInfoDlg::FillUpFeaturesInfo(const sdk::gdb::IEnumFeatureSP& features)
{
  try
  {

    while (ui->tree->topLevelItemCount())
    {
      qDeleteAll(ui->tree->topLevelItem(0)->takeChildren());
      delete ui->tree->takeTopLevelItem(0);
    }

    m_features.clear();
    uint tree_item_id = 1;

    if (!features)
      return;

    for (sdk::gdb::IFeatureSP feature; SDK_OK(features->Next(&feature)); feature.Release())
    {
      if (SDK_OK(feature->GetMaster(NULL)))
        continue; // Skip slaves.

      // Feature itself
      sdk::gdb::ObjectID feature_oid;
      if (SDK_FAILED(feature->GetObjectID(feature_oid)))
        throw "Failed to get ObjectID.";

      QTreeWidgetItem* feature_item = NULL;
      if (!AddFeatureToTree(feature, NULL, &feature_item, tree_item_id))
        break;

      // Feature slaves
      sdk::gdb::IEnumFeatureSP slaves;
      if (SDK_OK(feature->GetSlaves(&slaves)))
      {
        for (sdk::gdb::IFeatureSP slave; SDK_OK(slaves->Next(&slave)); slave.Release())
        {
          QTreeWidgetItem* slave_item = NULL;
          if (!AddFeatureToTree(slave, feature_item, &slave_item, tree_item_id))
            continue;
        }
      }

      // Feature relations
      sdk::gdb::IRelationshipCollectionSP asso;
      if (SDK_OK(feature->GetRelationships(sdk::gdb::kRelationshipClass_AssociationMaster,
        sdk::gdb::kRelationshipRole_Any, &asso)))
      {
        sdk::gdb::IObjectSP asso_obj;
        if (SDK_OK(asso->GetRelationship(0, &asso_obj, NULL, NULL)))
        {
          sdk::gdb::IRelationshipCollectionSP relations;
          if (SDK_OK(asso_obj->GetRelationships(
            sdk::gdb::kRelationshipClass_PeerToPeer,
            sdk::gdb::kRelationshipRole_Any, &relations)))
            {
              QTreeWidgetItem* r_item = new QTreeWidgetItem();
              r_item->setText(0, tr("Relationships"));
              r_item->setData(0, Qt::UserRole, 0);
              feature_item->addChild(r_item);

              SDKUInt32 r_count = 0;
              relations->GetRelationshipCount(r_count);
              for (SDKUInt32 r = 0; r < r_count; ++r)
              {
                sdk::gdb::IRelationshipSP rel;
                sdk::gdb::IObjectSP r_obj;
                if (SDK_OK(relations->GetRelationship(r, &r_obj, NULL, NULL)))
                {
                  sdk::gdb::ObjectID r_obj_oid;
                  if (SDK_FAILED(r_obj->GetObjectID(r_obj_oid)))
                    throw "Failed to get an object id.";

                  if (IsEqualObjectID(feature_oid, r_obj_oid))
                    continue; // Do not add the feature itself

                  QTreeWidgetItem* r_obj_item = NULL;
                  if (!AddFeatureToTree(sdk::GetInterfaceT<sdk::gdb::IFeature>(r_obj),
                    r_item, &r_obj_item, tree_item_id))
                    continue;
              }
            }
          }
        }
      }
    }
  }
  catch(const char* message)
  {
    QMessageBox::critical(this, tr("Warning"), tr(message));
  }
  catch(...)
  {
    QMessageBox::critical(this, tr("Warning"), tr("Unexpected exception."));
  }
}

bool FeatureInfoDlg::AddFeatureToTree(const sdk::gdb::IFeatureSP& feature,
  QTreeWidgetItem* parent, QTreeWidgetItem** feature_item, uint& tree_item_id)
{
  bool res = true;

  try
  {
    sdk::gdb::ObjectID oid;
    if (SDK_FAILED(feature->GetObjectID(oid)))
      throw "Failed to get the object id.";

    sdk::gdb::ClassCode class_code;
    if (SDK_FAILED(feature->GetObjectClassCode(class_code)))
      throw "Failed to get the object class code.";

    sdk::gdb::IFeatureDatasetSP dataset;
    if (SDK_FAILED(feature->GetFeatureDataset(&dataset)))
      throw "Failed to get dataset.";

    sdk::gdb::IWorkspaceSP wks;
    if (SDK_FAILED(dataset->GetWorkspace(&wks)))
      throw "Failed to get the workspace by the ObjectID.";

    sdk::ScopedAny prsp;
    if (SDK_FAILED(dataset->GetDatasetProperty(sdk::gdb::kDSP_PRSP, prsp)))
      throw "Failed to get dataset prsp.";
    prsp.ChangeType(kSDKAnyType_Uint32);

    sdk::gdb::IClassCatalogSP feature_catalog;
    if (SDK_FAILED(wks->GetFeatureCatalog(ANY_UI32(&prsp), &feature_catalog)))
      throw "Failed to get a feature catalog.";

    sdk::ScopedString dataset_name;
    if (SDK_FAILED(dataset->GetDatasetName(dataset_name)))
      throw "Failed to get dataset name.";

    std::wostringstream s;
    s << sdk::WideFromSDKString(dataset_name) << L" ";
    s << L"OID[" << DatasetID_WorkspaceID(oid.did) << L"."
      << DatasetID_DatasetID(oid.did) << L"." << ObjectID_Section(oid) << L"."
      << ObjectID_Record(oid) << L"] ";

    std::wstring feature_class_name;
    sdk::gdb::IClassSP feature_class;
    if (SDK_OK(feature_catalog->FindClassByCode(sdk::gdb::kClassType_FeatureClass,
      class_code, &feature_class)))
    {
      sdk::ScopedAny class_name;
      feature_class->GetProperty(sdk::gdb::kClassProperty_Name, class_name);
      if (ANY_IS_STR(&class_name))
        feature_class_name = sdk::WideFromSDKString(*ANY_STR(&class_name));
    }
    if (feature_class_name.empty())
      s << class_code;
    else
      s << feature_class_name;

    if (parent)
      *feature_item = new QTreeWidgetItem();
    else
      *feature_item = new QTreeWidgetItem(ui->tree);

    (*feature_item)->setText(0, QString::fromStdWString(s.str()));
    (*feature_item)->setData(0, Qt::UserRole, tree_item_id);
    if (parent)
      parent->addChild(*feature_item);
    else
      ui->tree->addTopLevelItem(*feature_item);

    m_features[tree_item_id++] = oid;

    sdk::gdb::IAttributeCollectionSP attributes;
    if (SDK_OK(feature->GetAttributes(&attributes)))
    {
      sdk::gdb::IAttributeSP attr;
      for (SDKUInt32 i = 0; SDK_OK(attributes->GetAttribute(i, &attr)); attr.Release(), ++i)
      {
        sdk::gdb::ClassCode attr_class_code;
        if (SDK_FAILED(attr->GetAttributeClassCode(attr_class_code)))
          throw L"Failed to get attribute class code.";

        std::wostringstream attr_s;
        sdk::gdb::IClassSP attr_class_base;
        sdk::ScopedAny attr_class_name;

        if (SDK_OK(feature_catalog->FindClassByCode(
          sdk::gdb::kClassType_AttributeClass, attr_class_code, &attr_class_base))
          && SDK_OK(attr_class_base->GetProperty(sdk::gdb::kClassProperty_Name, attr_class_name))
          && ANY_IS_STR(&attr_class_name))
          attr_s << sdk::WideFromSDKString(*ANY_STR(&attr_class_name));
        else
          attr_s << attr_class_code;

        attr_s << L" - ";

        sdk::ScopedAny v;
        if (SDK_OK(attr->GetValue(v)))
        {
          switch(ANY_TYPE(&v))
          {
          case   kSDKAnyType_Null:
            attr_s << "Undefined";
            break;
          case   kSDKAnyType_SDKTime:
            {
              SDKUInt64 flags;
              attr->GetFlags(flags);

              SDKTimeStruct exploaded;
              if (SDK_OK(SDKTimeToSDKTimeStruct(ANY_TIME(&v), exploaded)))
              {
                if (flags & sdk::gdb::kDTF_VALID_DAY)
                  attr_s <<  exploaded.mday << ".";
                else
                  attr_s <<  "-.";

                if (flags & sdk::gdb::kDTF_VALID_MONTH)
                  attr_s << exploaded.month << ".";
                else
                  attr_s <<  "-.";

                if (flags & sdk::gdb::kDTF_VALID_YEAR)
                  attr_s << exploaded.year << ";";
                else
                  attr_s <<  "-;";

                if (flags & sdk::gdb::kDTF_VALID_HOUR)
                  attr_s << exploaded.hour << ":";
                else
                  attr_s <<  "-:";

                if (flags & sdk::gdb::kDTF_VALID_MINUTE)
                  attr_s << exploaded.minute << ":";
                else
                  attr_s <<  "-:";

                if (flags & sdk::gdb::kDTF_VALID_SECOND)
                  attr_s << exploaded.second;
                else
                  attr_s <<  "-";
              }
            }
            break;
          case kSDKAnyType_SDKStringPtr:
            attr_s << sdk::WideFromSDKString(*ANY_STR(&v));
            break;
          case kSDKAnyType_Uint8:
          case kSDKAnyType_Uint16:
          case kSDKAnyType_Uint32:
          case kSDKAnyType_Uint64:
            {
              v.ChangeType(kSDKAnyType_Uint64);
              bool handled = false;

              do
              {
                if (!attr_class_base)
                  break;
                sdk::gdb::IAttributeClassSP attr_class = sdk::GetInterfaceT<
                  sdk::gdb::IAttributeClass>(attr_class_base);
                if (!attr_class)
                  break;
                sdk::gdb::AttributeClassType attr_class_type;
                if (SDK_FAILED(attr_class->GetAttributeClassType(attr_class_type)))
                  break;
                if (attr_class_type != sdk::gdb::kAttributeClassType_Enum)
                  break;
                sdk::gdb::IClassListedValueSP listed_val_iterface;
                if (SDK_FAILED(attr_class->FindListedValue(ANY_UI64(&v),
                  &listed_val_iterface)))
                  break;
                sdk::ScopedAny listed_val;
                if (SDK_FAILED(listed_val_iterface->GetProperty(
                  sdk::gdb::kClassListedValueProperty_Label, listed_val))
                  || !ANY_IS_STR(&listed_val))
                  break;
                attr_s << sdk::WideFromSDKString(*ANY_STR(&listed_val));
                handled = true;
              }
              while(false);

              if (!handled)
                attr_s << ANY_UI64(&v);
            }
            break;
          case kSDKAnyType_Int8:
          case kSDKAnyType_Int16:
          case kSDKAnyType_Int32:
          case kSDKAnyType_Int64:
            v.ChangeType(kSDKAnyType_Int64);
            attr_s << ANY_I64(&v);
            break;
          case kSDKAnyType_Float:
            attr_s << ANY_FLOAT(&v);
            break;
          case kSDKAnyType_Double:
            attr_s << ANY_DOUBLE(&v);
            break;
          case kSDKAnyType_SDKArrayPtr:
            {
              sdk::ScopedArray arr(*ANY_ARRAY(&v));
              v.Detach();

              std::vector<SDKUInt32> values;
                SDKUInt8* ptr = arr.GetBuffer();

              switch(arr.GetType())
              {
                case kSDKAnyType_Uint8:
                  values.insert(values.begin(), reinterpret_cast<SDKUInt8*>(ptr),
                    reinterpret_cast<SDKUInt8*>(&ptr[arr.GetElementsCount()]));
                  break;
                case kSDKAnyType_Uint16:
                  values.insert(values.begin(), reinterpret_cast<SDKUInt16*>(ptr),
                    reinterpret_cast<SDKUInt16*>(&ptr[arr.GetElementsCount()]));
                  break;
              }

              sdk::gdb::IAttributeClassSP attr_class =
                sdk::GetInterfaceT<sdk::gdb::IAttributeClass>(attr_class_base);

              for (std::vector<SDKUInt32>::iterator it = values.begin();
                it != values.end(); ++it)
              {
                if (attr_class)
                {
                  sdk::ScopedAny listed_val;
                  sdk::gdb::IClassListedValueSP listed_val_iterface;
                  if (SDK_OK(attr_class->FindListedValue(*it, &listed_val_iterface))
                   && SDK_OK(listed_val_iterface->GetProperty(
                    sdk::gdb::kClassListedValueProperty_Label, listed_val))
                   && ANY_IS_STR(&listed_val))
                  {
                    if (!attr_s.str().empty())
                      attr_s << L", ";
                    attr_s << sdk::WideFromSDKString(*ANY_STR(&listed_val));
                  }
                  else
                  {
                    if (!attr_s.str().empty())
                      attr_s << L", ";
                    attr_s << *it;
                  }
                }
              }
            }
            break;
          }
        }

        QTreeWidgetItem* attr_item = new QTreeWidgetItem();
        attr_item->setText(0, QString::fromStdWString(attr_s.str()));
        attr_item->setData(0, Qt::UserRole, 0);
        (*feature_item)->addChild(attr_item);
      }
    }
  }
  catch(const char* message)
  {
    res = false;
    QMessageBox::critical(this, tr("Warning"), tr(message));
  }
  catch(...)
  {
    res = false;
    QMessageBox(QMessageBox::Critical, tr("Warning"), tr("Unexpected exception."));
  }

  return res;
}

void FeatureInfoDlg::OnHighlightFeature()
{
  if (!m_mark_unmark_feature)
    return;

  do
  {
    QList<QTreeWidgetItem*> selected_items = ui->tree->selectedItems();
    if (!selected_items.count())
      break;

    QTreeWidgetItem* item = selected_items.at(0);
    while (item && (0 == item->data(0, Qt::UserRole).toInt()))
      item = item->parent();
    if (!item)
      break;

    FeatureContainer::iterator it = m_features.find(item->data(0, Qt::UserRole).toInt());
    if (it == m_features.end())
      QMessageBox::critical(this, tr("Warning"), tr("Failed to find the feature identifier."));
    else
      m_mark_unmark_feature->MarkFeature(it->second);

    return;
  }
  while (false);

  QMessageBox::warning(this, tr("Warning"), tr("Select an item."));
}

void FeatureInfoDlg::OnClearHighlight()
{
  if (!m_mark_unmark_feature)
    return;

  // Unmarking feature
  m_mark_unmark_feature->UnmarkFeature();
}
