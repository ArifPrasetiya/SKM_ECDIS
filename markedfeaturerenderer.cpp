// MarkedFeatureRenderer.cpp : Renders the marked feature object above chart
//

#include <base/inc/sdk_string_handler.h>
#include <base/inc/sdk_any_helpers.h>
#include <base/inc/sdk_any_handler.h>
#include <base/inc/math/matrix3x2.h>
#include <geometry/inc/coordinate_systems/crs_basic_transformation.inl>
#include <geometry/inc/coordinate_systems/crs_coordinate_transformation.h>
#include <visualizationlayer/inc/graphics/2d_render_target_factory_interface.h>

#include "markedfeaturerenderer.h"

using namespace SDK_GDB_NAMESPACE;
using namespace SDK_NAMESPACE::crs;
using namespace SDK_NAMESPACE::geometry;

// Converts 3D point to 2D point
inline sdk::GeoIntPoint GeoIntPointFromGeoIntPoint3D(
  const sdk::GeoIntPoint3D& gip)
{ 
  return sdk::GeoIntPoint(gip.x, gip.y); 
}

MarkedFeatureRenderer::MarkedFeatureRenderer(
  const sdk::gdb::IWorkspaceFactorySP wks_factory,
  const S52ResourceManagerSP& s52_res_manager)
  : m_wks_factory(wks_factory),
    m_s52_resource_manager(s52_res_manager),
    m_ref(0),
    m_render_target(),
    m_feature_object_path(),
    m_base_scale(0.0),
    m_base_center(),
    m_feature_geometry_type(sdk::geometry::kGMT_Null)
{
}

MarkedFeatureRenderer::~MarkedFeatureRenderer()
{
}

SDKUInt32 MarkedFeatureRenderer::AddRef() const throw()
{
  ++m_ref;
  return m_ref;
}

SDKUInt32 MarkedFeatureRenderer::Release() const throw()
{
  unsigned long references = --m_ref;
  if (0 == references)
    delete this;

  return references;
}

SDKResult MarkedFeatureRenderer::GetInterface(const sdk::Uuid& iid, 
  void** obj_ptr) throw() 
{
  if (NULL == obj_ptr)
    return sdk::Err_NULLPointer;
  if (NULL != *obj_ptr)
    return sdk::Err_AlreadyInitialized;

  if (sdk::IsEqualUuid(iid, sdk::vis::scene::IRenderer::IID()))
    *obj_ptr = static_cast<sdk::vis::scene::IRenderer*>(this);
  else if (sdk::IsEqualUuid(iid, ISDKComponent::IID()))
    *obj_ptr = static_cast<sdk::ISDKComponent*>(this);
  else
    return sdk::Err_NotImpl;

  AddRef();
  return sdk::Ok;
}

SDKResult MarkedFeatureRenderer::GetRendererID(SDKString& id) throw()
{
  id = sdk::ScopedString("Marked feature renderer").Detach();
  return sdk::Ok;
}

SDKResult MarkedFeatureRenderer::Cancel() throw()
{
  return sdk::Err_NotImpl; // Rendering operation can't be cancelled
}

SDKResult MarkedFeatureRenderer::SetContext(
  const sdk::vis::scene::ResourceType& layer_resource_type,
  const sdk::vis::scene::ILayerResourceSP& layer_resource,
  const sdk::vis::scene::IGraphicContextSP& graphic_context) throw()
{
  if (!graphic_context || !layer_resource)
    return sdk::Err_InvalidArg;

  sdk::vis::scene::ILayerResourceRenderTargetSP render_target_resource;
  if (SDK_FAILED(layer_resource->GetInterface(
    sdk::vis::scene::ILayerResourceRenderTarget::IID(),
    reinterpret_cast<void**>(&render_target_resource))))
    return sdk::Err_InternalError;

  // Getting render target
  if (SDK_FAILED(render_target_resource->GetRenderTarget(m_render_target)))
    return sdk::Err_InternalError;

  return sdk::Ok;
}

