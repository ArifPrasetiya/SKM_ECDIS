#include <sstream>
#include <QMessageBox>
#include <QFileDialog>
#include "portrayalparametersdlg.h"
#include "enterhwiddlg.h"
#include "addbookmarkdlg.h"
#include "bookmarksdlg.h"
#include "step_5_demo_widget.h"
#include "ui_step_5_demo_widget.h"

#if defined(SDK_OS_POSIX)
# include <QX11Info>
#endif
#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_any_handler.h>
#include <base/inc/math/matrix3x2.h>
#include <base/inc/base_library/framework_interface.h>

#include <base/inc/geometry/geometry_base_types_helpers.h>
#include <base/inc/util/string_coordinate_conversions.h>
#include <geometry/inc/coordinate_systems/crs_factory.h>
#include <geometry/inc/coordinate_systems/crs_basic_transformation.inl>
#include <visualizationlayer/inc/visman/component_ids.h>
#include <visualizationlayer/inc/visman/helpers/scene_manager_initialization_helpers.h>
#include <datalayer/inc/senc/component_ids.h>

// Lokasi directory menyimpan SENC chart
#define CHART_DIRECTORY QDir::homePath() + "/.MIT/MAP/"

using namespace SDK_NAMESPACE;
using namespace SDK_GDB_NAMESPACE;
using namespace SDK_VIS_NAMESPACE;
using namespace SDK_CRS_NAMESPACE;

step_5_demo_widget::step_5_demo_widget(QWidget *parent)
  : QWidget(parent),
    ui(new Ui::step_5_demo_widget),
    kDPI(96.0f),
    kDPM(kDPI / 2.54f * 100.0f),
    kTestDatabaseName(L"TDS"),
    kTestBaseInitialLatitude(-6.11f),
    kTestBaseInitialLongitude(106.83f),
    kTestBaseInitialScale(50000.0f),
    kSceneSize(2500.0f, 2500.0f), // Scene size should be big enough to fill up the whole client window
    kFindFeatureUnderCursorRectangleSize(10), // pixels
    kWHEEL_DELTA(120),
    m_scene_control(),
    m_scene_manager(),
    m_decoration_layer_renderer(),
    m_decoration_layer(),
    m_coverage_layer_renderer(),
    m_coverage_layer(),
    m_marked_feature_layer_renderer(),
    m_marked_feature_layer(),
    m_wks_factory(),
    m_feature_info_dlg(),
    m_updatehistory_dlg(),
    m_captured(false),
    m_captured_mouse_position(-1, -1),
    m_current_mouse_position(-1, -1),
    m_mousewheel_delta(0),
    m_status_bar_text(L""),
    m_s52_resource_manager(),
    m_portrayal_name()
{
  ui->setupUi(this);

  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_NoBackground);

  qApp->installEventFilter(this);

  connect(&m_mouse_wheel_timer, SIGNAL(timeout()), this, SLOT(OnMouseWheelTimeout()));
}

step_5_demo_widget::~step_5_demo_widget()
{
  // Closing the update history dialog
  if (m_updatehistory_dlg.get())
  {
    m_updatehistory_dlg->reject();
    m_updatehistory_dlg.reset(NULL);
  }

  // Closing the feature info dialog
  if (m_feature_info_dlg.get())
  {
    m_feature_info_dlg->reject();
    m_feature_info_dlg.reset(NULL);
  }

  // Releasing all of previously created SDK components
  m_s52_resource_manager.reset();

  m_wks_factory.Release();

  m_marked_feature_layer_renderer.Release();
  m_marked_feature_layer.Release();

  m_coverage_layer_renderer.Release();
  m_coverage_layer.Release();

  m_decoration_layer_renderer.Release();
  m_decoration_layer.Release();

  m_scene_control.Release();
  m_scene_manager.Release();

  // Destroying UI
  delete ui;
}

void step_5_demo_widget::Initialize()
{
  if (!CreateAndInitScene())
  {
    QMessageBox::critical(this, "Initialization error",
      "Failed to create and initialize scene");
    return;
  }

  // Applying default display mode and palette
  SetDisplayMode(kDisplayMode_Full); // Display mode - Full
  SetPaletteType(s52::kPaletteIndex_DAY); // Day palette

  // Applying S-52 portrayal by default
  SetPortrayalName(std::string(config::kPortrayal_s52));

  QDir dir(CHART_DIRECTORY);
  dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
  QFileInfoList list = dir.entryInfoList();
  QString path;
 if (!(list.empty()))
     {
      qDebug() << "load peta";
     for (int i = 0; i < list.size(); ++i) {
         QFileInfo fileInfo = list.at(i);
         path = fileInfo.absoluteFilePath();

          // Getting the
          std::wstring hw_id;
          std::wstring permits_path;
          // Getting HW_ID and path to permits file, if needed
          do
          {
            if (!IsDatabaseEncrypted(path.toStdWString()))
              break; // Database is not encrypted

            // Getting HW_ID and Permits file
            hw_id = L"56789";

            // HW_ID is specified, PERMITS.TXT file also required
            QString permit = path + "/PERMIT.TXT";
            permits_path = permit.toStdWString();
          }
          while (false);

      // Opening new geodatabase workspace
      OpenDatabaseWorkspace(path.toStdWString(), hw_id, permits_path);
      // Invalidating scene
      RenderScene();
     }
     }

  // First scene render
  m_scene_control->UpdateScene(kUpdateSceneFlags_StartRendering);
  update();
}

