/**
 * \file UpdateInterface.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef OSG_GRAPH_VIZ_UPDATE_INTERFACE_HPP
#define OSG_GRAPH_VIZ_UPDATE_INTERFACE_HPP

namespace osg_graph_viz {

  class Node;
  class Edge;

  class UpdateInterface {
  public:
    UpdateInterface() {}
    virtual ~UpdateInterface() {}
    
    virtual void nodeSelected(Node *node) {}
    virtual void edgeSelected(Edge *edge) {}
    virtual void nothingSelected() {}
    virtual bool removeEdge(Edge *edge) {return true;}
    virtual bool removeNode(Node *edge) {return true;}
    virtual bool updateNode(Node *node) {return true;}
    virtual bool updateEdge(Edge *edge) {return true;}
    virtual Node* addNode(configmaps::ConfigMap node) = 0;
    virtual void addEdge(configmaps::ConfigMap edgeMap, bool reload=false) = 0;
    virtual void newEdge(Edge *edge, Node* fromNode, int fromNodeIdx,
                         Node* toNode, int toNodeIdx) {}
    virtual void rightClick(Node *node, int inPort, int outPort,
                            Edge *edge, double x, double y) {}
    virtual void addHistoryEntry() {}
    virtual void loadTab(const std::string &s, configmaps::ConfigMap &map) {}
    virtual bool groupNodes(const std::string &parent,
			    const std::string &child) {return true;}

  };
  
} // end of namespace osg_graph_viz

#endif // OSG_GRAPH_VIZ_UPDATE_INTERFACE_HPP
  
