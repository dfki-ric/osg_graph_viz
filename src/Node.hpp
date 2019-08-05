/**
 * \file Node.hpp
 * \author Malte Langosz
 * \brief
 **/

#ifndef OSG_GRAPH_VIZ_NODE_HPP
#define OSG_GRAPH_VIZ_NODE_HPP

#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <mars/osg_text/Text.h>
#include <configmaps/ConfigMap.hpp>

#include "Edge.hpp"

namespace osg_material_manager {
  class OsgMaterialManager;
}

namespace osg_graph_viz {

  class View;

  struct Color {
    float r, g, b, a;
  };

  class Frame {
  public:
    double w, h, x, y;
    osg::Vec4 bc, c;
    bool defined;
  };

  struct NodeInfo {
    int numInputs, numOutputs;
    bool redrawEdges;   // signal that a node's edges have to be redrawn dur to port changes
    std::string type;
    std::vector<std::string> inTexNames, outTexNames;
    configmaps::ConfigMap map;
  };

  struct Port {
    std::vector<osg::ref_ptr<osg_text::Text> > labels;
    osg::ref_ptr<osg::Geode> geode, foldIcon;
    std::list<osg::ref_ptr<Edge> > edges;
    osg::ref_ptr<osg::Group> group;
    Frame frame;
    Port *foldPort;
    int portPos;
    int foldState;
    double width;
    bool hidden;
  };

  class Node : public osg::Group {
    friend class View;

  public:
    Node(View *v);
    Node(const NodeInfo &info, View *v);
    ~Node();

    virtual void init(void);
    virtual void initContent(void);
    virtual void dirty(void);
    virtual void setPosition2(double x, double y);
    virtual void setPosition(double x, double y);
    virtual void setAbsolutePosition( double x, double y);
    virtual void addPosition(double x, double y);
    virtual void savePosOffset(double x, double y);
    virtual void convertPos(double *x, double *y);
    void convertPosToWorld(double *x, double *y);
    virtual void getPosition(double *x, double *y)
    {*x = posX; *y = posY; convertPosToWorld(x, y);}
    virtual bool checkMousePress(double x, double y, bool handleFold=true);
    virtual bool checkMouseHeaderPress(double x, double y);
    virtual void getPosOffset(double x, double y, double *ox, double *oy);
    virtual double getWidth() {return width;}
    virtual double getHeight() {return height;}
    virtual void getRectangle(double *x1, double *x2, double *y1, double *y2);
    virtual osg::Vec3 getInPortPos(int index);
    virtual bool hasInPortConnection(int index);
    virtual osg::Vec3 getOutPortPos(int index);
    virtual void addInputEdge(int index, Edge* edge);
    virtual void addOutputEdge(int index, Edge* edge);
    virtual void setRenderOrder(int o);
    virtual void setLineWidth(double w);
    virtual bool checkMouseInPortPress(double x, double y,
                                       double *vX, double *vY, int *idx);
    virtual bool checkMouseOutPortPress(double x, double y,
                                        double *vX, double *vY, int *idx);
    virtual void removeOutputEdge(Edge *edge);
    virtual void removeInputEdge(Edge *edge);
    virtual void removeEdges();
    virtual void decoupleEdges();
    virtual void updateMap(const configmaps::ConfigMap &map);
    virtual const configmaps::ConfigMap& getMap() {return info.map;}
    virtual void setSelected(bool s);
    virtual bool isSelected() {return selected;}
    virtual std::vector<std::pair<int, std::string> > getOutFoldInfo(int index);
    virtual std::vector<std::pair<int, std::string> > getInFoldInfo(int index);
    virtual std::vector<std::pair<int, std::string> > getFoldInfo(std::string tag,
                                                                  int index);
    virtual std::string getInPortName(int index);
    virtual std::string getOutPortName(int index);
    virtual std::string getName();
    virtual bool isInput();
    virtual bool isOutput();
    virtual void reposition();
    virtual void derenderText(const bool readable);
    virtual NodeInfo getNodeInfo();
    virtual void setNodeInfo(NodeInfo new_info);
    virtual configmaps::ConfigMap getOutPortEdgeInfo(int index);
    virtual void markInputsByEdgeInfo(configmaps::ConfigMap &map) {}
    virtual void unmarkInputs() {}
    virtual bool isCompatible(configmaps::ConfigMap &map, size_t index) {return true;}
    /*void confMapToYml(Node *node);
    void SelRecResize(double w, double h,Node *node);*/
    virtual void updateEdges();
    virtual void addChildNode(osg::ref_ptr<osg_graph_viz::Node> node);
    virtual void removeChildNode(osg::ref_ptr<osg_graph_viz::Node> node);
    virtual void setParentNode(osg::ref_ptr<osg_graph_viz::Node> node)
    {parent = node.get();}
    virtual osg::ref_ptr<osg_graph_viz::Node> getParentNode() const
    { return parent; }
    virtual double getChildrenScale() {return childrenScale;}
    virtual double getMinChildX();
    virtual double getMaxChildY();
    virtual void updateSize();
    virtual void applyFontScale(double s);
    virtual void filterUpdate() {}
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > getOutputEdges();
    virtual void exportSvg(FILE *f, double ol, double ot) {}
    virtual void exportPortsSVG(FILE *f, double ol, double ot);
    std::string replaceString(const std::string &source, const std::string &s1,
                              const std::string &s2);

  protected:
    osg_material_manager::OsgMaterialManager *materialManager;
    NodeInfo info;
    View *view;
    double posX, posY, width, height, headerHeight, portStartY;
    double posOffsetX, posOffsetY;
    int maxPorts;
    bool ignoreNextInPort, ignoreNextOutPort, selected, hidden;
    double portFontSize, headerFontSize;
    double portSpaceY;
    double mergeIconSize, foldIconSize;
    double childrenScale;
    osg::ref_ptr<osg_graph_viz::Node> parent;
    std::vector<double> portOffsets;
    osg::ref_ptr<osg::PositionAttitudeTransform> pos, pos2;
    osg::ref_ptr<osg_text::Text> nodeName, textBody;
    osg::ref_ptr<osg::LineWidth> linew;
    std::map<std::string, std::string> mergeImages;
    osg::ref_ptr<osg::Vec4Array> bColors;
    osg::ref_ptr<osg::Geometry> bGeom;
    osg::ref_ptr<osg::Geode> bGeode;
    osg::ref_ptr<osg::Vec3Array> vertices;
    std::map<std::string, std::vector<osg::Vec4> > colorMap;
    std::vector<Port*> inPorts, outPorts;
    osg::ref_ptr<osg::MatrixTransform> children;
    double portScale;

    osg::Geode* createRect(double w, double h, double x, double y,
                           std::string textureFile);
    osg::Geode* createFrame(double w, double h, double x, double y,
                            osg::Vec4 color = osg::Vec4(0, 0, 0, 1),
                            osg::Vec4 bcolor = osg::Vec4(1, 1, 1, 1));

    virtual osg::Geode* createBody(double w, double h, double x, double y,
                                   std::string textureFile, bool gardientHeader=false);
    void handlePorts(bool update=false);
    void handleDescription();
    void handleMeta();
    virtual void resizeWidth();
    virtual void resizeHeight();
    virtual void resizeWidth(double v);
    virtual void resizeHeight(double v);
    void updateParentFromMap(configmaps::ConfigMap &map);
    //void resizeWidth(double w);
    //void resizeHeight(double h);
    //void derenderTextInPort(const bool readable);
  };

} // end of namespace: osg_graph_viz

#endif // OSG_GRAPH_VIZ_NODE_HPP
