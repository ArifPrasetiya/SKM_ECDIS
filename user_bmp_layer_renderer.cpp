// user_bmp_layer_renderer.cpp : Renders the bitmap layer
//

#include <base/inc/sdk_string_handler.h>
#include <base/inc/sdk_array_handler.h>
#include <base/inc/base_library/base_types_functions.h>
#include <base/inc/color/color_base_types_helpers.h>
#include <visualizationlayer/inc/vis_const.h>
#include <visualizationlayer/inc/graphics/2d_render_target_interface.h>
#include <visualizationlayer/inc/portrayal/sdl/sdl_interface.h>
#include <visualizationlayer/inc/portrayal/csp/s52_const.h>
#include <visualizationlayer/inc/visman/layers_manager_interface.h>
#include "user_bmp_layer_renderer.h"

#if defined(SDK_OS_LINUX)
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace SDK_NAMESPACE;
//using namespace SDK_GFX_NAMESPACE;
using namespace sdk::gfx; // Define has been added in the latest sdk release
using namespace SDK_VIS_NAMESPACE;
using namespace SDK_SCENE_NAMESPACE;

UserBmpLayerRenderer::UserBmpLayerRenderer()
  : m_ref(0),
    m_render_target(),
    m_bitmap(),
    m_image(),
    m_is_new_data(false) {
}

UserBmpLayerRenderer::~UserBmpLayerRenderer() {
}

SDKUInt32 UserBmpLayerRenderer::AddRef() const throw() {
  SDKAtomicRefCountInc(&m_ref);
  return m_ref;
}

SDKUInt32 UserBmpLayerRenderer::Release() const throw() {
  SDKResult is_non_zero = SDKAtomicRefCountDec(&m_ref);
  if (!is_non_zero)
    delete this;
  return is_non_zero ? m_ref : 0;
}

SDKResult UserBmpLayerRenderer::GetInterface(
  const Uuid& iid, void** obj_ptr) throw() {

  if (NULL == obj_ptr)
    return Err_NULLPointer;
  if (NULL != *obj_ptr)
    return Err_AlreadyInitialized;

  if (IsEqualUuid(iid, IRenderer::IID()))
    *obj_ptr = static_cast<IRenderer*>(this);
  else if (sdk::IsEqualUuid(iid, ISDKComponent::IID()))
    *obj_ptr = static_cast<ISDKComponent*>(this);
  else
    return Err_NotImpl;

  AddRef();
  return Ok;
}

SDKResult UserBmpLayerRenderer::GetRendererID(SDKString& id) throw() {
  id = ScopedString("User BMP layer renderer").Detach(); 
  return sdk::Ok;
}

SDKResult UserBmpLayerRenderer::Cancel() throw() { 
  return Err_NotImpl; // Rendering operation can't be cancelled
}

SDKResult UserBmpLayerRenderer::SetContext(
  const ResourceType& layer_resource_type,
  const ILayerResourceSP& layer_resource, 
  const IGraphicContextSP& graphic_context) throw() {

  try {
    if (SDK_SCENE_NAMESPACE::kResourceType_RenderTarget != layer_resource_type)
      return Err_NotSupported;
    if (!graphic_context || !layer_resource)
      return Err_InvalidArg;

    ILayerResourceRenderTargetSP render_target_resource;
    if (SDK_FAILED(layer_resource->GetInterface(
      ILayerResourceRenderTarget::IID(), 
      reinterpret_cast<void**>(&render_target_resource))) || !render_target_resource)
      return Err_InternalError;

    // Get render target (used only when bitmap is not found)
    if (SDK_FAILED(render_target_resource->GetRenderTarget(m_render_target)))
      return Err_InternalError;

    return Ok;
  }
  catch (...) {}

  return Err_InternalError;
}

SDKResult UserBmpLayerRenderer::Render(
  const sdk::vis::scene::IRenderContextSP& context) throw() {

  try {

    bool is_new_data = false;
    QImage image;
    {
      QMutexLocker lock(&m_lock);
      if (m_is_new_data) {
        image.swap(m_image);
        is_new_data = true;
        m_is_new_data = false; // Data has been taken
      }
    }

    if (is_new_data) { // Recreate bitmap
      image = image.mirrored(false, true);
      sdk::Size size(image.width(), image.height());
      sdk::gfx::RenderTargetBitmapSP bitmap;
      if (SDK_FAILED(m_render_target->CreateBitmapFromRawData(size, 
        image.bytesPerLine(), kPixelFormat_BGRA_8888, image.constBits(), bitmap)))
        return Err_InternalError;
      m_bitmap = bitmap;
    }

    // Draw bitmap
    if (m_bitmap) {
      RTAutoStartFinishDraw auto_start_finish_draw(m_render_target);
      if (!auto_start_finish_draw.IsDrawStarted())
        return Err_InternalError;

      m_render_target->FillBackground(sdk::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

      Size bitmap_size;
      if (SDK_FAILED(m_bitmap->GetSize(bitmap_size)))
        return Err_InternalError;

      // Source bitmap is centered and enlarged four times
      SizeF source_size(bitmap_size.width, bitmap_size.height);
      RectF2D source_rect(sdk::PointF2D(0.0f, 0.0f), source_size);
      SizeF dest_size(bitmap_size.width * 4, bitmap_size.height * 4);
      RectF2D dest_rect(
        sdk::PointF2D(-dest_size.width / 2, -dest_size.height / 2), dest_size);

      m_render_target->DrawBitmap(m_bitmap, source_rect, dest_rect);
    }

    return Ok;
  }
  catch (...) {}

  return Err_InternalError;
}

SDKResult UserBmpLayerRenderer::SetProperty(const sdk::SDKPropertyID& id, 
  const SDKAny& value) throw() { 
  // No properties supported
  return Err_NotImpl; 
}

SDKResult UserBmpLayerRenderer::GetProperty(const sdk::SDKPropertyID& id, 
  SDKAny& value) const throw() { 
// This property has been added in the latest sdk release
//   switch (id) {
//     case kSceneRendererProperty_IsDirty: {
//       QMutexLocker lock(&m_lock);
//       value = ScopedAny(m_is_new_data);
//       break;
//     }
//     default:
//       return Err_NotFound; 
//   }
//   return Ok;
  return Err_NotImpl; 
}

void UserBmpLayerRenderer::SetBits(QImage& image) {

  QMutexLocker lock(&m_lock);
  m_image.swap(image);
  m_is_new_data = true;
}


