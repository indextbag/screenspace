#include "screenspace/CustomGeometryOverride.hh"
#include "screenspace/Log.hh"
#include "screenspace/Types.hh"
#include "screenspace/PickerShape.hh"
#include "CustomGeometryOverride.hh"


#include <maya/MBoundingBox.h>
#include <maya/MUserData.h>
#include <maya/MPxNode.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MStringArray.h>
#include <maya/MNodeClass.h>
#include <maya/MFrameContext.h>
#include <maya/MFnPluginData.h>
#include <maya/MGeometryRequirements.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxGeometryOverride.h>
#include <maya/MVectorArray.h>
#include <maya/MShaderManager.h>
#include <maya/MUIDrawManager.h>
#include <maya/MUintArray.h>
#include <maya/MPlug.h>
#include <vector>


#include <cmath>
#include <maya/MPointArray.h>


namespace screenspace {

MString CustomGeometryOverride::classifcation = "drawdb/geometry/screenspace/custom";
MString CustomGeometryOverride::id = "custom";

const MString s_shadedItemName = "screenspaceMeshShaded";
const MString s_wireframeItemName = "axleMeshWireframe";
const MString s_selectedWireframeItemName = "axleSelectedWireframe";

struct PickerData {
  MColor color;
  MMatrix matrix;
};

MHWRender::MPxGeometryOverride* CustomGeometryOverride::creator(const MObject& obj) {
  return new CustomGeometryOverride(obj);
}

CustomGeometryOverride::CustomGeometryOverride(const MObject& obj)
    : MHWRender::MPxGeometryOverride(obj),
      m_node(obj),
      m_data(new PickerData())
{}

bool CustomGeometryOverride::isIndexingDirty(const MHWRender::MRenderItem& item) {
  return true;
}

bool CustomGeometryOverride::isStreamDirty(const MHWRender::MVertexBufferDescriptor& desc) {
  return true;
}

MHWRender::DrawAPI CustomGeometryOverride::supportedDrawAPIs() const
{
  return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

void CustomGeometryOverride::addUIDrawables(const MDagPath& pickerDag,
                                            MHWRender::MUIDrawManager& drawManager,
                                            const MHWRender::MFrameContext& frameContext) {
  TNC_DEBUG << "addUIDrawables...?";
}

void CustomGeometryOverride::updateDG()
{

  TNC_DEBUG << "updateDG...?";

  const MNodeClass pickerCls(PickerShape::id);

  MColor color;
  MFnNumericData colorData(MPlug(m_node, pickerCls.attribute("color")).asMObject());
  CHECK_MSTATUS(colorData.getData(color.r, color.g, color.b));
  CHECK_MSTATUS(MPlug(m_node, pickerCls.attribute("opacity")).getValue(color.a));

  MMatrix matrix;
  matrix[3][0] = 3.0f;
  m_data->matrix = matrix;

  m_data->color = color;
}

void CustomGeometryOverride::updateRenderItems(const MDagPath& path, MHWRender::MRenderItemList& renderItems)
{

  TNC_DEBUG << "updateRenderItems...?";

  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  if (!renderer)
    return;

  const MHWRender::MShaderManager* shaderManager = renderer->getShaderManager();
  if (!shaderManager)
    return;

  updateShadedRenderItem(path, renderItems);
  updateSelectedWireframeRenderItem(path, renderItems);
//  updateWireframeRenderItem(path, renderItems);
}

void CustomGeometryOverride::updateShadedRenderItem(const MDagPath& path, MHWRender::MRenderItemList& renderItems)
{
  TNC_DEBUG << "updateShadedRenderItem()";
  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  const MHWRender::MShaderManager* shaderManager = renderer->getShaderManager();

  MHWRender::MRenderItem* renderItem = nullptr;

  int index = renderItems.indexOf(s_shadedItemName);
  if (index == -1)
  {
    renderItem = MHWRender::MRenderItem::Create(s_shadedItemName,
                                                MHWRender::MRenderItem::DecorationItem,
                                                MHWRender::MGeometry::kTriangles);
    renderItem->setDrawMode(MHWRender::MGeometry::kShaded);
    renderItem->depthPriority(MHWRender::MRenderItem::sDormantFilledDepthPriority);

    MHWRender::MShaderInstance* shader = shaderManager->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
    if (shader)
    {
      renderItem->setShader(shader);
      shaderManager->releaseShader(shader);
    }

    renderItems.append(renderItem);
  } else {
    renderItem = renderItems.itemAt(index);
  }

  if (renderItem)
  {
    MHWRender::MShaderInstance* shader = renderItem->getShader();
    if (shader)
    {
      float color[4];
      m_data->color.get(MColor::kRGB, color[0], color[1], color[2], color[3]);
      shader->setParameter("solidColor", color);
    }
    renderItem->enable(true);
    renderItem->setMatrix(&m_data->matrix);
  }
}

void CustomGeometryOverride::updateSelectedWireframeRenderItem(const MDagPath& path, MHWRender::MRenderItemList& renderItems)
{
  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  const MHWRender::MShaderManager* shaderManager = renderer->getShaderManager();

  MHWRender::MRenderItem* renderItem = nullptr;

  int index = renderItems.indexOf(s_selectedWireframeItemName);

  if (-1 == index)
  {
    renderItem = MHWRender::MRenderItem::Create(s_selectedWireframeItemName,
                                                MHWRender::MRenderItem::DecorationItem,
                                                MHWRender::MGeometry::kLines);

    renderItem->setDrawMode(MHWRender::MGeometry::kAll);
    renderItem->depthPriority(MHWRender::MRenderItem::sActiveWireDepthPriority);

    MHWRender::MShaderInstance* shader = shaderManager->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
    if (shader)
    {
      renderItem->setShader(shader);
      shaderManager->releaseShader(shader);
    }

    renderItems.append(renderItem);
  } else {
    renderItem = renderItems.itemAt(index);
  }

  if (renderItem)
  {
    MHWRender::DisplayStatus displayStatus = MHWRender::MGeometryUtilities::displayStatus(path);
    MColor wireColor = MHWRender::MGeometryUtilities::wireframeColor(path);

    MHWRender::MShaderInstance* shader = renderItem->getShader();
    switch (displayStatus)
    {
      case MHWRender::kLead:
        renderItem->enable(true);
        if (shader)
        {
          static const float color[] = {0.0f, 0.8f, 0.0f, 1.0f};
          shader->setParameter("solidColor", color);
        }
        break;
      case MHWRender::kActive:
        if (shader)
          shader->setParameter("solidColor", &(wireColor.r));
        renderItem->enable(true);
        renderItem->setMatrix(&m_data->matrix);
        break;
      case MHWRender::kHilite:
      case MHWRender::kActiveComponent:
        if (shader)
        {
          static const float color[] = {0.0f, 0.5f, 0.7f, 1.0f};
          shader->setParameter("solidColor", color);
        }
        renderItem->enable(true);
        renderItem->setMatrix(&m_data->matrix);
        break;
      default:
        renderItem->enable(false);
        renderItem->setMatrix(&m_data->matrix);
        break;
    };
  }
}

void CustomGeometryOverride::updateWireframeRenderItem(const MDagPath& path, MHWRender::MRenderItemList& renderItems)
{
//  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
//  const MHWRender::MShaderManager* shaderManager = renderer->getShaderManager();
//
//  MHWRender::MRenderItem* renderItem = NULL;
//
//  int index = renderItems.indexOf(s_wireframeItemName);
//
//  if (-1 == index)
//  {
//    renderItem = MHWRender::MRenderItem::Create(s_wireframeItemName,
//                                                MHWRender::MRenderItem::DecorationItem,
//                                                MHWRender::MGeometry::kLines
//    );
//
//    renderItem->setDrawMode(MHWRender::MGeometry::kWireframe);
//    renderItem->depthPriority(MHWRender::MRenderItem::sDormantFilledDepthPriority);
//
//    MHWRender::MShaderInstance* shader = shaderManager->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
//
//    if (shader)
//    {
//      renderItem->setShader(shader);
//      shaderManager->releaseShader(shader);
//    }
//
//    renderItems.append(renderItem);
//  } else {
//    renderItem = renderItems.itemAt(index);
//  }
//
//  if (renderItem)
//  {
//    MHWRender::MShaderInstance* shader = renderItem->getShader();
//
//    if (shader)
//    {
//      MColor c = MHWRender::MGeometryUtilities::wireframeColor(path);
//      float color[] = { c.r, c.g, c.b, 1.0f };
//      shader->setParameter("solidColor", color);
//    }
//
//    renderItem->enable(true);
//  }
}

void CustomGeometryOverride::populateGeometry(const MHWRender::MGeometryRequirements& requirements, const MHWRender::MRenderItemList& renderItems, MHWRender::MGeometry& geo)
{
  fillVertexBuffers(requirements, geo);
  fillIndexBuffers(renderItems, geo);
}

void CustomGeometryOverride::fillVertexBuffers(const MHWRender::MGeometryRequirements& requirements, MHWRender::MGeometry& geo)
{
  const MHWRender::MVertexBufferDescriptorList& descList = requirements.vertexRequirements();

  for (int i = 0; i < descList.length(); i++) {

    MHWRender::MVertexBufferDescriptor desc;
    if (!descList.getDescriptor(i, desc)) {
      continue;
    }

    switch (desc.semantic())
    {
      case MHWRender::MGeometry::kPosition:
      {
        MHWRender::MVertexBuffer* vertexBuffer = geo.createVertexBuffer(desc);
        if (vertexBuffer) {
          MPointArray vertices;
          vertices.append(MPoint(0.0, 0.0, 0.0, 1.0));
          vertices.append(MPoint(1.0, 0.0, 0.0, 1.0));
          vertices.append(MPoint(1.0, 1.0, 0.0, 1.0));
          vertices.append(MPoint(0.0, 1.0, 0.0, 1.0));

          float* buffer = reinterpret_cast<float*>(vertexBuffer->acquire(vertices.length() * 3, true));
          if (buffer) {
            for (unsigned int i = 0; i < vertices.length(); i++) {
              buffer[i*3+0] = vertices[i][0];
              buffer[i*3+1] = vertices[i][1];
              buffer[i*3+2] = vertices[i][2];
            }
            vertexBuffer->commit(buffer);
          }
        }
        break;
      }
      case MHWRender::MGeometry::kNormal:
      case MHWRender::MGeometry::kInvalidSemantic:
      case MHWRender::MGeometry::kTexture:
      case MHWRender::MGeometry::kColor:
      case MHWRender::MGeometry::kTangent:
      case MHWRender::MGeometry::kBitangent:
      case MHWRender::MGeometry::kTangentWithSign:
      default:
        ;
    }
  }
}

void CustomGeometryOverride::fillIndexBuffers(const MHWRender::MRenderItemList& renderItems, MHWRender::MGeometry& geo)
{
  for (int i = 0; i < renderItems.length(); i++)
  {
    const MHWRender::MRenderItem* renderItem = renderItems.itemAt(i);
    if (!renderItem) {
      continue;
    }

    MHWRender::MIndexBuffer* indexBuffer = geo.createIndexBuffer(MHWRender::MGeometry::kUnsignedInt32);
    if (!indexBuffer) {
      continue;
    }

    if (renderItem->primitive() == MHWRender::MGeometry::kTriangles)
    {
      MUintArray indices;
      for (unsigned int index: {0, 1, 2, 0, 2, 3})
        indices.append(index);

      unsigned int* buffer = reinterpret_cast<unsigned int*>(indexBuffer->acquire(indices.length(), true));
      if (buffer) {
        for (unsigned int i = 0; i < indices.length(); i++) {
          buffer[i] = indices[i];
        }
        indexBuffer->commit(buffer);
      }
    }
    else if (renderItem->primitive() == MHWRender::MGeometry::kLines)
    {
      MUintArray indices;
      for (unsigned int index: {0, 1, 1, 2, 2, 3, 3, 0})
        indices.append(index);

      unsigned int* buffer = reinterpret_cast<unsigned int*>(indexBuffer->acquire(indices.length(), true));
      if (buffer) {
        for (unsigned int i = 0; i < indices.length(); i++) {
          buffer[i] = indices[i];
        }
        indexBuffer->commit(buffer);
      }
    }

    renderItem->associateWithIndexBuffer(indexBuffer);
  }
}


}