s52::PaletteIndexEnum step_5_demo_widget::GetPaletteType()
{
  if (!m_scene_manager)
    return s52::kPaletteIndex_DAY;

  IPortrayalManagerSP portrayal_manager;
  if (SDK_FAILED(m_scene_manager->GetPortrayalManager(portrayal_manager)))
    return s52::kPaletteIndex_DAY;

  IPortrayalParametersSP port_params;
  if (SDK_FAILED(portrayal_manager->GetPortrayalParameters(port_params)))
    return s52::kPaletteIndex_DAY;

  std::string palette_name;
  if (SDK_FAILED(port_params->GetParameter(kPP_DisplayPalette,
    SDKAnyReturnHelper<std::string>(palette_name))))
    return s52::kPaletteIndex_DAY;

  if (std::string(s52::kPaletteNames[0]) == palette_name)
    return s52::kPaletteIndex_DAY;
  else if (std::string(s52::kPaletteNames[1]) == palette_name)
    return s52::kPaletteIndex_DUSK;
  else if (std::string(s52::kPaletteNames[2]) == palette_name)
    return s52::kPaletteIndex_NIGHT;

  return s52::kPaletteIndex_DAY;
}

DisplayModeEnum step_5_demo_widget::GetDisplayModeType()
{
  if (!m_scene_manager)
    return kDisplayMode_Full;

  ISceneDisplayGroupsManagerSP dgroups_manager;
  if (SDK_FAILED(m_scene_manager->GetDisplayGroupsManager(dgroups_manager)) || !dgroups_manager)
    return kDisplayMode_Full;

  SDKUInt32 display_mode = 0;
  if (SDK_FAILED(dgroups_manager->GetProperty(
    kDisplayGroupsManagerProperty_DisplayMode_Short,
    SDKAnyReturnHelper<SDKUInt32>(display_mode))))
    return kDisplayMode_Full;

  return static_cast<DisplayModeEnum>(display_mode);
}

bool step_5_demo_widget::MarkFeature(ObjectID& feature_id)
{
  if (m_marked_feature_layer_renderer)
  {
    sdk::crs::IProjectionSP projection;
    SDKResult get_projection = m_scene_manager->GetProjection(projection);
    if (!IsSDKResultSucceeded(get_projection) || !projection)
      return false;

    GeoIntPoint feature_object_position;
    double dataset_min_disp_scale;
    if (m_marked_feature_layer_renderer->SetMark(feature_id, projection,
      feature_object_position, dataset_min_disp_scale) && projection)
    {
      // Applying new projection center and scale
      sdk::ISDKParametersSP scene_parameters;
      if (SDK_FAILED(m_scene_control->GetSceneParameters(scene_parameters)))
        return false;
      double curr_scale = 0.0f;
      if (SDK_FAILED(scene_parameters->GetParameterT(sdk::vis::kSceneParameters_Scale, curr_scale)))
        return false;
      double req_scale = curr_scale;
      if (req_scale > dataset_min_disp_scale)
        req_scale = dataset_min_disp_scale;

      ApplyProjectionParameters(DegFromGeoInt(feature_object_position.y),
        DegFromGeoInt(feature_object_position.x), req_scale);

      // Invalidating the scene
      RenderScene();
      return true;
    }
  }

  return false;
}

bool step_5_demo_widget::UnmarkFeature()
{
  if (m_marked_feature_layer_renderer && m_marked_feature_layer)
  {
    if (m_marked_feature_layer_renderer->RemoveMark())
    {
      // Invalidating the layer
      m_marked_feature_layer->SetDirty(true);

      m_scene_control->UpdateScene(kUpdateSceneFlags_StartRendering);
      update();

      return true;
    }
  }

  return false;
}

void step_5_demo_widget::paintEvent(QPaintEvent* evt)
{
  if (m_scene_control)
    m_scene_control->UpdateScene(kUpdateSceneFlags_Display);
}

void step_5_demo_widget::resizeEvent(QResizeEvent* e)
{
  // Resizing and centering the viewport
  ResizeViewport(e->size().width(), e->size().height());

  // Rendering scene
  RenderScene();
}

void step_5_demo_widget::mouseMoveEvent(QMouseEvent* e)
{
  if (m_captured && (e->buttons() & Qt::LeftButton))
  {
    float viewport_translate_x =  static_cast<float>(m_captured_mouse_position.x() - e->pos().x());
    float viewport_translate_y = -static_cast<float>(m_captured_mouse_position.y() - e->pos().y());
    m_current_mouse_position = e->pos();

    // Applying viewport shift
    SetViewportTranslation(viewport_translate_x, viewport_translate_y);

    // Invalidating the window
    update();
  }
  else
  {
    RenderScene();
  }

  m_current_mouse_position = e->pos();
  UpdateStatusBar();
}

void step_5_demo_widget::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    // Starting the viewport dragging
    m_captured = true;
    m_captured_mouse_position = e->pos();
  }
}

void step_5_demo_widget::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    do
    {
      // Stopping the viewport dragging
      m_captured = false;

      // Invalidating scene
      RenderScene();
    }
    while (false);
  }
  else if (e->button() == Qt::RightButton)
  {
    // Showing feature info dialog
    if (!m_feature_info_dlg.get())
    {
      m_feature_info_dlg.reset(new FeatureInfoDlg(this, this));
      m_feature_info_dlg->setModal(false);
    }

    // Trying to find features under cursor
    IEnumFeatureSP features;
    if (GetFeatureObjectsUnderCursorPosition(e->pos(), features))
      m_feature_info_dlg->FillUpFeaturesInfo(features);

    m_feature_info_dlg->show();
  }
}

void step_5_demo_widget::wheelEvent(QWheelEvent* e)
{
  // Zooming in/out the viewport by using zoom factor
  m_mousewheel_delta += e->delta();
  if (abs(m_mousewheel_delta) >= (kWHEEL_DELTA))
  {
    int ticks = (m_mousewheel_delta + 1) / kWHEEL_DELTA;
    m_mousewheel_delta -= ticks * kWHEEL_DELTA;

    // Applying new zoom factor
    float current_zoom = GetViewportZoomRatio();
    float zoom_ratio = current_zoom + (current_zoom * 0.08f * ticks);
    SetViewportZoomRatio(zoom_ratio);

    // Restarting the mousewheel timer
    m_mouse_wheel_timer.start(400);

    // Updating status bar
    UpdateStatusBar();

    // Invalidating the window
    update();
  }
}

