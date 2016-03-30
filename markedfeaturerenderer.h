// MarkedFeatureRenderer.h : Renders the marked feature object above chart
//
#ifndef MARKEDFEATURERENDERER_H
#define MARKEDFEATURERENDERER_H
#pragma once

#include <vector>
#include <base/inc/platform.h>
#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_ref_ptr.h>
#include <base/inc/color/color_base_types_helpers.h>
#include <geometry/inc/coordinate_systems/crs_factory.h>
#include <visualizationlayer/inc/visman/scene_manager_interface.h>
#include <visualizationlayer/inc/scene/renderer_interface.h>
#include <visualizationlayer/inc/scene/layer_resource_interface.h>
#include <visualizationlayer/inc/graphics/2d_render_target_interface.h>
#include "s52_resource_manager.h"

class MarkedFeatureRenderer;
typedef sdk::SDKRefPtr<MarkedFeatureRenderer> MarkedFeatureRendererSP;

class MarkedFeatureRenderer : public sdk::vis::scene::IRenderer
{
public:
  MarkedFeatureRenderer(
    const sdk::gdb::IWorkspaceFactorySP wks_factory,
    const S52ResourceManagerSP& s52_res_manager);
  virtual ~MarkedFeatureRenderer();

  virtual SDKUInt32 SDK_CALLTYPE AddRef() const throw();
  virtual SDKUInt32 SDK_CALLTYPE Release() const throw();
  virtual SDKResult SDK_CALLTYPE GetInterface(const sdk::Uuid& iid, 
    void** obj_ptr) throw();

  SDKResult SDK_CALLTYPE GetRendererID(SDKString& id) throw();
  SDKResult SDK_CALLTYPE Cancel() throw();
  SDKResult SDK_CALLTYPE SetContext(
    const sdk::vis::scene::ResourceType& layer_resource_type,
    const sdk::vis::scene::ILayerResourceSP& layer_resource,
    const sdk::vis::scene::IGraphicContextSP& graphic_context) throw();
  SDKResult SDK_CALLTYPE Render(
    const sdk::vis::scene::IRenderContextSP& context) throw();
  SDKResult SDK_CALLTYPE SetProperty(const sdk::SDKPropertyID& id, 
    const SDKAny& value) throw();
  SDKResult SDK_CALLTYPE GetProperty(const sdk::SDKPropertyID& id, 
    SDKAny& value) const throw();

  bool SetMark(const sdk::gdb::ObjectID& oid, const sdk::crs::IProjectionSP& projection,
    sdk::GeoIntPoint& feature_object_position, double& dataset_min_disp_scale);
  bool RemoveMark();

private:
  // Extracts points from IGeometry
  bool CrackGeometry(const sdk::geometry::IGeometrySP& geometry,
    std::vector<sdk::GeoIntPoint>& points);

private:
  // Workspaces factory
  const sdk::gdb::IWorkspaceFactorySP m_wks_factory;
  // S-52 resource manager
  const S52ResourceManagerSP          m_s52_resource_manager;

  // Number of references
  mutable SDKUInt32                   m_ref;

  // Render target to use for text drawing
  sdk::gfx::RenderTargetSP            m_render_target;

  // Feature object graphic path
  sdk::gfx::GraphicsPathSP            m_feature_object_path;

  // Dataset base scale
  double                              m_base_scale;
  // Dataset base center
  sdk::GeoIntPoint                    m_base_center;
  // Feature geometry type
  sdk::geometry::GeometryType         m_feature_geometry_type;
};
#endif // MARKEDFEATURERENDERER_H
