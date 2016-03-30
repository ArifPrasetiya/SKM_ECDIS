// CoverageRenderer.cpp : renderer for data coverages collection.
//

#include <algorithm>

#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_component_interface.h>
#include <base/inc/sdk_any_handler.h>
#include <base/inc/math/matrix3x2.h>
#include <base/inc/geometry/geometry_base_types.h>
#include <base/inc/color/color_base_types_helpers.h>
#include <base/inc/base_library/framework_interface.h>
#include <geometry/inc/coordinate_systems/crs_const.h>
#include <geometry/inc/coordinate_systems/crs_basic_transformation.inl>
#include <visualizationlayer/inc/scene/layer_interface.h>
#include <visualizationlayer/inc/graphics/2d_render_target_interface.h>
#include <visualizationlayer/inc/graphics/2d_graphics_objects.h>
#include <visualizationlayer/inc/scene/graphic_context_interface.h>
#include <visualizationlayer/inc/visman/scene_manager_interface.h>
#include <datalayer/inc/senc/senc_feature_catalog_s101.h>
#include <datalayer/inc/senc/component_ids.h>
#include <datalayer/inc/geodatabase/gdb_workspace.h>

#include "coverage_renderer.h"

using namespace SDK_NAMESPACE;
using namespace SDK_GDB_NAMESPACE;
using namespace SDK_CRS_NAMESPACE;
using namespace SDK_VIS_NAMESPACE;

CoverageRenderer::CoverageRenderer(
  const S52ResourceManagerSP& s52_res_manager,
  const IWorkspaceFactorySP& wks_factory)
  : m_s52_resource_manager(s52_res_manager),
    m_wks_factory(wks_factory),
    m_ref(0),
    m_render_target(),
    m_stroke(),
    m_bounds(),
    m_coverage_list(),
    m_wks_name()
{
}

CoverageRenderer::~CoverageRenderer()
{
}

SDKUInt32 CoverageRenderer::AddRef() const throw()
{
  ++m_ref;
  return m_ref;
}

SDKUInt32 CoverageRenderer::Release() const throw()
{
  unsigned long references = --m_ref;
  if (0 == references)
    delete this;

  return references;
}

SDKResult CoverageRenderer::GetInterface(const Uuid& iid, void** obj_ptr) throw()
{
  if (NULL == obj_ptr)
    return Err_NULLPointer;
  if (NULL != *obj_ptr)
    return Err_AlreadyInitialized;

  if (IsEqualUuid(iid, scene::IRenderer::IID()))
    *obj_ptr = static_cast<scene::IRenderer*>(this);
  else if (IsEqualUuid(iid, ISDKComponent::IID()))
    *obj_ptr = static_cast<ISDKComponent*>(this);
  else
    return Err_NotImpl;

  AddRef();
  return Ok;
}

SDKResult CoverageRenderer::GetRendererID(SDKString& id) throw()
{
  id = ScopedString("Coverage renderer").Detach();
  return Ok;
}

SDKResult CoverageRenderer::Cancel() throw()
{
  return Err_NotImpl; // Do not support canceling.
}

SDKResult CoverageRenderer::SetContext(
  const scene::ResourceType& layer_resource_type,
  const scene::ILayerResourceSP& layer_resource,
  const scene::IGraphicContextSP& graphic_context) throw()
{
  if (!graphic_context || !layer_resource)
    return Err_InvalidArg;

  scene::ILayerResourceRenderTargetSP render_target_resource;
  SDKResult res = layer_resource->GetInterface(
    scene::ILayerResourceRenderTarget::IID(),
    reinterpret_cast<void**>(&render_target_resource));
  if (SDK_FAILED(res) || !render_target_resource)
    return Err_InternalError;

  res = render_target_resource->GetRenderTarget(m_render_target);
  if (SDK_FAILED(res) || !m_render_target)
    return Err_InternalError;

  // Creating render target stroke style to draw bounds
  if (SDK_FAILED(m_render_target->CreateStrokeStyle(gfx::StrokeStyleOptions(),
    0, 0, m_stroke)) || !m_stroke)
    return Err_InternalError;

  return Ok;
}