bool step_5_demo_widget::eventFilter(QObject* o, QEvent* e)
{
  if (o == this || o == parent())
  {
    if (e->type() == QEvent::MouseMove)
      mouseMoveEvent(reinterpret_cast<QMouseEvent*>(e));
  }
  return false;
}

void step_5_demo_widget::OnAppClose()
{
}

void step_5_demo_widget::OnChangePalette(int palette_id)
{
  SetPaletteType(static_cast<s52::PaletteIndexEnum>(palette_id));

  RenderScene();
}

void step_5_demo_widget::OnChangeDisplay(int display_type)
{
  SetDisplayMode(static_cast<DisplayModeEnum>(display_type));
  RenderScene();
}

void step_5_demo_widget::OnOpenTestDatabaseWks()
{


  // Getting test database workspace path
  std::wstring test_db_wks_path = GetTestDatabasePath();
  if (test_db_wks_path.empty())
    return;

  // Opening it
  OpenDatabaseWorkspace(test_db_wks_path, L"", L"");

  // Applying test database workspace projection parameters
  ApplyProjectionParameters(kTestBaseInitialLatitude, kTestBaseInitialLongitude,
    kTestBaseInitialScale);

  // Rendering scene
  RenderScene();
}

void step_5_demo_widget::OnZoomIn()
{
  // Zooming in map
  if (!m_scene_control)
    return;

  sdk::ISDKParametersSP scene_parameters;
  if (SDK_FAILED(m_scene_control->GetSceneParameters(scene_parameters)))
    return;

  // Getting current scale
  double scale = 0.0f;
  if (SDK_FAILED(scene_parameters->GetParameter(
    sdk::vis::kSceneParameters_Scale, sdk::SDKAnyReturnHelper<double>(scale))))
    return;

  // Reducing it by 1.5 times
  scale /= 1.5f;
  if (scale < 10.0f)
    scale = 10.0f;
  if (SDK_FAILED(scene_parameters->SetParameter(
    sdk::vis::kSceneParameters_Scale, sdk::ScopedAny(scale))))
    return;

  // And applying to scene control
  m_scene_control->SetSceneParameters(scene_parameters);

  // And rendering scene with new parameters
  RenderScene();
}

void step_5_demo_widget::OnZoomOut()
{
  // Zooming out map
  if (!m_scene_control)
    return;

  sdk::ISDKParametersSP scene_parameters;
  if (SDK_FAILED(m_scene_control->GetSceneParameters(scene_parameters)))
    return;

  // Getting current scale
  double scale = 0.0f;
  if (SDK_FAILED(scene_parameters->GetParameter(
    sdk::vis::kSceneParameters_Scale, sdk::SDKAnyReturnHelper<double>(scale))))
    return;

  // Increasing it by 1.5 times
  scale *= 1.5f;
  if (scale > 100000000.0f)
    scale = 100000000.0f;
  if (SDK_FAILED(scene_parameters->SetParameter(
    sdk::vis::kSceneParameters_Scale, sdk::ScopedAny(scale))))
    return;

  // And applying to scene control
  m_scene_control->SetSceneParameters(scene_parameters);

  // And rendering scene with new parameters
  RenderScene();
}

void step_5_demo_widget::OnPortrayalParameters()
{
  PortrayalParametersDlg dlg(m_scene_manager, this);
  dlg.setModal(true);
  dlg.exec();
}

void step_5_demo_widget::OnMouseWheelTimeout()
{
  m_mouse_wheel_timer.stop();
  do
  {
    // And redrawing the scene
    RenderScene();
  }
  while (false);
}

void step_5_demo_widget::OnRotate()
{
  // Rotating the viewport
  float current_rotation_angle = GetViewportRotationAngle();
  current_rotation_angle += 45.0f;
  if (current_rotation_angle >= 360.f)
    current_rotation_angle -= 360.f;

  SetViewportRotationAngle(current_rotation_angle);

  // Rendering scene
  RenderScene();
}

void step_5_demo_widget::OnOpenDatabaseWorkspace()
{
  QString root_cat_path = QFileDialog::getOpenFileName(this,
    tr("Open SENC Geodatabase"), QString(), tr("SENC Geodatabase (root.cat)"));
  if (root_cat_path.isEmpty())
    return;
  QString wks_path = QFileInfo(root_cat_path).path();
  if (wks_path.isEmpty())
    return;

  // Getting the HW_ID and path to permits file, if needed
  std::wstring hw_id;
  std::wstring permits_path;

  do
  {
    if (!IsDatabaseEncrypted(root_cat_path.toStdWString()))
      break; // Database is not encrypted

    // Getting HW_ID and Permits file
    EnterHWIDDlg hw_id_dlg(this);
    hw_id_dlg.setModal(true);
    hw_id_dlg.exec();
    if (QDialog::Accepted != hw_id_dlg.result())
      return;

    hw_id = hw_id_dlg.GetHWID();
    if (hw_id.empty())
      return;

//    hw_id = L"56789";//arif

    // HW_ID is specified, PERMITS.TXT file also required
    QStringList name_filters;
    name_filters << tr("Permits file (*.txt)");
    name_filters << tr("All files (*.*)");

    permits_path = QFileDialog::getOpenFileName(this, tr("Path to permits"),
      wks_path, tr("Permits file (*.txt);;All files (*.*)")).toStdWString();
  }
  while (false);

  // Opening new geodatabase workspace
  OpenDatabaseWorkspace(wks_path.toStdWString(), hw_id, permits_path);
  // OpenDatabaseWorkspace(wks_path.toStdWString(), hw_id, L"/home/sembada/UJI_COBA_ECDIS_2015/PERMIT/PERMIT.TXT");//arif

  // Invalidating scene
  RenderScene();
}

