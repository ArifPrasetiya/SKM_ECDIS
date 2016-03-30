// DecorationRenderer.h : Renders the decoration texts above chart.
//
#ifndef DECORATION_RENDERER_H
#define DECORATION_RENDERER_H
#pragma once

#include <vector>
#include <base/inc/platform.h>
#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_ref_ptr.h>
#include <base/inc/color/color_base_types_helpers.h>
#include <visualizationlayer/inc/visman/scene_manager_interface.h>
#include <visualizationlayer/inc/scene/renderer_interface.h>
#include <visualizationlayer/inc/scene/layer_resource_interface.h>
#include <visualizationlayer/inc/graphics/2d_render_target_interface.h>
#include "s52_resource_manager.h"

class DecorationRenderer;
typedef sdk::SDKRefPtr<DecorationRenderer> DecorationRendererSP;

class DecorationRenderer : public sdk::vis::scene::IRenderer
{
public:
  DecorationRenderer(const S52ResourceManagerSP& s52_res_manager);
  virtual ~DecorationRenderer();

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

  typedef std::vector<std::wstring> DecorationLayerText;
  void SetDecorationText(const DecorationLayerText& text);

private:
  // Default font family name/style/size for text output
  const std::wstring              kFontFamilyName;
  const sdk::gfx::FontStyle       kFontStyle;
  const sdk::gfx::FontWeight      kFontWeight;
  const float                     kFontSize;
  const sdk::ColorF               kTextColor;

  // S-52 resource manager
  const S52ResourceManagerSP      m_s52_resource_manager;

  // Number of references
  mutable SDKUInt32               m_ref;

  // Decoration layer text
  DecorationLayerText             m_text;

  // Render target to use for text drawing
  sdk::gfx::RenderTargetSP        m_render_target;

  typedef std::vector<sdk::gfx::RenderTargetTextSP> RenderTargetTexts;
  RenderTargetTexts               m_rt_texts;
};
#endif // DECORATION_RENDERER_H
