#pragma once
#include "graph.hpp"

namespace scc {
	void vertexIterate(
		const Vertex& vertex,
		const EdgeFunc& func,
		uint32_t& currentDfs,
		VertexBindingMap<uint32_t>& user,
		VertexBindingMap<uint32_t>& color,
		std::vector<Ref<const Vertex>>& callTrace);
}

namespace ranking {
	void vertexIterate(
		const Vertex& vertex,
		const EdgeFunc& func,
		const uint32_t adder,
		const uint32_t currentRank,
		VertexBindingMap<uint32_t>& user,
		VertexBindingMap<uint32_t>& rank);
}

namespace acy {
	void simplify(const Graph& graph, bool allowCut);
	Graph buildGraph(const Graph& graph, VertexBindingMap<uint32_t>& color, const EdgeFunc& func);
}

inline bool followAlwaysTrue(const Edge&) { return true; }

// Algorithms - strongly connected components
VertexBindingMap<uint32_t> Strongly(const Graph& graph, EdgeFunc func = followAlwaysTrue);

VertexBindingMap<uint32_t> rank(const Graph& graph, EdgeFunc func = followAlwaysTrue, const uint32_t adder = 1);

void acylic(const Graph& graph, EdgeFunc func = followAlwaysTrue);