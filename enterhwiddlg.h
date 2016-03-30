#ifndef ENTERHWIDDLG_H
#define ENTERHWIDDLG_H

#include <string>
#include <QDialog>

namespace Ui { class EnterHWIDDlg; }

class EnterHWIDDlg : public QDialog
{
  Q_OBJECT
  
public:
  explicit EnterHWIDDlg(QWidget *parent = 0);
  ~EnterHWIDDlg();
  
  std::wstring GetHWID() const { return m_hw_id; }

protected:
  virtual void accept();

private:
  // UI
  Ui::EnterHWIDDlg *ui;

  // HW_ID
  std::wstring m_hw_id;
};

#endif // ENTERHWIDDLG_H
