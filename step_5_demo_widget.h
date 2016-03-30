#ifndef STEP_5_DEMO_WIDGET_H
#define STEP_5_DEMO_WIDGET_H

#include <string>

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QTime>
#include <QTimer>

#include "featureinfodlg.h"
#include "databaseupdatehistorydlg.h"

#include <base/inc/platform.h>
#include <base/inc/sdk_component_interface.h>
#include <geometry/inc/coordinate_systems/crs_coordinate_transformation.h>
#include <geometry/inc/coordinate_systems/crs_projection.h>
#include <visualizationlayer/inc/visman/scene_manager_interface.h>
#include <visualizationlayer/inc/portrayal/csp/s52_const.h>
#include <visualizationlayer/inc/visman/portrayal_parameters_interface.h>
#include <datalayer/inc/geodatabase/gdb_dataset.h>

#include "s52_resource_manager.h"
#include "mark_unmark_feature_interface.h"
#include "decoration_renderer.h"
#include "coverage_renderer.h"
#include "markedfeaturerenderer.h"

#include "user_bmp_layer_renderer.h" //des

namespace Ui { class step_5_demo_widget; }

class step_5_demo_widget : public QWidget, public MarkUnmarkFeature
{
  Q_OBJECT
  
public:
  explicit step_5_demo_widget(QWidget *parent = 0);
  ~step_5_demo_widget();
  
  void Initialize();

  // Callback from GL widget//des
  void GLDataUpdated() {
    m_user_bmp_layer->SetDirty(true);
    UpdateScene(SDK_VIS_NAMESPACE::kUpdateSceneFlags_StartRendering); }


  // Returns current palette type
  sdk::vis::s52::PaletteIndexEnum GetPaletteType();

  // Returns current display mode type
  sdk::vis::DisplayModeEnum GetDisplayModeType();

  // Returns current S-101 portrayal name
  std::string GetPortrayalName() const { return m_portrayal_name; }

  // Returns status bar text
  std::wstring GetStatusBarText() const { return m_status_bar_text; }

  // Marks feature specified by the ObjectID.
  bool MarkFeature(sdk::gdb::ObjectID& feature_id);
  // Removes the feature object marking.
  bool UnmarkFeature();

private:
  QPaintEngine *paintEngine() const { return 0; } 

  void paintEvent(QPaintEvent* e);
  void resizeEvent(QResizeEvent* e);
  void mouseMoveEvent(QMouseEvent* e);
  void mousePressEvent(QMouseEvent* e);
  void mouseReleaseEvent(QMouseEvent* e);
  void wheelEvent(QWheelEvent* e);

  bool eventFilter(QObject* o, QEvent* e);

  // Update scene
  void UpdateScene(SDKUInt64 flags);

signals:
  void signalUpdatePaletteMenuState();
  void signalUpdateDisplayMenuState();
  void signalUpdatePortrayalMenuState();
  void signalUpdateStatusBar();

private slots:
  void OnAppClose();
  void OnChangePalette(int);
  void OnChangeDisplay(int);
  void OnOpenTestDatabaseWks();
  void OnZoomIn();
  void OnZoomOut();
  void OnPortrayalParameters();
  void OnMouseWheelTimeout();
  void OnRotate();
  void OnOpenDatabaseWorkspace();
  void OnGeodatabaseUpdateHistory();
  void OnAddBookmark();
  void OnBookmarksList();
  void OnChangePortrayal(char*);

protected:
  // Creates new component by factory
  template<class T>
  SDKResult CreateComponent(const sdk::Uuid& clsid, T** component);

  // Creates and initializes Scene Control/Manager
  bool CreateAndInitScene();

  // Resizes the viewport to fit the client window
  void ResizeViewport(const unsigned int& width, const unsigned int& height);
  // Change viewport parameters
  void SetViewportTranslation(float viewport_translate_x, float viewport_translate_y);
  void SetViewportRotationAngle(float rotation_angle);
  void SetViewportZoomRatio(float zoom_ratio);
  // Retrieve viewport parameters
  float GetViewportRotationAngle() const;
  float GetViewportZoomRatio() const;

  // Converts point coordinates from Window coordinate system to Geographic coord. system
  inline sdk::PointF2D WinToGeo(const QPoint& pt);

