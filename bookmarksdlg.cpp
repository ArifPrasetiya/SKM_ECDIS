#include <QSettings>
#include <QMessageBox>
#include "bookmarksdlg.h"
#include "ui_bookmarksdlg.h"
#include "utils.h"

BookmarksDlg::BookmarksDlg(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::BookmarksDlg),
    m_projection_center(),
    m_scale(0.0f)
{
  ui->setupUi(this);

  PopulateList();
}

BookmarksDlg::~BookmarksDlg()
{
  delete ui;
}

void BookmarksDlg::OnDeleteBookmark()
{
  QList<QListWidgetItem*> sel = ui->listWidget->selectedItems();
  if (!sel.count())
    return;

  if (QMessageBox::Yes != QMessageBox::question(this, tr("Confirmation required"),
    tr("Are you sure you want to delete selected bookmark?"), QMessageBox::Yes, QMessageBox::No))
    return;

  QSettings bmk(QString::fromStdWString(GetBookmarksPath()), QSettings::IniFormat);
  bmk.remove(sel[0]->text());
  PopulateList();
}

void BookmarksDlg::accept()
{
  QList<QListWidgetItem*> sel = ui->listWidget->selectedItems();
  if (!sel.count())
  {
    QMessageBox::warning(this, tr("Warning"), tr("Select a bookmark."));
    return;
  }

  QSettings bmk(QString::fromStdWString(GetBookmarksPath()), QSettings::IniFormat);

  bmk.beginGroup(sel[0]->text());
  m_projection_center.y = bmk.value("lat").toInt();
  m_projection_center.x = bmk.value("lon").toInt();
  m_scale = static_cast<double>(bmk.value("scale").toInt());
  bmk.endGroup();

  return QDialog::accept();
}

void BookmarksDlg::PopulateList()
{
  ui->listWidget->clear();

  QSettings bmk(QString::fromStdWString(GetBookmarksPath()), QSettings::IniFormat);
  QStringList bmk_list = bmk.childGroups();
  for (QStringList::iterator it = bmk_list.begin(); it != bmk_list.end(); ++it)
    ui->listWidget->addItem(*it);
}
