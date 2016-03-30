#ifndef ADDBOOKMARKDLG_H
#define ADDBOOKMARKDLG_H

#include <string>
#include <QDialog>

#include <base/inc/platform.h>
#include <base/inc/base_types.h>
#include <base/inc/geometry/geometry_base_types.h>

namespace Ui { class AddBookmarkDlg; }

class AddBookmarkDlg : public QDialog
{
  Q_OBJECT
  
public:
  explicit AddBookmarkDlg(const SDKGeoIntPoint& pos, const double& scale, QWidget *parent = 0);
  ~AddBookmarkDlg();
  
protected:
  virtual void accept();

private:
  // UI
  Ui::AddBookmarkDlg *ui;

  // Position/Scale to mark
  const SDKGeoIntPoint m_position;
  const double         m_scale;
};
#endif // ADDBOOKMARKDLG_H