  // Initiates the scene rendering and displays it
  void RenderScene();

  // Returns path to TDS
  std::wstring GetTestDatabasePath();

  // Opens the database workspace
  void         OpenDatabaseWorkspace(const std::wstring& wks_path,
    const std::wstring& hw_id, const std::wstring& permits_path);
  // Checks, if database is encrypted
  bool         IsDatabaseEncrypted(const std::wstring& root_cat_path);

//  //AddWorkspace
//  SDKResult AddWorkspace(sdk::vis::ISceneManagerSP &scene_manager,
//    const sdk::gdb::IWorkspaceSP &ks);
  /*SDKResult RemoveWorkspace(const sdk::vis::ISceneManager& scene_manager,
    const sdk::gdb::IWorkspaceSP& wks);*/

  // Applies new projection parameters
  void         ApplyProjectionParameters(const double& latitude,
    const double& longitude, const double& scale);

  // Returns workspace factory
  sdk::gdb::IWorkspaceFactorySP GetWorkspaceFactory();

  // Applies new palette type to scene
  void SetPaletteType(
    const sdk::vis::s52::PaletteIndexEnum& palette_index);

  // Applies new display mode
  void SetDisplayMode(
    const sdk::vis::DisplayModeEnum& display_mode);

  // Applies new portrayal mode
  void SetPortrayalName(const std::string& portrayal_name);

  // Returns feature objects under cursor position
  bool GetFeatureObjectsUnderCursorPosition(const QPoint& cursor_position,
    sdk::gdb::IEnumFeatureSP& features);

  // Updates application status bar
  void UpdateStatusBar();

protected:
  // UI
  Ui::step_5_demo_widget *ui;

  // Predefined DPI/DPM values
  const float kDPI;
  const float kDPM;

  // Test database name
  const std::wstring kTestDatabaseName;

  // Test base initial projection center and scale
  const double       kTestBaseInitialLatitude;
  const double       kTestBaseInitialLongitude;
  const double       kTestBaseInitialScale;

  // Scene size
  const sdk::SizeF kSceneSize;

  // Find features under cursor rectangle side size
  const float kFindFeatureUnderCursorRectangleSize;

  // Wheel delta
  const unsigned short                  kWHEEL_DELTA;

  // Scene control and manager
  sdk::vis::ISceneControlSP             m_scene_control;
  sdk::vis::ISceneManagerSP             m_scene_manager;

  // Custom renderers
  // Decoration layer renderer
  DecorationRendererSP                  m_decoration_layer_renderer;
  sdk::vis::ISceneLayerSP               m_decoration_layer;

  // Datasets coverage layer renderer
  CoverageRendererSP                    m_coverage_layer_renderer;
  sdk::vis::ISceneLayerSP               m_coverage_layer;

  // Marked feature object renderer
  MarkedFeatureRendererSP               m_marked_feature_layer_renderer;
  sdk::vis::ISceneLayerSP               m_marked_feature_layer;

  // Workspace factory instance
  sdk::gdb::IWorkspaceFactorySP         m_wks_factory;

  // Feature info dialog
  std::auto_ptr<FeatureInfoDlg>         m_feature_info_dlg;
  // Update history dialog
  std::auto_ptr<DatabaseUpdateHistoryDlg> m_updatehistory_dlg;

  // In case user drags the map by holding left mouse button and moving mouse
  //  this flag will be set to true
  bool                                  m_captured;
  // First captured mouse position
  QPoint                                m_captured_mouse_position;
  // Current mouse position
  QPoint                                m_current_mouse_position;
  // Mouse wheel delta
  short                                 m_mousewheel_delta;
  // Mouse wheel timer
  QTimer                                m_mouse_wheel_timer;
  // Status bar text
  std::wstring                          m_status_bar_text;

  // S-52 resource manager
  S52ResourceManagerSP                  m_s52_resource_manager;

  // Current S-101 portrayal name
  std::string                           m_portrayal_name;

  // User bitmap layer renderer//des
  UserBmpLayerRendererSP              m_user_bmp_layer_renderer;
  SDK_VIS_NAMESPACE::ISceneLayerSP    m_user_bmp_layer;

  // Source of GL data
  GLWidget*                           m_glWidget;

};

#endif // STEP_5_DEMO_WIDGET_H