SDKResult CoverageRenderer::Render(
  const scene::IRenderContextSP& context) throw()
{
  if (!m_render_target || !m_stroke || !m_s52_resource_manager)
    return Err_Uninitialized;

  sdk::ScopedAny any_projection;
  if (SDK_FAILED(context->GetParameter(sdk::vis::kSceneRendererParameter_Projection, any_projection)))
    return sdk::Err_InternalError;
  if (ANY_TYPE(&any_projection) != kSDKAnyType_SDKComponentPtr)
    return sdk::Err_InternalError;
  sdk::crs::IProjectionSP projection(
    sdk::GetInterfaceT<sdk::crs::IProjection>(ANY_COMPONENT(&any_projection)));
  sdk::crs::ICoordinateTransformationSP coord_transform(
    sdk::GetInterfaceT<sdk::crs::ICoordinateTransformation>(projection));

  ProjectionParametersChanged(projection);

  double scale = 1.0;
  sdk::crs::IProjectionParametersSP proj_param;
  if (SDK_FAILED(projection->GetProjectionParameters(&proj_param)) || !proj_param) 
    return sdk::Err_InternalError;
  if (SDK_FAILED(proj_param->GetParameterValueByID(kProjPar_ScaleFactor, scale)))
    return sdk::Err_InternalError;

  m_render_target->SetAntiAliasingMode(gfx::AntiAliasingMode_None);

  // Drawing is started here
  gfx::RTAutoStartFinishDraw auto_start_finish_draw(m_render_target);
  if (!auto_start_finish_draw.IsDrawStarted())
    return Err_InternalError;

  // Filling up background with transparent color
  m_render_target->FillBackground(ColorF(0.0f, 0.0f, 0.0f, 0.0f));

  // Creating brush for drawing boundaries
  gfx::RenderTargetBrushSP brush;
  if (SDK_FAILED(m_render_target->CreateSolidColorBrush(
    m_s52_resource_manager->GetColor(s52::kColorIndex_NINFO), brush)) || !brush)
    return Err_InternalError;

  // Iterate all coverage one by one and draw them
  for (CoverageList::iterator it = m_coverage_list.begin();
    it != m_coverage_list.end(); ++it)
  {
    m_render_target->RemoveTransformationMatrixes();

    CoverageList::Entry& coverage = it->second;
    if (!coverage.m_visible || !coverage.m_path)
      continue;

    // Move coordinate system to the center of coverage
    CMatrix3X2 matrix_translate;
    SDKPointF2D base_center;
    coord_transform->ForwardIF(1, &coverage.m_base_center, &base_center);
    matrix_translate.Translate(base_center.x, base_center.y);

    // Calculate scaling matrix to compensate difference between
    // coverage scale and current scale
    CMatrix3X2 matrix_scale;
    double k = coverage.m_base_scale / scale;
    matrix_scale.Scale(float(k), float(k));

    m_render_target->PushTransformationMatrix(matrix_translate);
    m_render_target->PushTransformationMatrix(matrix_scale);

    // Draw coverage
    m_render_target->DrawPath(coverage.m_path, brush, float(1/k), m_stroke);
  }

  return Ok;
}

SDKResult CoverageRenderer::SetProperty(const SDKPropertyID& id,
  const SDKAny& value) throw()
{
  return Err_NotImpl;
}

SDKResult CoverageRenderer::GetProperty(const SDKPropertyID& id,
  SDKAny& value) const throw()
{
  return Err_NotImpl;
}