SDKResult MarkedFeatureRenderer::Render(
  const sdk::vis::scene::IRenderContextSP& context) throw()
{
  if (!m_render_target || !m_s52_resource_manager)
    return sdk::Err_Uninitialized;

  sdk::ScopedAny any_ss_catalog;
  if (SDK_FAILED(context->GetParameter(sdk::vis::kSceneRendererParameter_SymbolSetCatalog, any_ss_catalog)))
    return sdk::Err_InternalError;
  sdk::vis::sdl::ISymbolSetCatalogSP ss_catalog(
    sdk::GetInterfaceT<sdk::vis::sdl::ISymbolSetCatalog>(ANY_COMPONENT(&any_ss_catalog)));
  
  sdk::ScopedAny any_projection;
  if (SDK_FAILED(context->GetParameter(sdk::vis::kSceneRendererParameter_Projection, any_projection)))
    return sdk::Err_InternalError;
  if (ANY_TYPE(&any_projection) != kSDKAnyType_SDKComponentPtr)
    return sdk::Err_InternalError;
  sdk::crs::IProjectionSP projection(
    sdk::GetInterfaceT<sdk::crs::IProjection>(ANY_COMPONENT(&any_projection)));
  sdk::crs::ICoordinateTransformationSP coord_transform(
    sdk::GetInterfaceT<sdk::crs::ICoordinateTransformation>(projection));

  sdk::gfx::RTAutoStartFinishDraw auto_start_finish_draw(m_render_target);
  if (!auto_start_finish_draw.IsDrawStarted())
    return sdk::Err_InternalError;

  m_render_target->SetAntiAliasingMode(sdk::gfx::AntiAliasingMode_PerPrimitive);
  m_render_target->RemoveTransformationMatrixes();

  // Clean up the texture
  m_render_target->FillBackground(sdk::ColorF(0.f, 0.f, 0.f, 0.f));

  // Trying to draw the feature object mark, if it exists
  if (m_feature_object_path)
  {
    // Changing the center of coordinate system
    sdk::crs::ICoordinateTransformationSP coord_transform =
      sdk::GetInterfaceT<sdk::crs::ICoordinateTransformation>(projection);
    if (!coord_transform)
      return sdk::Err_InternalError;

    sdk::CMatrix3X2 matrix_translate;
    SDKPointF2D base_center;
    coord_transform->ForwardIF(1, &m_base_center, &base_center);
    matrix_translate.Translate(base_center.x, base_center.y);

    double scale = 1.0;
    sdk::crs::IProjectionParametersSP proj_param;
    if (SDK_FAILED(projection->GetProjectionParameters(&proj_param)) || !proj_param) 
      return sdk::Err_InternalError;
    if (SDK_FAILED(proj_param->GetParameterValueByID(kProjPar_ScaleFactor, scale)))
      return sdk::Err_InternalError;

    double k = m_base_scale / scale;
    sdk::CMatrix3X2 matrix_scale;
    matrix_scale.Scale(float(k), float(k));

    m_render_target->PushTransformationMatrix(matrix_translate);
    m_render_target->PushTransformationMatrix(matrix_scale);

    // Creating appropriate brushes and stroke style
    sdk::ColorF brush_color = m_s52_resource_manager->GetColor(
      ss_catalog, sdk::vis::s52::kColorIndex_NINFO);
    sdk::gfx::RenderTargetBrushSP draw_brush;
    if (SDK_FAILED(m_render_target->CreateSolidColorBrush(brush_color, 
      draw_brush)))
      return sdk::Err_InternalError;

    brush_color.a = 0.5f;
    sdk::gfx::RenderTargetBrushSP fill_brush;
    if (SDK_FAILED(m_render_target->CreateSolidColorBrush(brush_color, 
      fill_brush)))
      return sdk::Err_InternalError;

    sdk::gfx::RenderTargetStrokeStyleSP stroke_style;
    if (SDK_FAILED(m_render_target->CreateStrokeStyle(
      sdk::gfx::StrokeStyleOptions(), NULL, 0, stroke_style)) || !stroke_style)
      return sdk::Err_InternalError;

    // Rendering the appropriate feature object mark
    switch(m_feature_geometry_type)
    {
      case sdk::geometry::kGMT_Surface:
      case sdk::geometry::kGMT_MultiSurface:
        m_render_target->FillPath(m_feature_object_path, fill_brush);
      case sdk::geometry::kGMT_Point:
      case sdk::geometry::kGMT_Curve:
      case sdk::geometry::kGMT_CompositeCurve:
        m_render_target->DrawPath(m_feature_object_path, draw_brush, float(6/k),
          stroke_style);
      default:
        break;
    }
  }

  return sdk::Ok;
}

