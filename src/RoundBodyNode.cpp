/**
 * \file RoundBodyNode.cpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#include "View.hpp"
#include "RoundBodyNode.hpp"

#include <osg/Geode>
#include <osg/LineWidth>
#include <cstdio>
#include <configmaps/ConfigData.h>

#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <cassert>

// y is going from down to top
// x is going from left to right

#include <mars/osg_material_manager/OsgMaterialManager.h>
#include <mars/osg_material_manager/MaterialNode.h>

using namespace configmaps;
using namespace osg_material_manager;

namespace osg_graph_viz {

  RoundBodyNode::RoundBodyNode(View *v) : Node(v) {
    sizeOffset = 12;
  }

  RoundBodyNode::RoundBodyNode(const NodeInfo &info_, View *v) : Node(v) {
    sizeOffset = 12;
    info = info_;
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      sizeOffset = 4;
    }
    initContent();
    initRoundBody();
  }

  RoundBodyNode::~RoundBodyNode(void) {
  }

  void RoundBodyNode::initRoundBody() {
    std::string loadPath = view->getResourcesPath() + "/shader/";
    ConfigMap mMap = ConfigMap::fromYamlFile(loadPath + "round_rect.yml");
    mMap["loadPath"] = loadPath;
    materialManager->createMaterial("round_rect", mMap);
    materialGroup = materialManager->getNewMaterialGroup("round_rect");
    osg_material_manager::MaterialNode *materialNode = dynamic_cast<osg_material_manager::MaterialNode*>(materialGroup.get());
    //materialGroup->addChild(pos2.get());
    framePos->setStateSet(materialNode->getMaterial()->getOrCreateStateSet());
    bGeode->getOrCreateStateSet()->addUniform(sizeUniform.get());
    bGeode->getOrCreateStateSet()->addUniform(frameUniform.get());
    bGeode->getOrCreateStateSet()->addUniform(bodyColorUniform.get());
    bGeode->getOrCreateStateSet()->addUniform(frameColorUniform.get());
    pos->removeChild(bGeode.get());
    framePos->addChild(bGeode.get());
    pos->insertChild(0, framePos.get());
  }

  osg::Geode* RoundBodyNode::createBody(double w, double h, double x, double y,
                                        std::string color, bool gardientHeader) {
    sizeUniform = new osg::Uniform("size", osg::Vec3f(w, h, 1));
    frameUniform = new osg::Uniform("frame", osg::Vec3f(1, 8, 0));
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      frameUniform->set(osg::Vec3f(1, 6, 0));
    }
    osg::Geode *geode = new osg::Geode;
    bGeom = new osg::Geometry;
    vertices = new osg::Vec3Array();
    framePos = new osg::PositionAttitudeTransform();
    framePos->setPosition(osg::Vec3(x, y-h-sizeOffset*0.5, 0));
    frameY = y;
    frameX = x;
    // frame
    vertices->push_back(osg::Vec3(0, 0, 0.0));
    vertices->push_back(osg::Vec3(w, 0, 0.0));
    vertices->push_back(osg::Vec3(w, h+sizeOffset, 0.0));
    vertices->push_back(osg::Vec3(0, h+sizeOffset, 0.0));

    bGeom->setVertexArray(vertices);
    bGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    bGeom->setDataVariance(osg::Object::DYNAMIC);

    bColors = new osg::Vec4Array;

    std::vector<osg::Vec4>::iterator it;
    if(colorMap.find(color) != colorMap.end()) {
      std::vector<osg::Vec4> &colors = colorMap[color];
      for(it=colors.begin(); it!=colors.end(); ++it) {
        bColors->push_back(*it);
      }
    }
    else {
      //default colors
      // header color
      //bColors->push_back(osg::Vec4(0.9, 0.9, 0.9, 1.0));
      // frame color
      bColors->push_back(osg::Vec4(0.82, 0.87, 1.0, 1.0));
    }

    // frame color
    bColors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));

    bodyColorUniform = new osg::Uniform("bodyColor", (*bColors.get())[0]);
    frameColorUniform = new osg::Uniform("frameColor", (*bColors.get())[1]);

    bGeom->setColorArray(bColors);
    bGeom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);

    osg::Vec3Array *normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
    bGeom->setNormalArray(normals);
    bGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);

    /* need to be corrected if we want to use images for header or body
       osg::Vec2Array *texcoords = new osg::Vec2Array();
       double ep = 0.00;
       texcoords->push_back(osg::Vec2(-ep, -ep));
       texcoords->push_back(osg::Vec2(1+ep, -ep));
       texcoords->push_back(osg::Vec2(1+ep, ep+0.197));
       texcoords->push_back(osg::Vec2(-ep, ep+0.197));

       texcoords->push_back(osg::Vec2(-ep, -ep+0.197));
       texcoords->push_back(osg::Vec2(1+ep, -ep+0.197));
       texcoords->push_back(osg::Vec2(1+ep, ep+0.197+(1-2*0.197)));
       texcoords->push_back(osg::Vec2(-ep, ep+0.197+(1-2*0.197)));

       texcoords->push_back(osg::Vec2(-ep, -ep+0.197+(1-2*0.197)));
       texcoords->push_back(osg::Vec2(1+ep, -ep+0.197+(1-2*0.197)));
       texcoords->push_back(osg::Vec2(1+ep, ep+1));
       texcoords->push_back(osg::Vec2(-ep, ep+1));

       bGeom->setTexCoordArray(0, texcoords);
    */

    geode->addDrawable(bGeom);

    // todo: load materials only once
    /*
      if(!textureFile.empty()) {
      osg::Texture2D *texture = view->loadTexture(textureFile);
      geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture,
      osg::StateAdettribute::ON | osg::StateAttribute::PROTECTED);
      }
    */
    return geode;
  }

  void RoundBodyNode::resizeHeight(double v) {
    framePos->setPosition(osg::Vec3(frameX, frameY - v -sizeOffset*0.5, 0));
    vertices->at(2).y() = vertices->at(3).y() = v + sizeOffset;
    sizeUniform->set(osg::Vec3f(width, v+sizeOffset, 1));
  }

  void RoundBodyNode::resizeWidth(double v) {
    vertices->at(1).x() = vertices->at(2).x() = v;
    sizeUniform->set(osg::Vec3f(v, height+sizeOffset, 1));
    if((std::string)info.map["type"] != "INPUT" &&
       (std::string)info.map["type"] != "OUTPUT") {
      double left, right, top, bottom;
      nodeName->getRectangle(&left, &right, &top, &bottom);
      nodeName->setPosition(width*0.5, top);
    }
  }

  void RoundBodyNode::setRenderOrder(int o) {
    Node::setRenderOrder(o+1);
    materialGroup->getOrCreateStateSet()->setRenderBinDetails(o, "RenderBin");
  }

  void RoundBodyNode::setSelected(bool s) {
    selected = s;
    if(s) {
      bodyColorUniform->set(osg::Vec4(0.72, 1.0, 0.77, 1.0));
    }
    else {
      bodyColorUniform->set((*bColors.get())[0]);
    }
  }

  void RoundBodyNode::applyColor() {
    bodyColorUniform->set((*bColors.get())[0]);
    frameColorUniform->set((*bColors.get())[1]);
  }

  void RoundBodyNode::getRectangle(double *x1, double *x2, double *y1, double *y2) {
    Node::getRectangle(x1, x2, y1, y2);
    *y1 -=  0.5*sizeOffset;
    *y2 += 0.5*sizeOffset;
  }

  void RoundBodyNode::exportSvg(FILE *f, double ol, double ot) {
    std::string name = getName();
    double x, y;
    getPosition(&x, &y);
    x-=ol;
    y-=ot;
    fprintf(f, "  <g id=\"%s\" transform=\"translate(%g,%g)\">\n", name.c_str(), x, -y-.5*sizeOffset);
    double ry = 8;
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      ry = 6;;
    }
    fprintf(f, "    <rect style=\"opacity:1;fill:#%s;fill-opacity:1;fill-rule:nonzero;stroke:#%s;stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1\" id=\"frame2\" width=\"%g\" height=\"%g\" x=\"0\" y=\"0\" ry=\"%g\" />\n", View::getColor((*bColors.get())[0]).c_str(), View::getColor((*bColors.get())[1]).c_str(), width, height+sizeOffset, ry);

    View::exportLabelToSvg(nodeName, f, 0, 0.5*sizeOffset);

    exportPortsSVG(f, 0, 0.5*sizeOffset);

    fprintf(f, "  </g>\n");
  }

} // end of namespace: osg_graph_viz
