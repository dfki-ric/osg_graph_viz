/**
 * \file XRockNode.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef OSG_GRAPH_VIZ_XROCK_NODE_HPP
#define OSG_GRAPH_VIZ_XROCK_NODE_HPP

#include "RoundBodyNode.hpp"

namespace osg_graph_viz {

  class XRockNode : public RoundBodyNode {

  public:
    XRockNode(const NodeInfo &info, View *v);
    ~XRockNode();

    osg::Vec3 getInPortPos(int index);
    osg::Vec3 getOutPortPos(int index);
    void addInputEdge(int index, Edge* edge);
    void addOutputEdge(int index, Edge* edge);
    void setLineWidth(double w);
    void updateMap(const configmaps::ConfigMap &map);
    void setSelected(bool s);
    std::vector<std::pair<int, std::string> > getFoldInfo(std::string tag,
                                                          int index);
    void derenderText(const bool readable);
    /*void confMapToYml(Node *node);
    void SelRecResize(double w, double h,Node *node);*/
    configmaps::ConfigMap getOutPortEdgeInfo(size_t index);
    void markInputsByEdgeInfo(configmaps::ConfigMap &map);
    void unmarkInputs();
    bool isCompatible(configmaps::ConfigMap &map, size_t index);
    void filterUpdate();
    void resizeWidth(double h);
    void applyFontScale(double s);
    void exportSvg(FILE *f, double ol, double ot);

  protected:
    std::vector<Frame> frames;
    osg::ref_ptr<osg_text::Text> nodeType;

    void handlePorts(bool update=false);
    void resizeHeight();
    //void resizeWidth(double w);
    //void resizeHeight(double h);
    //void derenderTextInPort(const bool readable);
    void handleFilterEdges(bool hide);
    void handlePortEdgeVisibility(Port *p, bool hide);
    bool getMergeInfo(size_t i, double *bias, double *def, std::string *merge);

  };

} // end of namespace: osg_graph_viz

#endif // OSG_GRAPH_VIZ_XROCK_NODE_HPP
