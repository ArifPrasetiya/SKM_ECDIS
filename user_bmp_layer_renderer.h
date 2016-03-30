// user_bmp_layer_renderer.h : Renders the bitmap layer
//
#pragma once

#include <vector>
#include <base/inc/platform.h>
#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_ref_ptr.h>
#include <visualizationlayer/inc/scene/renderer_interface.h>
#include <visualizationlayer/inc/scene/layer_resource_interface.h>
#include <visualizationlayer/inc/scene/texture_interface.h>
#include "glwidget.h"

class UserBmpLayerRenderer;
typedef sdk::SDKRefPtr<UserBmpLayerRenderer> UserBmpLayerRendererSP;

class UserBmpLayerRenderer : public sdk::vis::scene::IRenderer
{
public:
  UserBmpLayerRenderer();
  virtual ~UserBmpLayerRenderer();

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
  SDKResult SDK_CALLTYPE SetProperty(
    const sdk::SDKPropertyID& id, 
    const SDKAny& value) throw();
  SDKResult SDK_CALLTYPE GetProperty(
    const sdk::SDKPropertyID& id, 
    SDKAny& value) const throw();

  void SetBits(QImage& image);

private:
  // Number of references
  mutable volatile SDKInt32 m_ref;

  // Render target
  sdk::gfx::RenderTargetSP  m_render_target;

  // Current bitmap
  sdk::gfx::RenderTargetBitmapSP m_bitmap;

  // Source of data
  mutable QMutex m_lock;
  QImage         m_image;
  bool           m_is_new_data;
};