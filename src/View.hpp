/**
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief The view class is the main class organizing an osg scene drawing the graph.
 *
 **/

#ifndef OSG_GRAPH_VIZ_VIEW_HPP
#define OSG_GRAPH_VIZ_VIEW_HPP

#include "Node.hpp"
#include "Edge.hpp"
#include "UpdateInterface.hpp"

#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgText/Text>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osgGA/GUIEventHandler>
#include <osg/Camera>
#include <list>

#include <mars/osg_text/Text.h>

namespace osg_material_manager {
  class OsgMaterialManager;
}

namespace osg_graph_viz {

  class NodeP;

  enum LineMode {
    DIRECT_LINE_MODE,
    ORTHO_LINE_MODE,
    SMOOTH_LINE_MODE,
  };

  class Tab {
  public:
    osg::ref_ptr<osg_text::Text> label;
    configmaps::ConfigMap map;
  };

  class View : public osgGA::GUIEventHandler {

  public:
    View();
    ~View();
    void init(double hFS, double pFS, double ps, bool classicLook=false);

    osg_graph_viz::Node* createNode(const NodeInfo &info);
    osg_graph_viz::Edge* createEdge(const configmaps::ConfigMap &info,
                                    int idx1, int idx2);
    double mergeIconSize, portFontSize, headerFontSize, portScale;

    osg::Group* getScene() {return scene.get();}
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator removeEdge(osg_graph_viz::Edge*);
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator removeNode(osg_graph_viz::Node*);

    void update(void);

    void mouseMove(double x, double y, int scaleX, int scaleY);
    void mousePress(double x, double y, int button);
    void mouseRelease(double x, double y, int button);
    void getPosition(double *x, double *y);
    void deleteKey();
    void scaleView(double newScale, double x=0, double y=0);
    void getViewScale(double *scale);
    void setViewPos(double x=0, double y=0);
    void getViewPos(double *x, double *y);
    osg::Texture2D *loadTexture(const std::string &file);

    void setUpdateInterface(UpdateInterface *ui) {this->ui = ui;}
    void updateMap(const configmaps::ConfigMap &map);
    void setLineMode(LineMode mode) {lineMode = mode;}
    void setModKey(bool v);
    void setScaleRatio(double value);
    void decoupleSelected();    
    void decoupleEdgesOfNodes(std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes);
    void repositionNodes();
    void repositionEdges();
    void decoupleLongEdges();
    std::list<osg::ref_ptr<osg_graph_viz::Node> > getSelectedNodes();
    void setResourcesPath(std::string path) {resourcesPath = path;}
    std::string getResourcesPath() {return resourcesPath;}

    // implements osgGA::GUIEventHandler::handle
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
    void setCamera(osg::Camera *c) {camera = c;}
    void resize(int w, int h);
    void createTab(const std::string &name);
    void saveTab(configmaps::ConfigMap &map);
    void clearSelection(bool whole);
    osg::ref_ptr<Node> getNodeByName(const std::string &name);
    void removeNodeFromView(osg::ref_ptr<osg::Node> node);
    void addNodeToView(osg::ref_ptr<osg::Node> node);
    bool groupNodes(const std::string &parent, const std::string &child);
    bool updateNode(Node *node);
    void setRetinaScale(double v) {retinaScale = v;}
    osg_material_manager::OsgMaterialManager* getOsgMaterialManager() {
      return materialManager;
    }
    void setFilter(const std::string &filter, int value);
    std::map<std::string, int>& getFilterMap() {return filterMap;}
    void removeOsgNodeFromView(osg::ref_ptr<osg::Node> node);
    void addOsgNodeToView(osg::ref_ptr<osg::Node> node);
    void setEdgesSmooth(bool v);
    void getWindowPixelSize(double *x, double *y);
    void exportSvg(const std::string &filename);
    static void exportLabelToSvg(osg::ref_ptr<osg_text::Text>,
                                 FILE *f, double ol, double ot);
    static std::string getColor(const osg::Vec4 &c);
    static std::string getColor(const osg_text::Color &c);
    void handleNodeTooltips(double mouseX, double mouseY);

  private:
    std::map<std::string, int> filterMap;
    LineMode lineMode;
    int numXTicks, numYTicks;
    float xTicksDiff, yTicksDiff;
    double windowHeight, windowWidth;
    osg::ref_ptr<osg_text::Text> infoText;
    osg::ref_ptr<osg::PositionAttitudeTransform> mainPos;
    osg::ref_ptr<osg::MatrixTransform> mainScale, cameraScale;
    osg::ref_ptr<osg::Group> scene, content;
    osg::ref_ptr<osg::Camera> camera;

    osg::ref_ptr<osg::Vec3Array> vertices, selectionVertices;
    osg::ref_ptr<osg::Geometry> geom, selectionGeom;
    osg::ref_ptr<osg::DrawArrays> xDrawArray;
    bool selecting;

    std::list<osg::ref_ptr<osg_graph_viz::Node> > nodeList;
    std::list<osg::ref_ptr<osg_graph_viz::Edge> > edgeList;

    int mouseMask;
    double mouseX, mouseY;
    double scale, scaleRatio;
    double posX, posY;
    double xStartSelection, yStartSelection, xEndSelection, yEndSelection;
    double scrollScale[4];
    double retinaScale;
    int renderBin;
    bool addEdge;
    bool pressed,changed;
    bool modKey, dontDeselectOnRelease;
    bool roundNodes, mouseMoved;
    bool inScale;
    static unsigned long labelID;
    static configmaps::ConfigMap bufferMap;

    osg_material_manager::OsgMaterialManager *materialManager;
    osg::ref_ptr<osg::Geode> selectionGeode;

    std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes;
    osg::ref_ptr<osg_graph_viz::Node> newEdgeFromNode, selectedNode;
    int newEdgeFromIdx;
    osg::ref_ptr<osg_graph_viz::Edge> newEdge, selectedEdge;
    std::list<osg::ref_ptr<osg_graph_viz::Edge> > selectedEdges;
    osg::ref_ptr<osg_graph_viz::Node> nodeToMove, addToGroupNode;

    std::map<std::string, osg::ref_ptr<osg::Texture2D> > texMap;
    std::string resourcesPath;
    std::map<std::string, Tab*> tabMap;
    Tab *currentTab;
    UpdateInterface *ui;

    void handleNewEdge(osg::ref_ptr<osg_graph_viz::Node> toNode, int toIdx);
    void makeSelcetionInRect();
    void makeSelRect(const double xStart, const double yStart, const double xEnd, const double yEnd);
    void duplicateSelection();
    void copySelection();
    void pasteSelection();
    void undoPreviousAction();
    void redoPreviousAction();
    /*osg_graph_viz::Node* makeSelNode();
    void deletSelNode(Node *node);*/

  };

} // end of namespace: osg_graph_viz

#endif // OSG_GRAPH_VIZ_VIEW_HPP
