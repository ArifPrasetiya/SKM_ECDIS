#include <sstream>
#include <QSettings>
#include <QMessageBox>
#include "addbookmarkdlg.h"
#include "ui_addbookmarkdlg.h"
#include "utils.h"

AddBookmarkDlg::AddBookmarkDlg(const SDKGeoIntPoint& pos, const double& scale, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::AddBookmarkDlg),
    m_position(pos),
    m_scale(scale)
{
  ui->setupUi(this);

  // Forming initial bookmark name
  time_t rawtime;
  time(&rawtime);

  tm exploded;
#if defined(SDK_OS_WIN)
  ::gmtime_s(&exploded, &rawtime);
#elif defined(SDK_OS_POSIX)
  tm* tm_val = ::gmtime(&rawtime);
  memcpy(&exploded, tm_val, sizeof(exploded));
#endif

  std::ostringstream ti_str;
  ti_str << exploded.tm_mday << (exploded.tm_mon + 1)
    << (exploded.tm_year + 1900) << exploded.tm_hour << exploded.tm_min;
  ui->bookmarkName->setText(QString::fromStdString(ti_str.str()));
}

AddBookmarkDlg::~AddBookmarkDlg()
{
  delete ui;
}

void AddBookmarkDlg::accept()
{
  QString text = ui->bookmarkName->text();
  if (text.isEmpty())
    return;

  QSettings bmk(QString::fromStdWString(GetBookmarksPath()), QSettings::IniFormat);

  bmk.remove(text);
  bmk.beginGroup(text);
  bmk.setValue("lat", QString::fromStdWString(Int64ToWString(m_position.y)));
  bmk.setValue("lon", QString::fromStdWString(Int64ToWString(m_position.x)));
  bmk.setValue("scale", QString::fromStdWString(Uint64ToWString(static_cast<SDKUInt64>(m_scale))));
  bmk.endGroup();

  QMessageBox::information(this, tr("Information"), tr("Done"));

  return QDialog::accept();
}