void step_5_demo_widget::OnGeodatabaseUpdateHistory()
{
  if (!m_updatehistory_dlg.get())
  {
    m_updatehistory_dlg.reset(new DatabaseUpdateHistoryDlg(this,
      GetWorkspaceFactory(), this));
    m_updatehistory_dlg->setModal(false);
  }

  // Filling up the update history info
  m_updatehistory_dlg->FillUpUpdateHistory();

  // Showing the window
  m_updatehistory_dlg->show();
}

void step_5_demo_widget::OnAddBookmark()
{
  QRect r = geometry();

  // Getting position of screen center
  PointF2D geo_pos = WinToGeo(QPoint(r.width() / 2, r.height() / 2));
  GeoIntPoint gip(sdk::GeoIntFromDeg(geo_pos.x), sdk::GeoIntFromDeg(geo_pos.y));

  // Getting current scale from projection
  if (!m_scene_control)
    return;
  ISDKParametersSP scene_param;
  if (SDK_FAILED(m_scene_control->GetSceneParameters(scene_param)))
    return;
  double scale = 0.0;
  if (SDK_FAILED(scene_param->GetParameter(kSceneParameters_Scale, 
    sdk::SDKAnyReturnHelper<double>(scale))))
    return;

  // Showing the add bookmark dialog
  AddBookmarkDlg dlg(gip, scale, this);
  dlg.setModal(true);
  dlg.exec();
}

void step_5_demo_widget::OnBookmarksList()
{
  BookmarksDlg dlg(this);
  dlg.setModal(true);
  dlg.exec();
  if (QDialog::Accepted != dlg.result())
    return;

  // Applying new projection center and scale
  GeoIntPoint gip = dlg.GetProjectionCenter();
  double scale = static_cast<double>(dlg.GetScale());

  // Applying new projection center position to scene control
  SDKGeoPoint gp = {{DegFromGeoInt(gip.x)}, {DegFromGeoInt(gip.y)}};
  ApplyProjectionParameters(gp.y, gp.x, scale);

  // Invalidating scene
  RenderScene();
}

void step_5_demo_widget::OnChangePortrayal(char* portrayal_name)
{
  std::string name;
  if (portrayal_name)
    name = std::string(portrayal_name);
  if (name.empty())
    return;

  SetPortrayalName(name);
  RenderScene();
}

template<class T>
SDKResult step_5_demo_widget::CreateComponent(const Uuid& clsid, T** component)
{
  if (NULL == component)
    return kSDKResult_NULLPointer;

  ISDKComponentSP obj;
  SDKResult res = SDKCreateComponentInstance(NULL, clsid, &obj);
  if (SDK_FAILED(res))
    return res;
  return obj->GetInterface(T::IID(), reinterpret_cast<void**>(component));
}

bool step_5_demo_widget::CreateAndInitScene()
{
  // Creating scene manager
  if (SDK_FAILED(CreateComponent<ISceneManager>(kSceneManagerCID, 
    &m_scene_manager)))
    return false;

  gfx::OSWindow window;
#if defined(SDK_OS_WIN)
  window.hwnd = winId();
#elif defined(SDK_OS_POSIX)
  window.display = x11Info().display();
  window.window = winId();
  window.visual_info = NULL;
#else
#endif
  SceneManagerFlags multithreaded_rendering = kSceneManagerFlag_NoFlag;

  ISceneManagerInitialParametersSP initial_parameters;
  if (SDK_FAILED(SceneManagerInitParametersHelper::CreateSceneManagerInitialParameters(
    &window,                                              // Window to attach scene to
    kSceneSize,                                           // Scene size
    kDPM,                                                 // Predefined DPM value
    multithreaded_rendering | kSceneManagerFlag_OutputTechnologyType_Auto // Auto choose best graphic platform
    | kSceneManagerFlag_OutputType_Window,      // Output destination is window)
    initial_parameters)))                                 // [Out] Initialization parameters for scene manager
    return false;

  // Now initializing scene with our window
  if (SDK_FAILED(m_scene_manager->Initialize(initial_parameters)))
    return false;

  // Getting scene control
  if (SDK_FAILED(m_scene_manager->GetSceneControl(m_scene_control)))
    return false;

  // Creating an instance of S-52 resource manager
  m_s52_resource_manager.reset(new S52ResourceManager());
  if (!m_s52_resource_manager)
    return false;

  // Applying current palette
  m_s52_resource_manager->SetPalette(GetPaletteType());

  // Adding all of custom layers to scene
  ISceneLayersManagerSP layers_manager;
  if (SDK_FAILED(m_scene_manager->GetSceneLayersManager(layers_manager)))
    return false;
  // Coverage renderer
  m_coverage_layer_renderer = CoverageRendererSP(
    new CoverageRenderer(m_s52_resource_manager, GetWorkspaceFactory()));
  if (!m_coverage_layer_renderer)
    return false;
  if (SDK_FAILED(layers_manager->CreateLayer(
    ScopedString(L"coverage"),                           // name
    kSceneLayerPriority_Chart_Decoration + 1,       // priority
    PointF2D(0, 0),                                      // position on scene
    SizeF(kSceneSize.width, kSceneSize.height),          // size of the layer
    kSceneLayerFlag_NoFlags,                        // flags
    ScopedString(L""),                                   // portrayal layer name
    ScopedString(L""),                                   // display groups filter
    ScopedAny(m_coverage_layer_renderer),                // renderer
    m_coverage_layer, NULL)))                                 // reference to the layer, created during addition
    return false;
  if (SDK_FAILED(layers_manager->AddLayer(m_coverage_layer, kSceneLayerID_Undefined)))
    return false;

  // Marked feature renderer
  m_marked_feature_layer_renderer = MarkedFeatureRendererSP(
    new MarkedFeatureRenderer(GetWorkspaceFactory(), m_s52_resource_manager));
  if (!m_marked_feature_layer_renderer)
    return false;
  if (SDK_FAILED(layers_manager->CreateLayer(
    ScopedString(L"marked_feature"),                         // name
    kSceneLayerPriority_Chart_Decoration + 2,           // priority
    PointF2D(0, 0),                                          // position on scene
    SizeF(kSceneSize.width, kSceneSize.height),              // size of the layer
    kSceneLayerFlag_NoFlags,                            // flags
    ScopedString(L""),                                       // portrayal layer name
    ScopedString(L""),                                       // display groups filter
    ScopedAny(m_marked_feature_layer_renderer),              // renderer
    m_marked_feature_layer, NULL)))                               // reference to the layer, created during addition
    return false;
  if (SDK_FAILED(layers_manager->AddLayer(m_marked_feature_layer, kSceneLayerID_Undefined)))
    return false;

  // Decoration layer
  m_decoration_layer_renderer = DecorationRendererSP(
    new DecorationRenderer(m_s52_resource_manager));
  if (!m_decoration_layer_renderer)
    return false;
  if (SDK_FAILED(layers_manager->CreateLayer(
    ScopedString(L"decoration"),                   // name
    kSceneLayerPriority_Chart_Decoration + 1, // priority
    PointF2D(0, 0),                                // position on scene
    SizeF(kSceneSize.width, kSceneSize.height),    // size of the layer
    kSceneLayerFlag_BindToViewport,           // layer is binded to viewport
    ScopedString(L""),                             // portrayal layer name
    ScopedString(L""),                             // display groups filter
    ScopedAny(m_decoration_layer_renderer),        // renderer
    m_decoration_layer, NULL)))                         // reference to the layer, created during addition
    return false;
  if (SDK_FAILED(layers_manager->AddLayer(m_decoration_layer, kSceneLayerID_Undefined)))
    return false;

//  // Add event listener
//  ISceneControlCallbackSP events_listener(
//    new SceneControlEventsListener(this));
//  res = AdviseToConnectionPoint(m_scene_control,
//    ISceneControlCallback::IID(),
//    events_listener,
//    m_scene_listener_cookie);
//  if (SDK_FAILED(res))
//    return false;
//Radar layer
  m_user_bmp_layer_renderer = UserBmpLayerRendererSP(new UserBmpLayerRenderer());
  if (!m_user_bmp_layer_renderer)
    return false;
  if (SDK_FAILED(layers_manager->CreateLayer(
    ScopedString(L"user_bmp_layer"),                         // name
    kSceneLayerPriority_Chart_Decoration + 1,                                   // priority
    PointF2D(0, 0),                             // position on scene
    SizeF(kSceneSize.width, kSceneSize.height),                                // size of the layer
    kSceneLayerFlag_PortrayalDependent,         // layer is binded to viewport
    ScopedString(kMainChartPortrayalLayerName), // portrayal layer name
    ScopedString(L""),                          // display groups filter
    ScopedAny(m_user_bmp_layer_renderer),                        // renderer
     m_user_bmp_layer, NULL)))                         // reference to the layer, created during addition
    return false;
  if (SDK_FAILED(layers_manager->AddLayer(m_user_bmp_layer, kSceneLayerID_Undefined)))
    return false;

  m_glWidget = new GLWidget(this, m_user_bmp_layer_renderer);
  m_glWidget->show();
  m_glWidget->hide();



  return true;
}