SDKResult MarkedFeatureRenderer::SetProperty(const sdk::SDKPropertyID& id,
  const SDKAny& value) throw()
{
  // No properties supported
  return sdk::Err_NotImpl;
}

SDKResult MarkedFeatureRenderer::GetProperty(const sdk::SDKPropertyID& id,
  SDKAny& value) const throw()
{
  // No properties supported
  return sdk::Err_NotImpl;
}

bool MarkedFeatureRenderer::SetMark(const sdk::gdb::ObjectID& oid, const sdk::crs::IProjectionSP& projection_source,
  sdk::GeoIntPoint& feature_object_position, double& dataset_min_disp_scale)
{
  m_feature_object_path.Release();
  m_base_scale = 0.0;
  m_base_center = sdk::GeoIntPoint();
  m_feature_geometry_type = sdk::geometry::kGMT_Null;

  if (!projection_source || !m_wks_factory || !m_render_target)
    return false; // Uninitialized.

  // Getting appropriate workspace, containing required feature
  sdk::gdb::IWorkspaceCollectionSP wks_collection;
  if (SDK_FAILED(m_wks_factory->GetWorkspaces(&wks_collection)))
    return false;

  sdk::gdb::IWorkspaceSP wks;
  if (SDK_FAILED(wks_collection->GetWorkspaceByID(
    DatasetID_WorkspaceID(oid.did), &wks)))
    return false;

  // Getting feature itself
  sdk::gdb::IFeatureSP feature;
  if (SDK_FAILED(wks->GetFeature(oid, &feature)) || !feature)
    return false;

  // And feature dataset
  sdk::gdb::IFeatureDatasetSP feature_dataset;
  if (SDK_FAILED(feature->GetFeatureDataset(&feature_dataset)))
    return false;
  sdk::gdb::IDatasetSP dataset = 
    sdk::GetInterfaceT<sdk::gdb::IDataset>(feature_dataset);
  if (!dataset)
    return false;

  // Getting dataset scale/center
  sdk::ScopedAny scale;
  if (SDK_FAILED(dataset->GetDatasetProperty(sdk::gdb::kDSP_CompilationScale, 
    scale)))
    return false;
  m_base_scale = static_cast<double>(ANY_UI32(&scale) / 2.0);

  sdk::ScopedAny min_disp_scale;
  if (SDK_FAILED(dataset->GetDatasetProperty(sdk::gdb::kDSP_MinDispScale,
    min_disp_scale)))
    return false;
  min_disp_scale.ChangeType(kSDKAnyType_Double);
  dataset_min_disp_scale = ANY_DOUBLE(&min_disp_scale);

  sdk::geometry::IEnvelopeSP dataset_envelope_ptr;
  if (SDK_FAILED(dataset->GetBounds(&dataset_envelope_ptr)))
    return false;
  SDKEnvelope2DI dataset_envelope;
  if (SDK_FAILED(dataset_envelope_ptr->GetCoordinates(kSDKAnyType_GeoInt,
    &dataset_envelope.xmin, &dataset_envelope.ymin,
    &dataset_envelope.xmax, &dataset_envelope.ymax)))
    return false;
  m_base_center.x = dataset_envelope.xmin +
    ((dataset_envelope.xmax - dataset_envelope.xmin) / 2);
  m_base_center.y = (dataset_envelope.ymin + dataset_envelope.ymax) / 2;

  // Preparing projection
  sdk::crs::IProjectionSP projection;
  if (SDK_FAILED(projection_source->Clone(&projection)) || !projection)
    return false;
  sdk::crs::IProjectionParametersSP proj_params;
  if (SDK_FAILED(projection->GetProjectionParameters(&proj_params)))
    return false;
  proj_params->SetParameterValueByID(kProjPar_LatitudeOfCenter, 0.0);
  proj_params->SetParameterValueByID(kProjPar_LatitudeOfOrigin,
    sdk::DegFromGeoInt(m_base_center.y));
  proj_params->SetParameterValueByID(kProjPar_LongitudeOfOrigin,
    sdk::DegFromGeoInt(m_base_center.x));
  proj_params->SetParameterValueByID(kProjPar_ScaleFactor,
    m_base_scale);
  if (SDK_FAILED(projection->SetProjectionParameters(proj_params)))
    return false;

  // Getting feature shape
  sdk::geometry::IGeometrySP feature_shape;
  if (SDK_FAILED(feature->GetShape(&feature_shape)) || !feature_shape)
    return false;
  if (SDK_FAILED(feature->GetShapeType(m_feature_geometry_type)))
    return false;

  // Getting feature shape envelope
  sdk::geometry::IEnvelopeSP feature_shape_envelope;
  if (SDK_FAILED(feature_shape->GetEnvelope(&feature_shape_envelope))
    || !feature_shape_envelope)
    return false;

  sdk::GeoIntRect feature_shape_envelope_rect;
  if (SDK_FAILED(feature_shape_envelope->GetCoordinates(kSDKAnyType_GeoInt,
    &feature_shape_envelope_rect.min.x, &feature_shape_envelope_rect.min.y,
    &feature_shape_envelope_rect.max.x, &feature_shape_envelope_rect.max.y)))
    return false;
  feature_object_position = feature_shape_envelope_rect.Center();

  // Processing the feature geometry
  std::vector<sdk::GeoIntPoint> geoint_points;
  if (!CrackGeometry(feature_shape, geoint_points) || geoint_points.empty())
    return false;
  sdk::crs::ICoordinateTransformationSP coord_transform =
    sdk::GetInterfaceT<sdk::crs::ICoordinateTransformation>(projection);
  if (!coord_transform)
    return false;
  coord_transform->ForwardIF(
    static_cast<SDKUInt32>(geoint_points.size()), &geoint_points.front(),
    reinterpret_cast<sdk::PointF2D*>(&geoint_points.front()));

  // Creating a graphics path from given geopoints
  sdk::gfx::RenderTargetFactorySP rtf;
  if (SDK_FAILED(m_render_target->GetParent(rtf)) || !rtf)
    return false;

  const sdk::PointF2D* points =
    reinterpret_cast<const sdk::PointF2D*>(&(geoint_points.front()));

  switch(m_feature_geometry_type)
  {
  case sdk::geometry::kGMT_Point:
    {
      // Creating a cross mark
      sdk::gfx::GraphicsPathSP path;
      if (SDK_FAILED(rtf->CreateGraphicsPath(path)) || !path)
        return false;
      sdk::gfx::GraphicsPathEditorSP path_editor;
      if (SDK_FAILED(path->StartEdit(path_editor)) || !path_editor)
        return false;

      sdk::PointF2D p[2];
      p[0] = points[0];
      p[0].x -= 10;
      p[1] = points[0];
      p[1].x += 10;

      path_editor->StartFigure(p[0], sdk::gfx::StartFigureStyle_Filled);
      path_editor->AddLine(p[1]);
      path_editor->FinishFigure(sdk::gfx::FinishFigureRule_LeaveOpened);

      p[0] = points[0];
      p[0].y += 10;
      p[1] = points[0];
      p[1].y -= 10;
      path_editor->StartFigure(p[0], sdk::gfx::StartFigureStyle_Filled);
      path_editor->AddLine(p[1]);
      path_editor->FinishFigure(sdk::gfx::FinishFigureRule_LeaveOpened);

      path->FinishEdit();
      m_feature_object_path = path;
    }
    break;
  case sdk::geometry::kGMT_Multipoint:
    {
      // Creating a set of cross marks
      sdk::gfx::GraphicsPathSP path;
      if (SDK_FAILED(rtf->CreateGraphicsPath(path)) || !path)
        return false;
      sdk::gfx::GraphicsPathEditorSP path_editor;
      if (SDK_FAILED(path->StartEdit(path_editor)) || !path_editor)
        return false;

      for (size_t c = 0; c < geoint_points.size(); ++c)
      {
        sdk::PointF2D p[2];
        p[0] = points[c];
        p[0].x -= 10;
        p[1] = points[c];
        p[1].x += 10;

        path_editor->StartFigure(p[0], sdk::gfx::StartFigureStyle_Filled);
        path_editor->AddLine(p[1]);
        path_editor->FinishFigure(sdk::gfx::FinishFigureRule_LeaveOpened);

        p[0] = points[c];
        p[0].y += 10;
        p[1] = points[c];
        p[1].y -= 10;
        path_editor->StartFigure(p[0], sdk::gfx::StartFigureStyle_Filled);
        path_editor->AddLine(p[1]);
        path_editor->FinishFigure(sdk::gfx::FinishFigureRule_LeaveOpened);
      }

      path->FinishEdit();
      m_feature_object_path = path;
    }
    break;
  case sdk::geometry::kGMT_Curve:
  case sdk::geometry::kGMT_CompositeCurve:
    {
      // Making a feature object contour mark
      sdk::gfx::GraphicsPathSP path;
      if (SDK_FAILED(rtf->CreateGraphicsPath(path)) || !path)
        return false;
      sdk::gfx::GraphicsPathEditorSP path_editor;
      if (SDK_FAILED(path->StartEdit(path_editor)) || !path_editor)
        return false;

      path_editor->StartFigure(points[0], sdk::gfx::StartFigureStyle_Filled);
      path_editor->AddLines(&points[1],
        static_cast<SDKUInt32>(geoint_points.size() - 1));
      path_editor->FinishFigure(sdk::gfx::FinishFigureRule_LeaveOpened);

      path->FinishEdit();
      m_feature_object_path = path;
    }
    break;
  case sdk::geometry::kGMT_Surface:
    {
      // Making a feature object contour mark
      sdk::gfx::GraphicsPathSP path;
      if (SDK_FAILED(rtf->CreateGraphicsPath(path)) || !path)
        return false;
      sdk::gfx::GraphicsPathEditorSP path_editor;
      if (SDK_FAILED(path->StartEdit(path_editor)) || !path_editor)
        return false;

      path_editor->StartFigure(points[0], sdk::gfx::StartFigureStyle_Filled);
      path_editor->AddLines(&points[1],
        static_cast<SDKUInt32>(geoint_points.size() - 1));
      path_editor->FinishFigure(sdk::gfx::FinishFigureRule_CloseFigure);

      path->FinishEdit();
      m_feature_object_path = path;
    }
    break;
  default:
    break;
  }

  return true;
}

