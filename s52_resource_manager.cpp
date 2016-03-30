// S52ResourceManager.cpp : manager of s52 resources.
//

#include <base/inc/platform.h>
#include <base/inc/base_types.h>
#include <base/inc/color/color_base_types_helpers.h>
#include <base/inc/base_library/framework_interface.h>
#include <visualizationlayer/inc/portrayal/sdl/sdl_ids.h>

#include "s52_resource_manager.h"

S52ResourceManager::S52ResourceManager()
  : m_palette_index(sdk::vis::s52::kPaletteIndex_DAY) {
}

S52ResourceManager::~S52ResourceManager() {
}

SDKColorF S52ResourceManager::GetColor(
  const sdk::vis::s52::ColorIndexEnum& color_index) {
  if (!m_sdl_catalog_factory && SDK_FAILED(Init()))
    return sdk::ColorF();

  sdk::vis::sdl::ISymbolSetCatalogSP sdl_catalog;
  if (SDK_FAILED(m_sdl_catalog_factory->GetCurrentCatalog(sdl_catalog)))
    return sdk::ColorF();

  return GetColor(sdl_catalog, color_index);
}

bool S52ResourceManager::SetPalette(
  const sdk::vis::s52::PaletteIndexEnum& palette_index) {
  if (palette_index >= SDK_ARRAY_LENGTH(sdk::vis::s52::kPaletteNames))
    return false;

  m_palette_index = palette_index;
  return true;
}

SDKColorF S52ResourceManager::GetColor(
  const sdk::vis::sdl::ISymbolSetCatalogSP& sdl_catalog,
  const sdk::vis::s52::ColorIndexEnum& color_index)
{
  if (!sdl_catalog)
    return sdk::ColorF();

  sdk::vis::sdl::IPaletteManagerSP palette_manager;
  SDKResult res = sdl_catalog->GetPaletteManager(palette_manager);
  if (SDK_FAILED(res) || !palette_manager)
    return sdk::ColorF();

  SDKUInt32 rgba;
  res = palette_manager->GetColor(
    sdk::ScopedString(sdk::vis::s52::kPaletteNames[m_palette_index]), 
    sdk::ScopedString(sdk::vis::s52::kSymbolSetName), 
    sdk::ScopedString(sdk::vis::s52::kColorIndexTable[color_index].name), rgba);
  if (SDK_FAILED(res))
    return sdk::ColorF();

  return sdk::ColorF(rgba);
}

SDKResult S52ResourceManager::Init() {
  if (m_sdl_catalog_factory)
    return sdk::Ok;

  sdk::SDKComponentSP sdl_catalog_factory_comp;
  SDKResult res = SDKCreateComponentInstance(NULL,
    sdk::vis::sdl::kSDLSymbolSetCatalogFactoryID, &sdl_catalog_factory_comp);
  if (SDK_FAILED(res))
    return res;

  m_sdl_catalog_factory =
    sdk::GetInterfaceT<sdk::vis::sdl::ISymbolSetCatalogFactory>(
      sdl_catalog_factory_comp);
  if (!m_sdl_catalog_factory)
    return sdk::Err_Unexpected;

  return sdk::Ok;
}
