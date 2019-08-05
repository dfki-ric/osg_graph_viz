/**
 * \file Edge.cpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

/*
 * todo: we can only have decouple or smooth activated
 */

#include "View.hpp"
#include "Node.hpp"

#include <cstdio>

#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <mars/osg_material_manager/OsgMaterialManager.h>
#include <mars/osg_material_manager/MaterialNode.h>

using namespace configmaps;

namespace osg_graph_viz {

  std::string replaceString(const std::string &source, const std::string &s1,
                            const std::string &s2) {
    std::string back = source;
    size_t found = back.find(s1);
    while(found != std::string::npos) {
      back.replace(found, s1.size(), s2);
      found = back.find(s1, found+s2.size());
    }
    return back;
  }


  Edge::Edge(const ConfigMap &map, View *v, const double port_size_y) : info(map) {
    offsetLimit = port_size_y / 2.0;
    view = v;
    hidden = false;
    materialManager = view->getOsgMaterialManager();
    selected = false;
    geode = new osg::Geode;
    geom = new osg::Geometry;
    geom->setDataVariance(osg::Object::DYNAMIC);
    decoupleGeom = new osg::Geometry;
    decoupleGeom->setDataVariance(osg::Object::DYNAMIC);

    smoothGeom = new osg::Geometry;
    smoothGeom->setDataVariance(osg::Object::DYNAMIC);

    std::string loadPath = view->getResourcesPath();
    if(loadPath[loadPath.size()-1] != '/') loadPath.append("/");
    loadPath.append("shader/");
    ConfigMap mMap = ConfigMap::fromYamlFile(loadPath + "smooth_edge.yml");
    mMap["loadPath"] = loadPath;
    materialManager->createMaterial("smooth_edge", mMap);
    osg::ref_ptr<osg::Node> materialGroup = materialManager->getNewMaterialGroup("smooth_edge");
    osg_material_manager::MaterialNode *materialNode = dynamic_cast<osg_material_manager::MaterialNode*>(materialGroup.get());

    osg::ref_ptr<osg::StateSet> st = new osg::StateSet(*(materialNode->getMaterial()->getOrCreateStateSet()));
    smoothGeom->setStateSet(st.get());
    startUniform = new osg::Uniform("start", osg::Vec3f(0, 0, 0));
    endUniform = new osg::Uniform("end", osg::Vec3f(1, 1, 0));
    colorUniform = new osg::Uniform("color", osg::Vec4f(0, 0, 0, 1));
    smoothGeom->getOrCreateStateSet()->addUniform(startUniform.get());
    smoothGeom->getOrCreateStateSet()->addUniform(endUniform.get());
    smoothGeom->getOrCreateStateSet()->addUniform(colorUniform.get());

    vertices = new osg::Vec3Array();
    double x, y, z;
    for(size_t i=0; i<info["vertices"].size(); ++i) {
      x = info["vertices"][i]["x"];
      y = info["vertices"][i]["y"];
      z = info["vertices"][i]["z"];
      vertices->push_back(osg::Vec3(x, y, z));
    }

    decoupleVertices = new osg::Vec3Array();
    if(!info.hasKey("decoupleVertices")) {
      x = (*vertices.get())[0].x();
      y = (*vertices.get())[0].y();
      z = (*vertices.get())[0].z();
      info["decoupleVertices"][0]["x"] = x;
      info["decoupleVertices"][0]["y"] = y;
      info["decoupleVertices"][0]["z"] = z;
      info["decoupleVertices"][1]["x"] = x+25;
      info["decoupleVertices"][1]["y"] = y;
      info["decoupleVertices"][1]["z"] = z;
      x = (*vertices.get()).back().x();
      y = (*vertices.get()).back().y();
      z = (*vertices.get()).back().z();
      info["decoupleVertices"][2]["x"] = x-25;
      info["decoupleVertices"][2]["y"] = y;
      info["decoupleVertices"][2]["z"] = z;
      info["decoupleVertices"][3]["x"] = x;
      info["decoupleVertices"][3]["y"] = y;
      info["decoupleVertices"][3]["z"] = z;
    }
    for(size_t i=0; i<info["decoupleVertices"].size(); ++i) {
      x = info["decoupleVertices"][i]["x"];
      y = info["decoupleVertices"][i]["y"];
      z = info["decoupleVertices"][i]["z"];
      decoupleVertices->push_back(osg::Vec3(x, y, z));
    }

    geom->setVertexArray(vertices.get());
    startPos = (*vertices.get())[0].y();
    endPos = (*vertices.get())[vertices->size()-1].y();
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0,
                                              vertices->size()));

    decoupleGeom->setVertexArray(decoupleVertices.get());
    decoupleGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0,
                                                      decoupleVertices->size()));

    smoothVertices = new osg::Vec3Array();
    for(int i=0; i<4; ++i) {
      smoothVertices->push_back(osg::Vec3(0, 0, 0));
    }
    updateSmoothPos();
    smoothGeom->setVertexArray(smoothVertices.get());
    smoothGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0,
                                                      smoothVertices->size()));


    color = new osg::Vec4Array;
    color->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    geom->setColorArray(color);
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    decoupleGeom->setColorArray(color);
    decoupleGeom->setColorBinding(osg::Geometry::BIND_OVERALL);
    smoothGeom->setColorArray(color);
    smoothGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec3Array *normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
    geom->setNormalArray(normals);
    geom->setNormalBinding(osg::Geometry::BIND_OVERALL);
    decoupleGeom->setNormalArray(normals);
    decoupleGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);
    smoothGeom->setNormalArray(normals);
    smoothGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);

    osg_text::Color c(0., 0., 0., 1.0);
    if(info.hasKey("weight")) {
      weight = new osg_text::Text(info["weight"].toString(), 8, c, 0, 0,
                                  osg_text::ALIGN_LEFT, 0, 0, 0, 0,
                                  osg_text::Color(), osg_text::Color(),
                                  0, view->getResourcesPath()+"/fonts/Stilu-Light.ttf");
      weight->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
      updateWeightPos();
      if((double)info["weight"] > 1.0000001 ||
         (double)info["weight"] < 0.9999999) {
        this->addChild((osg::Node*)weight->getOSGNode());
      }
    }
    geode->addDrawable(geom);
    decoupled = false;
    this->addChild(geode);

    toIdx = 0;
    fromIdx = 0;

    decoupleIn = new osg_text::Text("foo", 6, c, 0, 0,
                                    osg_text::ALIGN_RIGHT, 0, 0, 0, 0,
                                    osg_text::Color(), osg_text::Color(),
                                    0, view->getResourcesPath()+"/fonts/Stilu-Light.ttf");
    decoupleIn->setBackgroundColor(osg_text::Color(0.98, 0.7, 0.98, 1.0));
    decoupleIn->setBorderColor(osg_text::Color(0.3, 0.3, 0.3, 1.0));
    decoupleIn->setBorderWidth(3);
    decoupleIn->setPadding(6, 3, 6, 3);
    decoupleOut = new osg_text::Text("foo", 6, c, 0, 0,
                                     osg_text::ALIGN_LEFT, 0, 0, 0, 0,
                                     osg_text::Color(), osg_text::Color(),
                                     0, view->getResourcesPath()+"/fonts/Stilu-Light.ttf");
    decoupleOut->setBackgroundColor(osg_text::Color(0.98, 0.7, 0.98, 1.0));
    decoupleOut->setBorderColor(osg_text::Color(0.3, 0.3, 0.3, 1.0));
    decoupleOut->setBorderWidth(3);
    decoupleOut->setPadding(6, 3, 6, 3);
    smooth = (bool)info["smooth"];
    if((bool)info["decouple"]) {
      this->addChild((osg::Node*)decoupleIn->getOSGNode());
      this->addChild((osg::Node*)decoupleOut->getOSGNode());
      geode->removeDrawable(geom);
      geode->addDrawable(decoupleGeom);
      decoupled = true;
    }
    else if(smooth) {
      geode->removeDrawable(geom);
      geode->removeDrawable(decoupleGeom);
      geode->addDrawable(smoothGeom);
    }
    updateDecouplePos();
    linew = new osg::LineWidth(1);
    geode->getOrCreateStateSet()->setAttributeAndModes(linew.get(),
                                                       osg::StateAttribute::ON);

    startOffset = endOffset = 0.0;
  }


  Edge::~Edge(void) {
  }


  int Edge::checkMousePress(double x, double y) {
    double l, l2;
    osg::Vec3Array *v;
    int select = 0;
    if(hidden) return 0;
    if(smooth && !(bool)info["decouple"]) {
      osg::Vec3 start = (*smoothVertices.get())[0];
      osg::Vec3 end = (*smoothVertices.get())[2];
      double startx = start.x() < end.x()? start.x() : end.x();
      double endx = start.x() < end.x()? end.x() : start.x();
      double starty = start.y() < end.y()? start.y() : end.y();
      double endy = start.y() < end.y()? end.y() : start.y();
      if(endy - starty < 10) {
        endy += (9-(endy-starty))*0.5;
        starty -= (9-(endy-starty))*0.5;
      }
      if(endx - startx < 10) {
        endx += (9-(endx-startx))*0.5;
        startx -= (9-(endx-startx))*0.5;
      }

      if(x > startx && x < endx && y > starty && y < endy) {
        x -= startx;
        x *= 1./(endx-startx);
        if(x > 0.5) node = vertices->size()-1;
        else node = 0;
        if(endy - starty < 10) return 1;
        if(endx - startx < 10) return 1;
        if(start.x() > end.x()) x = 1-x;
        y -= starty;
        y *= 1./(endy-starty);
        if(start.y() > end.y()) y = 1-y;
        double e = 0.1;
        osg::Vec2 v1(x+e, 1/(1+exp(-5*((x+e)*2-1))));
        osg::Vec2 v2(x-e, 1/(1+exp(-5*((x-e)*2-1))));
        osg::Vec2 v3(x, y);
        v1 = v1-v2;
        v2 = v2-v3;
        v1 = v1 * (v1*v2) / (v1*v1);
        v1 = v2-v1;
        double d = sqrt(v1*v1);
        if(d>e*0.5) return 0;
        else return 1;
      }
    }
    else {
      if((bool)info["decouple"]) {
        v = decoupleVertices.get();
      }
      else {
        v = vertices.get();
      }

      if((bool)info["decouple"]) {
        double left, right, top, bottom;
        decoupleIn->getRectangle(&left, &right, &top, &bottom);
        if(x>left && x < right && y > bottom && y < top) {
          dOffsetX = (*v)[2].x()-x;
          dOffsetY = (*v)[2].y()-y;
          node = 2;
          return 1;
        }
        decoupleOut->getRectangle(&left, &right, &top, &bottom);
        if(x>left && x < right && y > bottom && y < top) {
          dOffsetX = (*v)[1].x()-x;
          dOffsetY = (*v)[1].y()-y;
          node = 1;
          return 1;
        }
      }
      for(size_t i=0; i<v->size()-1; ++i) {
        horizontal = (i+1)%2;
        node = i;
        l = sqrt(pow((*v)[i].x() - x, 2.) + pow((*v)[i].y() - y, 2.));
        l += sqrt(pow((*v)[i+1].x() - x, 2.) + pow((*v)[i+1].y() - y, 2.));
        l2 = sqrt(pow((*v)[i].x() - (*v)[i+1].x(), 2.) +
                  pow((*v)[i].y() - (*v)[i+1].y(), 2.));
        if(!(bool)info["decouple"] || i != 1) {
          if(fabs(l2-l) / l2 < 0.5 / (2*l2)) {
            select = 1;
            break;
          }
        }
      }
      if(select && v->size() < 4) {
        double min = sqrt(pow((*v)[0].x() - x, 2.) + pow((*v)[0].y() - y, 2.));
        node = 0;
        for(size_t i=1; i<v->size(); ++i) {
          double d = sqrt(pow((*v)[i].x() - x, 2.) + pow((*v)[i].y() - y, 2.));
          //fprintf(stderr, "check: %g %g\n", min, d);
          if(d < min) {
            min = d;
            node = i;
          }
        }
      }
    }
    return select;
  }

  void Edge::updateVPos(int i, osg::Vec3 v) {
    v.x() = (int)v.x();
    v.y() = (int)v.y();
    v.z() = (int)v.z();
    (*vertices.get())[i].set(v);
    info["vertices"][i]["x"] = v.x();
    info["vertices"][i]["y"] = v.y();
    info["vertices"][i]["z"] = v.z();
    updateWeightPos();
    updateDecouplePos();
    updateSmoothPos();
    dirty();
  }

  void Edge::updateStartPos(osg::Vec3 v) {
    size_t size = vertices->size();
    v.x() = (int)v.x();
    v.y() = (int)v.y();
    v.z() = (int)v.z();
    startPos = v.y();
    v.y() += startOffset;
    osg::Vec3 offset = v - (*vertices.get())[0];
    (*vertices.get())[0].set(v);
    info["vertices"][0]["x"] = v.x();
    info["vertices"][0]["y"] = v.y();
    info["vertices"][0]["z"] = v.z();

    (*decoupleVertices.get())[0].set(v);
    info["decoupleVertices"][0]["x"] = v.x();
    info["decoupleVertices"][0]["y"] = v.y();
    info["decoupleVertices"][0]["z"] = v.z();
    (*decoupleVertices.get())[1] += offset;
    info["decoupleVertices"][1]["x"] = (*decoupleVertices.get())[1].x();
    info["decoupleVertices"][1]["y"] = (*decoupleVertices.get())[1].y();
    info["decoupleVertices"][1]["z"] = (*decoupleVertices.get())[1].z();

    if(size > 3) {
      //(*vertices.get())[size-2].y() = v.y();
      //info["vertices"][size-2]["y"] = v.y();
      (*vertices.get())[1] += offset;
      info["vertices"][1]["x"] = (*vertices.get())[1].x();
      info["vertices"][1]["y"] = (*vertices.get())[1].y();
      info["vertices"][1]["z"] = (*vertices.get())[1].z();
      (*vertices.get())[2] += offset;
      info["vertices"][2]["x"] = (*vertices.get())[2].x();
      info["vertices"][2]["y"] = (*vertices.get())[2].y();
      info["vertices"][2]["z"] = (*vertices.get())[2].z();
      checkEndPositions();
    }

    // if(size > 3) {
    //   (*vertices.get())[1].y() = v.y();
    //   info["vertices"][1]["y"] = v.y();
    //   checkEndPositions();
    // }
    updateWeightPos();
    updateDecouplePos();
    updateSmoothPos();
    dirty();
  }

  void Edge::updateEndPos(osg::Vec3 v) {
    size_t size = vertices->size();
    v.x() = (int)v.x();
    v.y() = (int)v.y();
    v.z() = (int)v.z();
    endPos = v.y();
    v.y() += endOffset;
    osg::Vec3 offset = v - vertices->back();
    vertices->back().set(v);
    info["vertices"][size-1]["x"] = v.x();
    info["vertices"][size-1]["y"] = v.y();
    info["vertices"][size-1]["z"] = v.z();

    (*decoupleVertices.get())[3].set(v);
    info["decoupleVertices"][3]["x"] = v.x();
    info["decoupleVertices"][3]["y"] = v.y();
    info["decoupleVertices"][3]["z"] = v.z();
    (*decoupleVertices.get())[2] += offset;
    info["decoupleVertices"][2]["x"] = (*decoupleVertices.get())[2].x();
    info["decoupleVertices"][2]["y"] = (*decoupleVertices.get())[2].y();
    info["decoupleVertices"][2]["z"] = (*decoupleVertices.get())[2].z();

    if(size > 3) {
      //(*vertices.get())[size-2].y() = v.y();
      //info["vertices"][size-2]["y"] = v.y();
      (*vertices.get())[size-2] += offset;
      info["vertices"][size-2]["x"] = (*vertices.get())[size-2].x();
      info["vertices"][size-2]["y"] = (*vertices.get())[size-2].y();
      info["vertices"][size-2]["z"] = (*vertices.get())[size-2].z();
      (*vertices.get())[size-3] += offset;
      info["vertices"][size-3]["x"] = (*vertices.get())[size-3].x();
      info["vertices"][size-3]["y"] = (*vertices.get())[size-3].y();
      info["vertices"][size-3]["z"] = (*vertices.get())[size-3].z();
      checkEndPositions();
    }

    updateWeightPos();
    updateDecouplePos();
    updateSmoothPos();
    dirty();
  }

  void Edge::updateWeightPos() {
    if(!weight.valid()) return;

    double x=0, y=0;
    // Todo: need decouple handling
    if((bool)info["decouple"]) {

    }
    else {
      if(vertices->size() == 2) {
        x = (*vertices.get())[0].x();
        x += ((*vertices.get())[1].x() - (*vertices.get())[0].x())*0.5;
        y = (*vertices.get())[0].y();
        y += ((*vertices.get())[1].y() - (*vertices.get())[0].y())*0.5+8;
      }
      else {
        x = (*vertices.get())[2].x();
        x += ((*vertices.get())[3].x() - (*vertices.get())[2].x())*0.5+5;
        y = (*vertices.get())[2].y();
        y += ((*vertices.get())[2].y() - (*vertices.get())[2].y())*0.5+8;
      }
    }
    weight->setPosition(x, y);
  }

  void Edge::updateDecouplePos() {
    double x=0, y=0;
    x = (*decoupleVertices.get())[2].x()+5;
    y = (*decoupleVertices.get())[2].y()+3;
    decoupleIn->setPosition(x, y);
    x = (*decoupleVertices.get())[1].x()-5;
    y = (*decoupleVertices.get())[1].y()+3;
    decoupleOut->setPosition(x, y);
  }

  void Edge::checkEndPositions() {
    size_t size = vertices->size();
    /*
    if(size > 3) {
      if(((*info.vertices.get())[1].x() -
          (*info.vertices.get())[0].x()) < 5. &&
         ((*info.vertices.get())[3].x() -
          (*info.vertices.get())[2].x()) > 5.) {
        (*info.vertices.get())[1].x() = (*info.vertices.get())[0].x()+5.;
        (*info.vertices.get())[2].x() = (*info.vertices.get())[0].x()+5.;
      }
      if(((*info.vertices.get())[size-1].x() -
          (*info.vertices.get())[size-2].x()) < 5. &&
         ((*info.vertices.get())[size-3].x() -
          (*info.vertices.get())[size-4].x()) > 5.) {
        (*info.vertices.get())[size-2].x() = (*info.vertices.get())[size-1].x()-5.;
        (*info.vertices.get())[size-3].x() = (*info.vertices.get())[size-1].x()-5.;
      }
    }
    */
    if(size > 3) {
      if(((*vertices.get())[1].x() -
          (*vertices.get())[0].x()) < 5.) {
        info["vertices"][1]["x"] = (*vertices.get())[1].x() = (*vertices.get())[0].x()+5.;
        info["vertices"][2]["x"] = (*vertices.get())[2].x() = (*vertices.get())[0].x()+5.;

      }
      if(((*vertices.get())[size-1].x() -
          (*vertices.get())[size-2].x()) < 5.) {
        info["vertices"][size-2]["x"] = (*vertices.get())[size-2].x() = (*vertices.get())[size-1].x()-5.;
        info["vertices"][size-3]["x"] = (*vertices.get())[size-3].x() = (*vertices.get())[size-1].x()-5.;
      }
    }
  }

  void Edge::mouseMove(double x, double y) {
    int size = vertices->size();
    x = (int)x;
    y = (int)y;
    if((bool)info["decouple"]) {
      x += dOffsetX;
      y += dOffsetY;
      if(node == 0) {
        limit_y_offset(startPos, &y, &startOffset);
        (*decoupleVertices.get())[node].y() = y;
        info["decoupleVertices"][node]["y"] = y;
      }
      else if(node == 3) {
        limit_y_offset(endPos, &y, &endOffset);
        (*decoupleVertices.get())[node].y() = y;
        info["decoupleVertices"][node]["y"] = y;
      }
      else {
        (*decoupleVertices.get())[node].x() = x;
        info["decoupleVertices"][node]["x"] = x;
        (*decoupleVertices.get())[node].y() = y;
        info["decoupleVertices"][node]["y"] = y;
      }
    }
    else if(size > 3) {
      if(node == 0) {
        limit_y_offset(startPos, &y, &startOffset);
        info["vertices"][0]["y"] = (*vertices.get())[0].y() = y;
        info["vertices"][1]["y"] = (*vertices.get())[1].y() = y;
      }
      else if(node == size-2) {
        limit_y_offset(endPos, &y, &endOffset);
        info["vertices"][size-1]["y"] = (*vertices.get())[size-1].y() = y;
        info["vertices"][size-2]["y"] = (*vertices.get())[size-2].y() = y;
      }
      else if(node > 0 && node < size-2) {
        if(horizontal) {
          info["vertices"][node]["y"] = (*vertices.get())[node].y() = y;
          info["vertices"][node+1]["y"] = (*vertices.get())[node+1].y() = y;
        }
        else {
          info["vertices"][node]["x"] = (*vertices.get())[node].x() = x;
          info["vertices"][node+1]["x"] = (*vertices.get())[node+1].x() = x;
        }
      }
      checkEndPositions();
    }
    else {
      if(node == 0) {
        limit_y_offset(startPos, &y, &startOffset);
      }
      else if(node == size-1) {
        limit_y_offset(endPos, &y, &endOffset);
      }
      info["vertices"][node]["y"] = (*vertices.get())[node].y() = y;
    }
    updateDecouplePos();
    updateWeightPos();
    updateSmoothPos();
    dirty();
  }


  void Edge::limit_y_offset(const double middle, double *desire, double *offset){
    if(*desire > middle + offsetLimit) *desire = middle + offsetLimit;
    if(*desire < middle - offsetLimit) *desire = middle - offsetLimit;
    *offset = *desire - middle;
  }

  void Edge::setMiddlePos(double x) {
    size_t size = vertices->size();
    x = (int)x;
    if(size > 3) {
      info["vertices"][1]["x"] = (*vertices.get())[1].x() = (int)x;
      info["vertices"][2]["x"] = (*vertices.get())[2].x() = (int)x;
    }
    checkEndPositions();
    updateSmoothPos();
    dirty();
  }

  void Edge::setLineWidth(double w) {
    linew->setWidth(w);
    decoupleIn->setBorderWidth(w*1.6);
    decoupleOut->setBorderWidth(w*1.6);
  }

  void Edge::setColor(osg::Vec4 v) {
    colorUniform->set(v);
    (*color.get())[0] = v;
    dirty();
  }

  void Edge::updateMap(const ConfigMap &map) {
    info = map;
    for(size_t i=0; i<info["vertices"].size(); ++i) {
      (*vertices.get())[i].x() = (double)info["vertices"][i]["x"];
      (*vertices.get())[i].y() = (double)info["vertices"][i]["y"];
      (*vertices.get())[i].z() = (double)info["vertices"][i]["z"];
    }
    for(size_t i=0; i<info["decoupleVertices"].size(); ++i) {
      (*decoupleVertices.get())[i].x() = (double)info["decoupleVertices"][i]["x"];
      (*decoupleVertices.get())[i].y() = (double)info["decoupleVertices"][i]["y"];
      (*decoupleVertices.get())[i].z() = (double)info["decoupleVertices"][i]["z"];
    }
    if(weight.valid()) {
      weight->setText(info["weight"].toString());
      updateWeightPos();
      if((double)info["weight"] > 1.0000001 ||
         (double)info["weight"] < 0.9999999) {
        this->addChild((osg::Node*)weight->getOSGNode());
      }
      else {
        this->removeChild((osg::Node*)weight->getOSGNode());
      }
    }
    this->removeChild((osg::Node*)decoupleIn->getOSGNode());
    this->removeChild((osg::Node*)decoupleOut->getOSGNode());
    geode->removeDrawable(decoupleGeom);
    geode->removeDrawable(geom);
    geode->removeDrawable(smoothGeom);
    decoupled = false;
    smooth = false;

    if((bool)info["decouple"]) {
      this->addChild((osg::Node*)decoupleIn->getOSGNode());
      this->addChild((osg::Node*)decoupleOut->getOSGNode());
      geode->addDrawable(decoupleGeom);
      decoupled = true;
    }
    else if((bool)info["smooth"]) {
      smooth = true;
      geode->addDrawable(smoothGeom);
    }
    else {
      geode->addDrawable(geom);
    }
    updateSmoothPos();
    dirty();
  }

  void Edge::dirty(void) {
    geom->dirtyDisplayList();
    geom->dirtyBound();
    decoupleGeom->dirtyDisplayList();
    decoupleGeom->dirtyBound();
    smoothGeom->dirtyDisplayList();
    smoothGeom->dirtyBound();
    smoothVertices->dirty();
  }

  void Edge::setStartNode(osg_graph_viz::Node* node) {
    std::string name;
    startNode = node;
    name = startNode->getName();
    if(!startNode->isInput()) {
      name += " - ";
      name += startNode->getOutPortName(fromIdx);
      decoupleIn->setBackgroundColor(osg_text::Color(0.98, 0.7, 0.98, 1.0));
    }
    else {
      decoupleIn->setBackgroundColor(osg_text::Color(0.98, 0.98, 0.7, 1.0));
    }
    decoupleIn->setText(name);
  }

  osg_graph_viz::Node* Edge::getStartNode() {
    return startNode;
  }

  void Edge::setEndNode(osg_graph_viz::Node* node) {
    std::string name;
    endNode = node;
    name = endNode->getName();
    if(!endNode->isOutput()) {
      name += " - ";
      name += endNode->getInPortName(toIdx);
      decoupleOut->setBackgroundColor(osg_text::Color(0.98, 0.7, 0.98, 1.0));
    }
    else {
      decoupleOut->setBackgroundColor(osg_text::Color(0.98, 0.98, 0.7, 1.0));
    }
    decoupleOut->setText(name);
  }

  osg_graph_viz::Node* Edge::getEndNode() {
    return endNode.get();
  }

  void Edge::removeFromNodes() {
    if(startNode.valid()) {
      startNode->removeOutputEdge(this);
    }
    if(endNode.valid()) {
      endNode->removeInputEdge(this);
    }
  }

  void Edge::savePosOffset(double x, double y) {
    posOffsetX = (*vertices.get())[0].x() - x;
    posOffsetY = (*vertices.get())[0].y() - y;
    //fprintf(stderr, "savePosOffset %g/%g\n", posOffsetX, posOffsetY);
  }

  void Edge::setPosition2(double x, double y) {
    size_t size = vertices->size();
    x = posOffsetX + x - (*vertices.get())[0].x();
    y = posOffsetY + y - (*vertices.get())[0].y();

    if(size == 2) {
      info["vertices"][0]["x"] = (*vertices.get())[0].x() += (int)x;
      info["vertices"][0]["y"] = (*vertices.get())[0].y() += (int)y;
    }
    else {
      for(size_t i=0; i<size-2; ++i) {
        info["vertices"][i]["x"] = (*vertices.get())[i].x() += (int)x;
        info["vertices"][i]["y"] = (*vertices.get())[i].y() += (int)y;
      }
    }
    updateDecouplePos();
    updateSmoothPos();
    dirty();
  }

  void Edge::setSelected(bool v) {
    selected = v;
    if(v) {
      decoupleIn->setBackgroundColor(osg_text::Color(0.0, 0.7, 0.0, 1.0));
      decoupleOut->setBackgroundColor(osg_text::Color(0.0, 0.7, 0.0, 1.0));
      setColor(osg::Vec4(0.0, 0.7, 0.0, 1.0));
    }
    else {
      if(startNode.valid() && startNode->isInput()) {
        decoupleIn->setBackgroundColor(osg_text::Color(0.98, 0.98, 0.7, 1.0));
      }
      else {
        decoupleIn->setBackgroundColor(osg_text::Color(0.98, 0.7, 0.98, 1.0));
      }
      if(endNode.valid() && endNode->isOutput()) {
        decoupleOut->setBackgroundColor(osg_text::Color(0.98, 0.98, 0.7, 1.0));
      }
      else {
        decoupleOut->setBackgroundColor(osg_text::Color(0.98, 0.7, 0.98, 1.0));
      }
      setColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    }
  }

  void Edge::decouple(double threshhold) {
    if(!(bool)info["decouple"] && !decoupled) {
      osg::Vec3 v = getEndPosition()-getStartPosition();
      if(v.length() > threshhold) decoupleEdge();
    }
  }

  void Edge::decoupleEdge() {
    if(!(bool)info["decouple"] && !decoupled) {
      info["decouple"] = true;
      this->addChild((osg::Node*)decoupleIn->getOSGNode());
      this->addChild((osg::Node*)decoupleOut->getOSGNode());
      geode->removeDrawable(smoothGeom);
      geode->removeDrawable(geom);
      geode->addDrawable(decoupleGeom);
      decoupled = true;
      double v = 25;

      info["decoupleVertices"][0]["x"] = (*vertices.get())[0].x();
      (*decoupleVertices.get())[0].x() = (*vertices.get())[0].x();
      info["decoupleVertices"][0]["y"] = (*vertices.get())[0].y();
      (*decoupleVertices.get())[0].y() = (*vertices.get())[0].y();
      info["decoupleVertices"][1]["x"] = (*vertices.get())[0].x()+v;
      (*decoupleVertices.get())[1].x() = (*vertices.get())[0].x()+v;
      info["decoupleVertices"][1]["y"] = (*vertices.get())[0].y();
      (*decoupleVertices.get())[1].y() = (*vertices.get())[0].y();

      info["decoupleVertices"][2]["x"] = (*vertices.get()).back().x()-v;
      (*decoupleVertices.get())[2].x() = (*vertices.get()).back().x()-v;
      info["decoupleVertices"][2]["y"] = (*vertices.get()).back().y();
      (*decoupleVertices.get())[2].y() = (*vertices.get()).back().y();
      info["decoupleVertices"][3]["x"] = (*vertices.get()).back().x();
      (*decoupleVertices.get())[3].x() = (*vertices.get()).back().x();
      info["decoupleVertices"][3]["y"] = (*vertices.get()).back().y();
      (*decoupleVertices.get())[3].y() = (*vertices.get()).back().y();

      updateDecouplePos();
      updateWeightPos();
      dirty();
    }
  }

  bool Edge::isDecoupled(){
    return (bool)info["decouple"];
  }

  osg::Vec3 Edge::getEndPosition() {
    return (*vertices.get())[vertices->size()-1];
  }

  osg::Vec3 Edge::getStartPosition() {
    return (*vertices.get())[0];
  }

  void Edge::derenderText(const bool readable){
    if(!weight.valid()) return;
    if(readable){//adds text to the Graph, here the texts are the weight of the Edge
      if((double)info["weight"] > 1.0000001 ||
         (double)info["weight"] < 0.9999999) {
        this->removeChild((osg::Node*)weight->getOSGNode());
        this->addChild((osg::Node*)weight->getOSGNode());
      }
    }

    else{//removes text from the Graph
      this->removeChild((osg::Node*)weight->getOSGNode());
    }
  }

  void Edge::reposition() {
    info["vertices"][vertices->size()-1]["y"] = (*vertices.get())[vertices->size()-1].y() -= endOffset;
    if(vertices->size() > 3) {
      info["vertices"][vertices->size()-2]["y"] = (*vertices.get())[vertices->size()-2].y() -= endOffset;
      info["vertices"][vertices->size()-3]["y"] = (*vertices.get())[vertices->size()-3].y() -= endOffset;
    }
    info["vertices"][0]["y"] = (*vertices.get())[0].y() -= startOffset;
    if(vertices->size() > 3) {
      info["vertices"][1]["y"] = (*vertices.get())[1].y() -= startOffset;
      info["vertices"][2]["y"] = (*vertices.get())[2].y() -= startOffset;
    }
    (*decoupleVertices.get())[0] = (*vertices.get()).front();
    (*decoupleVertices.get())[1] = (*vertices.get()).front();
    (*decoupleVertices.get())[1].x() += 25;
    (*decoupleVertices.get())[3] = (*vertices.get()).back();
    (*decoupleVertices.get())[2] = (*vertices.get()).back();
    (*decoupleVertices.get())[2].x() -= 25;
    for(size_t i=0; i<4; ++i) {
      info["decoupleVertices"][i]["x"] = (*decoupleVertices.get())[i].x();
      info["decoupleVertices"][i]["y"] = (*decoupleVertices.get())[i].y();
      info["decoupleVertices"][i]["z"] = (*decoupleVertices.get())[i].z();
    }
    endOffset = startOffset = 0.0;
    updateWeightPos();
    updateDecouplePos();
    updateSmoothPos();
    dirty();
  }

  void Edge::updateToNode(std::string nodeName, std::string portName) {
    if(!nodeName.empty()) {
      info["toNode"] = nodeName;
      info["toNodeInput"] = portName;
    }
  }

  void Edge::updateFromNode(std::string nodeName, std::string portName) {
    if(!nodeName.empty()) {
      info["fromNode"] = nodeName;
      info["fromNodeOutput"] = portName;
    }
  }

  void Edge::hide(bool v) {
    if(!hidden && v) {
      if(decoupled) {
        this->removeChild((osg::Node*)decoupleIn->getOSGNode());
        this->removeChild((osg::Node*)decoupleOut->getOSGNode());
        geode->removeDrawable(decoupleGeom);
      }
      else if(smooth) {
        geode->removeDrawable(smoothGeom);
      }
      else {
        geode->removeDrawable(geom);
      }
      hidden = true;
    }
    else if(hidden && !v) {
      if(decoupled) {
        this->addChild((osg::Node*)decoupleIn->getOSGNode());
        this->addChild((osg::Node*)decoupleOut->getOSGNode());
        geode->addDrawable(decoupleGeom);
      }
      else if(smooth) {
        geode->addDrawable(smoothGeom);
      }
      else {
        geode->addDrawable(geom);
      }
      hidden = false;
    }
  }

  void Edge::updateSmoothPos() {
    int n = vertices->size()-1;
    (*smoothVertices.get())[0] = (*vertices.get())[0];
    (*smoothVertices.get())[1] = osg::Vec3((*vertices.get())[n].x(), (*vertices.get())[0].y(), 0);
    (*smoothVertices.get())[2] = (*vertices.get())[n];
    (*smoothVertices.get())[3] = osg::Vec3((*vertices.get())[0].x(), (*vertices.get())[n].y(), 0);
    double diff = endPos-startPos;
    if(fabs(diff) < 2) {
      if(diff < 0) {
        (*smoothVertices.get())[0].y() = startPos+1;
        (*smoothVertices.get())[1].y() = startPos+1;
        (*smoothVertices.get())[2].y() = endPos-1;
        (*smoothVertices.get())[3].y() = endPos-1;
      }
      else {
        (*smoothVertices.get())[0].y() = startPos-1;
        (*smoothVertices.get())[1].y() = startPos-1;
        (*smoothVertices.get())[2].y() = endPos+1;
        (*smoothVertices.get())[3].y() = endPos+1;
      }
    }
    startUniform->set((*smoothVertices.get())[0]);
    endUniform->set((*smoothVertices.get())[2]);
  }

  void Edge::setSmooth(bool v) {
    if(v && !smooth) {
      if(!decoupled) {
        geode->removeDrawable(geom);
        geode->addDrawable(smoothGeom);
      }
      smooth = true;
    }
    else if(!v && smooth) {
      if(!decoupled) {
        geode->removeDrawable(smoothGeom);
        geode->addDrawable(geom);
      }
      smooth = false;
    }
    info["smooth"] = smooth;
  }

  void Edge::exportSvg(FILE *f, double ol, double ot) {
    std::string toName = endNode->getName();
    std::string fromName = startNode->getName();
    char buffer[200];
    sprintf(buffer, "%s_out_%d", fromName.c_str(), fromIdx);
    std::string fromNodePort(buffer);
    sprintf(buffer, "%s_in_%d", toName.c_str(), toIdx);
    std::string toNodePort(buffer);
    std::string edgeName = info["id"].toString();
    fromNodePort = replaceString(fromNodePort, ">", "gt_");
    toNodePort = replaceString(toNodePort, ">", "gt_");
    double x1, x2, y1, y2;
    int n = vertices->size()-1;
    x1 = (*vertices.get())[0].x()-ol;
    x2 = (*vertices.get())[n].x()-ol;
    y1 = -(*vertices.get())[0].y()+ot;
    y2 = -(*vertices.get())[n].y()+ot;
    if((bool)info["decouple"]) {
      x1 = (*decoupleVertices.get())[0].x()-ol;
      x2 = (*decoupleVertices.get())[1].x()-ol-x1;
      y1 = -(*decoupleVertices.get())[0].y()+ot;
      y2 = -(*decoupleVertices.get())[1].y()+ot-y1;
      fprintf(f, "    <path style=\"fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:1.0;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" d=\"m %g,%g %g,%g\" id=\"%s_d1\" inkscape:connector-curvature=\"0\" />\n", x1, y1, x2, y2, edgeName.c_str());
      x1 = (*decoupleVertices.get())[2].x()-ol;
      x2 = (*decoupleVertices.get())[3].x()-ol-x1;
      y1 = -(*decoupleVertices.get())[2].y()+ot;
      y2 = -(*decoupleVertices.get())[3].y()+ot-y1;
      fprintf(f, "    <path style=\"fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:1.0;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" d=\"m %g,%g %g,%g\" id=\"%s_d2\" inkscape:connector-curvature=\"0\" />\n", x1, y1, x2, y2, edgeName.c_str());
      View::exportLabelToSvg(decoupleOut, f, -ol, ot);
      View::exportLabelToSvg(decoupleIn, f, -ol, ot);
    }
    else if(smooth) {
      fprintf(f, "    <path style=\"fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:1.0;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" d=\"M %g,%g C %g,%g %g,%g %g,%g\" id=\"%s\" inkscape:connector-curvature=\"0\" sodipodi:nodetypes=\"cc\" />\n",
              x1, y1, x1+(x2-x1)*0.6, y1, x2-(x2-x1)*0.6, y2, x2, y2,
              edgeName.c_str());
    }
    else {
      fprintf(f, "    <path style=\"fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:1.0;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" d=\"m");
      double lastx = 0;
      double lasty = 0;
      for(int i=0; i<=n; ++i) {
        fprintf(f, " %g,%g", (*vertices.get())[i].x()-ol-lastx, -(*vertices.get())[i].y()+ot-lasty);
        lastx = (*vertices.get())[i].x()-ol;
        lasty = -(*vertices.get())[i].y()+ot;
      }
      fprintf(f, "\" id=\"%s\" inkscape:connector-curvature=\"0\" />\n",
              edgeName.c_str());

      // fprintf(f, "\" id=\"%s\" inkscape:connector-type=\"polyline\" inkscape:connector-curvature=\"0\" inkscape:connection-start=\"#%s\" inkscape:connection-end=\"#%s\" sodipodi:nodetypes=\"cc\" />\n",
      //         edgeName.c_str(), fromNodePort.c_str(), toNodePort.c_str());
    }
    if((double)info["weight"] > 1.0000001 ||
       (double)info["weight"] < 0.9999999) {
      View::exportLabelToSvg(weight, f, -ol, ot);
    }
  }

  void Edge::getRectangle(double *x1, double *x2, double *y1, double *y2) {
    *x1 = (*vertices.get())[0].x();
    *y1 = (*vertices.get())[0].y();
    *x2 = *x1;
    *y2 = *y1;
    if(!smooth) {
      for(unsigned int i=1; i<vertices->size(); ++i) {
        if(*x1 > (*vertices.get())[i].x()) {
          *x1 = (*vertices.get())[i].x();
        }
        if(*x2 < (*vertices.get())[i].x()) {
          *x2 = (*vertices.get())[i].x();
        }
        if(*y1 > (*vertices.get())[i].y()) {
          *y1 = (*vertices.get())[i].y();
        }
        if(*y2 < (*vertices.get())[i].y()) {
          *y2 = (*vertices.get())[i].y();
        }
      }
    }
  }

} // end of namespace: osg_graph_viz
