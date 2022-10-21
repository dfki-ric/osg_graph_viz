/**
 * \file XRockNode.cpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#include "View.hpp"
#include "XRockNode.hpp"

#include <osg/Geode>
#include <osg/LineWidth>
#include <cstdio>
#include <configmaps/ConfigData.h>

#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <cassert>
#include <sstream>

// y is going from down to top
// x is going from left to right


using namespace configmaps;

namespace osg_graph_viz {

  std::vector<std::string> explodeString(const char c, const std::string &s) {
    std::vector<std::string> result;
    std::stringstream  stream(s);
    std::string entry;

    while(std::getline(stream,entry,c)) {
      result.push_back(entry);
    }
    return result;
  }

  std::string rockStripType(std::string type) {
    if(type.find("::RTT::extras::") != std::string::npos) {
      auto arrString = explodeString(' ', type);
      std::string s = arrString[1];
      for(size_t i=2; i<arrString.size()-1; ++i) {
        s += " " + arrString[i];
      }
      return s;
    }
    return type;
  }

  XRockNode::XRockNode(const NodeInfo &info_, View *v) : RoundBodyNode(v) {
    info = info_;
    hidden = false;
    maxPorts = info.numInputs;
    mergeImages["SUM"] = "images/SumPort.png";
    mergeImages["PRODUCT"] = "images/MulPort.png";
    mergeImages["MIN"] = "images/MinPort.png";
    mergeImages["MAX"] = "images/MaxPort.png";
    double namePosX, namePosY;
    if(info.numOutputs > maxPorts) {
      maxPorts = info.numOutputs;
    }
    namePosY = -0.5*headerFontSize;
    headerHeight = 2.5*headerFontSize;
    height = headerHeight + portSpaceY*(maxPorts);
    mergeIconSize = portFontSize;
    //portScale *= 1.5;
    portSpaceY = 3.0*mergeIconSize * 1.2;
    portStartY = -headerHeight - mergeIconSize*4;
    width = 150.;

    if((std::string)info.map["domain"] == "software") {
      colorMap["taskNode"].push_back(osg::Vec4(0.92, 1.0, 0.92, 1.0));
      colorMap["taskNode"].push_back(osg::Vec4(0.3, 0.5, 0.3, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 1.0, 0.77, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 1.0, 0.77, 1.0));
    }
    else if((std::string)info.map["domain"] == "electronics") {
      colorMap["taskNode"].push_back(osg::Vec4(0.85, .92, 1., 1.0));
      colorMap["taskNode"].push_back(osg::Vec4(0.3, 0.3, 0.5, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 0.77, 1., 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, .77, 1, 1.0));
    }
    else if((std::string)info.map["domain"] == "mechanics") {
      colorMap["taskNode"].push_back(osg::Vec4(0.95, .9, 0.8, 1.0));
      colorMap["taskNode"].push_back(osg::Vec4(0.4, 0.4, 0.3, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.95, 0.8, 0.5, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.95, 0.8, 0.5, 1.0));
    }
    else if((std::string)info.map["domain"] == "behavior") {
      colorMap["taskNode"].push_back(osg::Vec4(1.0, .8, 0.8, 1.0));
      colorMap["taskNode"].push_back(osg::Vec4(0.5, 0.3, 0.3, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(1.0, 0.6, 0.6, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(1.0, 0.6, 0.6, 1.0));
    }
    else if((std::string)info.map["domain"] == "computation") {
      colorMap["taskNode"].push_back(osg::Vec4(1.0, .8, 0.8, 1.0));
      colorMap["taskNode"].push_back(osg::Vec4(0.5, 0.3, 0.3, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(1.0, 0.6, 0.6, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(1.0, 0.6, 0.6, 1.0));
    }
    else { // default grey
      colorMap["taskNode"].push_back(osg::Vec4(0.94, .94, 0.94, 1.0));
      colorMap["taskNode"].push_back(osg::Vec4(0.3, 0.3, 0.3, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.6, 0.9, 0.6, 1.0));
      colorMap["taskNodeSelected"].push_back(osg::Vec4(0.6, 0.9, 0.6, 1.0));
    }
    if(info.map.hasKey("xrock_type")) {
      if((std::string)info.map["xrock_type"] == "ros_network") {
        colorMap.clear();
        colorMap["taskNode"].push_back(osg::Vec4(0.75, 1.0, 0.85, 1.0));
        colorMap["taskNode"].push_back(osg::Vec4(0.3, 0.5, 0.3, 1.0));
        colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 1.0, 0.77, 1.0));
        colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 1.0, 0.77, 1.0));
      }
      else if((std::string)info.map["xrock_type"] == "ROS Topic") {
        colorMap.clear();
        colorMap["taskNode"].push_back(osg::Vec4(1.0, 1.0, 0.8, 1.0));
        colorMap["taskNode"].push_back(osg::Vec4(0.3, 0.5, 0.3, 1.0));
        colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 1.0, 0.77, 1.0));
        colorMap["taskNodeSelected"].push_back(osg::Vec4(0.72, 1.0, 0.77, 1.0));
      }
    }
    bGeode = createBody(width, height, 0, 0, "taskNode", true);
    pos->addChild(bGeode.get());

    osg_text::Color c(0., 0., 0., 1.0);

    namePosX = 8;
    osg_text::TextAlign align = osg_text::ALIGN_LEFT;
    nodeName = new osg_text::Text((std::string)info.map["name"], headerFontSize, c,
                                  namePosX, namePosY, align, 0, 0, 0, 0,
                                  osg_text::Color(), osg_text::Color(),
                                  0, view->getResourcesPath()+"/fonts/stilu/Stilu-SemiBold.ttf");

    nodeName->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
    pos->addChild((osg::Node*)nodeName->getOSGNode());
    //c.r = c.g = c.b =0.3;
    nodeType = new osg_text::Text((std::string)info.map["type"], headerFontSize, c,
                                  namePosX, namePosY-headerFontSize*1.66, align, 0, 0, 0, 0,
                                  osg_text::Color(), osg_text::Color(),
                                  0, view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");
    nodeType->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
    pos->addChild((osg::Node*)nodeType->getOSGNode());

    initRoundBody();
    frameUniform->set(osg::Vec3f(2, 15, 0));
    handlePorts();
    Node::resizeWidth();
    handlePorts(true);
    portOffsets.push_back(3.0);
    portOffsets.push_back(-3.0);
    portOffsets.push_back(0.0);
    filterUpdate();
  }

  bool XRockNode::getMergeInfo(size_t i, double *bias, double *def, std::string *merge) {
    std::string domainData = (std::string)(info.map["domain"])+ "Data";
    std::string name = (std::string)info.map["inputs"][i]["name"];
    ConfigMap *map = &info.map;
    if((*map).hasKey(domainData)) {
      map = info.map[domainData];
      if((*map).hasKey("data")) {
        map = (*map)["data"];
        if((*map).hasKey("configuration")) {
          map = (*map)["configuration"];
          if((*map).hasKey("interfaces")) {
            map = (*map)["interfaces"];
            if((*map).hasKey(name)) {
              map = (*map)[name];
              if((*map).hasKey("bias")) {
                *bias << (*map)["bias"];
                *def << (*map)["default"];
                *merge << (*map)["merge"];
                return true;
              }
            }
          }
        }
      }
    }
    return false;
  }

  void XRockNode::handlePorts(bool update) {
    osg_text::Color c(0., 0., 0., 1.0);
    int portCnt = 0;
    int numOutPorts=0;
    maxPorts = 0;
    frames.clear();

    // handle the inputs
    for(int i=0; i<info.numInputs; ++i) {
      double portPosY = portStartY - portSpaceY*(portCnt);
      std::string name = (std::string)info.map["inputs"][i]["name"];
      std::string type = (std::string)info.map["inputs"][i]["type"];
      std::string domain;
      std::string merge;
      double biasValue, defValue;
      bool hasMerge = getMergeInfo(i, &biasValue, &defValue, &merge);
      if(info.map["domain"] == "assembly" && info.map["inputs"][i].hasKey("domain")) {
        domain << info.map["inputs"][i]["domain"];
      }
      Port *p;
      if(update) {
        p = inPorts[i];
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
      p->portPos = portCnt;
      std::string image = "images/OutPort.png";
      if(hasMerge) {
        if(mergeImages.find(merge) != mergeImages.end()) {
          image = mergeImages[merge];
        }
      }
      osg::Geode *g = createRect(portScale*mergeIconSize, portScale*mergeIconSize, -portScale*mergeIconSize*0.5, portPosY - portScale*mergeIconSize*0.5, image);
      p->geode = g;

      if(!update) {
        char bias[255], def[255];
        bias[0] = def[0] = '\0';
        if(hasMerge) {
          bool print = true;
          if(merge == "SUM" &&
             fabs(biasValue) < 0.0000001) {
            print = false;
          }
          else if(merge == "PRODUCT" &&
                  fabs(1-biasValue) < 0.0000001) {
            print = false;
          }
          if(print) {
            sprintf(bias, "[b%g] ", biasValue);
          }
          sprintf(def, "[d%g] ", defValue);
        }

        osg_text::Text *t = new osg_text::Text(std::string(bias) + def + name,
                                               portFontSize, c, 0.0,
                                               portPosY +0,
                                               osg_text::ALIGN_LEFT,
                                               0, 0, 0, 0,
                                               osg_text::Color(),
                                               osg_text::Color(1, 0, 0, 1),
                                               0, view->getResourcesPath()+"/fonts/stilu/Stilu-SemiBold.ttf");
        t->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
        p->labels.push_back(t);
        t = new osg_text::Text(type, portFontSize,c
                               /*osg_text::Color(0.3, 0.3, 0.3, 1)*/, 0.0, portPosY,
                               osg_text::ALIGN_LEFT, 0, 0, 0, 0,
                               osg_text::Color(), osg_text::Color(0, 0, 0, 1),
                               0, view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");
        t->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
        p->labels.push_back(t);
      }
      if(update) {
        double x1, x2, y1, y2;
        double w2 = 0;
        double w4;
        for(size_t n=0; n<p->labels.size(); ++n) {
          p->labels[n]->getRectangle(&x1, &x2, &y1, &y2);
          w4 = x2-x1;
          if(w4 > w2) w2 = w4;
          p->labels[n]->setPosition(mergeIconSize*0.5+2, portPosY + mergeIconSize*2 - portFontSize*0.6 - n*portFontSize*1.5);
        }
        int interface=0;
        std::string direction;
        if(info.map["inputs"][i].hasKey("interface")) {
          interface = info.map["inputs"][i]["interface"];
        }
        if(info.map["inputs"][i].hasKey("direction")) {
          direction << info.map["inputs"][i]["direction"];
        }
        if(p->hidden) {
          continue;
        }
        osg::Vec4 c(1.0, 1.0, 1.0, 1);
        if(!domain.empty()) {
          if(domain == "mechanics") {
            c = osg::Vec4(0.95, .9, 0.8, 1.0);
          }
          else if(domain == "computation") {
            c = osg::Vec4(1.0, .8, 0.8, 1.0);
          }
          else if(domain == "software") {
            c = osg::Vec4(0.92, 1.0, 0.92, 1.0);
          }
          else if(domain == "electronics") {
            c = osg::Vec4(0.85, .92, 1., 1.0);
          }
          else if(domain == "behavior") {
            c = osg::Vec4(1.0, .8, 0.8, 1.0);
          }
        }
        if(interface == 1) {
          c = osg::Vec4(1.0, 0.9, 0.7, 1);
        }
        else if(interface == 2) {
          c = osg::Vec4(1.0, 1.0, 0.7, 1);
        }
        Frame frame = {w2+4.5, mergeIconSize*3, 4.5, portPosY-mergeIconSize*1.4, osg::Vec4(0.3, 0.3, 0.3, 1), c, true};
        p->frame = frame;
        if(direction == "bidirectional") {
          frame.w = width-9;
          p->frame.w = width*0.5-4.5;
        }
        frames.push_back(frame);
        osg::Geode *g = createFrame(frame.w, frame.h, frame.x, frame.y, frame.bc, frame.c);
        if(p->group.valid()) {
          pos->removeChild(p->group);
        }
        p->group = new osg::Group;
        p->group->addChild(g);
        pos->addChild(p->group);
        p->width = w2+8;
      }
      for(size_t n=0; n<p->labels.size(); ++n) {
        pos->addChild((osg::Node*)p->labels[n]->getOSGNode());
      }
      if(!update) inPorts.push_back(p);
      ++portCnt;
      pos->addChild(g);
    }

    // handle the outputs
    maxPorts = portCnt;
    portCnt = 0;
    for(int i=0; i<info.numOutputs; ++i) {
      double portPosY = portStartY - portSpaceY*(portCnt);
      std::string name = (std::string)info.map["outputs"][i]["name"];
      std::string type = (std::string)info.map["outputs"][i]["type"];
      std::string domain;
      std::string direction;
      if(info.map["domain"] == "assembly" && info.map["outputs"][i].hasKey("domain")) {
        domain << info.map["outputs"][i]["domain"];
      }
      if(info.map["outputs"][i].hasKey("direction")) {
        direction << info.map["outputs"][i]["direction"];
      }
      Port *p;
      if(update) {
        p = outPorts[i];
        pos->removeChild(p->geode);
        for(size_t n=0; n<p->labels.size(); ++n) {
          pos->removeChild((osg::Node*)p->labels[n]->getOSGNode());
        }
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
      osg::Geode *g = createRect(portScale*mergeIconSize, portScale*mergeIconSize,
                                 width - portScale*mergeIconSize*0.5,
                                 portPosY - portScale*mergeIconSize*0.5,
                                 "images/OutPort.png");
      p->geode = g;
      p->portPos = portCnt;
      ++numOutPorts;

      if(!update && direction != "bidirectional") {
        osg_text::Text *t = new osg_text::Text(name, portFontSize, c,
                                               width - (mergeIconSize*2.0 + 2.0),
                                               portPosY + portFontSize/2.0,
                                               osg_text::ALIGN_RIGHT,
                                               0, 0, 0, 0,
                                               osg_text::Color(),
                                               osg_text::Color(),
                                               0, view->getResourcesPath()+"/fonts/stilu/Stilu-SemiBold.ttf");
        t->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
        p->labels.push_back(t);
        t = new osg_text::Text(type, portFontSize, c
                               /*osg_text::Color(0.3, 0.3, 0.3, 1)*/,
                               width - (mergeIconSize*2.0 + 2.0),
                               portPosY + portFontSize/2.0,
                               osg_text::ALIGN_RIGHT, 0, 0, 0, 0,
                               osg_text::Color(), osg_text::Color(), 0,
                               view->getResourcesPath()+"/fonts/stilu/Stilu-Light.ttf");
        t->setBackgroundColor(osg_text::Color(0.0, 0.0, 0.0, 0.0));
        p->labels.push_back(t);
      }
      if(update) {
        double x1, x2, y1, y2;
        double w2 = 0;
        double w4;
        for(size_t n=0; n<p->labels.size(); ++n) {
          p->labels[n]->getRectangle(&x1, &x2, &y1, &y2);
          w4 = x2-x1;
          if(w4 > w2) w2 = w4;
          p->labels[n]->setPosition(width - (mergeIconSize*0.5 + 2.0), portPosY + mergeIconSize*2 - portFontSize*0.6 - n*portFontSize*1.5);
        }
        int interface=0;
        if(p->hidden) {
          continue;
        }
        if(info.map["outputs"][i].hasKey("interface")) {
          interface = info.map["outputs"][i]["interface"];
        }
        osg::Vec4 c(1.0, 1.0, 1.0, 1);
        if(!domain.empty()) {
          if(domain == "mechanics") {
            c = osg::Vec4(0.95, .9, 0.8, 1.0);
          }
          else if(domain == "computation") {
            c = osg::Vec4(1.0, .8, 0.8, 1.0);
          }
          else if(domain == "software") {
            c = osg::Vec4(0.92, 1.0, 0.92, 1.0);
          }
          else if(domain == "electronics") {
            c = osg::Vec4(0.85, .92, 1., 1.0);
          }
          else if(domain == "behavior") {
            c = osg::Vec4(1.0, .8, 0.8, 1.0);
          }
        }
        if(interface == 1) {
          c = osg::Vec4(1.0, 0.9, 0.7, 1);
        }
        else if(interface == 2) {
          c = osg::Vec4(1.0, 1.0, 0.7, 1);
        }
        Frame frame = {w2+4.5, mergeIconSize*3, width-w2-9.0, portPosY-mergeIconSize*1.4, osg::Vec4(0.3, 0.3, 0.3, 1), c, true};
        if(direction == "bidirectional") {
          frame.x = 4.5;
          frame.w = width-9;
        }
        else {
          frames.push_back(frame);
          osg::Geode *g = createFrame(frame.w, frame.h, frame.x, frame.y, frame.bc, frame.c);
          if(p->group.valid()) {
            pos->removeChild(p->group);
          }
          p->group = new osg::Group;
          p->group->addChild(g);
          pos->addChild(p->group);
        }
        p->frame = frame;
        p->width = w2+8;

        for(size_t n=0; n<p->labels.size(); ++n) {
          pos->addChild((osg::Node*)p->labels[n]->getOSGNode());
        }
      }
      ++portCnt;
      if(!update) outPorts.push_back(p);
      pos->addChild(g);
    }

    if(portCnt > maxPorts) {
      maxPorts = portCnt;
    }
    resizeHeight();
  }

  void XRockNode::setLineWidth(double w) {
    if(w < 2) w = 2;
    linew->setWidth(w);
  }

  XRockNode::~XRockNode(void) {
  }

  osg::Vec3 XRockNode::getInPortPos(int index) {
    return Node::getInPortPos(index);
    //return osg::Vec3(posX, posY+portStartY-portSpaceY*(inPorts[index]->portPos), 0.0);
  }

  osg::Vec3 XRockNode::getOutPortPos(int index) {
    return Node::getOutPortPos(index);
    //return osg::Vec3(posX+width, posY+portStartY-portSpaceY*(outPorts[index]->portPos), 0.0);
  }

  void XRockNode::updateMap(const ConfigMap &map_) {
    ConfigMap map = map_;
    updateParentFromMap(map);
    info.map = map;
    setPosition(info.map["pos"]["x"], info.map["pos"]["y"]);
    nodeName->setText((std::string)info.map["name"]);
    for(size_t i=0; i<inPorts.size(); ++i) {
      std::string merge;
      double biasValue, defValue;
      bool hasMerge = getMergeInfo(i, &biasValue, &defValue, &merge);
      if(hasMerge) {
        bool print = true;
        if(merge == "SUM" &&
           fabs(biasValue) < 0.0000001) {
          print = false;
        }
        else if(merge == "PRODUCT" &&
                fabs(1-biasValue) < 0.0000001) {
          print = false;
        }
        char bias[255];
        bias[0] = '\0';
        if(print) {
          sprintf(bias, "[b%g] ", biasValue);
        }
        char def[255];
        def[0] = '\0';
        if(inPorts[i]->edges.size() == 0) {
          sprintf(def, "[d%g] ", defValue);
        }
        std::string name = (std::string)info.map["inputs"][i]["name"];
        inPorts[i]->labels[0]->setText((std::string)bias+def+name);
      }
    }
    //nodeName->setText("[ " + (std::string)info.map["type"] + " ]  " + (std::string)info.map["name"]);
    double x1, x2, y1, y2;
    nodeName->getRectangle(&x1, &x2, &y1, &y2);
    updateSize();
    handlePorts(true);
  }

  void XRockNode::addInputEdge(int index, Edge* edge) {
    //edge->setEndOffset(portOffsets[inPorts[index]->edges.size()%3]);
    inPorts[index]->edges.push_back(edge);

    std::string merge;
    double biasValue, defValue;
    if(getMergeInfo(index, &biasValue, &defValue, &merge)) {
      bool print = true;
      if(merge == "SUM" &&
         fabs(biasValue) < 0.0000001) {
        print = false;
      }
      else if(merge == "PRODUCT" &&
            fabs(1-biasValue) < 0.0000001) {
        print = false;
      }
      char bias[255];
      bias[0] = '\0';
      if(print) {
        sprintf(bias, "[b%g] ", biasValue);
      }
      std::string name = (std::string)info.map["inputs"][index]["name"];
      inPorts[index]->labels[0]->setText(bias+name);
    }
    edge->setEndNode(this);
    handlePorts(true);
  }

  void XRockNode::addOutputEdge(int index, Edge* edge) {
    //edge->setStartOffset(portOffsets[outPorts[index]->edges.size()%3]);
    outPorts[index]->edges.push_back(edge);
    edge->setStartNode(this);
    updateEdges();
  }

  void XRockNode::setSelected(bool b) {
    selected = b;

    std::vector<osg::Vec4> *colors;
    if(selected) {
      colors = &colorMap["taskNodeSelected"];
    }
    else {
      colors = &colorMap["taskNode"];
    }
    (*bColors.get())[0] = (*colors)[0];
    (*bColors.get())[1] = (*colors)[1];
    // (*bColors.get())[2] = (*colors)[1];
    // (*bColors.get())[3] = (*colors)[0];
    // for(int i=4; i<8; ++i)
    //   (*bColors.get())[i] = (*colors)[1];
    // for(int i=8; i<13; ++i)
    //   (*bColors.get())[i] = (*colors)[2];
    // bGeom->dirtyDisplayList();
    // bGeom->dirtyBound();
    applyColor();
  }

  void XRockNode::resizeHeight() {
    double h = 0;
    double h2 = 0;
    for(size_t i=0; i<children->getNumChildren(); ++i) {
      double x1, x2, y1, y2;
      osg_graph_viz::Node *node = dynamic_cast<osg_graph_viz::Node*>(children->getChild(i));
      if(node) {
        node->getRectangle(&x1, &x2, &y1, &y2);
        y1 *= childrenScale;
        if(-y1 > h) h = -y1;
      }
    }
    if((std::string)info.map["type"] == "DES") {
      h2 = headerHeight + portSpaceY*(maxPorts);
    }
    else {
      h2 = portSpaceY*(maxPorts-1)+headerHeight+mergeIconSize*6+3;
    }
    if(h2 > h) h = h2;
    height = h;
    RoundBodyNode::resizeHeight(h);
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
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
    }
  }*/

  std::vector<std::pair<int, std::string> > XRockNode::getFoldInfo(std::string tag,
                                                                  int index) {
    std::vector<std::pair<int, std::string> > r;
    std::pair<int, std::string> p;
    p.first = index;
    p.second = (std::string)info.map[tag][index]["name"];
    r.push_back(p);
    return r;
  }

  void XRockNode::derenderText(const bool readable){
    Node::derenderText(readable);
    pos->removeChild((osg::Node*)nodeType->getOSGNode());
    if(readable){
      pos->addChild((osg::Node*)nodeType->getOSGNode());
    }
  }

  configmaps::ConfigMap XRockNode::getOutPortEdgeInfo(int index) {
    ConfigMap map;
    assert(index >= 0 && index < info.map["outputs"].size());
    map["dataType"] = info.map["outputs"][index]["type"];
    // todo: hanel source node on name change
    map["sourceNode"] = info.map["name"];
    if(info.map["outputs"][index].hasKey("domain")) {
      map["domain"] = info.map["outputs"][index]["domain"];
    }
    return map;
  }

  // todo: move this to bagel model
  bool XRockNode::isCompatible(ConfigMap &map, size_t index) {
    assert(index >= 0 && index < info.map["inputs"].size());
    std::string sourceNode = map["sourceNode"];
    //if((std::string)info.map["name"] == sourceNode) return false;
    // todo: check domain
    osg::ref_ptr<Node> source = view->getNodeByName(sourceNode);
    ConfigMap sourceMap = source->getMap();
    if(sourceMap.hasKey("domain")) {
      std::string sourceDomain = sourceMap["domain"];
      if(sourceDomain == "assembly" && map.hasKey("domain")) {
        sourceDomain << map["domain"];
      }
      if(!info.map.hasKey("domain")) return false;
      std::string targetDomain = info.map["domain"];
      if(targetDomain == "assembly") {
        if(info.map["inputs"][index].hasKey("domain")) {
          targetDomain << info.map["inputs"][index]["domain"];
        }
      }
      //fprintf(stderr, "domains: %s %s\n", sourceDomain.c_str(), targetDomain.c_str());
      if(sourceDomain != targetDomain) return false;
      return (rockStripType(map["dataType"]) == rockStripType(info.map["inputs"][index]["type"]));
    }
    return false;
  }

  void XRockNode::markInputsByEdgeInfo(ConfigMap &map) {
    std::vector<Port*>::iterator it = inPorts.begin();
    size_t i=0;
    for(; it!=inPorts.end(); ++it, ++i) {
      if(isCompatible(map, i)) {
        osg::Vec4Array *colors;
        osg::Geometry *geom;
        geom = (*it)->group->getChild(0)->asGeode()->getDrawable(0)->asGeometry();
        colors = dynamic_cast< osg::Vec4Array *>(geom->getColorArray());
        colors->operator[](1) = osg::Vec4(0.75, 1, 0.75, 1);
        //fprintf(stderr, "change color in %s\n", info.map["name"].c_str());
        geom->dirtyDisplayList();
        geom->dirtyBound();
      }
    }
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
  }

  void XRockNode::unmarkInputs() {
    std::vector<Port*>::iterator it = inPorts.begin();
    for(int i=0; it!=inPorts.end(); ++it, ++i) {
      osg::Vec4Array *colors;
      osg::Geometry *geom;
      geom = (*it)->group->getChild(0)->asGeode()->getDrawable(0)->asGeometry();
      colors = dynamic_cast< osg::Vec4Array *>(geom->getColorArray());
      int interface = 0;
      if(info.map["inputs"][i].hasKey("interface")) {
        interface = info.map["inputs"][i]["interface"];
      }
      std::string domain;
      if(info.map["domain"] == "assembly" && info.map["inputs"][i].hasKey("domain")) {
        domain << info.map["inputs"][i]["domain"];
      }
      osg::Vec4 c(1.0, 1.0, 1.0, 1);
      if(!interface && !domain.empty()) {
        if(domain == "mechanics") {
          c = osg::Vec4(0.95, .9, 0.8, 1.0);
        }
        else if(domain == "computation") {
          c = osg::Vec4(1.0, .8, 0.8, 1.0);
        }
        else if(domain == "software") {
          c = osg::Vec4(0.92, 1.0, 0.92, 1.0);
        }
        else if(domain == "electronics") {
          c = osg::Vec4(0.85, .92, 1., 1.0);
        }
        else if(domain == "behavior") {
          c = osg::Vec4(1.0, .8, 0.8, 1.0);
        }
      }
      if(interface == 1) {
        c = osg::Vec4(1.0, 0.9, 0.7, 1);
      }
      else if(interface == 2) {
        c = osg::Vec4(1.0, 1.0, 0.7, 1);
      }
      colors->operator[](1) = c;
      geom->dirtyDisplayList();
      geom->dirtyBound();
    }
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
  }

  void XRockNode::filterUpdate() {
    std::map<std::string, int> &filterMap = view->getFilterMap();
    std::map<std::string, int>::iterator it;
    std::string domain = info.map["domain"];
    if(domain == "assembly") {
      for(size_t i=0; i<inPorts.size(); ++i) {
        Port *p = inPorts[i];
        std::string iDomain;
        if(info.map["inputs"][i].hasKey("domain")) {
          iDomain << info.map["inputs"][i]["domain"];
        }
        it=filterMap.find(iDomain);
        if(it!=filterMap.end()) {
          pos->removeChild(p->geode);
          pos->removeChild(p->group);
          for(size_t n=0; n<p->labels.size(); ++n) {
            pos->removeChild((osg::Node*)p->labels[n]->getOSGNode());
          }
          p->hidden = !it->second;
          if(it->second) {
            pos->addChild(p->geode);
            pos->addChild(p->group);
            for(size_t n=0; n<p->labels.size(); ++n) {
              pos->addChild((osg::Node*)p->labels[n]->getOSGNode());
            }
          }
          handlePortEdgeVisibility(inPorts[i], !it->second);
        }
      }
      for(size_t i=0; i<outPorts.size(); ++i) {
        Port *p = outPorts[i];
        std::string iDomain;
        if(info.map["outputs"][i].hasKey("domain")) {
          iDomain << info.map["outputs"][i]["domain"];
        }
        it=filterMap.find(iDomain);
        if(it!=filterMap.end()) {
          pos->removeChild(p->geode);
          pos->removeChild(p->group);
          for(size_t n=0; n<p->labels.size(); ++n) {
            pos->removeChild((osg::Node*)p->labels[n]->getOSGNode());
          }
          p->hidden = !it->second;
          if(it->second) {
            pos->addChild(p->geode);
            pos->addChild(p->group);
            for(size_t n=0; n<p->labels.size(); ++n) {
              pos->addChild((osg::Node*)p->labels[n]->getOSGNode());
            }
          }
          handlePortEdgeVisibility(outPorts[i], !it->second);
        }
      }
    }
    else {
      it=filterMap.find(domain);
      if(it!=filterMap.end()) {
        if(it->second && hidden) {
          view->addNodeToView(this);
          hidden = false;
          handleFilterEdges(hidden);
        }
        else if(!it->second && !hidden)  {
          view->removeNodeFromView(this);
          // loop over edges
          hidden = true;
          handleFilterEdges(hidden);
        }
      }
    }
    handlePorts(true);
    updateEdges();
  }

  void XRockNode::handlePortEdgeVisibility(Port *p, bool hide) {
    for(auto it: p->edges) {
      it->hide(hide);
    }
  }

  void XRockNode::handleFilterEdges(bool hide) {
    std::list<osg::ref_ptr<Edge> >::iterator it;
    for(size_t i=0; i<inPorts.size(); ++i) {
      handlePortEdgeVisibility(inPorts[i], hide);
    }
    for(size_t i=0; i<outPorts.size(); ++i) {
      handlePortEdgeVisibility(outPorts[i], hide);
    }
  }

  void XRockNode::resizeWidth(double h) {
    double x1, x2, y1, y2;
    nodeType->getRectangle(&x1, &x2, &y1, &y2);
    double w = x2-x1+16;
    if(w > h) {
      width = h = w;
    }
    RoundBodyNode::resizeWidth(h);
    // RoundBodyNode centers the node name, we have to put it back to the left
    double left, right, top, bottom;
    nodeName->getRectangle(&left, &right, &top, &bottom);
    nodeName->setPosition(8, top);
    bGeom->dirtyDisplayList();
    bGeom->dirtyBound();
  }

  void XRockNode::applyFontScale(double s) {
    Node::applyFontScale(s);
    double x, y;
    view->getWindowPixelSize(&x, &y);
    x = headerFontSize*(x/1920)*s;
    if(x<16) x = 16;
    else if(x<32) x=32;
    else if(x<64) x=64;
    else if(x<128) x=128;
    else if(x<256) x=256;
    else x=512;
    nodeType->setFontResolution(x, x);
  }

  void XRockNode::exportSvg(FILE *f, double ol, double ot) {
    std::string name = getName();
    double x, y;
    getPosition(&x, &y);
    x-=ol;
    y-=ot;
    fprintf(f, "  <g id=\"%s\" transform=\"translate(%g,%g)\">\n", name.c_str(), x, -y-.5*sizeOffset);
    double ry = 15;
    fprintf(f, "    <rect style=\"opacity:1;fill:#%s;fill-opacity:1;fill-rule:nonzero;stroke:#%s;stroke-width:2;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1\" id=\"frame2\" width=\"%g\" height=\"%g\" x=\"0\" y=\"0\" ry=\"%g\" />\n", View::getColor((*bColors.get())[0]).c_str(), View::getColor((*bColors.get())[1]).c_str(), width, height+sizeOffset, ry);

    View::exportLabelToSvg(nodeName, f, 0, 0.5*sizeOffset);
    View::exportLabelToSvg(nodeType, f, 0, 0.5*sizeOffset);

    int i=0;
    for(auto frame: frames) {
      fprintf(f, "    <rect style=\"opacity:1;fill:#%s;fill-opacity:1;fill-rule:nonzero;stroke:#%s;stroke-width:0.5;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1\" id=\"port_frame_%d\" width=\"%g\" height=\"%g\" x=\"%g\" y=\"%g\" ry=\"0\" />\n", View::getColor(frame.c).c_str(), View::getColor(frame.bc).c_str(), ++i, frame.w, frame.h, frame.x, 0.5*sizeOffset-frame.y-frame.h);
    }

    exportPortsSVG(f, 0, 0.5*sizeOffset);

    fprintf(f, "  </g>\n");
  }

} // end of namespace: osg_graph_viz
