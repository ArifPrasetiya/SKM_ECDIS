#ifndef BOOKMARKSDLG_H
#define BOOKMARKSDLG_H

#include <string>
#include <QDialog>

#include <base/inc/geometry/geometry_base_types_helpers.h>

namespace Ui { class BookmarksDlg; }

class BookmarksDlg : public QDialog
{
  Q_OBJECT
  
public:
  explicit BookmarksDlg(QWidget *parent = 0);
  ~BookmarksDlg();
  
  inline sdk::GeoIntPoint GetProjectionCenter() const { return m_projection_center; }
  inline SDKUInt32        GetScale() const            { return m_scale; }

private slots:
  void OnDeleteBookmark();

protected:
  virtual void accept();

  void PopulateList();

private:
  // UI
  Ui::BookmarksDlg *ui;

  // Projection center and scale of selected bookmark
  sdk::GeoIntPoint m_projection_center;
  SDKUInt32        m_scale;
};

#endif // BOOKMARKSDLG_H