// Reread current workspace coverages which are fit into the window.
bool CoverageRenderer::ProjectionParametersChanged(
  const sdk::crs::IProjectionSP& projection_source)
{
  if (m_wks_name.empty() || !m_wks_factory || !m_render_target)
    return false;

  IWorkspaceFactoryUtilSP wks_util = 
    m_wks_factory.GetInterface<IWorkspaceFactoryUtil>();
  if (!wks_util)
    return false;

  gfx::RenderTargetFactorySP rtf;
  if (SDK_FAILED(m_render_target->GetParent(rtf)) || !rtf)
    return false;

  IWorkspaceCollectionSP workspaces;
  if (SDK_FAILED(m_wks_factory->GetWorkspaces(&workspaces)) || !workspaces)
    return false;

  IWorkspaceSP workspace;
  if (SDK_FAILED(workspaces->GetWorkspace(
    ScopedString(m_wks_name), &workspace)) || !workspace)
    return false;

  std::vector<GeoIntPoint> points;
  points.reserve(1000);

  // Clone projection
  IProjectionSP projection;
  if (SDK_FAILED(projection_source->Clone(&projection)) || !projection)
    return false;

  ICoordinateTransformationSP coord_transform =
    GetInterfaceT<ICoordinateTransformation>(projection);
  if (!coord_transform)
    return false;

  IProjectionParametersSP projection_param;
  if (SDK_FAILED(projection->GetProjectionParameters(&projection_param)) || !projection_param)
    return false;
  double scale = 0.0;
  if (SDK_FAILED(projection_param->GetParameterValueByID(kProjPar_ScaleFactor, scale)))
    return false;
  double resolution = 0.0;
  if (SDK_FAILED(projection_param->GetParameterValueByID(kProjPar_CoordinateUnit, resolution)))
    return false;

  double bounds_width_in_meter = m_bounds.width * resolution * scale;

  for (CoverageList::iterator it = m_coverage_list.begin(); it != m_coverage_list.end(); ++it)
    it->second.m_visible = false;

  // Calculate visible geographic region
  GeoIntRect geo_bounds;
  PointF2D sw(m_bounds.x, m_bounds.y);
  coord_transform->InverseFI(1, &sw, &geo_bounds.sw);
  PointF2D ne(m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height);
  coord_transform->InverseFI(1, &ne, &geo_bounds.ne);

  // Fix geographic boundary if it is greater than the whole earth
  if (bounds_width_in_meter >= kEQUATOR)
  {
    geo_bounds.sw.lon = kGeoIntLonMin;
    geo_bounds.ne.lon = kGeoIntLonMax;
  }

  // Set spatial filter to the workspace to iterate only visible datasets
  geometry::IGeometrySP spatial_filter;
  if (SDK_FAILED(wks_util->CreateRectGeometryFilter(
    geo_bounds.sw.lat, geo_bounds.sw.lon,
    geo_bounds.ne.lat, geo_bounds.ne.lon, &spatial_filter)))
    return false;

  // Read coverages of visible datasets
  IEnumDatasetIDSP dataset_ids;
  if (SDK_OK(workspace->GetDatasetIDs(spatial_filter, NULL, &dataset_ids)))
  {
    DatasetID did;
    while(SDK_OK(dataset_ids->Next(&did)))
    {
      CoverageList::iterator it = m_coverage_list.Get(did);

      if (it != m_coverage_list.end())
      {
        it->second.m_visible = true;
        continue;
      }

      CoverageList::Entry entry;
      entry.m_visible = true;

      IDatasetSP dataset;
      if (SDK_FAILED(workspace->GetDataset(did, &dataset)))
        continue;

      ScopedAny compilation_scale;
      if (SDK_FAILED(dataset->GetDatasetProperty(
        kDSP_CompilationScale, compilation_scale)))
        continue;

      entry.m_base_scale = static_cast<double>(ANY_UI32(&compilation_scale) / 2);

      ScopedAny dataset_name;
      if (SDK_FAILED(dataset->GetDatasetProperty(kDSP_FileName, 
        dataset_name)))
        continue;
      if (!ANY_IS_STR(&dataset_name))
        continue;
      entry.m_dataset_name = ASCIIFromSDKString(*ANY_STR(&dataset_name));

      geometry::IEnvelopeSP dataset_envelope_ptr;
      if (SDK_FAILED(dataset->GetBounds(&dataset_envelope_ptr)))
        continue;

      SDKEnvelope2DI dataset_envelope;
      if (SDK_FAILED(dataset_envelope_ptr->GetCoordinates(kSDKAnyType_GeoInt,
        &dataset_envelope.xmin, &dataset_envelope.ymin,
        &dataset_envelope.xmax, &dataset_envelope.ymax)))
        continue;

      entry.m_base_center.x = dataset_envelope.xmin +
        ((dataset_envelope.xmax - dataset_envelope.xmin)/2);
      entry.m_base_center.y = (dataset_envelope.ymin + dataset_envelope.ymax)/2;

      IProjectionParametersSP projection_param;
      if (SDK_FAILED(projection->GetProjectionParameters(&projection_param)))
        continue;

      projection_param->SetParameterValueByID(kProjPar_LatitudeOfCenter, 0.0);
      projection_param->SetParameterValueByID(kProjPar_LatitudeOfOrigin,
        DegFromGeoInt(entry.m_base_center.y));
      projection_param->SetParameterValueByID(kProjPar_LongitudeOfOrigin,
        DegFromGeoInt(entry.m_base_center.x));
      projection_param->SetParameterValueByID(kProjPar_ScaleFactor,
        entry.m_base_scale);

      if (SDK_FAILED(projection->SetProjectionParameters(projection_param)))
        continue;

      geometry::IGeometrySP coverage;
      if (SDK_FAILED(dataset->GetCoverage(&coverage)))
        continue;

      // Create coverage graphic paths.
      geometry::GeometryType geometry_type;
      if (SDK_FAILED(coverage->GetGeometryType(geometry_type)))
        continue;
      if (geometry_type == geometry::kGMT_Surface ||
          geometry_type == geometry::kGMT_MultiCurve ||
          geometry_type == geometry::kGMT_MultiCompositeCurve)
      {
        if (!CrackSurface(coverage, points) || points.empty())
          continue;

        coord_transform->ForwardIF(static_cast<SDKUInt32>(points.size()),
            &points.front(), reinterpret_cast<PointF2D*>(&points.front()));

        gfx::GraphicsPathSP path;
        if (SDK_FAILED(rtf->CreateGraphicsPath(path)) || !path)
          continue;
        gfx::GraphicsPathEditorSP path_editor;
        if (SDK_FAILED(path->StartEdit(path_editor)) || !path_editor)
          continue;
        if (SDK_FAILED(path_editor->StartFigure(
          reinterpret_cast<SDKPointF2D&>(points[0]),
          gfx::StartFigureStyle_Filled)))
          continue;
        if (SDK_FAILED(path_editor->AddLines(
          reinterpret_cast<SDKPointF2D*>(&points[1]),
          static_cast<SDKUInt32>(points.size() - 1))))
          continue;
        if (SDK_FAILED(path_editor->FinishFigure(
          gfx::FinishFigureRule_CloseFigure)))
          continue;
        if (SDK_FAILED(path->FinishEdit()))
          continue;

        entry.m_path = path;
      }
      else if (geometry_type == geometry::kGMT_MultiSurface)
      {
        geometry::IGeometryCollectionSP geometry_collection =
          GetInterfaceT<geometry::IGeometryCollection>(coverage);
        if (!geometry_collection)
          continue;
        SDKUInt32 geometry_count = 0;
        if (SDK_FAILED(geometry_collection->GetGeometryCount(geometry_count)))
          continue;
        if (!geometry_count)
          continue;

        gfx::GraphicsPathSP path;
        if (SDK_FAILED(rtf->CreateGraphicsPath(path)) || !path)
          continue;
        gfx::GraphicsPathEditorSP path_editor;
        if (SDK_FAILED(path->StartEdit(path_editor)) || !path_editor)
          continue;

        for (SDKUInt32 gm = 0; gm < geometry_count; ++gm)
        {
          geometry::IGeometrySP surface;
          if (SDK_FAILED(geometry_collection->GetGeometry(gm, &surface)))
            continue;
          if (!CrackSurface(surface, points) || points.empty())
            continue;

          coord_transform->ForwardIF(static_cast<SDKUInt32>(points.size()),
            &points.front(), reinterpret_cast<PointF2D*>(&points.front()));

          if (SDK_FAILED(path_editor->StartFigure(
            reinterpret_cast<SDKPointF2D&>(points[0]),
            gfx::StartFigureStyle_Filled)))
            continue;
          path_editor->AddLines(reinterpret_cast<SDKPointF2D*>(&points[1]),
            static_cast<SDKUInt32>(points.size() - 1));
          path_editor->FinishFigure(gfx::FinishFigureRule_CloseFigure);
        }

        path->FinishEdit();
        entry.m_path = path;
      }
      else
        continue;

      m_coverage_list.Put(did, entry);
    }
  }

  // Limit coverage container to store not more than 5000 entries
  m_coverage_list.ShrinkToSize(5000);

  return true;
}