void step_5_demo_widget::UpdateScene(SDKUInt64 flags) {
  m_scene_control->UpdateScene(flags);
}

void step_5_demo_widget::ResizeViewport(
  const unsigned int& width, const unsigned int& height)
{
  if (!m_scene_control)
    return;

  // Getting scene viewport
  scene::IScene2DViewportBaseSP viewport;
  if (SDK_FAILED(m_scene_control->GetViewport(viewport)))
    return;

  // Updating viewport bounds
  RectF2D viewport_bounds(0.0f, 0.0f,
    static_cast<float>(width), static_cast<float>(height));
  viewport->SetBounds(viewport_bounds);

  // Informing coverage layer renderer about viewport change
  if (m_coverage_layer_renderer)
  {
    m_coverage_layer_renderer->SetViewportBounds(
      sdk::RectF2D(
      -static_cast<float>(width) / 2.0f, 
      -static_cast<float>(height) / 2.0f,
      static_cast<float>(width), 
      static_cast<float>(height)));
  }
}

void step_5_demo_widget::SetViewportTranslation(float viewport_translate_x, 
  float viewport_translate_y)
{
  if (!m_scene_control)
    return;

  scene::IScene2DViewportBaseSP viewport;
  if (SDK_FAILED(m_scene_control->GetViewport(viewport)))
    return;

  // Using simple viewport
  scene::IScene2DViewportSimpleSP viewport_simple;
  if (SDK_FAILED(viewport->GetInterface(
    scene::IScene2DViewportSimple::IID(),
    reinterpret_cast<void**>(&viewport_simple))))
    return;

  // Pivot point is in scene center
  viewport_simple->SetTranslate(viewport_translate_x, viewport_translate_y, NULL);
}

void step_5_demo_widget::SetViewportRotationAngle(float rotation_angle)
{
  if (!m_scene_control)
    return;

  scene::IScene2DViewportBaseSP viewport;
  if (SDK_FAILED(m_scene_control->GetViewport(viewport)))
    return;

  // Using simple viewport
  scene::IScene2DViewportSimpleSP viewport_simple;
  if (SDK_FAILED(viewport->GetInterface(
    scene::IScene2DViewportSimple::IID(),
    reinterpret_cast<void**>(&viewport_simple))))
    return;

  // Pivot point is in scene center
  viewport_simple->SetRotate(rotation_angle, NULL);
}

void step_5_demo_widget::SetViewportZoomRatio(float zoom_ratio)
{
  if (!m_scene_control)
    return;

  scene::IScene2DViewportBaseSP viewport;
  if (SDK_FAILED(m_scene_control->GetViewport(viewport)))
    return;

  // Using simple viewport
  scene::IScene2DViewportSimpleSP viewport_simple;
  if (SDK_FAILED(viewport->GetInterface(
    scene::IScene2DViewportSimple::IID(),
    reinterpret_cast<void**>(&viewport_simple))))
    return;

  // Pivot point is in scene center
  viewport_simple->SetScale(zoom_ratio, NULL);
}

