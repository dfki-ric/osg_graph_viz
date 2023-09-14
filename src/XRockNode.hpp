/**
 * \file XRockNode.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef OSG_GRAPH_VIZ_XROCK_NODE_HPP
#define OSG_GRAPH_VIZ_XROCK_NODE_HPP

#include "RoundBodyNode.hpp"
#include <mars/osg_text/Text.h>
#include <memory>
#include <map>

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
    configmaps::ConfigMap getOutPortEdgeInfo(int index);
    void markInputsByEdgeInfo(configmaps::ConfigMap &map);
    void unmarkInputs();
    bool isCompatible(configmaps::ConfigMap &map, size_t index);
    void filterUpdate();
    void resizeWidth(double h);
    void applyFontScale(double s);
    void exportSvg(FILE *f, double ol, double ot);

    struct Tooltip
    {
      osg::ref_ptr<osg::Group> root;
      osg::ref_ptr<osg_text::Text> textDrawable;
      osg_text::Color backgroundColor{0.20, 0.20, 0.20, 1.0};
      osg_text::Color textColor{1.0, 1.0, 1.0, 1};
      Tooltip(View *view, const std::string &text, double x, double y)
      {
        root = new osg::Group;
        textDrawable = new osg_text::Text(text, view->headerFontSize, textColor,
                                          x, y,
                                          osg_text::TextAlign::ALIGN_CENTER, 6, 6, 6, 6,
                                          backgroundColor, osg_text::Color(), 0.0,
                                          view->getResourcesPath() + "/fonts/stilu/Stilu-SemiBold.ttf");
        root->addChild((osg::Node *)textDrawable->getOSGNode());
      }
    };

    void showTooltip(const std::string &key, double x, double y, const std::string &text)
    {
      if (tooltips.count(key))
        return;
      tooltips[key].reset(new Tooltip(view, text, x, y));
      pos->addChild(tooltips[key]->root);
    }
    void hideTooltip(const std::string &key)
    {
      if (!tooltips.count(key))
        return;
      pos->removeChild(tooltips[key]->root);
      tooltips.erase(key);
    }
  protected:
    std::map<std::string, std::unique_ptr<Tooltip>> tooltips;
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
