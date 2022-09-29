#include "graphalg.hpp"

using namespace graph::core;

inline bool followEdge(const Edge& edge, const EdgeFunc& func) {
	return edge.weight() && func(edge);
}

namespace scc
{
	void vertexIterate(
		const Vertex& vertex,
		const EdgeFunc& func,
		uint32_t& currentDfs,
		VertexBindingMap<uint32_t>& user,
		VertexBindingMap<uint32_t>& color,
		std::vector<Ref<const Vertex>>& callTrace)
	{
		const uint32_t thisDfsNum = currentDfs++;
		user[vertex] = thisDfsNum;
		color[vertex] = 0;
		for (const Edge& edge : vertex.outEdges()) {
			if (followEdge(edge, func)) {
				const Vertex& to = edge.to();
				if (!user[to]) {  // Dest not computed yet
					vertexIterate(to, func, currentDfs, user, color, callTrace);
				}
				if (!color[to]) {  // Dest not in a component
					user[vertex] = std::min(user[vertex], user[to]);
				}
			}
		}
		if (user[vertex] == thisDfsNum) {  // New head of subtree
			color[vertex] = thisDfsNum;  // Mark as component
			while (!callTrace.empty()) {
				const Vertex& popVertex = callTrace.back();
				if (user[popVertex] >= thisDfsNum) {  // Lower node is part of this subtree
					callTrace.pop_back();
					color[popVertex] = thisDfsNum;
				}
				else {
					break;
				}
			}
		}
		else {  // In another subtree (maybe...)
			callTrace.push_back(vertex);
		}
	}
}

namespace ranking
{
	void vertexIterate(
		const Vertex& vertex,
		const EdgeFunc& func,
		const uint32_t adder,
		const uint32_t currentRank,
		VertexBindingMap<uint32_t>& user,
		VertexBindingMap<uint32_t>& rank)
	{
		// Assign rank to each unvisited node
		// If larger rank is found, assign it and loop back through
		// If we hit a back node make a list of all loops
		if (user[vertex] == 1) {
#if 0
			m_graphp->reportLoops(m_edgeFuncp, vertexp);
			m_graphp->loopsMessageCb(vertexp);
#endif
			return;
		}

		if (rank[vertex] >= currentRank) return;  // Already processed it
		user[vertex] = 1;
		rank[vertex] = currentRank;
		for (const auto& edge : vertex.outEdges()) {
			if (followEdge(edge, func)) {
				vertexIterate(edge.to(), func, adder, currentRank + adder, user, rank);
			}
		}
		user[vertex] = 2;
	}
}

namespace acy
{
	using EdgeList = std::list<Ref<const Edge>>;  // List of orig edges, see also GraphAcycEdge's decl
	void addOrigEdge(const Edge& toEdge, const Edge& addEdge) {
		EdgeBindingMap<EdgeList> edgeLists;
		// Add addEdge (or it's list) to list of edges that break edge represents
		// Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
		auto it = edgeLists.find(addEdge);
		if (it != end(edgeLists)) {
			edgeLists[toEdge].splice(edgeLists[toEdge].end(), it->second);
			edgeLists.erase(it);
		}
		else {
			it->second.push_back(addEdge);
		}
	}
	void buildGraphIterate(
		const Vertex& overtex,
		Vertex& avertex,
		Graph& breakGraph,
		VertexBindingMap<uint32_t>& color,
		VertexBindingMap<Ref<Vertex>>& Acyc,
		const EdgeFunc& func)
	{
		// Make new edges
		for (const Edge& edge : overtex.outEdges()) {
			if (followEdge(edge, func)) {  // not cut
				const Vertex& toVertex = edge.to();
				if (color[toVertex]) {
					Vertex& toAVertex = Acyc[toVertex];
					// Replicate the old edge into the new graph
					// There may be multiple edges between same pairs of vertices
					const Edge& breakEdge = breakGraph.newEdge(avertex, toAVertex,
						edge.weight()/*, edgep->cutable()*/);
					addOrigEdge(breakEdge, edge);  // So can find original edge
				}
			}
		}
	}

	Graph buildGraph(const Graph& graph, VertexBindingMap<uint32_t>& color, const EdgeFunc& func)
	{
		VertexBindingMap<Ref<Vertex>> Acyc;
		Graph breakGraph;
		for (const Vertex& overtex : graph.vertices())
			if (color[overtex]) {
				Vertex& avertex = breakGraph.newVertex();
				Acyc[overtex] = avertex; // Stash so can look up later
			}

		// Build edges between logic vertices
		for (const Vertex& overtex : graph.vertices())
			if (color[overtex]) {
				buildGraphIterate(overtex, Acyc[overtex], breakGraph, color, Acyc, func);
			}
		return breakGraph;
	}

	void simplify(const Graph& graph, bool allowCut)
	{
#if 0
		for (const Vertex& vertex : graph.vertices())
			workPush(vertex);
#endif
	}
}

namespace graph::alg
{

	VertexBindingMap<uint32_t> strongly(const Graph& graph, EdgeFunc func)
	{
		// Use Tarjan's algorithm to find the strongly connected subgraphs.
		// State:
		//     user     // DFS number indicating possible root of subtree, 0=not iterated
		//     color       // Output subtree number (fully processed)
		VertexBindingMap<uint32_t> color, user;
		uint32_t currentDfs = 0;
		std::vector<Ref<const Vertex>> callTrace;  // List of everything we hit processing so far

		// Color graph
		for (const Vertex& vertex : graph.vertices()) {
			if (!user[vertex]) {
				currentDfs++;
				scc::vertexIterate(vertex, func, currentDfs, user, color, callTrace);
			}
		}

		// If there's a single vertex of a color, it doesn't need a subgraph
		// This simplifies the consumer's code, and reduces graph debugging clutter
		for (const Vertex& vertex : graph.vertices()) {
			bool onecolor = true;
			for (const Edge& edge : vertex.outEdges()) {
				if (followEdge(edge, func)) {
					if (color[vertex] == color[edge.to()]) {
						onecolor = false;
						break;
					}
				}
			}
			if (onecolor) color[vertex] = 0;
		}

		return color;
	}

	VertexBindingMap<uint32_t> rank(const Graph& graph, EdgeFunc func, const uint32_t adder)
	{
		VertexBindingMap<uint32_t> rank, user;
		for (const Vertex& vertex : graph.vertices())
			if (!user[vertex]) {  //
				ranking::vertexIterate(vertex, func, adder, 1, user, rank);
			}
		return rank;
	}

	void acylic(const Graph& graph, EdgeFunc func)
	{
		auto color = strongly(graph);
		const Graph breakGraph = acy::buildGraph(graph, color, func);
		acy::simplify(breakGraph, false);
	}
}