/**
 * \file View.cpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#include "View.hpp"
#include "Node.hpp"
#include "RoundBodyNode.hpp"
#include "XRockNode.hpp"

#include <osg/Geode>
#include <osg/LineWidth>
#include <cstdio>
#include <osgDB/ReadFile>

#include <mars/osg_material_manager/OsgMaterialManager.h>
#ifdef _WIN32
  #include <io.h>
#endif
#include <unistd.h>
//#include <configmaps/ConfigData.h>

// todo: set width and height externally

using namespace configmaps;

namespace osg_graph_viz {

  unsigned long View::labelID = 0;
  ConfigMap View::bufferMap;

  bool pathExists(const std::string &path) {
#ifdef _WIN32
      return (_access(path.c_str(), 0) == 0);
#else
      return (access(path.c_str(), F_OK) == 0);
#endif
  }

  std::string View::getColor(const osg::Vec4 &c) {
    char s[10];
    unsigned char r, g, b;
    r = c[0]*255;
    g = c[1]*255;
    b = c[2]*255;
    sprintf(s, "%x%x%x", r, g, b);
    return std::string(s);
  }

  std::string View::getColor(const osg_text::Color &c) {
    char s[10];
    unsigned char r, g, b;
    r = c.r*255;
    g = c.g*255;
    b = c.b*255;
    sprintf(s, "%x%x%x", r, g, b);
    return std::string(s);
  }

  View::View() : numXTicks(2), numYTicks(10) {
    currentTab = NULL;
    roundNodes = true;
    for(int i=0; i<4; ++i) {
      scrollScale[i] = 30.0;
    }
    retinaScale = 1.0;
    inScale = false;
    resourcesPath = OSG_GRAPH_VIZ_DEFAULT_RESOURCES_PATH;
    resourcesPath += "/";
  }

  View::~View(void) {
    delete materialManager;
  }

  void View::init(double hFS, double pFS, double ps, bool classicLook) {
    roundNodes = !classicLook;
    materialManager = new osg_material_manager::OsgMaterialManager(NULL);
    headerFontSize = hFS;
    portFontSize = pFS;
    mergeIconSize = portFontSize;
    portScale = ps;
    windowWidth = 1920;
    windowHeight = 1080;
    ui = NULL;
    lineMode = DIRECT_LINE_MODE;
    pressed = selecting = false;
    modKey = false;
    dontDeselectOnRelease = false;

    scene = new osg::Group();
    content = new osg::Group();
    mainPos = new osg::PositionAttitudeTransform();
    mainScale = new osg::MatrixTransform();
    cameraScale = new osg::MatrixTransform();

    osg::ref_ptr<osg::LineWidth> linew;
    linew = new osg::LineWidth(2.0);

    posX = posY = 0;
    scale = 1.0;
    scaleRatio = 1.0;
    mainScale->setMatrix(osg::Matrix::scale(scale, scale*scaleRatio, 1.));
    cameraScale->setMatrix(osg::Matrix::scale(1., 1., 1.));
    //mainScale->addChild(mainPos.get());
    //this->addChild(mainScale.get());
    materialManager->getMainStateGroup()->addChild(content.get());
    mainScale->addChild(materialManager->getMainStateGroup());
    mainPos->addChild(mainScale.get());
    cameraScale->addChild(mainPos.get());
    scene->addChild(cameraScale.get());


    osg_text::Color c(0., 0., 0., 1.0);
    infoText = new osg_text::Text("mouse [] ...", 15, c, 10, 1055,
                                  osg_text::ALIGN_LEFT, 0, 0, 0, 0,
                                  osg_text::Color(), osg_text::Color(),
                                  0, resourcesPath+"fonts/Stilu-Light.ttf");

    cameraScale->addChild((osg::Node*)infoText->getOSGNode());

    mouseMask = 0;
    nodeToMove = 0;
    renderBin = 30;
    addEdge = false;


    selectionGeode = new osg::Geode;
    selectionGeom = new osg::Geometry;

    selectionVertices = new osg::Vec3Array();
    selectionVertices->push_back(osg::Vec3(0, 0, 0.0));
    selectionVertices->push_back(osg::Vec3(0, 200, 0.0));
    selectionVertices->push_back(osg::Vec3(200, 200, 0.0));
    selectionVertices->push_back(osg::Vec3(200, 0, 0.0));
    selectionVertices->push_back(osg::Vec3(0, 0, 0.0));
    selectionGeom->setDataVariance(osg::Object::DYNAMIC);
    selectionGeom->setVertexArray(selectionVertices);
    selectionGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, 5));

    osg::Vec4Array *selectionColors = new osg::Vec4Array;
    selectionColors->push_back(osg::Vec4(1.0, 0.0, 1.0, 1.0));
    selectionGeom->setColorArray(selectionColors);
    selectionGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec3Array* selectionNormals = new osg::Vec3Array;
    selectionNormals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
    selectionGeom->setNormalArray(selectionNormals);
    selectionGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);
    selectionGeom->getOrCreateStateSet()->setRenderBinDetails(10, "RenderBin");

    osg::LineWidth *selectionLineW = new osg::LineWidth(2);
    selectionGeode->getOrCreateStateSet()->setAttributeAndModes(selectionLineW,
                                                                osg::StateAttribute::ON);
    selectionGeode->addDrawable(selectionGeom);
    //mainScale->addChild(selectionGeode.get());
  }

  Node* View::createNode(const NodeInfo &info) {
    Node *bgNode;
    ConfigMap map = info.map;
    if(map.hasKey("NodeClass")) {
      if((std::string)map["NodeClass"] == "xrock") {
        bgNode = new XRockNode(info, this);
      }
      else {
        if(roundNodes) {
          bgNode = new RoundBodyNode(info, this);
        }
        else {
          bgNode = new Node(info, this);
        }
      }
    }
    else {
      if(roundNodes) {
        bgNode = new RoundBodyNode(info, this);
      }
      else {
        bgNode = new Node(info, this);
      }
    }
    nodeList.push_front(bgNode);
    if(map.hasKey("parentName")) {
      osg::ref_ptr<Node> parent = getNodeByName((std::string)map["parentName"]);
      if(parent) {
        parent->addChildNode(bgNode);
        bgNode->setParentNode(parent);
        parent->setRenderOrder(++renderBin);
      }
      else {
        content->addChild(bgNode);
        bgNode->setRenderOrder(++renderBin);
      }
    }
    else {
      content->addChild(bgNode);
      bgNode->setRenderOrder(++renderBin);
    }
    return bgNode;
  }

  void View::removeNodeFromView(osg::ref_ptr<osg::Node> node) {
    content->removeChild(node.get());
  }

  void View::addNodeToView(osg::ref_ptr<osg::Node> node) {
    content->addChild(node.get());
  }

  osg::ref_ptr<osg_graph_viz::Node> View::getNodeByName(const std::string &name) {
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin();
    for(; it!=nodeList.end(); ++it) {
      if((*it)->getName() == name) {
        return *it;
      }
    }
    return osg::ref_ptr<Node>();
  }

  osg_graph_viz::Edge* View::createEdge(const ConfigMap &info,
                                        int idx1, int idx2) {
    Edge *bgEdge = new Edge(info, this, mergeIconSize);
    bgEdge->fromIdx = idx1;
    bgEdge->toIdx = idx2;
    edgeList.push_front(bgEdge);
    content->addChild(bgEdge);
    bgEdge->getOrCreateStateSet()->setRenderBinDetails(renderBin,
                                                       "RenderBin");
    return bgEdge;
  }

  osg::Texture2D* View::loadTexture(const std::string &file) {
    if(texMap.find(file) == texMap.end()) {
      osg::Texture2D *texture = new osg::Texture2D();
      texture->setDataVariance(osg::Object::DYNAMIC);
      texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
      texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
      osg::Image* textureImage = osgDB::readImageFile(resourcesPath + file);
      texture->setImage(textureImage);
      texMap[file] = texture;
    }
    return texMap[file];
  }

  void View::update(void) {
    // for(int i=0; i<4; ++i) {
    //   if(scrollScale[i] > 1.0) scrollScale[i] -= 1;
    // }
  }

  void View::scaleView(double newScale, double x, double y) {
    double lastMX = (mouseX*1920 - posX) / scale;
    double lastMY = (mouseY*1080 - posY) / (scale*scaleRatio);
    scale = newScale;
    inScale = true;
    if(scale < 0.9){ //if the scaling is to low, functions to derender all texts (Ports, NodeNames, and weights of edges)

      std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
      std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it2;
      for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
        (*it)->derenderText(false);
      }
      for(it2=edgeList.begin(); it2!=edgeList.end(); ++it2) {
        (*it2)->derenderText(false);
      }
    }
    if(scale > 0.9){ //if the scaling gets bigger every thing is reverted, and shown again.
      std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
      std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it2;
      for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
        (*it)->derenderText(true);
      }
      for(it2=edgeList.begin(); it2!=edgeList.end(); ++it2) {
        (*it2)->derenderText(true);
      }
    }

    if(scale < 0.1) {
      scale = 0.1;
    }
    mainScale->setMatrix(osg::Matrix::scale(scale, scale*scaleRatio, 1.));
    double newX = (x*1920 - posX) / scale;
    double newY = (y*1080 - posY) / (scale*scaleRatio);
    posX += (newX-lastMX)*scale;
    posY += (newY-lastMY)*(scale*scaleRatio);
    mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
    for(std::list<osg::ref_ptr<Edge> >::iterator it=edgeList.begin();
        it!=edgeList.end(); ++it) {
      (*it)->setLineWidth(scale*0.5);
    }
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it=nodeList.begin();
        it!=nodeList.end(); ++it) {
      (*it)->setLineWidth(scale*0.5);
    }
    if(addEdge) {
      newEdge->setLineWidth(scale*0.5);
    }
  }

  void View::setViewPos(double x, double y) {
    //fprintf(stderr, "%g %g %g\n", windowWidth, windowHeight, scale);
    posX = x+1920*0.5;
    posY = y+1080*0.5;
    mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
  }

  void View::getViewScale(double *s) {
    *s = scale;
  }

  void View::getViewPos(double *x, double *y) {
    osg::Vec3 p = mainPos->getPosition();
    *x = p.x() - 1920*0.5;
    *y = p.y() - 1080*0.5;
  }

  void View::mouseMove(double x, double y, int scaleX, int scaleY) {
    char da[255];
    double scale_y = 1080. / scaleY;

    double lastMX = (mouseX*1920 - posX) / scale;
    double lastMY = (mouseY*1080 - posY) / (scale*scaleRatio);
    double cPosX = (x*1920 - posX) / scale;
    double cPosY = (y*1080 - posY) / (scale*scaleRatio);
    mouseMoved = true;

    sprintf(da, "mouse [%g, %g] [%g, %g] --- moved %d", x, y, cPosX, cPosY, mouseMask);

    infoText->setText(da);
    // handling of middle mouse button for scaling
    if(mouseMask & 1<<1) {
      scaleView(scale + scale_y*scaleY*0.05*(y-mouseY), x, y);
    }
    // handling of right mouse button for moving window
    if(mouseMask & 1<<2) {
      posX += (cPosX-lastMX)*scale;
      posY += (cPosY-lastMY)*(scale*scaleRatio);
      //posX += scale_x*scaleX*(x-mouseX);
      //posY += scale_y*scaleY*(y-mouseY);
      mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
    }

    // handling of left mouse button for interaction
    if(mouseMask & 1) {
      dontDeselectOnRelease = true;
      // move nodes only if we don't create a selection rectangle
      if(!pressed && !addEdge && (nodeToMove || selectedEdge.valid())) {
        bool checkHeader = false;
        std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
        for(it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
          //double oldX, oldY, newX, newY;
          //(*it)->getPosition(&oldX, &oldY);
          (*it)->setPosition2(cPosX, cPosY);
          //(*it)->getPosition(&newX, &newY);
          checkHeader = true;
        }
        /*
          std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator jt;
          for(jt = selectedEdges.begin(); jt != selectedEdges.end(); ++jt) {
          (*jt)->setPosition2(cPosX, cPosY);
          }
        */
        // this is when a node is just selected by pressing and then moved before releasing
        if(nodeToMove) {
          //double oldX, oldY, newX, newY;
          //nodeToMove->getPosition(&oldX, &oldY);
          nodeToMove->setPosition2(cPosX, cPosY);
          checkHeader = true;
          //nodeToMove->getPosition(&newX, &newY);
          ui->updateNode(nodeToMove.get());
        }
        else if(selectedEdge.valid()) {
          selectedEdge->mouseMove(cPosX, cPosY);
          ui->updateEdge(selectedEdge.get());
        }
        if(checkHeader) {
          if(addToGroupNode.valid()) {
            if(!addToGroupNode->checkMouseHeaderPress(cPosX, cPosY)) {
              addToGroupNode->setSelected(false);
              addToGroupNode = NULL;
            }
          }
          if(!addToGroupNode.valid()) {
            for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
              if(!(*it)->isSelected() && (*it)->checkMouseHeaderPress(cPosX, cPosY)) {
                addToGroupNode = *it;
                addToGroupNode->setSelected(true);
                break;
              }
            }
          }
        }
      }
    }

    //if(mouseMask & 1 && addEdge) {
    if(addEdge) {
      newEdge->updateEndPos(osg::Vec3(cPosX, cPosY, 0.0));
    }

    mouseX = x;
    mouseY = y;

    //Part of  the multiSelection. Used to save the current x and y coordinates for every mouseMove-event.
    if(pressed) {
      xEndSelection = cPosX;
      yEndSelection = cPosY;
      if(!selecting) {
        //fprintf(stderr, "add selection rect\n");
        content->addChild(selectionGeode.get());
        selecting = true;
      }
      makeSelRect(xStartSelection,yStartSelection,xEndSelection,yEndSelection);
    }
  }//mouseMove

  void View::mousePress(double x, double y, int button) {
    char da[255];
    double vX, vY;
    double cPosX = (x*1920 - posX) / scale;
    double cPosY = (y*1080 - posY) / (scale*scaleRatio);
    mouseMoved = false;
    sprintf(da, "mouse [%g, %g] --- pressed %d", x, y, button);
    infoText->setText(da);
    mouseMask = button;
    mouseX = x;
    mouseY = y;

    //Also part of the multiSelection. Checks if button 1(left-mouse) and modKey(shift) were pressed and sets the first
    // values, where the button was pressed.
    //std::cout were previously used to debug

    if(button & 1 && modKey){

    }

    // mouse press for selection
    if(button == 1) {
      osg_graph_viz::Node *oldNode = selectedNode.get();
      osg_graph_viz::Edge *oldEdge = selectedEdge.get();
      std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it, jt;
      // handleEdgeInfo
      if(addEdge) {
        int toIdx;
        bool found = false;
        addEdge = false;
        for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
          if((*it)->checkMouseInPortPress(cPosX, cPosY,
                                          &vX, &vY, &toIdx)) {
            //fprintf(stderr, "mousePress %g/%g\n", vX, vY);
            ConfigMap info = newEdgeFromNode->getOutPortEdgeInfo(newEdgeFromIdx);
            if((*it)->isCompatible(info, toIdx)) {
              newEdge->updateEndPos(osg::Vec3(vX, vY, 0.0));
              handleNewEdge(*it, toIdx);
              found = true;
            }
            break;
          }
        }
        for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
          (*it)->unmarkInputs();
        }
        if(!found) {
          content->removeChild(newEdge.get());
          newEdge = NULL;
        }
      }
      else {

        { // always store the last selected edge or node in the list
          if(selectedEdge.valid()) {
            bool found = false;
            std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator jt;
            for(jt=selectedEdges.begin(); jt!=selectedEdges.end(); ++jt) {
              if(*jt == selectedEdge) {
                found = true;
                break;
              }
            }
            if(!found) {
              selectedEdges.push_back(selectedEdge);
            }
            selectedEdge = NULL;
          }
          if(selectedNode.valid()) {
            bool found = false;
            std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator jt;
            for(jt=selectedNodes.begin(); jt!=selectedNodes.end(); ++jt) {
              if(*jt == selectedNode) {
                found = true;
                break;
              }
            }
            if(!found) {
              selectedNodes.push_back(selectedNode);
            }
            selectedNode = NULL;
          }
        }

        if(modKey) {
          { // check if we click on an edge
            std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator jt;
            for(jt=edgeList.begin(); jt!=edgeList.end(); ++jt) {
              if((*jt)->checkMousePress(cPosX, cPosY)) {
                selectedEdge = (*jt);
                if(selectedEdge.get() != oldEdge && ui) {
                  ui->edgeSelected(*jt);
                }
                if(!selectedEdge->isSelected()) {
                  dontDeselectOnRelease = true;
                }
                selectedEdge->setSelected(true);
                selectedEdge->getOrCreateStateSet()->setRenderBinDetails(++renderBin,
                                                                         "RenderBin");
                break;
              }
            }
          }
          if(!selectedEdge.valid()) {
            // check if we click on a node
            for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
              if((*it)->checkMousePress(cPosX, cPosY)) {
                selectedNode = *it;
                if(!selectedNode->isSelected()) {
                  dontDeselectOnRelease = true;
                }
                selectedNode->setSelected(true);
                if(selectedNode.get() != oldNode && ui) {
                  ui->nodeSelected(*it);
                }
                (*it)->savePosOffset(cPosX, cPosY);
                nodeToMove = *it;
                nodeList.erase(it);
                nodeList.push_front(selectedNode.get());
                selectedNode.get()->setRenderOrder(++renderBin);
                break;
              }
            }
          }
          // only open selection rectange if we didn't click on an edge or node
          if(!selectedEdge.valid() && !selectedNode.valid()) {
            sprintf(da, "mouse [%g, %g] --- pressed %d at [x = %g | y = %g]",
                    x, y, button, x, y);
            infoText->setText(da);
            pressed = true;
            xStartSelection = cPosX;
            yStartSelection = cPosY;
            // to prevent misdetection of selection, in case someone just
            // clicks on the spot
            xEndSelection = cPosX;
            yEndSelection = cPosY;
          }
        }
        else {
          bool clearSelection_ = true;

          {
            std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
            for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
              if((*it)->checkMousePress(cPosX, cPosY)) {
                selectedEdge = (*it);
                if(selectedEdge.get() != oldEdge && ui) {
                  ui->edgeSelected(*it);
                }
                if(selectedEdge->isSelected()) {
                  clearSelection_ = false;
                }
                selectedEdge->setSelected(true);
                selectedEdge->getOrCreateStateSet()->setRenderBinDetails(++renderBin,
                                                                         "RenderBin");
                break;
              }
            }
          }

          if(!selectedEdge.valid()) {
            for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
              if((*it)->checkMousePress(cPosX, cPosY) ||
                 (*it)->checkMouseOutPortPress(cPosX, cPosY, &vX, &vY, &newEdgeFromIdx)) {
                selectedNode = *it;
                if(selectedNode->isSelected()) {
                  clearSelection_ = false;
                }
                selectedNode->setSelected(true);
                if(selectedNode.get() != oldNode && ui) {
                  ui->nodeSelected(*it);
                }
                if((*it)->checkMouseOutPortPress(cPosX, cPosY,
                                                 &vX, &vY, &newEdgeFromIdx)) {
                  newEdgeFromNode = *it;
                  osg::Vec3 outV = newEdgeFromNode->getOutPortPos(newEdgeFromIdx);
                  ConfigMap info;
                  info = newEdgeFromNode->getOutPortEdgeInfo(newEdgeFromIdx);
                  osg::Vec3 inV = osg::Vec3(cPosX, cPosY, 0.0);
                  info["vertices"][0]["x"] = outV.x();
                  info["vertices"][0]["y"] = outV.y();
                  info["vertices"][0]["z"] = outV.z();

                  if(lineMode == ORTHO_LINE_MODE) {
                    info["vertices"][1]["x"] = outV.x()+(inV.x()-outV.x())*.333;
                    info["vertices"][1]["y"] = outV.y();
                    info["vertices"][1]["z"] = outV.z();

                    info["vertices"][2]["x"] = outV.x()+(inV.x()-outV.x())*.333;
                    info["vertices"][2]["y"] = outV.y()+(inV.y()-outV.y())*.5;
                    info["vertices"][2]["z"] = outV.z();

                    info["vertices"][3]["x"] = outV.x()+(inV.x()-outV.x())*.666;
                    info["vertices"][3]["y"] = outV.y()+(inV.y()-outV.y())*.5;
                    info["vertices"][3]["z"] = outV.z();

                    info["vertices"][4]["x"] = outV.x()+(inV.x()-outV.x())*.666;
                    info["vertices"][4]["y"] = inV.y();
                    info["vertices"][4]["z"] = inV.z();

                    info["vertices"][5]["x"] = inV.x();
                    info["vertices"][5]["y"] = inV.y();
                    info["vertices"][5]["z"] = inV.z();
                  }
                  else {
                    info["vertices"][1]["x"] = inV.x();
                    info["vertices"][1]["y"] = inV.y();
                    info["vertices"][1]["z"] = inV.z();
                    if(lineMode == SMOOTH_LINE_MODE) {
                      info["smooth"] = true;
                    }
                  }
                  info["weight"] = 1.0;
                  newEdge = new osg_graph_viz::Edge(info, this, mergeIconSize);
                  content->addChild(newEdge);
                  newEdge->getOrCreateStateSet()->setRenderBinDetails(renderBin,
                                                                      "RenderBin");
                  addEdge = true;
                  nodeToMove = 0;
                  newEdge->setLineWidth(scale*0.5);
                  for(jt=nodeList.begin(); jt!=nodeList.end(); ++jt) {
                    (*jt)->markInputsByEdgeInfo(info);
                  }
                }
                else {
                  //if((*it)->checkMouseHeaderPress(cPosX, cPosY)) {
                  (*it)->savePosOffset(cPosX, cPosY);
                  nodeToMove = *it;
                }
                nodeList.erase(it);
                nodeList.push_front(selectedNode.get());
                selectedNode.get()->setRenderOrder(++renderBin);
                break;
              }
            }
          }
          if(clearSelection_) {
            clearSelection(false);
          }
          if(!selectedEdge.valid() && !selectedNode.valid()) {
            ui->nothingSelected();
          }
        }
        // save current click position for all selected edges and nodes
        {
          for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator jt=selectedNodes.begin(); jt!=selectedNodes.end(); ++jt) {
            (*jt)->savePosOffset(cPosX, cPosY);
          }
          if(selectedNode.valid()) {
            selectedNode->savePosOffset(cPosX, cPosY);
          }
        }
        {
          for(std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator jt=selectedEdges.begin(); jt!=selectedEdges.end(); ++jt) {
            (*jt)->savePosOffset(cPosX, cPosY);
          }
          if(selectedEdge.valid()) {
            selectedEdge->savePosOffset(cPosX, cPosY);
          }
        }
      }
    }
  }

  void View::mouseRelease(double x, double y, int button) {
    char da[255];
    double cPosX = (x*1920 - posX) / scale;
    double cPosY = (y*1080 - posY) / (scale*scaleRatio);

    if(inScale) {
      inScale = false;
      for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it=nodeList.begin();
          it!=nodeList.end(); ++it) {
        (*it)->applyFontScale(scale);
      }
    }

    sprintf(da, "mouse [%g, %g] [%g, %g] --- moved", x, y, cPosX, cPosY);
    infoText->setText(da);
    if(mouseMask & 1<<2 && !mouseMoved) {
      osg_graph_viz::Edge *e = NULL;
      osg_graph_viz::Node *n = NULL;
      int inPort = -1, outPort = -1;
      {
        std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
        for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
          if((*it)->checkMousePress(cPosX, cPosY)) {
            e = (*it);
            break;
          }
        }
      }
      if(!e) {
        double vX, vY;
        std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
        for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
          int idx;
          if((*it)->checkMouseInPortPress(cPosX, cPosY,
                                          &vX, &vY, &idx)) {
            n = *it;
            inPort = idx;
            break;
          }
          if((*it)->checkMouseOutPortPress(cPosX, cPosY,
                                           &vX, &vY, &idx)) {
            n = *it;
            outPort = idx;
            break;
          }
          if((*it)->checkMousePress(cPosX, cPosY)) {
            n = *it;
            break;
          }
        }
      }
      ui->rightClick(n, inPort, outPort, e, x, y);
    }
    mouseX = x;
    mouseY = y;
    nodeToMove = 0;

    int toIdx;
    double vX, vY;
    if(addEdge && !(button & 1) && (mouseMask & 1)) {
      std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
      for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
        if((*it)->checkMouseInPortPress(cPosX, cPosY,
                                        &vX, &vY, &toIdx)) {
          ConfigMap info = newEdgeFromNode->getOutPortEdgeInfo(newEdgeFromIdx);
          if((*it)->isCompatible(info, toIdx)) {
            //fprintf(stderr, "mouseRelease %g/%g\n", vX, vY);
            newEdge->updateEndPos(osg::Vec3(vX, vY, 0.0));
            handleNewEdge(*it, toIdx);
          }
          else {
            content->removeChild(newEdge.get());
          }
          addEdge = false;
          newEdge = NULL;
          for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
            (*it)->unmarkInputs();
          }
          break;
        }
      }
    }
    else if(modKey) {
      if(selectedEdge.valid() && !dontDeselectOnRelease) {
        selectedEdge->setSelected(false);
        selectedEdge = NULL;
      }
      else if(selectedNode.valid() && !dontDeselectOnRelease) {
        std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
        for(it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
          if(it->get() == selectedNode.get()) {
            selectedNodes.erase(it);
            break;
          }
        }
        selectedNode->setSelected(false);
        selectedNode = NULL;
      }
    }
    mouseMask = button;
    dontDeselectOnRelease = false;

    //Part of the multiSelection, uses the last updated(from mouseMove) x and y values as end of selection.
    //std::cout were previously used to debug
    if(selecting) {
      selecting =false;
      //fprintf(stderr, "remove selection rect\n");
      content->removeChild(selectionGeode.get());
    }
    if(pressed && ((xStartSelection != xEndSelection) && (yStartSelection != yEndSelection))){
      pressed =false;
      //std::cout << "[SELECTION] released at x: "<< xEndSelection << " | y: " << yEndSelection << std::endl;
      makeSelcetionInRect();
    }
    if(addToGroupNode.valid()) {
      ConfigMap parentMap = addToGroupNode->getMap();
      std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
      for(it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        ConfigMap map = (*it)->getMap();
        map["parentName"] = addToGroupNode->getName();
        map["order"] = (unsigned long)parentMap["order"] +1;
        (*it)->updateMap(map);
      }
      if(selectedNode.valid()) {
        ConfigMap map = selectedNode->getMap();
        map["parentName"] = addToGroupNode->getName();
        map["order"] = (unsigned long)parentMap["order"] +1;
        selectedNode->updateMap(map);
      }
      addToGroupNode->setSelected(false);
      addToGroupNode = NULL;
    }

  }//mouseRelease

  void View::handleNewEdge(osg::ref_ptr<osg_graph_viz::Node> toNode, int toIdx) {
    // handle fold state here?
    addEdge = false;
    std::vector<std::pair<int, std::string> > fromFoldInfo;
    std::vector<std::pair<int, std::string> > toFoldInfo;

    fromFoldInfo = newEdgeFromNode->getOutFoldInfo(newEdgeFromIdx);
    toFoldInfo = toNode->getInFoldInfo(toIdx);
    newEdge->fromIdx = newEdgeFromIdx;
    newEdge->toIdx = toIdx;
    if(fromFoldInfo.size() <= 1 && toFoldInfo.size() <= 1) {
      edgeList.push_front(newEdge.get());
      newEdgeFromNode->addOutputEdge(newEdgeFromIdx, newEdge.get());
      toNode->addInputEdge(toIdx, newEdge.get());
      ui->newEdge(newEdge.get(), newEdgeFromNode, newEdgeFromIdx,
                  toNode, toIdx);
      newEdge = NULL;
      return;
    }
    std::vector<std::pair<int, std::string> >::iterator it, it2;
    size_t p1, p2;
    std::string s1, s2;
    for(it = fromFoldInfo.begin(); it!=fromFoldInfo.end(); ++it) {
      p1 = it->second.rfind("/");
      if( p1 != std::string::npos) {
        s1 = it->second.substr(p1+1);
        for(it2 = toFoldInfo.begin(); it2!=toFoldInfo.end(); ++it2) {
          p2 = it2->second.rfind("/");
          if( p2 != std::string::npos) {
            s2 = it2->second.substr(p2+1);
            if(s1 == s2) {
              osg_graph_viz::Edge *edge = new osg_graph_viz::Edge(newEdge->getMap(), this, mergeIconSize);
              edgeList.push_front(edge);
              content->addChild(edge);
              edge->getOrCreateStateSet()->setRenderBinDetails(renderBin,
                                                               "RenderBin");
              edge->setLineWidth(scale*0.5);

              newEdgeFromNode->addOutputEdge(it->first, edge);
              toNode->addInputEdge(it2->first, edge);
              ui->newEdge(edge, newEdgeFromNode, it->first,
                          toNode, it2->first);
            }
          }
        }
      }
    }
    content->removeChild(newEdge.get());
    newEdge = NULL;
  }

  void View::getPosition(double *x, double *y){
    *x = (0.5*1920 - posX) / scale;;
    *y = (0.5*1080 - posY) / (scale*scaleRatio);
  }

  void View::setModKey(bool v) {
    modKey = v;
  }

  void View::deleteKey() {
    for(std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it = edgeList.begin(); it != edgeList.end();){
      if((*it)->isSelected()){
        (*it)->setSelected(false);
        it = removeEdge(*it);
        selectedEdge = NULL;
      }else{
        ++it;
      }
    }
    selectedEdges.clear();


    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin(); it != nodeList.end();){
      if((*it)->isSelected()){
        (*it)->setSelected(false);
        it = removeNode(*it);
        selectedNode = NULL;
      }else{
        ++it;
      }
    }
    selectedNodes.clear();
  }

  std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator View::removeEdge(osg_graph_viz::Edge *edge) {
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
    for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
      if(it->get() == edge) {
        break;
      }
    }
    if(ui->removeEdge(edge)) {
      it = edgeList.erase(it);
      edge->removeFromNodes();
      content->removeChild(edge);
      return it;
    }
    return ++it;
  }

  std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator View::removeNode(osg_graph_viz::Node *node) {
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
      if(it->get() == node) {
        break;
      }
    }
    if(ui->removeNode(node)) {
      it = nodeList.erase(it);
      node->removeEdges();
      content->removeChild(node);
      return it;
    }
    return ++it;
  }

  void View::updateMap(const ConfigMap &map) {
    if(selectedNode.valid()) {
      selectedNode->updateMap(map);
    }
    else if(selectedEdge.valid()) {
      selectedEdge->updateMap(map);
    }
  }

  void View::setScaleRatio(double value) {
    // 1920 / 1080 = 1.7777
    // 1 = 1.77777*x
    // x = 0.5625
    // w / h = s
    scaleRatio = value*0.5625;
    mainScale->setMatrix(osg::Matrix::scale(scale, scale*scaleRatio, 1.));
  }

  void View::decoupleSelected() {
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator jt;
    for(jt=selectedEdges.begin(); jt!=selectedEdges.end(); ++jt) {
      (*jt)->decoupleEdge();
    }
    if(selectedEdge.valid()) {
      selectedEdge->decoupleEdge();
      ui->updateEdge(selectedEdge.get());
    }
  }

  void View::decoupleEdgesOfNodes(std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes) {
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=selectedNodes.begin(); it!=selectedNodes.end(); ++it) {
      (*it)->decoupleEdges();
    }
  }

  void View::repositionNodes() {
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin();
    for(;it!=nodeList.end(); ++it) {
      (*it)->reposition();
    }
  }

  void View::repositionEdges() {
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;

    for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
      (*it)->reposition();
    }
  }

  void View::decoupleLongEdges() {
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;

    for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
      (*it)->decouple(100.);
    }
  }

  //Part of multiSelect, uses the assigned values of x,y at press and release.
  //this Adds the objects found in the selection frame to the selected list.
  //std::cout were previously used to debug

  void View::makeSelcetionInRect(){
    double xNodePos=0, yNodePos=0;
    double xUpper=0, xLower=0, yUpper=0, yLower=0;
    double xStartCalc, xEndCalc, yStartCalc, yEndCalc;
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;

    xStartCalc = xStartSelection;
    yStartCalc = yStartSelection;
    xEndCalc = xEndSelection;
    yEndCalc = yEndSelection;


    /*std::cout <<"[SELECTION] Selected Area: from x | y [" << xStartSelection <<"] | [" << yStartSelection;
      std::cout <<"] to [" << xEndSelection << "] | [" << yEndSelection<<"]"<<std::endl;
      std::cout <<"[SELECTION] Selected relativArea: from x | y [" << xStartCalc <<"] | [" << yStartCalc;
      std::cout <<"] to [" << xEndCalc << "] | [" << yEndCalc<<"]"<<std::endl;*/

    if(xStartCalc > xEndCalc){
      xUpper = xStartCalc;
      xLower = xEndCalc;
    }
    else /*if(xEndCalc > xStartCalc)*/{
      xUpper = xEndCalc;
      xLower = xStartCalc;
    }

    if (yStartCalc > yEndCalc){
      yUpper = yStartCalc;
      yLower = yEndCalc;
    }
    else {
      yUpper = yEndCalc;
      yLower = yStartCalc;
    }

    /*std::cout << "[SELECTION] xLower: " << xLower <<", xUpper: " << xUpper << "| yLower: " << yLower <<", yUpper: "
      <<yUpper<<std::endl;*/

    for(it=nodeList.begin(); it!=nodeList.end(); ++it) {
      (*it)->getPosition(&xNodePos, &yNodePos);
      /*std::cout << "[SELECTION] NodePos x: " << xNodePos << ", NodePos y: " << yNodePos<<std::endl;*/
      if((xNodePos>= xLower && xNodePos<= xUpper) && (yNodePos>= yLower && yNodePos <= yUpper)){
        (*it)->setSelected(true);
        //(*it)->confMapToYml((*it));
        selectedNodes.push_back((*it));
      }
    }
  }


  void View::makeSelRect(const double xStart, const double yStart, const double xEnd, const double yEnd) {

    //osg_graph_viz::Node node= makeSelNode();
    //if (xStart == 0 && yStart ==0 && xEnd ==0 && yEnd ==0){
    //deletSelNode(*node);
    //
    // }
    double x = xStart;
    double y = yStart;
    double width, height;
    width = (xEnd - xStart);
    height = (yEnd - yStart);

    //height =width= 200;

    //fprintf(stderr, "%g %g %g %g\n", x, y, width, height);
    //node.SelRecResize(width, height, node);

    osg::Vec3Array *v = selectionVertices.get();
    v->at(0) = osg::Vec3(x, y, 0);
    v->at(1) = osg::Vec3(x+width, y, 0);
    v->at(2) = osg::Vec3(x+width, y+height, 0);
    v->at(3) = osg::Vec3(x, y+height, 0);
    v->at(4) = osg::Vec3(x, y, 0);
    selectionGeom->dirtyDisplayList();
    selectionGeom->dirtyBound();
  }

  void View::resize(int w, int h) {
    //fprintf(stderr, "%d %d\n", w, h);
    windowWidth = w*retinaScale;
    windowHeight = h*retinaScale;
    setScaleRatio(windowWidth/windowHeight);
    if(camera.valid()) {
      cameraScale->setMatrix(osg::Matrix::scale(windowWidth/1920,
                                                windowHeight/1080, 1.));
      camera->setViewport(0, 0, windowWidth, windowHeight);
      camera->setProjectionMatrix(osg::Matrix::ortho2D(0,windowWidth,
                                                       0,windowHeight));
    }
  }

  bool View::handle(const osgGA::GUIEventAdapter& ea,
                    osgGA::GUIActionAdapter& aa) {
    double w = windowWidth;
    double h = windowHeight;
    //double scrollUpdate = 0.8;
    static osg::Vec3 p1(0,0,0), p2(0,0,0);
    if(ea.isMultiTouchEvent() && ea.getTouchData()->getNumTouchPoints() == 2) {
      osg::Vec3 t1(ea.getTouchData()->get(0).x,
                   ea.getTouchData()->get(0).y, 0);
      osg::Vec3 t2(ea.getTouchData()->get(1).x,
                   ea.getTouchData()->get(1).y, 0);
      osg::Vec3 d1 = t1-p1;
      osg::Vec3 d2 = t2-p1;
      if(d1.length2() < d2.length2()) {
        d2 = t2-p2;
      }
      else {
        d1 = t2-p1;
        d2 = t1-p2;
      }
      osg::Quat q;
      q.makeRotate(d1, d2);
      osg::Vec3 d3;
#ifdef OSG_USE_FLOAT_QUAT
      float angle;
#else
      double angle;
#endif
      q.getRotate(angle, d3);
      if(angle > 3.14) {
        // scaling
        scaleView(scale+d1.length(), retinaScale*0.5/w, retinaScale*0.5/h);
      }
      else {
        // moving
        mouseMove(d1.x(), d1.y(), w, h);
      }
      return true;
    }
    switch(ea.getEventType())
      {
      case osgGA::GUIEventAdapter::RESIZE: {
        resize(ea.getWindowWidth(), ea.getWindowHeight());
        break;
      }
      case osgGA::GUIEventAdapter::MOVE:
      case osgGA::GUIEventAdapter::DRAG: {
        mouseMove(retinaScale*ea.getX()/w, retinaScale*ea.getY()/h, w, h);
        break;
      }
      case osgGA::GUIEventAdapter::SCROLL: {
        switch(ea.getScrollingMotion())
          {
          case osgGA::GUIEventAdapter::SCROLL_UP: {
            if(ea.getModKeyMask() == osgGA::GUIEventAdapter::MODKEY_CTRL) {
              scaleView(scale+0.05, retinaScale*ea.getX()/w, retinaScale*ea.getY()/h);
            }
            else {
              // if(scrollScale[0] < 60)
              //   scrollScale[0] += scrollUpdate;
              posY -= scrollScale[0]*(scaleRatio);
              mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
            }
            break;
          }
          case osgGA::GUIEventAdapter::SCROLL_DOWN: {
            if(ea.getModKeyMask() == osgGA::GUIEventAdapter::MODKEY_CTRL) {
              scaleView(scale-0.05, retinaScale*ea.getX()/w, retinaScale*ea.getY()/h);
            }
            else {
              // if(scrollScale[1] < 60)
              //   scrollScale[1] += scrollUpdate;
              posY += scrollScale[1]*(scaleRatio);
              mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
            }
            break;
          }
          case osgGA::GUIEventAdapter::SCROLL_LEFT: {
            // if(scrollScale[2] < 60)
            //   scrollScale[2] += scrollUpdate;
            posX += scrollScale[2];
            mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
            break;
          }
          case osgGA::GUIEventAdapter::SCROLL_RIGHT: {
            // if(scrollScale[3] < 60)
            //   scrollScale[3] += scrollUpdate;
            posX -= scrollScale[3];
            mainPos->setPosition(osg::Vec3(posX, posY, 0.0));
            break;
          }
          default:
            break;
          }
           break;
      }
      case osgGA::GUIEventAdapter::PUSH: {
        mousePress(retinaScale*ea.getX()/windowWidth, retinaScale*ea.getY()/windowHeight,
                   ea.getButtonMask());
        break;
      }
      case osgGA::GUIEventAdapter::RELEASE: {
        mouseRelease(retinaScale*ea.getX()/windowWidth, retinaScale*ea.getY()/windowHeight,
                     ea.getButtonMask());
        break;
      }
      case osgGA::GUIEventAdapter::KEYUP: {
        if(ea.getKey() == osgGA::GUIEventAdapter::KEY_Delete) {
          deleteKey();
        }
        if(ea.getModKeyMask() == osgGA::GUIEventAdapter::MODKEY_SHIFT) {
          setModKey(false);
        }
        if(ea.getKey() == osgGA::GUIEventAdapter::KEY_Escape) {
          if(addEdge) {
            content->removeChild(newEdge.get());
            newEdge = NULL;
            addEdge = false;
          }
          else {
            // clear selection
            clearSelection(true);
          }
          return true;
        }
        break;
      }
      case osgGA::GUIEventAdapter::KEYDOWN: {
        if(ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT or
           ea.getKey() == osgGA::GUIEventAdapter::KEY_Shift_L or
           ea.getKey() == osgGA::GUIEventAdapter::KEY_Shift_R) {
          setModKey(true);
        }
        if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Escape)
        {
          return true;
        }
        if ((ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) &&
            (ea.getKey() == 'D' - 'A' + 1 || ea.getKey() == 'd' - 'a' + 1))
        {
          fprintf(stderr, "duplicate\n");
          duplicateSelection();
          return true;
        }
        if ((ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) &&
            (ea.getKey() == 'C' - 'A' + 1 || ea.getKey() == 'c' - 'a' + 1))
        {
          fprintf(stderr, "copy\n");
          copySelection();
          return true;
        }
        if ((ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) &&
            (ea.getKey() == 'V' - 'A' + 1 || ea.getKey() == 'v' - 'a' + 1))
        {
          fprintf(stderr, "paste\n");
          pasteSelection();
          return true;
        }
        if ((ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) &&
            (ea.getKey() == 'Z' - 'A' + 1 || ea.getKey() == 'z' - 'a' + 1))
        {
          fprintf(stderr, "undo \n");
          undoPreviousAction();
          return true;
        }
        if ((ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) &&
            (ea.getKey() == 'Y' - 'A' + 1 || ea.getKey() == 'y' - 'a' + 1))
        {
          fprintf(stderr, "redo \n");
          redoPreviousAction();
          return true;
        }
        break;
      }
      default:
        break;
      }
    return false;
  }

  std::list<osg::ref_ptr<osg_graph_viz::Node> > View::getSelectedNodes(){
    std::list<osg::ref_ptr<osg_graph_viz::Node> > returnList = selectedNodes;
    if(selectedNode.valid()) {
      returnList.push_back(selectedNode);
    }
    return returnList;
  }

  void View::createTab(const std::string &name) {
    double startPos = 10;
    double left, right, top, bottom;
    std::map<std::string, Tab*>::iterator it=tabMap.begin();
    for(; it!=tabMap.end(); ++it) {
      it->second->label->getRectangle(&left, &right, &top, &bottom);
      if(right > startPos) startPos = right;
    }
    Tab *t = new Tab();
    size_t p = name.rfind('/');
    std::string n = name;
    if(p != std::string::npos) n = name.substr(p+1);
    t->label = new osg_text::Text(n, 15,
                                  osg_text::Color(0.1, 0.1, 0.1, 1.),
                                  15+startPos, 1075,
                                  osg_text::ALIGN_LEFT, 10, 2, 10, 2,
                                  osg_text::Color(1.0, 1.0, 0.9, 1.),
                                  osg_text::Color(0.1, 0.1, 0.1, 1),
                                  3, resourcesPath+"fonts/Stilu-Light.ttf");
    cameraScale->addChild((osg::Node*)t->label->getOSGNode());
    tabMap[name] = t;
    if(currentTab) {
      currentTab->label->setBackgroundColor(osg_text::Color(0.95, 0.95, 0.95, 1.0));
    }
    currentTab = t;
  }

  void View::saveTab(configmaps::ConfigMap &map) {
    if(currentTab) {
      currentTab->map = map;
    }
  }

  void View::clearSelection(bool whole) {
    bool saveHistory = false;
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator jt = selectedNodes.begin(); jt!=selectedNodes.end(); ++jt) {
      (*jt)->setSelected(false);
      saveHistory = true;
    }
    selectedNodes.clear();
    for(std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator jt=selectedEdges.begin(); jt!=selectedEdges.end(); ++jt) {
      (*jt)->setSelected(false);
      saveHistory = true;
    }
    selectedEdges.clear();
    if(saveHistory) {
      ui->addHistoryEntry();
    }
    if(whole) {
      if(selectedEdge.valid()) {
        selectedEdge->setSelected(false);
        selectedEdge = NULL;
      }
      if(selectedNode.valid()) {
        selectedNode->setSelected(false);
        selectedNode = NULL;
      }
    }
  }

  bool View::groupNodes(const std::string &parent, const std::string &child) {
    return ui->groupNodes(parent, child);
  }

  bool View::updateNode(osg_graph_viz::Node *node) {
    return ui->updateNode(node);
  }

  void View::setFilter(const std::string &filter, int value) {
    filterMap[filter] = value;
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin();
    for(;it!=nodeList.end(); ++it) {
      (*it)->filterUpdate();
    }
  }

  void View::setEdgesSmooth(bool v) {
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
    for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
      (*it)->setSmooth(v);
    }
  }

  void View::duplicateSelection() {
    std::list<osg::ref_ptr<osg_graph_viz::Node> > newSelection;
    std::list<osg::ref_ptr<osg_graph_viz::Node> > oldSelection;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > duplicateEdges;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > outEdges;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> >::iterator outEdgesIt;
    osg_graph_viz::Node *newNode;
    std::map<std::string, std::string> nameMapping;
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin(); it != nodeList.end(); ++it){
      if((*it)->isSelected()){
        ConfigMap map = (*it)->getMap();
        if((newNode = ui->addNode(map))) {
          ConfigMap m2 = newNode->getMap();
          map["id"] = m2["id"];
          map["order"] = m2["order"];
          nameMapping[(*it)->getName()] = newNode->getName();
          newSelection.push_back(newNode);
          oldSelection.push_back(*it);
          map["name"] = newNode->getName();
          newNode->updateMap(map);
          // identify edges to duplicate
          outEdges = (*it)->getOutputEdges();
          for(outEdgesIt=outEdges.begin();
              outEdgesIt!=outEdges.end();
              ++outEdgesIt) {
            if((*outEdgesIt)->getEndNode()->isSelected()) {
              duplicateEdges.push_back(*outEdgesIt);
            }
          }
        }
        selectedNode = NULL;
      }
    }
    for(outEdgesIt=duplicateEdges.begin();
        outEdgesIt!=duplicateEdges.end();
        ++outEdgesIt) {
      ConfigMap map = (*outEdgesIt)->getMap();
      map["fromNode"] = nameMapping[map["fromNode"].getString()];
      map["toNode"] = nameMapping[map["toNode"].getString()];
      ui->addEdge(map);
    }
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = oldSelection.begin(); it != oldSelection.end(); ++it) {
      (*it)->setSelected(false);
    }
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = newSelection.begin(); it != newSelection.end(); ++it) {
      (*it)->setSelected(true);
    }
    newSelection.swap(selectedNodes);
  }

  void View::copySelection() {
    std::list<osg::ref_ptr<osg_graph_viz::Node> > newSelection;
    std::list<osg::ref_ptr<osg_graph_viz::Node> > oldSelection;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > duplicateEdges;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > outEdges;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> >::iterator outEdgesIt;
    //osg_graph_viz::Node *newNode;
    bufferMap.clear();
    std::map<std::string, std::string> nameMapping;
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin(); it != nodeList.end(); ++it){
      if((*it)->isSelected()){
        bufferMap["nodes"].append((*it)->getMap());
        outEdges = (*it)->getOutputEdges();
        for(outEdgesIt=outEdges.begin();
            outEdgesIt!=outEdges.end();
            ++outEdgesIt) {
          if((*outEdgesIt)->getEndNode()->isSelected()) {
            duplicateEdges.push_back(*outEdgesIt);
          }
        }
        selectedNode = NULL;
      }
    }
    for(outEdgesIt=duplicateEdges.begin();
        outEdgesIt!=duplicateEdges.end();
        ++outEdgesIt) {
      bufferMap["edges"].append((*outEdgesIt)->getMap());
    }
  }

  void View::pasteSelection() {
    std::list<osg::ref_ptr<osg_graph_viz::Node> > newSelection;
    std::list<osg::ref_ptr<osg_graph_viz::Node> > oldSelection;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > duplicateEdges;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> > outEdges;
    std::vector<osg::ref_ptr<osg_graph_viz::Edge> >::iterator outEdgesIt;
    osg_graph_viz::Node *newNode;
    std::map<std::string, std::string> nameMapping;

    if(!bufferMap.hasKey("nodes")) {
      return;
    }

    // clear selection
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = nodeList.begin(); it != nodeList.end(); ++it){
      if((*it)->isSelected()) {
          oldSelection.push_back(*it);
        selectedNode = NULL;
      }
    }
    for(auto node: bufferMap["nodes"]) {
      if((newNode = ui->addNode(node))) {
        ConfigMap m2 = newNode->getMap();
        node["id"] = m2["id"];
        node["order"] = m2["order"];
        nameMapping[node["name"]] = newNode->getName();
        newSelection.push_back(newNode);
        node["name"] = newNode->getName();
        newNode->updateMap(node);
      }
    }
    if(bufferMap.hasKey("edges")) {
      for(auto edge: bufferMap["edges"]) {
        edge["fromNode"] = nameMapping[edge["fromNode"].getString()];
        edge["toNode"] = nameMapping[edge["toNode"].getString()];
        ui->addEdge(edge);
      }
    }
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = oldSelection.begin(); it != oldSelection.end(); ++it) {
      (*it)->setSelected(false);
    }
    for(std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it = newSelection.begin(); it != newSelection.end(); ++it) {
      (*it)->setSelected(true);
    }
    newSelection.swap(selectedNodes);
  }
  void View::undoPreviousAction() {
     ui->undo();
  }
  void View::redoPreviousAction() {
     ui->redo();
  }

  void View::getWindowPixelSize(double *x, double *y) {
    *x = windowWidth;
    *y = windowHeight;
  }

  void View::exportSvg(const std::string &filename) {
    FILE *f = fopen(filename.c_str(), "w");
    double l, r, t, b;
    double x1, x2, y1, y2;
    bool first = true;
    labelID = 1;
    for(auto it: nodeList) {
      it->getRectangle(&x1, &x2, &y1, &y2);
      if(first) {
        l = x1;
        r = x2;
        b = y1;
        t = y2;
        first = false;
      }
      else {
        if(x1 < l) l = x1;
        if(x2 > r) r = x2;
        if(y1 < b) b = y1;
        if(y2 > t) t = y2;
      }
    }
    for(auto it: edgeList) {
      it->getRectangle(&x1, &x2, &y1, &y2);
      if(x1 < l) l = x1;
      if(x2 > r) r = x2;
      if(y1 < b) b = y1;
      if(y2 > t) t = y2;
    }
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(f, "<svg\n");
    fprintf(f, "   xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:cc=\"http://creativecommons.org/ns#\"\n");
   fprintf(f, "   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n");
   fprintf(f, "   xmlns:svg=\"http://www.w3.org/2000/svg\"\n");
   fprintf(f, "   xmlns=\"http://www.w3.org/2000/svg\"\n");
   fprintf(f, "   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
   fprintf(f, "   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n");
   fprintf(f, "   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n");
   fprintf(f, "   version=\"1.1\" id=\"graph\" viewBox=\"0 0 %g %g\" height=\"%gmm\" width=\"%gmm\">\n", r-l+4, 4-(b-t), 4-(b-t), 4+r-l);


    for(auto it: nodeList) {
      it->exportSvg(f, l-2, t+2);
    }
    for(auto it: edgeList) {
      it->exportSvg(f, l-2, t+2);
    }

    fprintf(f, "</svg>");

    fclose(f);
  }


  void View::exportLabelToSvg(osg::ref_ptr<osg_text::Text> label,
                                     FILE *f, double ol, double ot) {
    double l, r, t, b, x2, y2, pl, pt, pr, pb;
    double fz = label->getFontsize()*0.95;
    label->getRectangle(&l, &r, &t, &b);
    label->getPosition(&x2, &y2);
    label->getPadding(&pl, &pt, &pr, &pb);
    std::string font = label->getFont();
    std::string weight = "normal";
    if(font.find("bold") != std::string::npos) {
      weight = "600";
    }
    osg_text::Color backC = label->getBackgroundColor();
    osg_text::Color borderC = label->getBorderColor();
    double borderW = label->getBorderWidth()*0.2;
    int borderO = 0;
    int backO = 0;
    if(backC.a > 0.0001) backO = 1;
    if(borderW > 0) borderO = 1;
    x2 = ol+r-pr;
    y2 = ot-t+pt+fz*0.8;

    if(backO + borderO > 0) {
      fprintf(f, "    <rect style=\"opacity:1;fill:#%s;fill-opacity:%d;fill-rule:nonzero;stroke:#%s;stroke-width:%g;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:%d\" id=\"labelback_%lu\" width=\"%g\" height=\"%g\" x=\"%g\" y=\"%g\" ry=\"0\" />\n", View::getColor(backC).c_str(), backO, View::getColor(borderC).c_str(), borderW, borderO, labelID, r-l, t-b, l+ol, ot-t);
    }

    fprintf(f, "    <text xml:space=\"preserve\" style=\"font-style:normal;font-weight:%s;font-size:10px;line-height:125%%;font-family:Stilu;letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" x=\"%g\" y=\"%g\" id=\"label_%lu\">\n", weight.c_str(), x2, y2, labelID);
    osg_text::TextAlign align = label->getAlign();
    std::string a1 = "end";
    std::string a2 = "end";
    if(align == osg_text::ALIGN_LEFT) {
      x2 = ol+l+pr;
      a1 = "start";
      a2 = "start";
    }
    else if(align == osg_text::ALIGN_CENTER) {
      x2 = ol+l+(r-l)*0.5;
      a1 = "center";
      a2 = "middle";
    }
    fprintf(f, "      <tspan id=\"labeltext_%lu\" x=\"%g\" y=\"%g\" style=\"font-style:normal;font-variant:normal;font-weight:%s;font-stretch:normal;font-size:%gpx;font-family:'Stilu';-inkscape-font-specification:'Stilu';text-align:%s;text-anchor:%s\">%s</tspan>\n    </text>\n", labelID, x2, y2, weight.c_str(), fz, a1.c_str(), a2.c_str(), label->getText().c_str());
    ++labelID;
  }

} // end of namespace: osg_graph_viz
