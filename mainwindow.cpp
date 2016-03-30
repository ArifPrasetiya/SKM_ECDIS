#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  m_widget = new step_5_demo_widget(this);
  setCentralWidget(m_widget);

  // Signals
  connect(this, SIGNAL(signalAppClose()), m_widget, SLOT(OnAppClose()));
  connect(m_widget, SIGNAL(signalUpdatePaletteMenuState()), this, SLOT(OnUpdatePaletteMenuState()));
  connect(m_widget, SIGNAL(signalUpdateDisplayMenuState()), this, SLOT(OnUpdateDisplayMenuState()));
  connect(this, SIGNAL(signalChangePalette(int)), m_widget, SLOT(OnChangePalette(int)));
  connect(this, SIGNAL(signalChangeDisplay(int)), m_widget, SLOT(OnChangeDisplay(int)));
  connect(this, SIGNAL(signalOpenTestDatabaseWks()), m_widget, SLOT(OnOpenTestDatabaseWks()));
  connect(this, SIGNAL(signalZoomIn()), m_widget, SLOT(OnZoomIn()));
  connect(this, SIGNAL(signalZoomOut()), m_widget, SLOT(OnZoomOut()));
  connect(this, SIGNAL(signalPortrayalParameters()), m_widget, SLOT(OnPortrayalParameters()));
  connect(m_widget, SIGNAL(signalUpdateStatusBar()), this, SLOT(OnUpdateStatusBar()));
  connect(this, SIGNAL(signalRotate()), m_widget, SLOT(OnRotate()));
  connect(this, SIGNAL(signalOpenDatabaseWorkspace()), m_widget, SLOT(OnOpenDatabaseWorkspace()));
  connect(this, SIGNAL(signalGeodatabaseUpdateHistory()), m_widget, SLOT(OnGeodatabaseUpdateHistory()));
  connect(this, SIGNAL(signalAddBookmark()), m_widget, SLOT(OnAddBookmark()));
  connect(this, SIGNAL(signalBookmarksList()), m_widget, SLOT(OnBookmarksList()));
  connect(this, SIGNAL(signalChangePortrayal(char*)), m_widget, SLOT(OnChangePortrayal(char*)));
  connect(m_widget, SIGNAL(signalUpdatePortrayalMenuState()), this, SLOT(OnUpdatePortrayalMenuState()));

  // Initializing widget
  m_widget->Initialize();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
  OnAppClose();
}

void MainWindow::OnAppClose()
{
  emit signalAppClose();
  close();
}

void MainWindow::OnUpdatePaletteMenuState()
{
  if (!m_widget)
    return;

  sdk::vis::s52::PaletteIndexEnum palette_type = m_widget->GetPaletteType();
  ui->actionDay->setChecked(sdk::vis::s52::kPaletteIndex_DAY == palette_type);
  ui->actionDusk->setChecked(sdk::vis::s52::kPaletteIndex_DUSK == palette_type);
  ui->actionNight->setChecked(sdk::vis::s52::kPaletteIndex_NIGHT == palette_type);
}

void MainWindow::OnUpdateDisplayMenuState()
{
  if (!m_widget)
    return;

  sdk::vis::DisplayModeEnum display_mode = m_widget->GetDisplayModeType();
  ui->actionBase->setChecked(sdk::vis::kDisplayMode_Base == display_mode);
  ui->actionStandart->setChecked(sdk::vis::kDisplayMode_Standard == display_mode);
  ui->actionFull->setChecked(sdk::vis::kDisplayMode_Full == display_mode);
}

void MainWindow::OnDayPalette()
{
  emit signalChangePalette(sdk::vis::s52::kPaletteIndex_DAY);
}

void MainWindow::OnDuskPalette()
{
  emit signalChangePalette(sdk::vis::s52::kPaletteIndex_DUSK);
}

void MainWindow::OnNightPalette()
{
  emit signalChangePalette(sdk::vis::s52::kPaletteIndex_NIGHT);
}

void MainWindow::OnDisplayBase()
{
  emit signalChangeDisplay(sdk::vis::kDisplayMode_Base);
}

void MainWindow::OnDisplayStandart()
{
  emit signalChangeDisplay(sdk::vis::kDisplayMode_Standard);
}

void MainWindow::OnDisplayFull()
{
  emit signalChangeDisplay(sdk::vis::kDisplayMode_Full);
}

void MainWindow::OnOpenTestDatabaseWks()
{
  emit signalOpenTestDatabaseWks();
}

void MainWindow::OnZoomIn()
{
  emit signalZoomIn();
}

void MainWindow::OnZoomOut()
{
  emit signalZoomOut();
}

void MainWindow::OnPortrayalParameters()
{
  emit signalPortrayalParameters();
}

void MainWindow::OnUpdateStatusBar()
{
  if (statusBar() && m_widget)
    statusBar()->showMessage(QString::fromStdWString(m_widget->GetStatusBarText()));
}

void MainWindow::OnRotate()
{
  emit signalRotate();
}

void MainWindow::OnOpenDatabaseWorkspace()
{
  emit signalOpenDatabaseWorkspace();
}

void MainWindow::OnGeodatabaseUpdateHistory()
{
  emit signalGeodatabaseUpdateHistory();
}

void MainWindow::OnAddBookmark()
{
  emit signalAddBookmark();
}

void MainWindow::OnBookmarksList()
{
  emit signalBookmarksList();
}

void MainWindow::OnS52Portrayal()
{
  emit signalChangePortrayal(const_cast<char*>(sdk::config::kPortrayal_s52));
}

void MainWindow::OnINT1Portrayal()
{
  emit signalChangePortrayal(const_cast<char*>(sdk::config::kPortrayal_Int1));
}

void MainWindow::OnUpdatePortrayalMenuState()
{
  if (!m_widget)
    return;

  std::string portrayal_name = m_widget->GetPortrayalName();
  ui->actionS_52->setChecked(std::string(sdk::config::kPortrayal_s52) == portrayal_name);
  ui->actionINT_1->setChecked(std::string(sdk::config::kPortrayal_Int1) == portrayal_name);
}
