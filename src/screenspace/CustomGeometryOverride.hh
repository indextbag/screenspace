#ifndef SAMPLEPLUGIN_CUSTOMGEOMETRYOVERRIDE_HH
#define SAMPLEPLUGIN_CUSTOMGEOMETRYOVERRIDE_HH

#include <maya/MPxGeometryOverride.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

namespace screenspace {

class PickerData;

class CustomGeometryOverride : public MHWRender::MPxGeometryOverride
{
public:
  static MString classifcation;
  static MString id;
  static MHWRender::MPxGeometryOverride* creator(const MObject& obj);

public:
  ~CustomGeometryOverride() override = default;
  MHWRender::DrawAPI supportedDrawAPIs() const override;

  bool isIndexingDirty(const MHWRender::MRenderItem& item) override;
  bool isStreamDirty(const MHWRender::MVertexBufferDescriptor& desc) override;

  void updateDG() override;
  void updateRenderItems(const MDagPath& path, MHWRender::MRenderItemList& renderItems) override;
  void updateWireframeRenderItem(const MDagPath& path, MHWRender::MRenderItemList& renderItems);
  void updateShadedRenderItem(const MDagPath& path, MHWRender::MRenderItemList& renderItems);
  void updateSelectedWireframeRenderItem(const MDagPath& path, MHWRender::MRenderItemList& renderItems);
  void populateGeometry(const MHWRender::MGeometryRequirements& requirements,
                        const MHWRender::MRenderItemList& renderItems,
                        MHWRender::MGeometry& geo) override;

  void fillVertexBuffers(const MHWRender::MGeometryRequirements& requirements, MHWRender::MGeometry& geo);
  void fillIndexBuffers(const MHWRender::MRenderItemList& renderItems, MHWRender::MGeometry& geo);
  void cleanUp() override {}

  bool hasUIDrawables() const override { return true; }

  void addUIDrawables(const MDagPath& pickerDag,
                      MHWRender::MUIDrawManager& drawManager,
                      const MHWRender::MFrameContext& frameContext) override;

protected:
  CustomGeometryOverride(const MObject& obj);

  MObject m_node;
  PickerData* m_data;
};

}
#endif // SAMPLEPLUGIN_CUSTOMGEOMETRYOVERRIDE_HH