bool CoverageRenderer::CrackSurface(const geometry::IGeometrySP& geometry,
  std::vector<GeoIntPoint>& points)
{
  geometry::IGeometryCollectionSP geometry_collection =
    GetInterfaceT<geometry::IGeometryCollection>(geometry);

  // If surface geometry is not a collection, read first polygon points
  if (!geometry_collection)
  {
    geometry::IPointCollectionSP point_collection;
    if (SDK_FAILED(geometry->GetPointCollection(NULL, &point_collection)))
      return false;
    SDKUInt32 item_count = 0;
    if (SDK_FAILED(point_collection->GetItemCount(item_count)))
      return false;
    if (!item_count)
      return false;
    SDKUInt32 point_count = 0;
    // Take only an external ring
    if (SDK_FAILED(point_collection->GetItemPointCount(0, point_count)))
      return false;

    points.resize(point_count);
    return SDK_OK(point_collection->GetPoints(0, point_count, 
      geometry::kGMPT_GeoIntPoint, &points.front()));
  }
  else // If surface geometry is a collection, read first ring points
  {
    SDKUInt32 ring_count = 0;
    if (SDK_FAILED(geometry_collection->GetGeometryCount(ring_count)))
      return false;
    if (!ring_count)
      return false;
    // Take only the external ring
    geometry::IGeometrySP ring;
    if (SDK_FAILED(geometry_collection->GetGeometry(0, &ring)))
      return false;
    geometry::IPointCollectionSP point_collection;
    if (SDK_FAILED(ring->GetPointCollection(NULL, &point_collection)))
      return false;
    SDKUInt32 point_count = 0;
    if (SDK_FAILED(point_collection->GetPointCount(point_count)))
      return false;

    points.resize(point_count);
    return SDK_OK(point_collection->GetPoints(0, point_count,
      geometry::kGMPT_GeoIntPoint, &points.front()));
  }
}

