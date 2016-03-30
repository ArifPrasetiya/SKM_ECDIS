#include "enterhwiddlg.h"
#include "ui_enterhwiddlg.h"

EnterHWIDDlg::EnterHWIDDlg(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::EnterHWIDDlg)
{
  ui->setupUi(this);
}

EnterHWIDDlg::~EnterHWIDDlg()
{
  delete ui;
}

void EnterHWIDDlg::accept()
{
  QLineEdit* line_edit = findChild<QLineEdit*>("lineEdit");
  if (line_edit)
    m_hw_id = line_edit->text().toStdWString();

  return QDialog::accept();
}
