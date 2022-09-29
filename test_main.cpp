#include "graph.hpp"
#include "graphalg.hpp"
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

class StrGraph {
	Graph graph;
	VertexBindingMap<std::string> nameMap;
public:
	Vertex& newVertex(std::string_view name) {
		Vertex& vertex = graph.newVertex();
		nameMap.emplace(vertex, name);
		return vertex;
	}
	Edge& newEdge(Vertex& from, Vertex& to, int weight)
	{
		return graph.newEdge(from, to, weight);
	}
	operator Graph&() { return graph; }
};

TEST_CASE("test Strongly", "Graph") {
	StrGraph graph;
	Vertex& i = graph.newVertex("*INPUTS*");
	Vertex& a = graph.newVertex("a");
	Vertex& b = graph.newVertex("b");
	Vertex& g1 = graph.newVertex("g1");
	Vertex& g2 = graph.newVertex("g2");
	Vertex& g3 = graph.newVertex("g3");
	Vertex& q = graph.newVertex("q");
	graph.newEdge(i, a, 2);
	graph.newEdge(a, b, 2);
	graph.newEdge(b, g1, 2);
	graph.newEdge(b, g2, 2);
	graph.newEdge(b, g3, 2);
	graph.newEdge(g1, a, 2);
	graph.newEdge(g3, g2, 2);
	graph.newEdge(g2, g3, 2);
	graph.newEdge(g1, q, 2);
	graph.newEdge(g2, q, 2);
	graph.newEdge(g3, q, 2);

	auto color = Strongly(graph);
	REQUIRE(color[i] != color[a]);
	REQUIRE(color[a] != color[g2]);
	REQUIRE(color[g2] != color[q]);
	REQUIRE(color[a] == color[b]);
	REQUIRE(color[a] == color[g1]);
	REQUIRE(color[g2] == color[g3]);
}
