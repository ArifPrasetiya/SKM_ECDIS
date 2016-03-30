// S52ResourceManager.h : manager of s52 resources.
//

#ifndef S52_RESOURCE_MANAGER_H
#define S52_RESOURCE_MANAGER_H
#pragma once

#include <memory>
#include <base/inc/platform.h>
#include <base/inc/base_types.h>
#include <base/inc/sdk_results_enum.h>
#include <visualizationlayer/inc/portrayal/sdl/sdl_interface.h>
#include <visualizationlayer/inc/portrayal/csp/s52_const.h>

class S52ResourceManager;
typedef std::tr1::shared_ptr<S52ResourceManager> S52ResourceManagerSP;

class S52ResourceManager
{
public:
  S52ResourceManager();
  ~S52ResourceManager();

  SDKColorF GetColor(const sdk::vis::s52::ColorIndexEnum& color_index);
  bool      SetPalette(const sdk::vis::s52::PaletteIndexEnum& palette_index);

  SDKColorF GetColor(const sdk::vis::sdl::ISymbolSetCatalogSP& sdl_catalog,
    const sdk::vis::s52::ColorIndexEnum& color_index);

protected:
  SDKResult Init();

private:
  sdk::vis::sdl::ISymbolSetCatalogFactorySP m_sdl_catalog_factory;
  sdk::vis::s52::PaletteIndexEnum m_palette_index;
};
#endif // S52_RESOURCE_MANAGER_H
