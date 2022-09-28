#include "graph.hpp"
#include <iostream>
#include <list>
#include <map>
#include <memory>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
Graph test() {
  Graph tmp;
  return tmp;
}

TEST_CASE("test Strongly", "Graph") {
  Graph m_graph;
  Vertex &i = m_graph.newVertex();
  Vertex &a = m_graph.newVertex();
  Vertex &b = m_graph.newVertex();
  Vertex &g1 = m_graph.newVertex();
  Vertex &g2 = m_graph.newVertex();
  Vertex &g3 = m_graph.newVertex();
  Vertex &q = m_graph.newVertex();
  m_graph.newEdge(i, a, 2);
  m_graph.newEdge(a, b, 2);
  m_graph.newEdge(b, g1, 2);
  m_graph.newEdge(b, g2, 2);
  m_graph.newEdge(b, g3, 2);
  m_graph.newEdge(g1, a, 2);
  m_graph.newEdge(g3, g2, 2);
  m_graph.newEdge(g2, g3, 2);
  m_graph.newEdge(g1, q, 2);
  m_graph.newEdge(g2, q, 2);
  m_graph.newEdge(g3, q, 2);
}
