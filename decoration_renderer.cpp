// DecorationRenderer.cpp : Renders the decoration texts above chart.
//

#include <base/inc/sdk_string_handler.h>
#include <base/inc/math/matrix3x2.h>
#include <visualizationlayer/inc/graphics/2d_render_target_factory_interface.h>

#include "decoration_renderer.h"

DecorationRenderer::DecorationRenderer(const S52ResourceManagerSP& s52_res_manager)
  : kFontFamilyName(L"Segoe UI"),
    kFontStyle(sdk::gfx::FontStyle_Default),
    kFontWeight(sdk::gfx::FontWeight_Bold),
    kFontSize(18.0f),
    kTextColor(0.0f, 0.0f, 0.0f, 1.0f),
    m_s52_resource_manager(s52_res_manager),
    m_ref(0),
    m_text(),
    m_render_target(),
    m_rt_texts()
{
}

DecorationRenderer::~DecorationRenderer()
{
}

SDKUInt32 DecorationRenderer::AddRef() const throw()
{
  ++m_ref;
  return m_ref;
}

SDKUInt32 DecorationRenderer::Release() const throw()
{
  unsigned long references = --m_ref;
  if (0 == references)
    delete this;

  return references;
}

SDKResult DecorationRenderer::GetInterface(const sdk::Uuid& iid, void** obj_ptr) throw()
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

SDKResult DecorationRenderer::GetRendererID(SDKString& id) throw()
{
  id = sdk::ScopedString("Decoration renderer").Detach();
  return sdk::Ok;
}

SDKResult DecorationRenderer::Cancel() throw()
{
  return sdk::Err_NotImpl; // Rendering operation can't be cancelled
}

SDKResult DecorationRenderer::SetContext(
  const sdk::vis::scene::ResourceType& layer_resource_type,
  const sdk::vis::scene::ILayerResourceSP& layer_resource,
  const sdk::vis::scene::IGraphicContextSP& graphic_context) throw()
{
  if (!graphic_context || !layer_resource)
    return sdk::Err_InvalidArg;
  if (sdk::vis::scene::kResourceType_RenderTarget != layer_resource_type)
    return sdk::Err_NotSupported;

  sdk::vis::scene::ILayerResourceRenderTargetSP render_target_resource;
  if (SDK_FAILED(layer_resource->GetInterface(
    sdk::vis::scene::ILayerResourceRenderTarget::IID(),
    reinterpret_cast<void**>(&render_target_resource))) || !render_target_resource)
    return sdk::Err_InternalError;

  // Getting render target
  if (SDK_FAILED(render_target_resource->GetRenderTarget(m_render_target)) || !m_render_target)
    return sdk::Err_InternalError;

  return sdk::Ok;
}

SDKResult DecorationRenderer::Render(
  const sdk::vis::scene::IRenderContextSP& context) throw()
{
  if (!m_render_target || !m_s52_resource_manager)
    return sdk::Err_Uninitialized;

  sdk::gfx::RTAutoStartFinishDraw auto_start_finish_draw(m_render_target);
  if (!auto_start_finish_draw.IsDrawStarted())
    return sdk::Err_InternalError;

  m_render_target->SetAntiAliasingMode(sdk::gfx::AntiAliasingMode_PerPrimitive);

  SDKSize size;
  if (SDK_FAILED(m_render_target->GetRenderTargetSize(size)))
    return sdk::Err_InternalError;

  // Changing coordinate system
  sdk::SizeF render_target_size(float(size.width), float(size.height));

  sdk::CMatrix3X2 matrix_translate;
  matrix_translate.Translate(-render_target_size.width / 2.0f + 10,
    render_target_size.height / 2.0f - 10);
  m_render_target->PushTransformationMatrix(matrix_translate);

  // Filling up the background with transparent color
  m_render_target->FillBackground(sdk::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

  // Creating a brush to use for drawing
  sdk::gfx::RenderTargetBrushSP brush;
  if (SDK_FAILED(m_render_target->CreateSolidColorBrush(
    m_s52_resource_manager->GetColor(sdk::vis::s52::kColorIndex_CHBLK), brush))
    || !brush)
    return sdk::Err_InternalError;

  // Writing each of texts now
  sdk::PointF2D origin_point;
  for (size_t c = 0; c < m_rt_texts.size(); ++c)
  {
    m_render_target->WriteText(origin_point, m_rt_texts[c], brush);

    // Calculating next text position
    sdk::RectF2D text_bounds;
    m_render_target->CalcTextBounds(origin_point, m_rt_texts[c], text_bounds);
    origin_point.y -= text_bounds.height;
  }

  return sdk::Ok;
}

SDKResult DecorationRenderer::SetProperty(const sdk::SDKPropertyID& id,
  const SDKAny& value) throw()
{
  // No properties supported
  return sdk::Err_NotImpl;
}

SDKResult DecorationRenderer::GetProperty(const sdk::SDKPropertyID& id,
  SDKAny& value) const throw()
{
  // No properties supported
  return sdk::Err_NotImpl;
}

void DecorationRenderer::SetDecorationText(const DecorationLayerText& text)
{
  if (!m_render_target)
    return;

  sdk::gfx::RenderTargetFactorySP rtf;
  if (SDK_FAILED(m_render_target->GetParent(rtf)) || !rtf)
    return;

  // Building graphic text objects for each of strings in source text
  m_rt_texts.resize(text.size());
  m_text.resize(text.size());

  for (size_t c = 0; c < text.size(); ++c)
  {
    if (c < m_text.size() && m_text[c] == text[c])
      continue; // Render target text object does not requires any changes

    // Creating appropriate render target text object
    sdk::gfx::RenderTargetTextSP rt_text;
    if (SDK_FAILED(m_render_target->CreateText(sdk::ScopedString(text[c]),
      sdk::ScopedString(kFontFamilyName), kFontStyle, kFontWeight, kFontSize,
      rt_text)) || !rt_text)
      continue;

    // Using anti-aliased text
    rt_text->SetAntiAliasingMode(sdk::gfx::TextAntiAliasingMode_GrayScale);

    m_rt_texts[c] = rt_text;
    m_text[c] = text[c];
  }
}
