/**
 * \file Node.cpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#include "View.hpp"

#include <osg/Geode>
#include <osg/LineWidth>
#include <cstdio>
#include <configmaps/ConfigData.h>

#include <osg/Texture2D>
#include <osgDB/ReadFile>

#include <mars/osg_material_manager/OsgMaterialManager.h>

// y is going from down to top
// x is going from left to right

using namespace configmaps;

namespace osg_graph_viz {

  std::string Node::replaceString(const std::string &source,
                                  const std::string &s1,
                                  const std::string &s2) {
    std::string back = source;
    size_t found = back.find(s1);
    while(found != std::string::npos) {
      back.replace(found, s1.size(), s2);
      found = back.find(s1, found+s2.size());
    }
    return back;
  }

  Node::Node(View *v) : view(v) {
    init();
  }

  Node::Node(const NodeInfo &info_, View *v) : info(info_), view(v) {
    init();
    initContent();
  }

  void Node::init() {
    materialManager = view->getOsgMaterialManager();
    hidden = false;
    childrenScale = 0.66;
    headerFontSize = view->headerFontSize;
    portFontSize = view->portFontSize;
    mergeIconSize = view->portFontSize;
    portScale = view->portScale;
    portSpaceY = mergeIconSize * 2.0;
    foldIconSize = mergeIconSize / 2.0;
    selected = false;
    ignoreNextInPort = false;
    ignoreNextOutPort = false;
    pos = new osg::PositionAttitudeTransform();
    pos2 = new osg::PositionAttitudeTransform();
    children = new osg::MatrixTransform();
    children->setMatrix(osg::Matrix::scale(childrenScale, childrenScale, 1.));
    pos->addChild(children.get());
    this->addChild(pos.get());
    linew = new osg::LineWidth(1);
    osg::StateSet *state = this->getOrCreateStateSet();
    state->setAttributeAndModes(linew.get(), osg::StateAttribute::ON);
    state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    state->setMode(GL_BLEND, osg::StateAttribute::OFF);
    state->setMode(GL_FOG, osg::StateAttribute::OFF);
    posX = posY = 0.0;
  }

  void Node::initContent() {
    mergeImages["SUM"] = "images/SumPort.png";
    mergeImages["PRODUCT"] = "images/MulPort.png";
    mergeImages["MIN"] = "images/MinPort.png";
    mergeImages["MAX"] = "images/MaxPort.png";

    maxPorts = info.numInputs;
    double namePosX, namePosY;
    if(info.numOutputs > maxPorts) {
      maxPorts = info.numOutputs;
    }
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      headerHeight = 0;
      height = std::max(portSpaceY - 2.0, headerFontSize + 2.0);
      namePosY = -(height - headerFontSize) / 2.0;
      portStartY = -height / 2.0;
    }
    else {
      namePosY = -1.0;
      headerHeight = headerFontSize - 2.0 * namePosY;
      height = headerHeight + portSpaceY*(maxPorts);
      portStartY = -headerHeight - mergeIconSize;
    }
    width = 150.;

    colorMap["INPUT"].push_back(osg::Vec4(0.82, 1.0, 0.87, 1.0));
    colorMap["OUTPUT"].push_back(osg::Vec4(0.52, 0.67, 1.0, 1.0));
    colorMap["DES"].push_back(osg::Vec4(1.0, 1.0, 0.8, 1.0));
    colorMap["DES"].push_back(osg::Vec4(1.0, 1.0, 0.9, 1.0));
    colorMap["META"].push_back(osg::Vec4(1.0, 0.5, 0.5, 1.0));
    colorMap["META"].push_back(osg::Vec4(1.0, .6, 0.6, 1.0));

    bGeode = createBody(width, height, 0, 0, info.map["type"]);
    pos->addChild(bGeode);

    osg_text::Color c(0., 0., 0., 1.0);

    namePosX = width*0.5;
    osg_text::TextAlign align = osg_text::ALIGN_CENTER;
    if((std::string)info.map["type"] == "OUTPUT") {
      namePosX = 5+mergeIconSize / 2.0;
      align = osg_text::ALIGN_LEFT;
    }
    else if((std::string)info.map["type"] == "INPUT") {
      namePosX = 5;
      align = osg_text::ALIGN_LEFT;
    }
    nodeName = new osg_text::Text((std::string)info.map["name"], headerFontSize, c,
                                  namePosX, namePosY, align, 0, 0, 0, 0,
                                  osg_text::Color(), osg_text::Color(),
                                  0, view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");

    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      nodeName->setText((std::string)info.map["name"]);
    }
    else {
      nodeName->setText("[ " + (std::string)info.map["type"] + " ]  " + (std::string)info.map["name"]);
    }
    nodeName->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
    pos->addChild((osg::Node*)nodeName->getOSGNode());

    if((std::string)info.map["type"] == "DES"){
      handleDescription();
    }
    else if((std::string)info.map["type"] == "META"){
      handleMeta();
    }

    handlePorts();
    resizeWidth();
    handlePorts(true);
    portOffsets.push_back(3.0);
    portOffsets.push_back(-3.0);
    portOffsets.push_back(0.0);
  }

  void Node::handlePorts(bool update) {
    osg_text::Color c(0., 0., 0., 1.0);
    int f1 = 0, f2 = 0, f3 = 0, foldState = 0, foldPortIndex=0;
    Port *foldPort;
    int portCnt = 0, foldPortCnt = 0;
    int numOutPorts=0;
    maxPorts = 0;

    // handle the inputs
    for(int i=0; i<info.numInputs; ++i) {
      double portPosY = portStartY - portSpaceY*(portCnt);
      std::string name = (std::string)info.map["inputs"][i]["name"];
      int interface = 0;
      if(info.map["inputs"][i].hasKey("interface")) {
        interface = info.map["inputs"][i]["interface"];
      }
      Port *p;
      if(update) {
        p = inPorts[i];
        if(p->foldIcon.valid()) {
          pos->removeChild(p->foldIcon);
        }
        pos->removeChild(p->geode);
        for(size_t n=0; n<p->labels.size(); ++n) {
          pos->removeChild((osg::Node*)p->labels[n]->getOSGNode());
        }
        std::list<osg::ref_ptr<Edge> >::iterator it = p->edges.begin();
        for(; it!=p->edges.end(); ++it) {
          (*it)->updateToNode((std::string)info.map["name"], name);
        }
      }
      else {
        p = new Port();
        p->hidden = false;
        p->width = -1;
        p->frame.defined = false;
      }
      p->foldPort = 0;
      p->portPos = portCnt;

      if(f2 == 0) {
        if(info.map["inputs"][i].hasKey("fold")) {
          foldState = info.map["inputs"][i]["fold"]["state"];
          f2 = info.map["inputs"][i]["fold"]["num"];
          f3 = 0;
          foldPort = p;
          f1 = 0;
        }
      }
      if(f3 == 0 || foldState == 0) {
        std::string image = "images/OutPort.png";
        if(mergeImages.find(info.map["inputs"][i]["type"]) != mergeImages.end()) {
          image = mergeImages[info.map["inputs"][i]["type"]];
        }
        osg::Geode *g = createRect(portScale*mergeIconSize, portScale*mergeIconSize, -(portScale*mergeIconSize)/2.0, portPosY - (portScale*mergeIconSize)/2.0, image);
        ++maxPorts;
        p->foldState = 0;
        p->geode = g;
        pos->addChild(g);
      }
      else {
        p->foldState = 1;
      }
      if(f2 == 0) {
        if(f1 == 0 && name.find("/x") == name.size()-2) {
          f1 = 1;
          foldPort = p;
          foldPortIndex = i;
          foldPortCnt = portCnt;
        }
        else if(f1 == 1 && name.find("/y") == name.size()-2) {
          f1 = 2;
          p->foldPort = foldPort;
        }
        else if(f1 == 2 && name.find("/z") == name.size()-2) {
          f1 = 3;
          p->foldPort = foldPort;
        }
        else if(f1 == 3 && name.find("/w") == name.size()-2) {
          f1 = 4;
          p->foldPort = foldPort;
        }
        else if(f1 == 3 && name.find("/x") == name.size()-2) {
          f1 = 1;
          foldPort = p;
          foldPortIndex = i;
          foldPortCnt = portCnt;
        }
        else if(f1 == 4 && name.find("/x") == name.size()-2) {
          f1 = 1;
          foldPort = p;
          foldPortIndex = i;
          foldPortCnt = portCnt;
        }
        else {
          f1 = 0;
          foldPort = NULL;
          foldPortIndex = 0;
        }
      }

      if(f1==3) {
        osg::Geode *g = createRect(foldIconSize, foldIconSize, mergeIconSize/2.0, portPosY + mergeIconSize/2.0,
                                   "images/Fold.png");
        foldPort->foldIcon = g;
        pos->addChild(g);
        info.map["inputs"][foldPortIndex]["fold"]["num"] = 3;
        info.map["inputs"][foldPortIndex]["fold"]["state"] = 0;
      }
      else if(f1==4) {
        info.map["inputs"][foldPortIndex]["fold"]["num"] = 4;
        info.map["inputs"][foldPortIndex]["fold"]["state"] = 0;
      }
      else if(f3==0 && f2 > 0) {
        osg::Geode *g = createRect(foldIconSize, foldIconSize, mergeIconSize/2.0, portPosY + mergeIconSize/2.0,
                                   "images/Fold.png");
        p->foldIcon = g;
        pos->addChild(g);
      }

      if(!update) {
        double b = info.map["inputs"][i]["bias"];
        bool print = true;
        if((std::string)info.map["inputs"][i]["type"] == "SUM" &&
           fabs(b) < 0.0000001) {
          print = false;
        }
        else if((std::string)info.map["inputs"][i]["type"] == "PRODUCT" &&
                fabs(1-b) < 0.0000001) {
          print = false;
        }
        char bias[255], def[255];
        bias[0] = def[0] = '\0';
        if(print) {
          sprintf(bias, "[b%g] ", b);
        }
        sprintf(def, "[d%g] ", (double)info.map["inputs"][i]["default"]);
        osg_text::Text *t = new osg_text::Text(std::string(bias)+def+name,
                                               portFontSize, c, 0.0,
                                               portPosY + portFontSize/2.0,
                                               osg_text::ALIGN_LEFT,
                                               0, 0, 0, 0,
                                               osg_text::Color(),
                                               osg_text::Color(),
                                               0, view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");
        t->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
        p->labels.push_back(t);
      }
      if(f3 == 0 || foldState == 0) {
        if(update) {
          p->labels[0]->setPosition(mergeIconSize/2.0 + 2, portPosY + portFontSize/2.0);
          p->labels[0]->setPadding(1, 2, 1, 2);
          p->labels[0]->setBorderColor(osg_text::Color(0.1, 0.1, 0.1, 1));
          p->labels[0]->setBorderWidth(3);
          if(interface == 0) {
            p->labels[0]->setBackgroundColor(osg_text::Color(0.98, 0.98, 0.98, 1));
          }
          else if(interface == 1) {
            p->labels[0]->setBackgroundColor(osg_text::Color(1.0, 0.9, 0.7, 1));
          }
          else if(interface == 2) {
            p->labels[0]->setBackgroundColor(osg_text::Color(1.0, 1, 0.7, 1));
          }
        }
        pos->addChild((osg::Node*)p->labels[0]->getOSGNode());
      }
      if(p->hidden) {
        continue;
      }
      if(f2 > 0) {
        if(++f3 == f2) {
          ++portCnt;
          f1 = f2 = f3 = foldState = 0;
        }
        else if(foldState == 0) {
          ++portCnt;
        }
      }
      else {
        ++portCnt;
      }
      if(!update) inPorts.push_back(p);
    }

    // handle the outputs
    f1 = f2 = f3 = foldState = foldPortIndex = 0;
    portCnt = foldPortCnt = 0;
    for(int i=0; i<info.numOutputs; ++i) {
      double portPosY = portStartY - portSpaceY*(portCnt);
      std::string name = (std::string)info.map["outputs"][i]["name"];
      Port *p;
      int interface = 0;
      if(info.map["outputs"][i].hasKey("interface")) {
        interface = info.map["outputs"][i]["interface"];
      }
      if(update) {
        p = outPorts[i];
        if(p->foldIcon.valid()) {
          pos->removeChild(p->foldIcon);
        }
        pos->removeChild(p->geode);
        pos->removeChild((osg::Node*)p->labels[0]->getOSGNode());
        std::list<osg::ref_ptr<Edge> >::iterator it = p->edges.begin();
        for(; it!=p->edges.end(); ++it) {
          (*it)->updateFromNode((std::string)info.map["name"], name);
        }
      }
      else {
        p = new Port();
        p->hidden = false;
        p->width = -1;
        p->frame.defined = false;
      }
      p->foldPort = 0;
      p->portPos = portCnt;
      if(f2 == 0) {
        if(info.map["outputs"][i].hasKey("fold")) {
          foldState = info.map["outputs"][i]["fold"]["state"];
          f2 = info.map["outputs"][i]["fold"]["num"];
          f3 = 0;
          foldPort = p;
          f1 = 0;
        }
      }
      if(f3 == 0 || foldState == 0) {
        osg::Geode *g = createRect(portScale*mergeIconSize, portScale*mergeIconSize, width - (portScale*mergeIconSize)/2.0, portPosY - (portScale*mergeIconSize)/2.0,
                                   "images/OutPort.png");
        ++numOutPorts;
        p->foldState = 0;
        p->geode = g;
        pos->addChild(g);
      }
      else {
        p->foldState = 1;
      }
      if(f2 == 0) {
        if(f1 == 0 && name.find("/x") == name.size()-2) {
          f1 = 1;
          foldPort = p;
          foldPortIndex = i;
          foldPortCnt = portCnt;
        }
        else if(f1 == 1 && name.find("/y") == name.size()-2) {
          f1 = 2;
          p->foldPort = foldPort;
        }
        else if(f1 == 2 && name.find("/z") == name.size()-2) {
          f1 = 3;
          p->foldPort = foldPort;
        }
        else if(f1 == 3 && name.find("/w") == name.size()-2) {
          f1 = 4;
          p->foldPort = foldPort;
        }
        else if(f1 == 3 && name.find("/x") == name.size()-2) {
          f1 = 1;
          foldPort = p;
          foldPortIndex = i;
          foldPortCnt = portCnt;
        }
        else if(f1 == 4 && name.find("/x") == name.size()-2) {
          f1 = 1;
          foldPort = p;
          foldPortIndex = i;
          foldPortCnt = portCnt;
        }
        else {
          f1 = 0;
          foldPort = NULL;
          foldPortIndex = 0;
        }
      }

      if(f1==3) {
        osg::Geode *g = createRect(foldIconSize, foldIconSize, width - mergeIconSize/2.0 - foldIconSize, portPosY + mergeIconSize/2.0,
                                   "images/Fold.png");
        foldPort->foldIcon = g;
        pos->addChild(g);
        info.map["outputs"][foldPortIndex]["fold"]["num"] = 3;
        info.map["outputs"][foldPortIndex]["fold"]["state"] = 0;
      }
      else if(f1==4) {
        info.map["outputs"][foldPortIndex]["fold"]["num"] = 4;
        info.map["outputs"][foldPortIndex]["fold"]["state"] = 0;
      }
      else if(f3==0 && f2 > 0) {
        osg::Geode *g = createRect(foldIconSize, foldIconSize, width - mergeIconSize/2.0 - foldIconSize, portPosY + mergeIconSize/2.0,
                                   "images/Fold.png");
        p->foldIcon = g;
        pos->addChild(g);
      }

      if(!update) {
        osg_text::Text *t = new osg_text::Text(name, portFontSize, c,
                                               width - (mergeIconSize/2.0 + 2.0),
                                               portPosY + portFontSize/2.0,
                                               osg_text::ALIGN_RIGHT,
                                               0, 0, 0, 0,
                                               osg_text::Color(),
                                               osg_text::Color(),
                                               0, view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");
        t->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
        p->labels.push_back(t);
      }
      if(f3 == 0 || foldState == 0) {
        if(update) {
          p->labels[0]->setPosition(width - (mergeIconSize/2.0 + 2.0), portPosY + portFontSize/2.0);
          p->labels[0]->setPadding(1, 2, 1, 2);
          p->labels[0]->setBorderColor(osg_text::Color(0.1, 0.1, 0.1, 1));
          p->labels[0]->setBorderWidth(3);
          if(interface == 0) {
            p->labels[0]->setBackgroundColor(osg_text::Color(0.98, 0.98, 0.98, 1));
          }
          else if(interface == 1) {
            p->labels[0]->setBackgroundColor(osg_text::Color(1.0, 0.9, 0.7, 1));
          }
          else if(interface == 2) {
            p->labels[0]->setBackgroundColor(osg_text::Color(1.0, 1, 0.7, 1));
          }
        }
        pos->addChild((osg::Node*)p->labels[0]->getOSGNode());
      }
      if(p->hidden) {
        continue;
      }
      if(f2 > 0) {
        if(++f3 == f2) {
          ++portCnt;
          f1 = f2 = f3 = foldState = 0;
        }
        else if(foldState == 0) {
          ++portCnt;
        }
      }
      else {
        ++portCnt;
      }
      if(!update) outPorts.push_back(p);
    }

    if(numOutPorts > maxPorts) {
      maxPorts = numOutPorts;
    }
    resizeHeight();
  }

  void Node::handleDescription(){
    osg_text::Color c(0., 0., 0., 1.0);
    double space = 5.0;
    double font_size;
    std::string text = "please enter text here";
    if(info.map.find("font_size") != info.map.end()){
      font_size = info.map["font_size"];
    }else{
      font_size = portFontSize;
      info.map["font_size"] = font_size;
    }

    if(info.map.find("text") != info.map.end()){
      text = (std::string)info.map["text"];
    }

    // exchange characters \n to new line
    std::string from = "\\n";
    std::string to = "\n";
    size_t start_pos = 0;
    while((start_pos = text.find(from, start_pos)) != std::string::npos) {
      text.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }

    if(textBody){
      textBody->setText(text);
      textBody->setFontSize(font_size);
    }else{
      textBody = new osg_text::Text(text, font_size, c,
                                    space, -(headerHeight + 2.0), osg_text::ALIGN_LEFT, 0, 0, 0, 0,
                                    osg_text::Color(), osg_text::Color(),
                                    0, view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");
      textBody->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
      pos->addChild((osg::Node*)textBody->getOSGNode());
    }

    // adapt the box size
    double left, right, top, bottom;
    textBody->getRectangle(&left, &right, &top, &bottom);
    double x1, x2, y1, y2;
    nodeName->getRectangle(&x1, &x2, &y1, &y2);

    width = std::max(right - left, x2 - x1) + 2.0 * space;
    height = headerHeight + top - bottom + space;
    resizeWidth(width);
    resizeHeight(height);
    // vertices->at(2).x() = width;
    // vertices->at(3).x() = width;
    // vertices->at(6).x() = width;
    // vertices->at(7).x() = width;
    // vertices->at(10).x() = width;
    // vertices->at(11).x() = width;
    // vertices->at(5).y() = -height;
    // vertices->at(9).y() = -height;
    // vertices->at(6).y() = -height;
    // vertices->at(10).y() = -height;
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
    vertices->dirty();
  }

  void Node::handleMeta(){
    osg_text::Color c(0., 0., 0., 1.0);
    double space = 5.0;
    // adapt the box size
    double x1, x2, y1, y2;
    nodeName->getRectangle(&x1, &x2, &y1, &y2);

    width = x2 - x1 + 2.0 * space;
    height = headerHeight + space;
    vertices->at(2).x() = width;
    vertices->at(3).x() = width;
    vertices->at(6).x() = width;
    vertices->at(7).x() = width;
    vertices->at(10).x() = width;
    vertices->at(11).x() = width;
    vertices->at(5).y() = -height;
    vertices->at(9).y() = -height;
    vertices->at(6).y() = -height;
    vertices->at(10).y() = -height;
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
    vertices->dirty();
  }

  void Node::setLineWidth(double w) {
    if(parent.valid()) {
      w *= parent->getChildrenScale();
    }
    linew->setWidth(w);
  }

  void Node::applyFontScale(double s) {
    double x, y;
    view->getWindowPixelSize(&x, &y);
    x = headerFontSize*(x/1920)*s;
    if(x<16) x = 16;
    else if(x<32) x=32;
    else if(x<64) x=64;
    else if(x<128) x=128;
    else if(x<256) x=256;
    else x=512;
    nodeName->setFontResolution(x, x);
    for(int i=0; i<info.numInputs; ++i) {
      for(size_t n=0; n<inPorts[i]->labels.size(); ++n) {
        inPorts[i]->labels[n]->setFontResolution(x, x);
      }
    }
    for(int i=0; i<info.numOutputs; ++i) {
      for(size_t n=0; n<outPorts[i]->labels.size(); ++n) {
        outPorts[i]->labels[n]->setFontResolution(x, x);
      }
    }
  }

  Node::~Node(void) {
  }

  void Node::addChildNode(osg::ref_ptr<osg_graph_viz::Node> node) {
    children->addChild(node.get());
    node->setPosition(0, 0);
  }

  void Node::removeChildNode(osg::ref_ptr<osg_graph_viz::Node> node) {
    children->removeChild(node.get());
  }

  void Node::setPosition2(double x, double y) {
    convertPos(&x, &y);
    setPosition(x+posOffsetX, y+posOffsetY);
  }

  void Node::setAbsolutePosition(double x, double y) {
    convertPos(&x, &y);
    setPosition( x, y );
  }

  void Node::setPosition(double x, double y) {
    posX = (int)x;
    posY = (int)y;
    if(parent.valid()) {
      double minX = mergeIconSize + parent->getMinChildX() / parent->getChildrenScale();
      double maxY = parent->getMaxChildY() / parent->getChildrenScale();
      maxY -= 10;
      if(posX<minX) posX = minX;
      if(posY>maxY) posY = maxY;
      parent->updateSize();
    }
    info.map["pos"]["x"] = posX;
    info.map["pos"]["y"] = posY;
    pos->setPosition(osg::Vec3(posX, posY, 0.0));
    pos2->setPosition(osg::Vec3(posX, posY, 0.0));
    updateEdges();
  }

  double Node::getMaxChildY() {
    return -headerHeight;
  }

  double Node::getMinChildX() {
    double x1, x2, y1, y2;
    double x = 0;
    if((std::string)info.map["type"] == "OUTPUT") {
      inPorts[0]->labels[0]->getRectangle(&x1, &x2, &y1, &y2);
      x = x2-x1;
    }
    else if((std::string)info.map["type"] == "DES") {
      // the description nodes are handled differently
      return x;
    } else if((std::string)info.map["type"] == "META") {
      // the meta nodes are handled differently
      return x;
    } else {
      for(int i=0; i<info.numInputs; ++i) {
        if(inPorts[i]->width > 0) {
          if(inPorts[i]->width > x) x = inPorts[i]->width;
        }
        else {
          std::vector<osg::ref_ptr<osg_text::Text> >::iterator it = inPorts[i]->labels.begin();
          for(; it!=inPorts[i]->labels.end(); ++it) {
            (*it)->getRectangle(&x1, &x2, &y1, &y2);
            if(x2 > x) x = x2;
          }
        }
      }
    }
    return x;
  }

  void Node::updateEdges() {
    for(size_t i=0; i<inPorts.size(); ++i) {
      std::list<osg::ref_ptr<Edge> >::iterator it;
      for(it=inPorts[i]->edges.begin(); it!=inPorts[i]->edges.end(); ++it) {
        (*it)->updateEndPos(getInPortPos(i));
      }
    }

    for(size_t i=0; i<outPorts.size(); ++i) {
      std::list<osg::ref_ptr<Edge> >::iterator it;
      for(it=outPorts[i]->edges.begin(); it!=outPorts[i]->edges.end(); ++it) {
        (*it)->updateStartPos(getOutPortPos(i));
      }
    }
    for(size_t i=0; i<children->getNumChildren(); ++i) {
      osg_graph_viz::Node *node = dynamic_cast<osg_graph_viz::Node*>(children->getChild(i));
      if(node) {
        node->updateEdges();
      }
    }
  }

  void Node::addPosition(double x, double y) {
    if(parent.valid()) {
      x /= parent->getChildrenScale();
      y /= parent->getChildrenScale();
    }
    posX += x;
    posY += y;
    pos->setPosition(osg::Vec3(posX, posY, 0.0));
    pos2->setPosition(osg::Vec3(posX, posY, 0.0));
  }

  void Node::getPosOffset(double x, double y, double *ox, double *oy) {
    convertPos(&x, &y);
    *ox = posX - x;
    *oy = posY - y;
    if(parent.valid()) {
      *ox /= parent->getChildrenScale();
      *oy /= parent->getChildrenScale();
    }
  }

  void Node::savePosOffset(double x, double y) {
    convertPos(&x, &y);
    posOffsetX = posX-x;
    posOffsetY = posY-y;
  }

  osg::Vec3 Node::getInPortPos(int index) {
    double x = posX-mergeIconSize/2.0;
    double y = posY+portStartY-portSpaceY*(inPorts[index]->portPos);
    convertPosToWorld(&x, &y);
    return osg::Vec3(x, y, 0.0);
  }

  bool Node::hasInPortConnection(int index) {
    return (inPorts[index]->edges.size() > 0);
  }

  osg::Vec3 Node::getOutPortPos(int index) {
    double x = posX+width+mergeIconSize/2.0;
    double y = posY+portStartY-portSpaceY*(outPorts[index]->portPos);
    convertPosToWorld(&x, &y);
    return osg::Vec3(x, y, 0);
  }

  void Node::updateParentFromMap(ConfigMap &m) {
    std::string parentName;
    bool hadKey = false;
    // handle position of node
    double x, y;
    getPosition(&x, &y);
    if(info.map.hasKey("parentName")) {
      hadKey = true;
      parentName << info.map["parentName"];
    }
    if(m.hasKey("parentName")) {
      std::string newParentName = m["parentName"];
      if(newParentName != parentName) {
  if(view->groupNodes(newParentName, getName())) {
    info.map["parentName"] = newParentName;
    if(!view->updateNode(this)) {
      if(hadKey) {
        info.map["parentName"] = parentName;
      }
      fprintf(stderr, "update node failed!\n");
      return;
    }
    if(parentName.empty()) {
      view->removeNodeFromView(this);
    }
    else {
      osg::ref_ptr<Node> parent_ = view->getNodeByName(parentName);
      if(parent_.valid()) {
        parent_->removeChildNode(this);
        parent = NULL;
      }
      else {
        view->removeNodeFromView(this);
      }
    }
    if(newParentName.empty()) {
      view->addNodeToView(this);
    }
    else {
      parent = view->getNodeByName(newParentName);
      if(parent.valid()) {
        parent->addChildNode(this);
      }
      else {
        view->addNodeToView(this);
      }
    }
    // restore position
    setAbsolutePosition(x, y);
    m["pos"]["x"] = info.map["pos"]["x"];
    m["pos"]["y"] = info.map["pos"]["y"];
  }
  else {
    info.map["parentName"] = "";
    m["parentName"] = "";
  }
      }
      else if(info.map.hasKey("parentName")) {
        m["parentName"] = parentName;
      }
    }
  }

  void Node::updateMap(const ConfigMap &map_) {
    ConfigMap map = map_;
    updateParentFromMap(map);
    info.map = map;
    setPosition(info.map["pos"]["x"], info.map["pos"]["y"]);
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      nodeName->setText((std::string)info.map["name"]);
    }
    else {
      nodeName->setText("[ " + (std::string)info.map["type"] + " ]  " + (std::string)info.map["name"]);
    }
    for(size_t i=0; i<inPorts.size(); ++i) {
      double b = info.map["inputs"][i]["bias"];
      double d = info.map["inputs"][i]["default"];
      bool print = true;
      if((std::string)info.map["inputs"][i]["type"] == "SUM" &&
         fabs(b) < 0.0000001) {
        print = false;
      }
      else if((std::string)info.map["inputs"][i]["type"] == "PRODUCT" &&
              fabs(1-b) < 0.0000001) {
        print = false;
      }
      char bias[255];
      bias[0] = '\0';
      if(print) {
        sprintf(bias, "[b%g] ", b);
      }
      char def[255];
      def[0] = '\0';
      if(inPorts[i]->edges.size() == 0) {
        sprintf(def, "[d%g] ", d);
      }
      std::string name = (std::string)info.map["inputs"][i]["name"];
      inPorts[i]->labels[0]->setText((std::string)bias+def+name);
    }
    for(size_t i=0; i<outPorts.size(); ++i) {
      std::string name = (std::string)info.map["outputs"][i]["name"];
      outPorts[i]->labels[0]->setText((std::string)name);
    }
    double x1, x2, y1, y2;
    nodeName->getRectangle(&x1, &x2, &y1, &y2);
    if((std::string)info.map["type"] == "DES"){
      handleDescription();
    }
    else if((std::string)info.map["type"] == "META"){
      handleMeta();
    }
    updateSize();
    handlePorts(true);
  }

  void Node::addInputEdge(int index, Edge* edge) {
    //edge->setEndOffset(portOffsets[inPorts[index]->edges.size()%3]);
    inPorts[index]->edges.push_back(edge);
    double b = info.map["inputs"][index]["bias"];
    bool print = true;
    if((std::string)info.map["inputs"][index]["type"] == "SUM" &&
       fabs(b) < 0.0000001) {
      print = false;
    }
    else if((std::string)info.map["inputs"][index]["type"] == "PRODUCT" &&
            fabs(1-b) < 0.0000001) {
      print = false;
    }
    char bias[255];
    bias[0] = '\0';
    if(print) {
      sprintf(bias, "[b%g] ", b);
    }
    std::string name = (std::string)info.map["inputs"][index]["name"];
    inPorts[index]->labels[0]->setText(bias+name);
    edge->setEndNode(this);
    resizeWidth();
    handlePorts(true);
  }

  void Node::addOutputEdge(int index, Edge* edge) {
    //edge->setStartOffset(portOffsets[outPorts[index]->edges.size()%3]);
    outPorts[index]->edges.push_back(edge);
    edge->setStartNode(this);
    updateEdges();
  }

  void Node::removeOutputEdge(Edge *edge) {
    std::list<osg::ref_ptr<Edge> >::iterator it;
    for(size_t i=0; i<outPorts.size(); ++i) {
      for(it=outPorts[i]->edges.begin(); it!=outPorts[i]->edges.end(); ++it) {
        while(it->get() == edge) {
          it = outPorts[i]->edges.erase(it);
        }
        if(it==outPorts[i]->edges.end()) break;
      }
    }
  }

  void Node::removeInputEdge(Edge *edge) {
    std::list<osg::ref_ptr<Edge> >::iterator it;
    for(size_t i=0; i<inPorts.size(); ++i) {
      for(it=inPorts[i]->edges.begin(); it!=inPorts[i]->edges.end(); ++it) {
        while(it->get() == edge) {
          it = inPorts[i]->edges.erase(it);
        }
        if(it==inPorts[i]->edges.end()) break;
      }
    }
  }

  void Node::removeEdges() {
    size_t t;
    for(size_t i=0; i<inPorts.size(); ++i) {
      while(inPorts[i]->edges.size() > 0) {
        t = inPorts[i]->edges.size();
        view->removeEdge(inPorts[i]->edges.begin()->get());
        if(t==inPorts[i]->edges.size()) {
          // todo: handle error
          break;
        }
      }
    }
    for(size_t i=0; i<outPorts.size(); ++i) {
      while(outPorts[i]->edges.size() > 0) {
        t = outPorts[i]->edges.size();
        view->removeEdge(outPorts[i]->edges.begin()->get());
        if(t==outPorts[i]->edges.size()) {
          // todo: handle error
          break;
        }
      }
    }
  }

  void Node::decoupleEdges() {
    for(size_t i=0; i<inPorts.size(); ++i) {
      for (auto it=inPorts[i]->edges.begin(); it!=inPorts[i]->edges.end(); it++) {
        (*it)->decoupleEdge();
      }
    }
    for(size_t i=0; i<outPorts.size(); ++i) {
      for (auto it=outPorts[i]->edges.begin(); it!=outPorts[i]->edges.end(); it++) {
        (*it)->decoupleEdge();
      }
    }
  }

  void Node::convertPos(double *x, double *y) {
    if(parent.valid()) {
      // convert mouse pos
      double x1, y1;
      parent->getPosition(&x1, &y1);
      *x -= x1;
      *y -= y1;
      *x /= parent->getChildrenScale();
      *y /= parent->getChildrenScale();
    }
  }

  void Node::convertPosToWorld(double *x, double *y) {
    if(parent.valid()) {
      // convert mouse pos
      double x1, y1;
      parent->getPosition(&x1, &y1);
      *x *= parent->getChildrenScale();
      *y *= parent->getChildrenScale();
      *x += x1;
      *y += y1;
    }
  }

  bool Node::checkMousePress(double x, double y, bool handleFold) {
    if(hidden) return false;
    convertPos(&x, &y);

    // first check the children
    for(size_t i=0; i<children->getNumChildren(); ++i) {
      osg_graph_viz::Node *node = dynamic_cast<osg_graph_viz::Node*>(children->getChild(i));
      if(node) {
        if(node->checkMousePress(x, y, false)) return false;
      }
    }

    if(handleFold) {
      for(size_t i=0; i<inPorts.size(); ++i) {
        Port* p = inPorts[i];
        if(p->foldIcon.valid()) {
          double mergeIconPosY = portStartY-portSpaceY*(p->portPos);
          double foldIconCornerX = posX + mergeIconSize/2.0;
          double foldIconCornerY = posY + mergeIconPosY + mergeIconSize/2.0;
          //fprintf(stderr, "fold in check: %g-%g / %g-%g\n", foldIconCornerX, foldIconCornerX + foldIconSize, foldIconCornerY, foldIconCornerY + foldIconSize);
          if(x > foldIconCornerX && x < foldIconCornerX + foldIconSize) {
            if(y > foldIconCornerY  && foldIconCornerY + foldIconSize) {
              int state = info.map["inputs"][i]["fold"]["state"];
              state++;
              state %= 2;
              info.map["inputs"][i]["fold"]["state"] = state;
              handlePorts(true);
              updateEdges();
              ignoreNextInPort = true;
              ignoreNextOutPort = true;
              //fprintf(stderr, "hit fold port %s\n", std::string(info.map["inputs"][i]["name"]).c_str());
              return true;
            }
          }
        }
      }
      for(size_t i=0; i<outPorts.size(); ++i) {
        Port* p = outPorts[i];
        if(p->foldIcon.valid()) {
          double mergeIconPosY = portStartY-portSpaceY*(p->portPos);
          double foldIconCornerX = posX + width - mergeIconSize/2.0 - foldIconSize;
          double foldIconCornerY = posY + mergeIconPosY + mergeIconSize/2.0;
          //fprintf(stderr, "fold out check: %g-%g / %g-%g\n", foldIconCornerX, foldIconCornerX + foldIconSize, foldIconCornerY, foldIconCornerY + foldIconSize);
          if(x > foldIconCornerX && x < foldIconCornerX + foldIconSize) {
            if(y > foldIconCornerY  && foldIconCornerY + foldIconSize) {
              int state = info.map["outputs"][i]["fold"]["state"];
              state++;
              state %= 2;
              info.map["outputs"][i]["fold"]["state"] = state;
              handlePorts(true);
              updateEdges();
              ignoreNextInPort = true;
              ignoreNextOutPort = true;
              //fprintf(stderr, "hit out port %s\n", std::string(info.map["outputs"][i]["name"]).c_str());
              return true;
            }
          }
        }
      }
    }
    if(x > posX && x < posX + width) {
      if(y > posY - height && y < posY) {
        //fprintf(stderr, "hit node\n");
        return true;
      }
    }
    return false;
  }

  bool Node::checkMouseHeaderPress(double x, double y) {
    if(hidden) return false;
    convertPos(&x, &y);
    if(x > posX && x < posX + width) {
      if(y > posY - headerHeight && y < posY) {
        //fprintf(stderr, "hit header\n");
        return true;
      }
    }
    return false;
  }
  bool Node::checkMouseInPortHover(double x, double y, double *vX, double *vY, Port *p)
  {
    if (hidden)
      return false;
    convertPos(&x, &y);
    if (ignoreNextInPort)
    {
      ignoreNextInPort = false;
      return false;
    }

    if (p->hidden)
    {
      return false;
    }
    double iconPosY = portStartY - portSpaceY * (p->portPos);
    fprintf(stderr, "hover check: %g-%g / %g-%g\n", posX - mergeIconSize / 2.0, posX + mergeIconSize / 2.0, posY + iconPosY - mergeIconSize / 2.0, posY + iconPosY + mergeIconSize / 2.0);
    if (x > posX - (portScale * mergeIconSize) / 2.0 && x < posX + (portScale * mergeIconSize) / 2.0)
    {
      if (y > posY + iconPosY - (portScale * mergeIconSize) / 2.0 &&
          y < posY + iconPosY + (portScale * mergeIconSize) / 2.0)
      {
        *vX = posX - (portScale * mergeIconSize) / 2.0;
        *vY = posY + iconPosY;
        convertPosToWorld(vX, vY);
        convertPosToWorld(vX, vY);
        return true;
      }
    }
    double x1, x2, y1, y2, y3;
    if (p->frame.defined)
    {
      x1 = p->frame.x;
      x2 = x1 + p->frame.w;
      y2 = p->frame.y;
      y1 = y2 + p->frame.h;
    }
    else
    {
      double w2 = 0;
      double w4;
      for (size_t n = 0; n < p->labels.size(); ++n)
      {
        p->labels[n]->getRectangle(&x1, &x2, &y1, &y2);
        w4 = x2 - x1;
        if (w4 > w2)
          w2 = w4;
        if (n == 0)
          y3 = y1;
      }
      y1 = y3;
      x2 = x1 + w2;
    }
    x1 += posX;
    x2 += posX;
    y1 += posY;
    y2 += posY;
    if (x > x1 && x < x2 && y > y2 && y < y1)
    {
      fprintf(stderr, "hover in port\n");
      *vX = posX - (portScale * mergeIconSize) / 2.0;
      *vY = posY + iconPosY;
      convertPosToWorld(vX, vY);
      return true;
    }
    return false;
  }

  bool Node::checkMouseInPortPress(double x, double y,
                                   double *vX, double *vY, int *idx) {
    if(hidden) return false;
    convertPos(&x, &y);
    if(ignoreNextInPort) {
      ignoreNextInPort = false;
      return false;
    }
    for(size_t i=0; i<inPorts.size(); ++i) {
      Port* p = inPorts[i];
      if(p->hidden) {
        continue;
      }
      double iconPosY = portStartY - portSpaceY*(p->portPos);
      //fprintf(stderr, "in check: %g-%g / %g-%g\n", posX - mergeIconSize/2.0, posX + mergeIconSize/2.0, posY + iconPosY - mergeIconSize/2.0, posY + iconPosY + mergeIconSize/2.0);
      if(x > posX - (portScale*mergeIconSize)/2.0 && x < posX + (portScale*mergeIconSize)/2.0) {
        if(y > posY + iconPosY - (portScale*mergeIconSize)/2.0 &&
           y < posY + iconPosY + (portScale*mergeIconSize)/2.0) {
          *vX = posX - (portScale*mergeIconSize)/2.0;
          *vY = posY + iconPosY;
          convertPosToWorld(vX, vY);
          *idx = i;
          //fprintf(stderr, "hit in port\n");
          return true;
        }
      }
      double x1, x2, y1, y2, y3;
      if(p->frame.defined) {
        x1 = p->frame.x;
        x2 = x1+p->frame.w;
        y2 = p->frame.y;
        y1 = y2+p->frame.h;
      }
      else {
        double w2 = 0;
        double w4;
        for(size_t n=0; n<p->labels.size(); ++n) {
          p->labels[n]->getRectangle(&x1, &x2, &y1, &y2);
          w4 = x2-x1;
          if(w4 > w2) w2 = w4;
          if(n==0) y3 = y1;
        }
        y1 = y3;
        x2 = x1+w2;
      }
      x1 += posX;
      x2 += posX;
      y1 += posY;
      y2 += posY;
      if(x>x1 && x<x2 && y>y2 && y<y1) {
        *vX = posX - (portScale*mergeIconSize)/2.0;
        *vY = posY + iconPosY;
        convertPosToWorld(vX, vY);
        *idx = i;
        //fprintf(stderr, "hit in port\n");
        return true;
      }
    }
    return false;
  }

  bool Node::checkMouseOutPortHover(double x, double y, double *vX, double *vY, Port *p)
  {
    if (hidden)
      return false;
    convertPos(&x, &y);
    if (ignoreNextOutPort)
    {
      ignoreNextOutPort = false;
      return false;
    }
    if (p->hidden)
    {
      return false;
    }
    if (p->foldState == 0)
    {
      double iconPosY = portStartY - portSpaceY * (p->portPos);
      fprintf(stderr, "out hover check: %g-%g / %g-%g\n", posX + width - mergeIconSize / 2.0, posX + width + mergeIconSize / 2.0, posY + iconPosY - mergeIconSize / 2.0, posY + iconPosY + mergeIconSize / 2.0);
      if (x > posX + width - (portScale * mergeIconSize) / 2.0 && x < posX + width + (portScale * mergeIconSize) / 2.0)
      {
        if (y > posY + iconPosY - (portScale * mergeIconSize) / 2.0 &&
            y < posY + iconPosY + (portScale * mergeIconSize) / 2.0)
        {
          *vX = posX + width - (portScale * mergeIconSize) / 2.0;
          *vY = posY + iconPosY;
          fprintf(stderr, "hover out port\n");
          return true;
        }
      }
      double x1, x2, y1, y2, y3;
      if (p->frame.defined)
      {
        x1 = p->frame.x;
        x2 = x1 + p->frame.w;
        y2 = p->frame.y;
        y1 = y2 + p->frame.h;
      }
      else
      {
        double w2 = 0;
        double w4;
        for (size_t n = 0; n < p->labels.size(); ++n)
        {
          p->labels[n]->getRectangle(&x1, &x2, &y1, &y2);
          w4 = x2 - x1;
          if (w4 > w2)
            w2 = w4;
          if (n == 0)
            y3 = y1;
        }
        y1 = y3;
        x1 = x2 - w2;
      }
      x1 += posX;
      x2 += posX;
      y1 += posY;
      y2 += posY;
      if (x > x1 && x < x2 && y > y2 && y < y1)
      {
        fprintf(stderr, "hover out port\n");
        *vX = posX - (portScale * mergeIconSize) / 2.0;
        *vY = posY + iconPosY;
        convertPosToWorld(vX, vY);
        return true;
      }
    }

    return false;
  }

  bool Node::checkMouseOutPortPress(double x, double y,
                                    double *vX, double *vY, int *idx) {
    if(hidden) return false;
    convertPos(&x, &y);
    if(ignoreNextOutPort) {
      ignoreNextOutPort = false;
      return false;
    }
    for(size_t i=0; i<outPorts.size(); ++i) {
      Port* p = outPorts[i];
      if(p->hidden) {
        continue;
      }
      if(outPorts[i]->foldState == 0) {
        double iconPosY = portStartY - portSpaceY*(p->portPos);
        //fprintf(stderr, "out check: %g-%g / %g-%g\n", posX + width - mergeIconSize/2.0, posX + width + mergeIconSize/2.0, posY + iconPosY - mergeIconSize/2.0, posY + iconPosY + mergeIconSize/2.0);
        if(x > posX + width - (portScale*mergeIconSize)/2.0 && x < posX + width + (portScale*mergeIconSize)/2.0) {
          if(y > posY + iconPosY - (portScale*mergeIconSize)/2.0 &&
             y < posY + iconPosY + (portScale*mergeIconSize)/2.0) {
            *vX = posX + width - (portScale*mergeIconSize)/2.0;
            *vY = posY + iconPosY;
            *idx = i;
            //fprintf(stderr, "hit out port\n");
            return true;
          }
        }
        double x1, x2, y1, y2, y3;
        if(p->frame.defined) {
          x1 = p->frame.x;
          x2 = x1+p->frame.w;
          y2 = p->frame.y;
          y1 = y2+p->frame.h;
        }
        else {
          double w2 = 0;
          double w4;
          for(size_t n=0; n<p->labels.size(); ++n) {
            p->labels[n]->getRectangle(&x1, &x2, &y1, &y2);
            w4 = x2-x1;
            if(w4 > w2) w2 = w4;
            if(n==0) y3 = y1;
          }
          y1 = y3;
          x1 = x2-w2;
        }
        x1 += posX;
        x2 += posX;
        y1 += posY;
        y2 += posY;
        if(x>x1 && x<x2 && y>y2 && y<y1) {
          *vX = posX - (portScale*mergeIconSize)/2.0;
          *vY = posY + iconPosY;
          convertPosToWorld(vX, vY);
          *idx = i;
          //fprintf(stderr, "hit in port\n");
          return true;
        }
      }
    }
    return false;
  }


  void Node::setRenderOrder(int o) {
    this->getOrCreateStateSet()->setRenderBinDetails(o, "RenderBin");
    for(size_t i=0; i<inPorts.size(); ++i) {
      std::list<osg::ref_ptr<Edge> >::iterator it;
      for(it=inPorts[i]->edges.begin(); it!=inPorts[i]->edges.end(); ++it) {
        (*it)->getOrCreateStateSet()->setRenderBinDetails(o, "RenderBin");
      }
    }
    for(size_t i=0; i<outPorts.size(); ++i) {
      std::list<osg::ref_ptr<Edge> >::iterator it;
      for(it=outPorts[i]->edges.begin(); it!=outPorts[i]->edges.end(); ++it) {
        (*it)->getOrCreateStateSet()->setRenderBinDetails(o, "RenderBin");
      }
    }
    // todo: set order for children
    for(size_t i=0; i<children->getNumChildren(); ++i) {
      osg_graph_viz::Node *node = dynamic_cast<osg_graph_viz::Node*>(children->getChild(i));
      if(node) {
        node->setRenderOrder(o+1);
      }
    }
  }


  void Node::setSelected(bool b) {
    selected = b;
    if(b) {
      if((std::string)info.map["type"] == "INPUT" ||
         (std::string)info.map["type"] == "OUTPUT") {
        (*bColors.get())[0] = osg::Vec4(0.62, 1.0, 0.67, 1.0);
        (*bColors.get())[1] = osg::Vec4(0.0, 0.7, 0.0, 1.0);
      }
      else {
        (*bColors.get())[1] = osg::Vec4(0.62, 1.0, 0.67, 1.0);
        (*bColors.get())[2] = osg::Vec4(0.0, 0.7, 0.0, 1.0);
      }
    }
    else {
      if((std::string)info.map["type"] == "INPUT") {
        (*bColors.get())[0] = osg::Vec4(0.82, 1.0, 0.87, 1.0);
        (*bColors.get())[1] = osg::Vec4(0.0, 0.0, 0.0, 1.0);
      }
      else if((std::string)info.map["type"] == "OUTPUT") {
        (*bColors.get())[0] = osg::Vec4(0.52, 0.67, 1.0, 1.0);
        (*bColors.get())[1] = osg::Vec4(0.0, 0.0, 0.0, 1.0);
      }else if((std::string)info.map["type"] == "DES") {
        (*bColors.get())[1] = osg::Vec4(1.0, 1.0, 0.9, 1.0);
        (*bColors.get())[2] = osg::Vec4(0.0, 0.0, 0.0, 1.0);
      }else if((std::string)info.map["type"] == "META") {
        (*bColors.get())[1] = osg::Vec4(1.0, 0.5, 0.5, 1.0);
        (*bColors.get())[2] = osg::Vec4(0.0, 0.0, 0.0, 1.0);
      }
      else {
        (*bColors.get())[1] = osg::Vec4(0.82, 0.87, 1.0, 1.0);
        (*bColors.get())[2] = osg::Vec4(0.0, 0.0, 0.0, 1.0);
      }
    }
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
    vertices->dirty();
  }

  osg::Geode* Node::createRect(double w, double h, double x, double y,
                               std::string textureFile) {
    osg::Geode *geode = new osg::Geode;
    osg::Geometry *geom = new osg::Geometry;

    osg::Vec3Array *vertices = new osg::Vec3Array();
    vertices->push_back(osg::Vec3(x, y, 0.0));
    vertices->push_back(osg::Vec3(x+w, y, 0.0));
    vertices->push_back(osg::Vec3(x+w, y+h, 0.0));
    vertices->push_back(osg::Vec3(x, y+h, 0.0));
    geom->setVertexArray(vertices);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

    osg::Vec4Array *bColors = new osg::Vec4Array;
    bColors->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    geom->setColorArray(bColors);
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec3Array *normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
    geom->setNormalArray(normals);
    geom->setNormalBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec2Array *texcoords = new osg::Vec2Array();
    double ep = 0.001;
    texcoords->push_back(osg::Vec2(-ep, -ep));
    texcoords->push_back(osg::Vec2(1+ep, -ep));
    texcoords->push_back(osg::Vec2(1+ep, 1+ep));
    texcoords->push_back(osg::Vec2(-ep, 1+ep));

    geom->setTexCoordArray(0, texcoords);


    geode->addDrawable(geom);

    // todo: load materials only once
    if(!textureFile.empty()) {
      osg::Texture2D *texture = view->loadTexture(textureFile);
      geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture,
                                                                osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);
    }

    return geode;
  }

  osg::Geode* Node::createFrame(double w, double h, double x, double y,
                                osg::Vec4 color, osg::Vec4 bcolor) {
    osg::Geode *geode = new osg::Geode;
    osg::Geometry *geom = new osg::Geometry;

    osg::Vec3Array *vertices = new osg::Vec3Array();
    vertices->push_back(osg::Vec3(x, y, -0.01));
    vertices->push_back(osg::Vec3(x+w, y, -0.01));
    vertices->push_back(osg::Vec3(x+w, y+h, -0.01));
    vertices->push_back(osg::Vec3(x, y+h, -0.01));
    vertices->push_back(osg::Vec3(x, y, -0.01));
    geom->setVertexArray(vertices);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, 5));
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

    osg::Vec4Array *bColors = new osg::Vec4Array;
    bColors->push_back(color);
    bColors->push_back(bcolor);
    geom->setColorArray(bColors);
    geom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);

    geode->addDrawable(geom);

    return geode;
  }

  void Node::getRectangle(double *x1, double *x2, double *y1, double *y2) {
    double pm = 0.5*portScale*mergeIconSize;
    *x1 = posX;
    *x2 = posX + width;
    *y1 = posY - height;
    *y2 = posY;
    if(info.numInputs > 0) {
      *x1 -= pm;
    }
    if(info.numOutputs > 0) {
      *x2 += pm;
    }
  }

  void Node::updateSize() {
    resizeHeight();
    resizeWidth();
  }

  void Node::resizeHeight() {
    double h = 0;
    double h2 = 0;
    for(size_t i=0; i<children->getNumChildren(); ++i) {
      double x1, x2, y1, y2;
      osg_graph_viz::Node *node = dynamic_cast<osg_graph_viz::Node*>(children->getChild(i));
      if(node) {
        node->getRectangle(&x1, &x2, &y1, &y2);
        y1 *= childrenScale;
        y1 -= 15;
        if(-y1 > h) h = -y1;
      }
    }
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      h2 = std::max(portSpaceY - 2.0, headerFontSize + 2.0);
    }
    else if((std::string)info.map["type"] == "DES") {
      h2 = headerHeight + portSpaceY*(maxPorts);
    }
    else if((std::string)info.map["type"] == "META") {
      h2 = headerHeight + portSpaceY*(maxPorts);
    }
    else {
      h2 = portSpaceY*(maxPorts-1)+headerHeight+mergeIconSize*1.5;
    }
    if(h2 > h) h = h2;
    height = h;
    resizeHeight(h);
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
    vertices->dirty();
  }

  void Node::resizeHeight(double v) {
    vertices->at(5).y() = vertices->at(9).y() = -v;
    vertices->at(6).y() = vertices->at(10).y() = -v;
  }

  void Node::resizeWidth() {
    if((std::string)info.map["type"] == "INPUT" ||
       (std::string)info.map["type"] == "OUTPUT") {
      double x1, x2, y1, y2;
      nodeName->getRectangle(&x1, &x2, &y1, &y2);
      width = x2-x1+16;
      if((std::string)info.map["type"] == "INPUT") {
        outPorts[0]->labels[0]->getRectangle(&x1, &x2, &y1, &y2);
        width += x2-x1+10;
      }
      else {
        inPorts[0]->labels[0]->getRectangle(&x1, &x2, &y1, &y2);
        width += x2-x1+10;
        nodeName->setPosition(x2+10, y1);
      }
    }else if((std::string)info.map["type"] == "DES") {
      // the description nodes are handled differently
      return;
    }else if((std::string)info.map["type"] == "META") {
      // the meta nodes are handled differently
      return;
    }else {
      double x1, x2, y1, y2;
      nodeName->getRectangle(&x1, &x2, &y1, &y2);
      double w1 = x2-x1;
      double w2 = 0;
      double w3 = 0, w4;
      if(children->getNumChildren()) {
        for(size_t i=0; i<children->getNumChildren(); ++i) {
          osg_graph_viz::Node *node = dynamic_cast<osg_graph_viz::Node*>(children->getChild(i));
          if(node) {
            node->getRectangle(&x1, &x2, &y1, &y2);
            x2 *= childrenScale;
            if(x2 > w2) w2 = x2;
          }
        }
      }
      else {
        for(int i=0; i<info.numInputs; ++i) {
          if(inPorts[i]->width > 0) {
            if(inPorts[i]->width > w2) w2 = inPorts[i]->width;
          }
          else {
            std::vector<osg::ref_ptr<osg_text::Text> >::iterator it = inPorts[i]->labels.begin();
            for(; it!=inPorts[i]->labels.end(); ++it) {
              (*it)->getRectangle(&x1, &x2, &y1, &y2);
              w4 = x2-x1;
              if(w4 > w2) w2 = w4;
            }
          }
        }
      }

      for(int i=0; i<info.numOutputs; ++i) {
        if(outPorts[i]->width > 0) {
          if(outPorts[i]->width > w3) w3 = outPorts[i]->width;
        }
        else {
          std::vector<osg::ref_ptr<osg_text::Text> >::iterator it = outPorts[i]->labels.begin();
          for(; it!=outPorts[i]->labels.end(); ++it) {
            (*it)->getRectangle(&x1, &x2, &y1, &y2);
            w4 = x2-x1;
            if(w4 > w3) w3 = w4;
          }
        }
      }
      if(w1 > w2 + w3) {
        width = w1 + 20;
      }
      else {
        width = w2+w3+20;
      }
    }
    resizeWidth(width);
    updateEdges();
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
    vertices->dirty();
  }

  void Node::resizeWidth(double v) {
    vertices->at(2).x() = v;
    vertices->at(3).x() = v;
    vertices->at(6).x() = v;
    vertices->at(7).x() = v;
    vertices->at(10).x() = v;
    vertices->at(11).x() = v;
    if((std::string)info.map["type"] != "INPUT" &&
       (std::string)info.map["type"] != "OUTPUT") {
      double left, right, top, bottom;
      nodeName->getRectangle(&left, &right, &top, &bottom);
      nodeName->setPosition(width*0.5, top);
    }
  }

  /*void resizeHeight(double h); {
    if((std::string)info.map["type"] == "INPUT" ||
    (std::string)info.map["type"] == "OUTPUT") {
    height = h;
    vertices->at(2) = osg::Vec3(width, -height, 0.0);
    vertices->at(3) = osg::Vec3(width, -10, 0.0);
    vertices->at(5) = osg::Vec3(width, 0, 0.0);
    vertices->at(6) = osg::Vec3(width, -10, 0.0);
    vertices->at(10) = osg::Vec3(width, -height, 0.0);
    vertices->at(11) = osg::Vec3(width, 0, 0.0);
    updateEdges();
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
    vertices->dirty();
    }
    }*/


  osg::Geode* Node::createBody(double w, double h, double x, double y,
                               std::string color, bool gardientHeader) {
    osg::Geode *geode = new osg::Geode;
    bGeom = new osg::Geometry;

    vertices = new osg::Vec3Array();

    // header
    vertices->push_back(osg::Vec3(x, y, 0.0));
    vertices->push_back(osg::Vec3(x, y-headerHeight, 0.0));
    vertices->push_back(osg::Vec3(x+w, y-headerHeight, 0.0));
    vertices->push_back(osg::Vec3(x+w, y, 0.0));

    // body
    vertices->push_back(osg::Vec3(x, y-headerHeight, 0.0));
    vertices->push_back(osg::Vec3(x, y-h, 0.0));
    vertices->push_back(osg::Vec3(x+w, y-h, 0.0));
    vertices->push_back(osg::Vec3(x+w, y-headerHeight, 0.0));

    // frame
    vertices->push_back(osg::Vec3(x, y, 0.0));
    vertices->push_back(osg::Vec3(x, y-h, 0.0));
    vertices->push_back(osg::Vec3(x+w, y-h, 0.0));
    vertices->push_back(osg::Vec3(x+w, y, 0.0));
    vertices->push_back(osg::Vec3(x, y, 0.0));

    bGeom->setVertexArray(vertices);
    if(headerHeight > 0.000001) {
      bGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    }
    bGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 4, 4));
    bGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 8, 5));
    bGeom->setDataVariance(osg::Object::DYNAMIC);

    bColors = new osg::Vec4Array;

    std::vector<osg::Vec4>::iterator it;
    if(colorMap.find(color) != colorMap.end()) {
      std::vector<osg::Vec4> &colors = colorMap[color];
      if(gardientHeader) {
        bColors->push_back(colors[0]);
        bColors->push_back(colors[1]);
        bColors->push_back(colors[1]);
        bColors->push_back(colors[0]);
        for(int l=0; l<4; ++l)
          bColors->push_back(colors[1]);
        for(int l=0; l<5; ++l)
          bColors->push_back(colors[2]);
      }
      else {
        for(it=colors.begin(); it!=colors.end(); ++it) {
          bColors->push_back(*it);
        }
      }
    }
    else {
      //default colors
      // header color
      bColors->push_back(osg::Vec4(0.9, 0.9, 0.9, 1.0));
      // frame color
      bColors->push_back(osg::Vec4(0.82, 0.87, 1.0, 1.0));
    }

    // frame color
    bColors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    bGeom->setColorArray(bColors);
    if(gardientHeader) {
      bGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    else {
      bGeom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    }

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
      osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);
      }
    */
    return geode;
  }

  std::vector<std::pair<int, std::string> > Node::getOutFoldInfo(int index) {
    if((std::string)info.map["type"] == "INPUT") {
      std::vector<std::pair<int, std::string> > r;
      std::pair<int, std::string> p;
      p.first = 0;
      p.second = (std::string)info.map["name"];
      r.push_back(p);
      return r;
    }
    return getFoldInfo("outputs", index);
  }

  std::vector<std::pair<int, std::string> > Node::getInFoldInfo(int index) {
    if((std::string)info.map["type"] == "OUTPUT") {
      std::vector<std::pair<int, std::string> > r;
      std::pair<int, std::string> p;
      p.first = 0;
      p.second = (std::string)info.map["name"];
      r.push_back(p);
      return r;
    }
    return getFoldInfo("inputs", index);
  }

  std::vector<std::pair<int, std::string> > Node::getFoldInfo(std::string tag,
                                                              int index) {
    std::vector<std::pair<int, std::string> > r;
    std::pair<int, std::string> p;
    p.first = index;
    p.second = (std::string)info.map[tag][index]["name"];
    r.push_back(p);
    int state = info.map[tag][index]["fold"]["state"];
    if(state == 1) {
      int i = 1;
      int num = info.map[tag][index]["fold"]["num"];
      while(i<num) {
        p.first = index+i;
        p.second = (std::string)info.map[tag][index+i]["name"];
        r.push_back(p);
        ++i;
      }
    }
    return r;
  }

  std::string Node::getInPortName(int index) {
    if((std::string)info.map["type"] == "INPUT") {
      return (std::string)info.map["name"];
    }
    else {
      return (std::string)info.map["inputs"][index]["name"];
    }
  }

  std::string Node::getOutPortName(int index) {
    if((std::string)info.map["type"] == "OUTPUT") {
      return (std::string)info.map["name"];
    }
    else {
      return (std::string)info.map["outputs"][index]["name"];
    }
  }

  std::string Node::getName() {
    return (std::string)info.map["name"];
  }

  bool Node::isInput() {
    return (std::string)info.map["type"] == "INPUT";
  }

  bool Node::isOutput() {
    return (std::string)info.map["type"] == "OUTPUT";
  }

  void Node::reposition() {
    if((std::string)info.map["type"] == "INPUT") {
      if(outPorts[0]->edges.size() > 0) {
        osg::Vec3 v = outPorts[0]->edges.front()->getEndPosition();
        setPosition(v.x()-50-width, v.y()+height*0.5);
      }
    }
    else if((std::string)info.map["type"] == "OUTPUT") {
      if(inPorts[0]->edges.size() > 0) {
        osg::Vec3 v = inPorts[0]->edges.front()->getStartPosition();
        setPosition(v.x()+50, v.y()+height*0.5);
      }
    }
  }

  NodeInfo Node::getNodeInfo(){
    return info;
  }

  void Node::setNodeInfo(NodeInfo new_info){
    info = new_info;
  }

  void Node::derenderText(const bool readable){
    if(readable){//adds text to the Graph, here the texts are the names a Node has.
      pos->removeChild((osg::Node*)nodeName->getOSGNode());
      pos->addChild((osg::Node*)nodeName->getOSGNode());
      //derenderTextInPort(readable);
      for(size_t i=0; i<inPorts.size(); ++i) { // it is necessary to add the "names" of the ports (in- and outPorts).
        if(inPorts[i]->hidden) continue;
        for(size_t n=0; n<inPorts[i]->labels.size(); ++n) {
          pos->removeChild((osg::Node*) inPorts[i]->labels[n]->getOSGNode());
          pos->addChild((osg::Node*) inPorts[i]->labels[n]->getOSGNode());
        }
      }

      for(size_t i=0; i<outPorts.size(); ++i) {//same for outPorts
        if(outPorts[i]->hidden) continue;
        for(size_t n=0; n<outPorts[i]->labels.size(); ++n) {
          pos->removeChild((osg::Node*) outPorts[i]->labels[n]->getOSGNode());
          pos->addChild((osg::Node*) outPorts[i]->labels[n]->getOSGNode());
        }
      }
    }
    else{//removes text from the Graph
      pos->removeChild((osg::Node*)nodeName->getOSGNode());
      //derenderTextInPort(readable);
      for(size_t i=0; i<inPorts.size(); ++i) {// // it is necessary to remove the "names" of the ports (in- and outPorts).
        if(inPorts[i]->hidden) continue;
        for(size_t n=0; n<inPorts[i]->labels.size(); ++n) {
          pos->removeChild((osg::Node*) inPorts[i]->labels[n]->getOSGNode());
        }
      }

      for(size_t i=0; i<outPorts.size(); ++i) {//same for outPorts
        if(outPorts[i]->hidden) continue;
        for(size_t n=0; n<outPorts[i]->labels.size(); ++n) {
          pos->removeChild((osg::Node*) outPorts[i]->labels[n]->getOSGNode());
        }
      }
    }
  }


  void Node::dirty(void) {
    // linesGeom->dirtyDisplayList();
    // linesGeom->dirtyBound();
  }

  configmaps::ConfigMap Node::getOutPortEdgeInfo(int index) {
    return ConfigMap();
  }

  std::vector<osg::ref_ptr<osg_graph_viz::Edge> > Node::getOutputEdges() {
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > outEdges;
    for(size_t i=0; i<outPorts.size(); ++i) {
      std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
      for(it=outPorts[i]->edges.begin(); it!=outPorts[i]->edges.end(); ++it) {
        outEdges.push_back(*it);
      }
    }
    return outEdges;
  }

  void Node::exportPortsSVG(FILE *f, double ol, double ot) {
    std::string name = getName();
    double pm = portScale*mergeIconSize;
    int i=0;
    // todo: handle fold state
    for(auto it: inPorts) {
      double portPosY = portStartY - portSpaceY*(i);
      std::string linkname = replaceString(name, ">", "gt_");
      fprintf(f, "    <rect ry=\"2.873735\" x=\"%g\" y=\"%g\" height=\"%g\" width=\"%g\" id=\"%s_in_%d\" style=\"opacity:1;fill:#ffffff;fill-opacity:1;fill-rule:nonzero;stroke:#000000;stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1\" />\n", -pm*0.5+ol, -(portPosY+pm*0.5)+ot, pm, pm, linkname.c_str(), i);
      for(auto label: it->labels) {
        View::exportLabelToSvg(label, f, ol, ot);
      }
      std::string merge = info.map["inputs"][i]["type"];
      std::string rotate = " ";
      double x2 = ol;
      double y2 = ot-portPosY+portFontSize*0.41;
      double size = pm*1.5;
      if(merge == "SUM") {
        merge = "+";
      }
      else if(merge == "PRODUCT") {
        merge = "*";
        y2 = ot-portPosY+portFontSize*0.76;
      }
      else if(merge == "MIN") {
        merge = "&gt;";
        rotate = " rotate=\"90 0\"";
        y2 = ot-portPosY-portFontSize*0.36;
        size = pm*1.25;
      }
      else if(merge == "MAX") {
        merge = "&lt;";
        rotate = " rotate=\"90 0\"";
        y2 = ot-portPosY-portFontSize*0.41;
        size = pm*1.25;
      }
      else {
        merge = "";
      }
      if(!merge.empty()) {
        fprintf(f, "    <text xml:space=\"preserve\" style=\"font-style:normal;font-weight:normal;font-size:%gpx;line-height:125%%;font-family:Stilu;letter-spacing:0px;word-spacing:0px;fill:#ff0000;fill-opacity:1;stroke:none;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" x=\"%g\" y=\"%g\" id=\"merge_%s\">\n", pm, x2, y2, linkname.c_str());
        fprintf(f, "      <tspan id=\"mergetext_%s\" x=\"%g\" y=\"%g\" style=\"font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:%gpx;font-family:'Stilu';-inkscape-font-specification:'Stilu';text-align:center;text-anchor:middle\"%s>%s</tspan>\n    </text>\n", linkname.c_str(), x2, y2, size, rotate.c_str(), merge.c_str());
      }
      ++i;
    }

    i=0;
    // todo: handle fold state
    for(auto it: outPorts) {
      double portPosY = portStartY - portSpaceY*(i);
      std::string linkname = replaceString(name, ">", "gt_");
      fprintf(f, "    <rect ry=\"2.873735\" x=\"%g\" y=\"%g\" height=\"%g\" width=\"%g\" id=\"%s_out_%d\" style=\"opacity:1;fill:#ffffff;fill-opacity:1;fill-rule:nonzero;stroke:#000000;stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1\" />\n", width-pm*0.5+ol, -(portPosY+pm*0.5)+ot, pm, pm, linkname.c_str(), i);
      ++i;
      for(auto label: it->labels) {
        View::exportLabelToSvg(label, f, ol, ot);
      }
    }
  }

} // end of namespace: osg_graph_viz
