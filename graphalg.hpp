#pragma once
#include "graph.hpp"

namespace graph::alg {
	using namespace graph::core;

	inline bool followAlwaysTrue(const Edge&) { return true; }

	// Algorithms - strongly connected components
	VertexBindingMap<uint32_t> strongly(const Graph& graph, EdgeFunc func = followAlwaysTrue);

	VertexBindingMap<uint32_t> rank(const Graph& graph, EdgeFunc func = followAlwaysTrue, const uint32_t adder = 1);

	void acylic(const Graph& graph, EdgeFunc func = followAlwaysTrue);
}