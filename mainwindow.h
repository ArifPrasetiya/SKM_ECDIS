#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>

#include "step_5_demo_widget.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = NULL);
  ~MainWindow();
  
private:
  void closeEvent(QCloseEvent *);

signals:
  void signalAppClose();
  void signalChangePalette(int);
  void signalChangeDisplay(int);
  void signalOpenTestDatabaseWks();
  void signalZoomIn();
  void signalZoomOut();
  void signalPortrayalParameters();
  void signalRotate();
  void signalOpenDatabaseWorkspace();
  void signalGeodatabaseUpdateHistory();
  void signalAddBookmark();
  void signalBookmarksList();
  void signalChangePortrayal(char*);

private slots:
  void OnAppClose();
  void OnUpdatePaletteMenuState();
  void OnUpdateDisplayMenuState();
  void OnDayPalette();
  void OnDuskPalette();
  void OnNightPalette();
  void OnDisplayBase();
  void OnDisplayStandart();
  void OnDisplayFull();
  void OnOpenTestDatabaseWks();
  void OnZoomIn();
  void OnZoomOut();
  void OnPortrayalParameters();
  void OnUpdateStatusBar();
  void OnRotate();
  void OnOpenDatabaseWorkspace();
  void OnGeodatabaseUpdateHistory();
  void OnAddBookmark();
  void OnBookmarksList();
  void OnS52Portrayal();
  void OnINT1Portrayal();
  void OnUpdatePortrayalMenuState();

private:
  // UI
  Ui::MainWindow     *ui;

  // Main widget
  step_5_demo_widget *m_widget;
};

#endif // MAINWINDOW_H