bool MarkedFeatureRenderer::RemoveMark()
{
  // Releasing the graphic path, which belongs to marked feature
  m_feature_object_path.Release();
  m_base_scale = 0.0;
  m_base_center = sdk::GeoIntPoint();
  m_feature_geometry_type = sdk::geometry::kGMT_Null;

  return true;
}

bool MarkedFeatureRenderer::CrackGeometry(
  const sdk::geometry::IGeometrySP& geometry,
  std::vector<sdk::GeoIntPoint>& points) {

  points.clear();

  sdk::geometry::GeometryType geometry_type;
  if (SDK_FAILED(geometry->GetGeometryType(geometry_type)))
    return false;

  switch(geometry_type)
  {
  case sdk::geometry::kGMT_Point:
    {
      sdk::geometry::IPointSP point_ptr;
      if (SDK_FAILED(geometry->GetInterface(sdk::geometry::IPoint::IID(),
        reinterpret_cast<void**>(&point_ptr))) || !point_ptr)
        return false;
      sdk::GeoIntPoint point;
      if (SDK_FAILED(point_ptr->GetCoords(
        kSDKAnyType_GeoInt, &point.x, &point.y)))
        return false;
      points.push_back(point);
    }
    break;
  case sdk::geometry::kGMT_Multipoint:
    {
      sdk::geometry::IPointCollectionSP point_collection;
      if (SDK_FAILED(geometry->GetPointCollection(NULL, &point_collection)))
        return false;

      SDKUInt32 point_count;
      if (SDK_FAILED(point_collection->GetPointCount(point_count)))
        return false;

      std::vector<sdk::GeoIntPoint3D> points_3d(point_count);
      if (SDK_FAILED(point_collection->GetPoints(0, point_count,
        kGMPT_GeoIntPoint3D, &points_3d.front())))
        return false;

      points.resize(point_count);
      std::transform(points_3d.begin(), points_3d.end(),
        points.begin(), GeoIntPointFromGeoIntPoint3D);
    }
    break;
  case sdk::geometry::kGMT_Curve:
    {
      sdk::geometry::IPointCollectionSP point_collection;
      if (SDK_FAILED(geometry->GetPointCollection(NULL, &point_collection)))
        return false;

      SDKUInt32 point_count;
      if (SDK_FAILED(point_collection->GetPointCount(point_count)))
        return false;

      points.resize(point_count);
      if (SDK_FAILED(point_collection->GetPoints(0, point_count,
        kGMPT_GeoIntPoint, &points.front())))
        return false;
    }
    break;
  case sdk::geometry::kGMT_CompositeCurve:
    {
      sdk::geometry::IPointCollectionSP point_collection;
      if (SDK_FAILED(geometry->GetPointCollection(NULL, &point_collection))
        || !point_collection)
        return false;

      SDKUInt32 point_count;
      if (SDK_FAILED(point_collection->GetPointCount(point_count)))
        return false;

      points.resize(point_count);
      if (SDK_FAILED(point_collection->GetPoints(0, point_count,
        kGMPT_GeoIntPoint, &points.front())))
        return false;
    }
    break;
  case sdk::geometry::kGMT_Surface:
  case sdk::geometry::kGMT_MultiCurve:
  case sdk::geometry::kGMT_MultiCompositeCurve:
    {
      sdk::geometry::IGeometryCollectionSP geometry_collection;
      if (SDK_FAILED(geometry->GetInterface(IGeometryCollection::IID(),
        reinterpret_cast<void**>(&geometry_collection))))
        return false;

      SDKUInt32 geometry_count = 0;
      if (SDK_FAILED(geometry_collection->GetGeometryCount(geometry_count))
        || !geometry_count)
        return false;

      // We take only exterior ring. We draw without holes.
      sdk::geometry::IGeometrySP ring;
      if (SDK_FAILED(geometry_collection->GetGeometry(0, &ring)))
        return false;

      sdk::geometry::IPointCollectionSP point_collection;
      if (SDK_FAILED(ring->GetPointCollection(NULL, &point_collection)))
        return false;

      SDKUInt32 point_count;
      if (SDK_FAILED(point_collection->GetPointCount(point_count)))
        return false;

      points.resize(point_count);
      if (SDK_FAILED(point_collection->GetPoints(0, point_count,
        kGMPT_GeoIntPoint, &points.front())))
        return false;
    }
    break;
  default:
    break;
  }

  return true;
}