float step_5_demo_widget::GetViewportRotationAngle() const
{
  if (!m_scene_control)
    return 0.0f;

  sdk::vis::scene::IScene2DViewportBaseSP viewport;
  if (SDK_FAILED(m_scene_control->GetViewport(viewport)) || !viewport)
    return 0.0f;

  // Using simple viewport
  sdk::vis::scene::IScene2DViewportSimpleSP viewport_simple;
  if (SDK_FAILED(viewport->GetInterface(
    sdk::vis::scene::IScene2DViewportSimple::IID(), 
    reinterpret_cast<void**>(&viewport_simple))) || !viewport_simple) 
    return 0.0f;

  float rotation_angle = 1.0f;
  if (SDK_FAILED(viewport_simple->GetRotate(&rotation_angle, NULL)))
    return 0.0f;
  
  return rotation_angle;
}

float step_5_demo_widget::GetViewportZoomRatio() const
{
  if (!m_scene_control)
    return 1.0f;

  sdk::vis::scene::IScene2DViewportBaseSP viewport;
  if (SDK_FAILED(m_scene_control->GetViewport(viewport)) || !viewport)
    return 1.0f;

  // Using simple viewport
  sdk::vis::scene::IScene2DViewportSimpleSP viewport_simple;
  if (SDK_FAILED(viewport->GetInterface(
    sdk::vis::scene::IScene2DViewportSimple::IID(), 
    reinterpret_cast<void**>(&viewport_simple))) || !viewport_simple) 
    return 1.0f;

  float zoom_ratio = 1.0f;
  if (SDK_FAILED(viewport_simple->GetScale(&zoom_ratio, NULL)))
    return 1.0f;

  return zoom_ratio;
}


PointF2D step_5_demo_widget::WinToGeo(const QPoint& pt)
{
  if (!m_scene_control)
    return PointF2D();

  sdk::vis::ISceneInformationSP scene_info;
  if (SDK_FAILED(m_scene_control->GetSceneInfo(
    sdk::vis::kSceneInfoFlags_NoFlags, scene_info)) || !scene_info)
    return PointF2D();

  PointD2D pos(static_cast<double>(pt.x()), 
               static_cast<double>(pt.y()));

  sdk::PointD2D geo_pos;
  if (SDK_FAILED(scene_info->CoordinateTransform(
    sdk::vis::kTransformType_WinToGeo, pos, geo_pos)))
    return PointF2D();

  return PointF2D(static_cast<float>(geo_pos.x), 
                  static_cast<float>(geo_pos.y));
}

void step_5_demo_widget::RenderScene()
{
  if (!m_scene_control)
    return;

  // Updating decoration layer
  if (m_decoration_layer_renderer && m_decoration_layer)
    m_decoration_layer->SetDirty(true);

  // Updating coverage layer
  if (m_coverage_layer_renderer && m_coverage_layer)
    m_coverage_layer->SetDirty(true);

  // Updating marked feature layer
  if (m_marked_feature_layer_renderer && m_marked_feature_layer)
    m_marked_feature_layer->SetDirty(true);

  // Rendering the scene
  if (SDK_FAILED(m_scene_control->UpdateScene(kUpdateSceneFlags_StartRendering)))
    return;

  update();
}

std::wstring step_5_demo_widget::GetTestDatabasePath()
{

  IWorkspaceFactorySP wks_factory = GetWorkspaceFactory();
  if (!wks_factory)
    return std::wstring();

  IEnumSDKStringSP gdb_names;
  if (SDK_FAILED(wks_factory->GetGeodatabasesNamesFromWKNLocation(&gdb_names,
    NULL)))
    return std::wstring();

  gdb_names->Reset();
  do
  {
    // Getting path to database
    ScopedString gdb_path;
    if (SDK_FAILED(gdb_names->Next(&gdb_path)))
      break;

    // Trying to open the database root catalog
    IRootCatalogSP cat;
    if (SDK_FAILED(wks_factory->OpenRootCatalog(gdb_path, &cat)))
      continue;

    // Getting database name
    ScopedAny db_name;
    if (SDK_FAILED(cat->GetGeodatabaseProperty(
      kRootCatGeodatabaseProperty_GeodatabaseName, db_name)))
      continue;
    if (!ANY_IS_STR(&db_name))
      continue;
    if (kTestDatabaseName != db_name.GetAsWString())
      continue;

    return WideFromSDKString(gdb_path);
  }
  while (true);

  return std::wstring();
}

void step_5_demo_widget::OpenDatabaseWorkspace(const std::wstring& wks_path,
  const std::wstring& hw_id, const std::wstring& permits_path)
{
  if (wks_path.empty())
    return;

  IWorkspaceFactorySP wks_factory = GetWorkspaceFactory();
  if (!wks_factory)
    return;
  IWorkspaceFactoryUtilSP wks_util = 
    wks_factory.GetInterface<IWorkspaceFactoryUtil>();
  if (!wks_util)
    return;

  // Opening workspace
  IWorkspaceConfigurationSP config;
  if (SDK_FAILED(wks_factory->CreateWorkspaceConfiguration(&config)))
    return;

  if (SDK_FAILED(config->SetConfigurationParameter(
    kWorkspaceConfigurationParameter_RootPath,
    ScopedAny(wks_path.c_str()))))
    return;

  // In case HW_ID and Permits file specified, they also should be added to
  //  configuration parameters
  if (!hw_id.empty() && !permits_path.empty())
  {
    IEncryptionParametersSP encryption_parameters;
    if (SDK_OK(wks_util->CreateEncryptionParameters(&encryption_parameters))) {
      encryption_parameters->SetParameter( kEncryptionParameter_S63_HWID, 
        ScopedAny(hw_id));
      encryption_parameters->SetParameter(kEncryptionParameter_S63_PermitsPath,
        ScopedAny(permits_path));
    }
    config->SetConfigurationParameter(
      kWorkspaceConfigurationParameter_EncryptionParameters,
      ScopedAny(encryption_parameters));
  }

  IWorkspaceSP wks;
  if (SDK_FAILED(wks_factory->Open(config, &wks)))
    return;

  // Adding new workspace to scene
  if (!m_scene_manager)
    return;

  IDataSourceViewSP datasource_view;
  if (SDK_FAILED(m_scene_manager->AddDataSourceView(ScopedAny(wks),
    SDKStringHandler(kDataSourceView_TypeName_Navigational),
    kAddDataSourceFlag_ReplaceView, &datasource_view, NULL)))
    return;

  // Inform coverage layer renderer about workspace change
  if (m_coverage_layer_renderer)
    m_coverage_layer_renderer->SetWorkspaceName(wks_path);
}

