/**
 * \file RoundBodyNode.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef OSG_GRAPH_VIZ_ROUND_BODY_NODE_HPP
#define OSG_GRAPH_VIZ_ROUND_BODY_NODE_HPP

#include "Node.hpp"
#include <osg/Uniform>

namespace osg_graph_viz {

  class RoundBodyNode : public Node {

  public:
    RoundBodyNode(View *v);
    RoundBodyNode(const NodeInfo &info, View *v);
    virtual ~RoundBodyNode();

    void initRoundBody();
    void setRenderOrder(int o);
    virtual void setSelected(bool s);
    void applyColor();
    void exportSvg(FILE *f, double ol, double ot);
    void getRectangle(double *x1, double *x2, double *y1, double *y2);

  protected:
    osg::ref_ptr<osg::Uniform> sizeUniform, frameUniform, bodyColorUniform, frameColorUniform;
    osg::ref_ptr<osg::PositionAttitudeTransform> framePos;
    osg::ref_ptr<osg::Group> materialGroup;
    double frameY, frameX, sizeOffset;
    
    osg::Geode* createBody(double w, double h, double x, double y,
                           std::string textureFile, bool gardientHeader=false);
    virtual void resizeHeight(double w);
    virtual void resizeWidth(double h);
  };

} // end of namespace: osg_graph_viz

#endif // OSG_GRAPH_VIZ_ROUND_BODY_NODE_HPP
