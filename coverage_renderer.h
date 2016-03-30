// CoverageRenderer.h : renderer for data coverages collection.
//
#ifndef COVERAGE_RENDERER_H
#define COVERAGE_RENDERER_H
#pragma once

#include <vector>
#include <list>
#include <map>

#include <base/inc/platform.h>
#include <base/inc/sdk_results_enum.h>
#include <base/inc/sdk_ref_ptr.h>
#include <base/inc/geometry/geometry_base_types_helpers.h>
#include <datalayer/inc/geodatabase/gdb_dataset.h>
#include <geometry/inc/coordinate_systems/crs_factory.h>
#include <geometry/inc/coordinate_systems/crs_coordinate_transformation.h>
#include <visualizationlayer/inc/scene/renderer_interface.h>
#include <visualizationlayer/inc/scene/layer_resource_interface.h>
#include <visualizationlayer/inc/graphics/2d_graphics_path_interface.h>
#include <visualizationlayer/inc/graphics/2d_render_target_brush_interface.h>
#include <visualizationlayer/inc/graphics/2d_render_target_stroke_style_interface.h>
#include <visualizationlayer/inc/graphics/2d_render_target_factory_interface.h>
#include <visualizationlayer/inc/graphics/2d_graphics_objects.h>

#include "s52_resource_manager.h"

class CoverageRenderer;
typedef sdk::SDKRefPtr<CoverageRenderer> CoverageRendererSP;

class CoverageRenderer : public sdk::vis::scene::IRenderer
{
public:
  CoverageRenderer(
    const S52ResourceManagerSP& s52_res_manager,
    const sdk::gdb::IWorkspaceFactorySP& wks_factory);
  ~CoverageRenderer();

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

  void SetViewportBounds(const sdk::RectF2D bounds) { m_bounds = bounds; }
  void SetWorkspaceName(const std::wstring& name) { m_wks_name = name; }
  bool ProjectionParametersChanged(const sdk::crs::IProjectionSP& projection);

private:
  bool CrackSurface(const sdk::geometry::IGeometrySP& geometry,
    std::vector<sdk::GeoIntPoint>& points);

private:
  // Container for coverage
  class CoverageList
  {
  public:
    typedef sdk::gdb::DatasetID KeyType;

    struct Entry
    {
      sdk::gfx::GraphicsPathSP m_path;
      double                   m_base_scale;
      sdk::GeoIntPoint         m_base_center;
      bool                     m_visible;
      std::string              m_dataset_name;

      Entry() throw() : m_visible(true) {}

      typedef std::pair<KeyType, Entry> value_type;
      typedef std::list<value_type> container;
    };
    typedef Entry::container::iterator iterator;
    typedef Entry::container::reverse_iterator reverse_iterator;
    typedef std::map<KeyType, Entry::container::iterator> KeyIndex;

    iterator Put(const KeyType& key, const Entry& entry)
    {
      KeyIndex::iterator index_iter = index_.find(key);
      if (index_iter != index_.end())
        Erase(index_iter->second);
      ordering_.push_front(Entry::value_type(key, entry));
      index_.insert(std::make_pair(key, ordering_.begin()));
      return ordering_.begin();
    }

    iterator Erase(iterator pos)
    {
      index_.erase(pos->first);
      return ordering_.erase(pos);
    }

    reverse_iterator Erase(reverse_iterator pos)
    {
      return reverse_iterator(Erase((++pos).base()));
    }

    void ShrinkToSize(size_t new_size)
    {
      for (size_t i = size(); i > new_size; i--)
      {
        reverse_iterator iter = rbegin();
        if (iter->second.m_visible)
          break;
        Erase(iter);
      }
    }

    // Retrieves the contents of the given key, or end() if not found.
    // This method moves the requested item to the front of the recency list.
    iterator Get(const KeyType& key)
    {
      KeyIndex::iterator index_iter = index_.find(key);
      if (index_iter == index_.end())
        return end();
      iterator iter = index_iter->second;
      // Move the touched item to the front of the recency ordering.
      ordering_.splice(ordering_.begin(), ordering_, iter);
      return ordering_.begin();
    }

    // Retrieves the contents of the given key, or end() if not found.
    // No ordering.
    iterator Peek(const KeyType& key) {
      KeyIndex::iterator index_iter = index_.find(key);
      if (index_iter == index_.end())
        return end();
      return index_iter->second;
    }

    void Clear()
    {
      index_.clear();
      ordering_.clear();
    }

    size_t size() const
    {
      return index_.size();
    }

    iterator begin() { return ordering_.begin(); }
    iterator end() { return ordering_.end(); }
    reverse_iterator rbegin() { return ordering_.rbegin(); }

  private:
    Entry::container ordering_;
    KeyIndex index_;
  };

  // S-52 resource manager
  const S52ResourceManagerSP                    m_s52_resource_manager;
  // Workspaces factory
  const sdk::gdb::IWorkspaceFactorySP           m_wks_factory;

  // References counter
  mutable SDKUInt32                             m_ref;

  // Render target resources
  sdk::gfx::RenderTargetSP                      m_render_target;
  sdk::gfx::RenderTargetStrokeStyleSP           m_stroke;

  sdk::RectF2D                                  m_bounds;
  CoverageList                                  m_coverage_list;
  std::wstring                                  m_wks_name;
};
#endif // COVERAGE_RENDERER_H