bool step_5_demo_widget::IsDatabaseEncrypted(const std::wstring& root_cat_path)
{
  if (root_cat_path.empty())
    return false; // No workspace path provided

  IWorkspaceFactorySP wks_factory = GetWorkspaceFactory();
  if (!wks_factory)
    return false;

  IRootCatalogSP root_catalog;
  if (SDK_FAILED(wks_factory->OpenRootCatalog(ScopedString(root_cat_path),
    &root_catalog)))
    return false;

  ScopedAny encryption;
  if (SDK_FAILED(root_catalog->GetGeodatabaseProperty(
    kRootCatGeodatabaseProperty_Encryption, encryption)))
    return false;

  encryption.ChangeType(kSDKAnyType_Uint32);
  return (ANY_UI32(&encryption) == senc::kENCA_None) ? false : true;
}

void step_5_demo_widget::ApplyProjectionParameters(const double& latitude,
  const double& longitude, const double& scale)
{
  if (!m_scene_control)
    return;

  sdk::ISDKParametersSP scene_parameters;
  if (SDK_FAILED(m_scene_control->GetSceneParameters(scene_parameters)))
    return;
  scene_parameters->SetParameter(sdk::vis::kSceneParameters_Lat, sdk::ScopedAny(latitude));
  scene_parameters->SetParameter(sdk::vis::kSceneParameters_Lon, sdk::ScopedAny(longitude));
  scene_parameters->SetParameter(sdk::vis::kSceneParameters_Scale, sdk::ScopedAny(scale));
  m_scene_control->SetSceneParameters(scene_parameters);
}

IWorkspaceFactorySP step_5_demo_widget::GetWorkspaceFactory()
{
  if (!m_wks_factory)
  {
    if (SDK_FAILED(CreateComponent<IWorkspaceFactory>(
      kSencGdbWorkspaceFactoryCID, &m_wks_factory)))
      return IWorkspaceFactorySP();
  }

  return m_wks_factory;
}

void step_5_demo_widget::SetPaletteType(
  const s52::PaletteIndexEnum& palette_index)
{
  if (!m_scene_manager)
    return;

  IPortrayalManagerSP portrayal_manager;
  if (SDK_FAILED(m_scene_manager->GetPortrayalManager(portrayal_manager)))
    return;

  IPortrayalParametersSP port_params;
  if (SDK_FAILED(portrayal_manager->GetPortrayalParameters(port_params)))
    return;

  if (SDK_FAILED(port_params->SetParameter(kPP_DisplayPalette,
    SDKAnyHandler(s52::kPaletteNames[palette_index]))))
    return;

  // Applying new palette to scene
  portrayal_manager->SetPortrayalParameters(port_params);

  // And to S-52 resource manager, if it exists
  if (m_s52_resource_manager)
    m_s52_resource_manager->SetPalette(palette_index);

  emit signalUpdatePaletteMenuState();
}

void step_5_demo_widget::SetDisplayMode(
  const DisplayModeEnum& display_mode)
{
  if (!m_scene_manager)
    return;

  ISceneDisplayGroupsManagerSP dgroups_manager;
  if (SDK_FAILED(m_scene_manager->GetDisplayGroupsManager(dgroups_manager)))
    return;

  IPortrayalParametersSP port_params;
  if (SDK_FAILED(dgroups_manager->SetProperty(
    kDisplayGroupsManagerProperty_DisplayMode, ScopedAny(display_mode))))
    return;

  emit signalUpdateDisplayMenuState();
}

void step_5_demo_widget::SetPortrayalName(const std::string& portrayal_name)
{
  if (!m_scene_manager)
    return;

  IPortrayalManagerSP portrayal_manager;
  if (SDK_FAILED(m_scene_manager->GetPortrayalManager(portrayal_manager)))
    return;
  if (SDK_FAILED(portrayal_manager->SetCurrentPortrayal(kPRSP_S101,
    ScopedString(portrayal_name))))
    return;

  m_portrayal_name = portrayal_name;

  emit signalUpdatePortrayalMenuState();
}

