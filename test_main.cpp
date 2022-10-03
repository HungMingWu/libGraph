#include "graph.hpp"
#include "graphalg.hpp"
#include <iostream>
#include <list>
#include <map>
#include <memory>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace graph::core;
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
	operator Graph& () { return graph; }
};

TEST_CASE("test strongly connected component", "Graph") {
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

	auto color = graph::alg::strongly(graph);
	REQUIRE(color[i] != color[a]);
	REQUIRE(color[a] != color[g2]);
	REQUIRE(color[g2] != color[q]);
	REQUIRE(color[a] == color[b]);
	REQUIRE(color[a] == color[g1]);
	REQUIRE(color[g2] == color[g3]);
}

TEST_CASE("test rank algorithm", "Graph") {
	Graph graph;
	Vertex& v1 = graph.newVertex();
	Vertex& v2 = graph.newVertex();
	Vertex& v3 = graph.newVertex();
	Vertex& v4 = graph.newVertex();
	Vertex& v5 = graph.newVertex();
	Vertex& v6 = graph.newVertex();
	Vertex& v7 = graph.newVertex();
	graph.newEdge(v1, v2, 1);
	graph.newEdge(v2, v3, 1);
	graph.newEdge(v3, v1, 1);
	graph.newEdge(v4, v5, 1);
	graph.newEdge(v5, v4, 1);
	graph.newEdge(v6, v7, 1);
	auto [rank, loopsMap] = graph::alg::rank(graph);
	REQUIRE(rank[v1] == 1);
	REQUIRE(rank[v2] == 2);
	REQUIRE(rank[v3] == 3);
	REQUIRE(rank[v4] == 1);
	REQUIRE(rank[v5] == 2);
	REQUIRE(rank[v6] == 1);
	REQUIRE(rank[v7] == 2);
	REQUIRE(loopsMap.count(v1) != 0); // Loop start from v1
	REQUIRE(loopsMap[v1].size() == 4);
	REQUIRE(loopsMap[v1][0] == v1);
	REQUIRE(loopsMap[v1][1] == v2);
	REQUIRE(loopsMap[v1][2] == v3);
	REQUIRE(loopsMap[v1][3] == v1);
	REQUIRE(loopsMap.count(v4) != 0); // Loop start from v4
	REQUIRE(loopsMap[v4].size() == 3);
	REQUIRE(loopsMap[v4][0] == v4);
	REQUIRE(loopsMap[v4][1] == v5);
	REQUIRE(loopsMap[v4][2] == v4);
	REQUIRE(loopsMap.count(v6) == 0); // No loop start from v6
}

TEST_CASE("test acyclic algorithm", "Graph") {
	Graph graph;
	Vertex& i = graph.newVertex();
	Vertex& a = graph.newVertex();
	Vertex& b = graph.newVertex();
	Vertex& g1 = graph.newVertex();
	Vertex& g2 = graph.newVertex();
	Vertex& g3 = graph.newVertex();
	graph.newEdge(i, a, 2);
	graph.newEdge(a, b, 2);
	graph.newEdge(b, g1, 2);
	graph.newEdge(b, g2, 2);
	graph.newEdge(b, g3, 2);
	graph.newEdge(g1, a, 2);
	graph.newEdge(g2, a, 2);
	graph.newEdge(g3, a, 2);
	graph::alg::acylic(graph);
}