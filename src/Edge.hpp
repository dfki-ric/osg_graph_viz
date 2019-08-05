/**
 * \file Edge.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef OSG_GRAPH_VIZ_EDGE_HPP
#define OSG_GRAPH_VIZ_EDGE_HPP

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/LineWidth>

#include <configmaps/ConfigMap.hpp>
#include <mars/osg_text/Text.h>

namespace osg_material_manager {
  class OsgMaterialManager;
}

namespace osg_graph_viz {

  class View;
  class Node;

  class Edge : public osg::Group {
    friend class View;

  public:
    Edge(const configmaps::ConfigMap &map, View *v, const double port_size_y);
    ~Edge();

    void dirty(void);
    int checkMousePress(double x, double y);
    void updateVPos(int i, osg::Vec3 v);
    void updateStartPos(osg::Vec3 v);
    void updateEndPos(osg::Vec3 v);
    void setLineWidth(double w);
    void setColor(osg::Vec4 v);
    void setMiddlePos(double x);
    void mouseMove(double x, double y);
    const configmaps::ConfigMap& getMap() {return info;}
    void setStartOffset(double v) {startOffset = v;}
    void setEndOffset(double v) {endOffset = v;}
    void setStartNode(osg_graph_viz::Node* node);
    osg_graph_viz::Node* getStartNode();
    void setEndNode(osg_graph_viz::Node* node);
    osg_graph_viz::Node* getEndNode();
    osg::Vec3 getEndPosition();
    osg::Vec3 getStartPosition();
    void removeFromNodes();
    void updateMap(const configmaps::ConfigMap &map);
    void savePosOffset(double x, double y);
    void setPosition2(double x, double y);
    void setSelected(bool v);
    bool isSelected() {return selected;}
    void decoupleEdge();
    void decouple(double threshhold);
    bool isDecoupled();
    void derenderText(const bool readable);
    void reposition();
    void updateToNode(std::string nodeName, std::string portName);
    void updateFromNode(std::string nodeName, std::string portName);
    void limit_y_offset(const double middle, double *desire, double *offset);
    void hide(bool v);
    void setSmooth(bool v);
    void exportSvg(FILE *f, double ol, double ot);
    void getRectangle(double *x1, double *x2, double *y1, double *y2);

  private:
    View *view;
    osg_material_manager::OsgMaterialManager *materialManager;
    osg::ref_ptr<osg::Uniform> startUniform, endUniform, colorUniform;
    configmaps::ConfigMap info;
    osg::ref_ptr<osg::Geode> geode;
    osg::ref_ptr<osg::Geometry> geom, decoupleGeom, smoothGeom;
    osg::ref_ptr<osg::LineWidth> linew;
    osg::ref_ptr<osg::Vec4Array> color;
    osg::ref_ptr<osg_graph_viz::Node> startNode, endNode;
    osg::ref_ptr<osg::Vec3Array> vertices, decoupleVertices, smoothVertices;
    osg::ref_ptr<osg_text::Text> weight, decoupleIn, decoupleOut;
    bool horizontal, selected, decoupled, smooth, hidden;
    int node;
    int fromIdx, toIdx; // used for decouple information only
    double startOffset, endOffset, startPos, endPos, offsetLimit;
    double dOffsetX, dOffsetY;
    double posOffsetX, posOffsetY;
    void checkEndPositions();
    void updateWeightPos();
    void updateDecouplePos();
    void updateSmoothPos();
  };

} // end of namespace: osg_graph_viz

#endif // OSG_GRAPH_VIZ_EDGE_HPP