bool step_5_demo_widget::GetFeatureObjectsUnderCursorPosition(
    const QPoint& cursor_position, IEnumFeatureSP& features)
{
  features.Release();

  if (!m_scene_manager)
    return false;
  if (!m_scene_control)
    return false;

  sdk::vis::ISceneInformationSP scene_info;
  if (SDK_FAILED(m_scene_control->GetSceneInfo(
    sdk::vis::kSceneInfoFlags_NoFlags, scene_info)) || !scene_info)
    return false;

  // Converting cursor position from Window coordinate system to scene coord. system
  PointD2D cursor_pos(static_cast<double>(cursor_position.x()),
                      static_cast<double>(cursor_position.y()));
  sdk::PointD2D sw_pos(cursor_pos.x - kFindFeatureUnderCursorRectangleSize / 2.0f,
                       cursor_pos.y - kFindFeatureUnderCursorRectangleSize / 2.0f);
  sdk::PointD2D nw_pos(cursor_pos.x - kFindFeatureUnderCursorRectangleSize / 2.0f,
                       cursor_pos.y + kFindFeatureUnderCursorRectangleSize / 2.0f);
  sdk::PointD2D ne_pos(cursor_pos.x + kFindFeatureUnderCursorRectangleSize / 2.0f,
                       cursor_pos.y + kFindFeatureUnderCursorRectangleSize / 2.0f);
  sdk::PointD2D se_pos(cursor_pos.x + kFindFeatureUnderCursorRectangleSize / 2.0f,
                       cursor_pos.y - kFindFeatureUnderCursorRectangleSize / 2.0f);
  sdk::PointD2D geo_pos[4];
  if (SDK_FAILED(scene_info->CoordinateTransform(
    sdk::vis::kTransformType_WinToGeo, sw_pos, geo_pos[0])))
    return false;
  if (SDK_FAILED(scene_info->CoordinateTransform(
    sdk::vis::kTransformType_WinToGeo, nw_pos, geo_pos[1])))
    return false;
  if (SDK_FAILED(scene_info->CoordinateTransform(
    sdk::vis::kTransformType_WinToGeo, ne_pos, geo_pos[2])))
    return false;
  if (SDK_FAILED(scene_info->CoordinateTransform(
    sdk::vis::kTransformType_WinToGeo, se_pos, geo_pos[3])))
    return false;

  sdk::PointD2D geo_min_pos = geo_pos[0], geo_max_pos = geo_pos[0];
  for (size_t i = 1; i < 4; ++i)
  {
    if (geo_pos[i].x < geo_min_pos.x) geo_min_pos.x = geo_pos[i].x;
    if (geo_pos[i].x > geo_min_pos.x) geo_max_pos.x = geo_pos[i].x;
    if (geo_pos[i].y < geo_min_pos.y) geo_min_pos.y = geo_pos[i].y;
    if (geo_pos[i].y > geo_min_pos.y) geo_max_pos.y = geo_pos[i].y;
  }

  // Making a rectangle, which will set as a filter to find up the objects,
  // which geometry intersects with given rectangle
  sdk::GeoIntRect query_rect(
    sdk::GeoIntFromDeg(geo_min_pos.x), sdk::GeoIntFromDeg(geo_min_pos.y),
    sdk::GeoIntFromDeg(geo_max_pos.x), sdk::GeoIntFromDeg(geo_max_pos.y));

  // Making a new geometry filter
  IWorkspaceFactorySP wks_factory = GetWorkspaceFactory();
  if (!wks_factory)
    return false;
  IWorkspaceFactoryUtilSP wks_util = 
    wks_factory.GetInterface<IWorkspaceFactoryUtil>();
  if (!wks_util)
    return false;

  geometry::IGeometrySP geometry_filter;
  if (SDK_FAILED(wks_util->CreateRectGeometryFilter(
    query_rect.sw.lat, query_rect.sw.lon, query_rect.ne.lat, query_rect.ne.lon,
    &geometry_filter)))
    return false;

  // Finally, looking for features inside given rectangle
  if (SDK_FAILED(scene_info->FindFeatures(geometry_filter, NULL, NULL, features)))
    return false;

  return true;
}

void step_5_demo_widget::UpdateStatusBar()
{
  // Getting current mouse geo position and scale
  PointF2D geo_pos = WinToGeo(m_current_mouse_position);
  GeoIntPoint gip(sdk::GeoIntFromDeg(geo_pos.x), sdk::GeoIntFromDeg(geo_pos.y));

  ISDKParametersSP scene_param;
  if (SDK_FAILED(m_scene_control->GetSceneParameters(scene_param)))
    return;
  double scale = 0.0;
  if (SDK_FAILED(scene_param->GetParameter(kSceneParameters_Scale, 
    sdk::SDKAnyReturnHelper<double>(scale))))
    return;

  // Forming output string
  std::wstring lat = conversions::LatToWString(gip.lat);
  std::wstring lon = conversions::LonToWString(gip.lon);
  std::wostringstream scale_woss;
  scale_woss << static_cast<SDKUInt64>(scale / GetViewportZoomRatio());
  std::wostringstream angle_woss;
  angle_woss << static_cast<SDKUInt64>(GetViewportRotationAngle());
  m_status_bar_text = lat + L" " +  lon + L"  1 : " + scale_woss.str() +
    + L", Rotation angle: " + angle_woss.str();

  // Updating decoration layer
  if (m_decoration_layer_renderer && m_scene_control && m_decoration_layer)
  {
    DecorationRenderer::DecorationLayerText decoration_text;
    decoration_text.push_back(L"Latitude: " + lat);
    decoration_text.push_back(L"Longitude: " + lon);
    decoration_text.push_back(L"Test Scale: " + scale_woss.str());
    decoration_text.push_back(L"Rotation angle: " + angle_woss.str());
    decoration_text.push_back(L"Ini untuk menuliskan tulisan");


    m_decoration_layer_renderer->SetDecorationText(decoration_text);

    // Updating decoration layer
//    m_decoration_layer->SetDirty(true);

    update();
  }

  emit signalUpdateStatusBar();
}


//SDKResult step_5_demo_widget::AddWorkspace(sdk::vis::ISceneManager& scene_manager,
//  const sdk::gdb::IWorkspaceSP& wks) {
//    sdk::vis::IDataSourceViewSP datasource_view;
//    sdk::vis::DataSourceViewID datasource_view_id;
//    SDKResult res = scene_manager.AddDataSourceView(sdk::ScopedAny(wks),
//            sdk::SDKStringHandler(sdk::vis::kDataSourceView_TypeName_Navigational),
//            sdk::vis::kAddDataSourceFlag_ReplaceView, &datasource_view, &datasource_view_id);
//      return res;
//  }

//SDKResult RemoveWorkspace(const sdk::vis::ISceneManager& scene_manager,
//                          const sdk::gdb::IWorkspaceSP& wks){
//    sdk::vis::DataSourceViewID datasource_view_id;
//    SDKResult res = scene_manager->RemoveDataSourceView(datasource_view_id);
//      return res;
//}
