#include <algorithm>
#include "graphalg.hpp"

using namespace graph::core;

auto followEdge(const EdgeFunc& func)
{
	return [&func](const Edge& edge) {
		return edge.weight() && func(edge);
	};
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
			if (func(edge)) {
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

namespace report
{
	bool vertexIterate(const Vertex& vertex,
		const EdgeFunc& func,
		VertexBindingVec& callTrace,
		VertexBindingMap<uint32_t>& visited) {
		callTrace.push_back(vertex);
		if (visited[vertex] == 1) return true;
		if (visited[vertex] == 2) {
			callTrace.pop_back();
			return false;  // Already processed it
		}
		visited[vertex] = 1;
		for (const auto& edge : vertex.outEdges()) {
			if (func(edge) && vertexIterate(edge.to(), func, callTrace, visited))
				return true;
		}
		visited[vertex] = 2;
		callTrace.pop_back();
		return false;
	}
}

namespace ranking
{
	void vertexIterate(
		const Vertex& vertex,
		const EdgeFunc& func,
		const uint32_t adder,
		const uint32_t currentRank,
		VertexBindingMap<uint32_t>& visited,
		VertexBindingMap<uint32_t>& rank,
		VertexBindingMap<VertexBindingVec>& loopsMap)
	{
		// Assign rank to each unvisited node
		// If larger rank is found, assign it and loop back through
		// If we hit a back node make a list of all loops
		if (visited[vertex] == 1) {
			loopsMap[vertex] = graph::alg::reportLoops(vertex, func);
			return;
		}

		if (rank[vertex] >= currentRank) return;  // Already processed it
		visited[vertex] = 1;
		rank[vertex] = currentRank;
		for (const auto& edge : vertex.outEdges()) {
			if (func(edge))
				vertexIterate(edge.to(), func, adder, currentRank + adder, visited, rank, loopsMap);
		}
		visited[vertex] = 2;
	}
}

namespace acy
{
	struct AcycAttribute {
		uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
		uint32_t rank = 0;
		bool m_onWorkList = false;  // True if already on list of work to do
		bool m_deleted = false;  // True if deleted
	};

	class WorkList {
	protected:
		std::list<Ref<Vertex>> work;  // List of vertices with optimization work left
		VertexBindingMap<AcycAttribute>& acyc_attribs;
	public:
		WorkList(VertexBindingMap<AcycAttribute>& attribs) : acyc_attribs(attribs) {}
		void push(Vertex& vertex) {
			AcycAttribute& attrib = acyc_attribs[vertex];
			// Add vertex to list of nodes needing further optimization trials
			if (!attrib.m_onWorkList) {
				attrib.m_onWorkList = true;
				work.emplace_back(vertex);
			}
		}
		Vertex& pop()
		{
			Vertex& vertex = work.front();
			work.pop_front();
			AcycAttribute& attrib = acyc_attribs[vertex];
			attrib.m_onWorkList = false;
			return vertex;
		}
		bool empty() const { return work.empty(); }
		VertexBindingMap<AcycAttribute>& attributes() { return acyc_attribs; }
	};

	using EdgeList = std::list<Ref<const Edge>>;  // List of orig edges, see also GraphAcycEdge's decl
	void addOrigEdge(const Edge& toEdge, const Edge& addEdge, EdgeBindingMap<EdgeList>& edgeLists) {
		// Add addEdge (or it's list) to list of edges that break edge represents
		// Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
		auto& insertList = edgeLists[toEdge];
		if (auto it = edgeLists.find(addEdge); it != end(edgeLists)) {
			insertList.splice(edgeLists[toEdge].end(), it->second);
			edgeLists.erase(it);
		}
		else {
			insertList.push_back(addEdge);
		}
	}

	void buildGraphIterate(
		const Vertex& originVertex,
		Vertex& acyclicVertex,
		Graph& breakGraph,
		const Graph& originGraph,
		VertexBindingMap<uint32_t>& color,
		VertexBindingMap<Ref<Vertex>>& Acyc,
		const EdgeFunc& func)
	{
		EdgeBindingMap<EdgeList> edgeLists;
		// Make new edges
		for (const Edge& edge : originVertex.outEdges()) {
			if (func(edge)) {  // not cut
				const Vertex& toVertex = edge.to();
				if (color[toVertex]) {
					Vertex& toAVertex = Acyc[toVertex];
					// Replicate the old edge into the new graph
					// There may be multiple edges between same pairs of vertices
					const Edge& breakEdge = breakGraph.newEdge(acyclicVertex, toAVertex,
						edge.weight()/*, edgep->cutable()*/);
					breakGraph.cutable(breakEdge, originGraph.cutable(edge));
					addOrigEdge(breakEdge, edge, edgeLists);  // So can find original edge
				}
			}
		}
	}

	Graph buildGraph(const Graph& graph, VertexBindingMap<uint32_t>& color, const EdgeFunc& func)
	{
		VertexBindingMap<Ref<Vertex>> Acyc;
		Graph breakGraph;
		for (const Vertex& originVertex : graph.vertices())
			if (color[originVertex]) {
				Vertex& vertex = breakGraph.newVertex();
				Acyc[originVertex] = vertex; // Stash so can look up later
			}

		// Build edges between logic vertices
		for (const Vertex& originVertex : graph.vertices())
			if (color[originVertex]) {
				buildGraphIterate(originVertex, Acyc[originVertex], breakGraph, graph, color, Acyc, followEdge(func));
			}
		return breakGraph;
	}


	void simplifyNone(Graph& graph, Vertex& vertex, AcycAttribute& attrib, WorkList& list) {
		// Don't need any vertices with no inputs, There's no way they can have a loop.
		// Likewise, vertices with no outputs
		if (attrib.m_deleted) return;
		if (vertex.inEdges().empty() || vertex.outEdges().empty()) {
			//UINFO(9, "  SimplifyNoneRemove " << avertexp << endl);
			attrib.m_deleted = true;  // Mark so we won't delete it twice
			// Remove edges
			for (auto& edge : vertex.outEdges()) {
				Vertex& otherVertex = edge.to();
				// UINFO(9, "  out " << otherVertexp << endl);
				graph.remove(edge);

				list.push(otherVertex);
			}
			for (auto& edge : vertex.inEdges()) {
				Vertex& otherVertexp = edge.from();
				// UINFO(9, "  in  " << otherVertexp << endl);
				graph.remove(edge);
				list.push(otherVertexp);
			}
		}
	}

	Edge& edgeFromEdge(Graph& graph, Edge& oldedgep, Vertex& fromp, Vertex& top) {
		EdgeBindingMap<Ref<const Edge>> vertexMap;
		// Make new breakGraph edge, with old edge as a template
		Edge& newEdgep = graph.newEdge(fromp, top, oldedgep.weight());
		graph.cutable(newEdgep, graph.cutable(oldedgep));
		vertexMap[newEdgep] = vertexMap[oldedgep];  // Keep pointer to OrigEdgeList
		return newEdgep;
	}

	void cutOrigEdge(Edge& breakEdgep, const char* why) {
		// From the break edge, cut edges in original graph it represents
#if 0
		UINFO(8, why << " CUT " << breakEdgep->fromp() << endl);

		breakEdgep->cut();
		const OrigEdgeList* const oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
		if (!oEListp) {
			v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
		}
		// The breakGraph edge may represent multiple real edges; cut them all
		for (const auto& origEdgep : *oEListp) {
			origEdgep->cut();
			UINFO(8,
				"  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->top() << endl);
		}
#endif
	}

	bool placeIterate(Graph& graph, Vertex& vertexp, uint32_t currentRank, uint32_t placeStep, WorkList& list) {
		// Assign rank to each unvisited node
		//   rank() is the "committed rank" of the graph known without loops
		// If larger rank is found, assign it and loop back through
		// If we hit a back node make a list of all loops
		VertexBindingMap<uint32_t> user;
		AcycAttribute& attrib = list.attributes()[vertexp];
		if (attrib.rank >= currentRank) return false;  // Already processed it
		if (user[vertexp] == placeStep) return true;  // Loop detected
		user[vertexp] = placeStep;
		// Remember we're changing the rank of this node; might need to back out
		if (!attrib.m_onWorkList) {
			attrib.m_storedRank = attrib.rank;
			list.push(vertexp);
		}
		attrib.rank = currentRank;
		// Follow all edges and increase their ranks
		for (auto& edgep : vertexp.outEdges()) {
			if (edgep.weight() && !graph.cutable(edgep)) {
				if (placeIterate(graph, edgep.to(), currentRank + 1, placeStep, list)) {
					// We don't need to reset user(); we'll use a different placeStep for the next edge
					return true;  // Loop detected
				}
			}
		}
		user[vertexp] = 0;
		return false;
	}

	void placeTryEdge(Graph& graph, Edge& edgep, uint32_t& m_placeStep, WorkList& list) {
		// Try to make this edge uncutable
		m_placeStep++;
		VertexBindingMap<uint32_t> rank;
#if 0
		UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp()
			<< endl);
#endif
		// Make the edge uncutable so we detect it in placement
		graph.cutable(edgep, false);
		// Vertex::m_user begin: number indicates this edge was completed
		// Try to assign ranks, presuming this edge is in place
		// If we come across user()==placestep, we've detected a loop and must back out
		const bool loop
			= placeIterate(graph, edgep.to(), rank[edgep.from()] + 1, m_placeStep, list);
#if 1
		if (!loop) {
			// No loop, we can keep it as uncutable
			// Commit the new ranks we calculated
			// Just cleanup the list.  If this is slow, we can add another set of
			// user counters to avoid cleaning up the list.
			while (!list.empty()) list.pop();
		}
		else {
			// Adding this edge would cause a loop, kill it
			graph.cutable(edgep, true);  // So graph still looks pretty
			cutOrigEdge(edgep, "  Cut loop");
			graph.remove(edgep);
			// Back out the ranks we calculated
			while (!list.empty()) {
				Vertex& vertex = list.pop();
				auto& attrib = list.attributes()[vertex];
				attrib.rank = attrib.m_storedRank;
			}
		}
#endif
	}

	void place(Graph& graph, WorkList& list) {
		// Input is m_breakGraph with ranks already assigned on non-breakable edges

		// Make a list of all cutable edges in the graph
		int numEdges = 0;
		for (Vertex& vertexp : graph.vertices())
			for (Edge& edgep : vertexp.outEdges())
				if (edgep.weight() && graph.cutable(edgep)) ++numEdges;

		// UINFO(4, "    Cutable edges = " << numEdges << endl);

		std::vector<std::reference_wrapper<Edge>> edges;  // List of all edges to be processed
		// Make the vector properly sized right off the bat -- faster than reallocating
		edges.reserve(numEdges + 1);
		for (Vertex& vertexp : graph.vertices())
			for (Edge& edgep : vertexp.outEdges())
				if (edgep.weight() && graph.cutable(edgep)) edges.push_back(edgep);

		// Sort by weight, then by vertex (so that we completely process one vertex, when possible)
		stable_sort(edges.begin(), edges.end(), [](const Edge& lhs, const Edge& rhs) {
			return lhs.weight() > rhs.weight();
			});

		// Process each edge in weighted order
		uint32_t m_placeStep = 10;
		for (Edge& edgep : edges) placeTryEdge(graph, edgep, m_placeStep, list);
	}

	void simplifyOne(Graph& graph, Vertex& vertex, AcycAttribute& attrib, WorkList& list) {
		// If a node has one input and one output, we can remove it and change the edges
		if (attrib.m_deleted) return;
		if (vertex.inEdges().size() > 1 && vertex.outEdges().size() > 1) {
			Edge& inEdgep = vertex.inEdges().begin();
			Edge& outEdgep = vertex.outEdges().begin();
			Vertex& inVertexp = inEdgep.from();
			Vertex& outVertexp = outEdgep.to();
			// The in and out may be the same node; we'll make a loop
			// The in OR out may be THIS node; we can't delete it then.
			if (std::addressof(inVertexp) != std::addressof(vertex) &&
				std::addressof(outVertexp) != std::addressof(vertex)) {
				// UINFO(9, "  SimplifyOneRemove " << avertexp << endl);
				attrib.m_deleted = true;  // Mark so we won't delete it twice
				// Make a new edge connecting the two vertices directly
				// If both are breakable, we pick the one with less weight, else it's arbitrary
				// We can forget about the origEdge list for the "non-selected" set of edges,
				// as we need to break only one set or the other set of edges, not both.
				// (This is why we must give preference to the cutable set.)
				Edge& templateEdgep
					= (graph.cutable(inEdgep)
						&& (!graph.cutable(outEdgep)) || inEdgep.weight() < outEdgep.weight())
					? inEdgep
					: outEdgep;
				// cppcheck-suppress leakReturnValNotUsed
				edgeFromEdge(graph, templateEdgep, inVertexp, outVertexp);
				// Remove old edge
				graph.remove(inEdgep);
				graph.remove(outEdgep);
				list.push(inVertexp);
				list.push(outVertexp);
			}
		}
	}

	void simplifyOut(Graph& graph, Vertex& avertexp, AcycAttribute& attrib, WorkList& list) {
		// If a node has one output that's not cutable, all its inputs can be reassigned
		// to the next node in the list
		if (attrib.m_deleted) return;
		if (avertexp.outEdges().size() > 1) {
			Edge& outEdgep = avertexp.outEdges().begin();
			if (!graph.cutable(outEdgep)) {
				Vertex& outVertexp = outEdgep.to();
				// UINFO(9, "  SimplifyOutRemove " << avertexp << endl);
				attrib.m_deleted = true;  // Mark so we won't delete it twice
				for (auto& inEdgep : avertexp.inEdges()) {
					Vertex& inVertexp = inEdgep.from();
					if (std::addressof(inVertexp) == std::addressof(avertexp)) {
#if 0
						if (debug()) v3error("Non-cutable edge forms a loop, vertex=" << avertexp);
						v3error("Circular logic when ordering code (non-cutable edge loop)");
						m_origGraphp->reportLoops(
							&V3GraphEdge::followNotCutable,
							avertexp->origVertexp());  // calls OrderGraph::loopsVertexCb
#endif
						// Things are unlikely to end well at this point,
						// but we'll try something to get to further errors...
						graph.cutable(inEdgep, true);
						return;
					}
					// Make a new edge connecting the two vertices directly
					// cppcheck-suppress leakReturnValNotUsed
					edgeFromEdge(graph, inEdgep, inVertexp, outVertexp);
					// Remove old edge
					graph.remove(inEdgep);
					list.push(inVertexp);
				}
				graph.remove(outEdgep);
				list.push(outVertexp);
			}
		}
	}

	void addOrigEdgep(Edge& toEdgep, Edge& addEdgep) {
		// Add addEdge (or it's list) to list of edges that break edge represents
		// Note addEdge may already have a bunch of similar linked edge representations.  Yuk.

		using OrigEdgeList = std::list<Ref<Edge>>;  // List of orig edges
		EdgeBindingMap<OrigEdgeList> edgeListMap;
		if (edgeListMap.count(addEdgep) != 0) {

			//for (const auto& itr : *addListp) oEListp->push_back(itr);
			edgeListMap.erase(addEdgep);
		}
		else {
			edgeListMap[toEdgep].push_back(addEdgep);
		}
	}

	void simplifyDup(Graph& graph, Vertex& avertexp, AcycAttribute& attrib, WorkList& list) {
		// Remove redundant edges
		if (attrib.m_deleted) return;
		VertexBindingMap<Ref<Edge>> prevEdges;
		// Mark edges and detect duplications
		for (auto& edgep : avertexp.outEdges()) {
			Vertex& outVertexp = edgep.to();
			const bool prevEdgeFound = prevEdges.count(outVertexp) == 0;
			if (prevEdgeFound) {
				Edge& prevEdge = prevEdges[outVertexp];
				if (!graph.cutable(prevEdge)) {
					// !cutable duplicates prev !cutable: we can ignore it, redundant
					//  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
//					UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->top() << endl);
					graph.remove(edgep);
				}
				else if (!graph.cutable(edgep)) {
					// !cutable duplicates prev  cutable: delete the earlier cutable
					// UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->top() << endl);
					graph.remove(prevEdge);
					prevEdges[outVertexp] = edgep;
				}
				else {
					//  cutable duplicates prev  cutable: combine weights
					// UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->top() << endl);
					prevEdge.weight(prevEdge.weight() + edgep.weight());
					addOrigEdgep(prevEdge, edgep);
					graph.remove(edgep);
				}
				list.push(outVertexp);
				list.push(avertexp);
			}
			else {
				// No previous assignment
				prevEdges[outVertexp] = edgep;
			}
		}
	}

	void cutBasic(Graph& graph, Vertex& avertexp, AcycAttribute& attrib, WorkList& list) {
		// Detect and cleanup any loops from node to itself
		if (attrib.m_deleted) return;
		for (auto& edgep : avertexp.outEdges()) {
			if (graph.cutable(edgep) && std::addressof(edgep.to()) == std::addressof(avertexp)) {
				cutOrigEdge(edgep, "  Cut Basic");
				graph.remove(edgep);
				list.push(avertexp);
			}
		}
	}

	void cutBackward(Graph& graph, Vertex& avertexp, AcycAttribute& attrib, WorkList& list) {
		// If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
		if (attrib.m_deleted) return;
		VertexBindingMap<bool> maps;
		for (auto& edgep : avertexp.inEdges())
			if (!graph.cutable(edgep)) maps[edgep.from()] = true;
		// Detect duplications
		for (auto& edgep : avertexp.outEdges()) {
			if (graph.cutable(edgep) && maps[edgep.to()]) {
				cutOrigEdge(edgep, "  Cut A->B->A");
				graph.remove(edgep);
				list.push(avertexp);
			}
		}
	}

	void simplify(Graph& graph, bool allowCut, WorkList& workList)
	{
		// Optimize till everything finished
		while (!workList.empty()) {
			Vertex& vertex = workList.pop();
			AcycAttribute& attrib = workList.attributes()[vertex];
			simplifyNone(graph, vertex, attrib, workList);
			simplifyOne(graph, vertex, attrib, workList);
			simplifyOut(graph, vertex, attrib, workList);
			simplifyDup(graph, vertex, attrib, workList);
			if (allowCut) {
				// The main algorithm works without these, though slower
				// So if changing the main algorithm, comment these out for a test run
				if (1) { //v3Global.opt.fAcycSimp()) {
					cutBasic(graph, vertex, attrib, workList);
					cutBackward(graph, vertex, attrib, workList);
				}
			}

		}

		for (Vertex& vertex : graph.vertices())
			workList.push(vertex);
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

		const auto followEdgeFunc = followEdge(func);

		// Color graph
		for (const Vertex& vertex : graph.vertices()) {
			if (!user[vertex]) {
				currentDfs++;
				scc::vertexIterate(vertex, followEdgeFunc, currentDfs, user, color, callTrace);
			}
		}

		// If there's a single vertex of a color, it doesn't need a subgraph
		// This simplifies the consumer's code, and reduces graph debugging clutter
		for (const Vertex& vertex : graph.vertices()) {
			bool onecolor = true;
			for (const Edge& edge : vertex.outEdges()) {
				if (followEdgeFunc(edge)) {
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

	std::tuple<VertexBindingMap<uint32_t>, VertexBindingMap<VertexBindingVec>>
		rank(const Graph& graph, EdgeFunc func, const uint32_t adder)
	{
		VertexBindingMap<uint32_t> rank, visited;
		VertexBindingMap<VertexBindingVec> loopsMap;
		for (const Vertex& vertex : graph.vertices())
			if (!visited[vertex]) {
				ranking::vertexIterate(vertex, followEdge(func), adder, 1, visited, rank, loopsMap);
			}
		return { rank, loopsMap };
	}

	void acylic(const Graph& graph, EdgeFunc func)
	{
		auto color = strongly(graph);
		Graph breakGraph = acy::buildGraph(graph, color, func);

		VertexBindingMap<acy::AcycAttribute> acyc_attribs;

		// Work Queue
		acy::WorkList workList(acyc_attribs);

		acy::simplify(breakGraph, false, workList);
		//if (dumpGraph() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

		//UINFO(4, " Cutting trivial loops\n");
		acy::simplify(breakGraph, true, workList);
		//if (dumpGraph() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

		//UINFO(4, " Ranking\n");
		auto followNotCutable = [&](const Edge& edgep) {
			return !breakGraph.cutable(edgep);
		};
		auto result = graph::alg::rank(breakGraph, followNotCutable);
		//if (dumpGraph() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

		// UINFO(4, " Placement\n");
		acy::place(breakGraph, workList);

		// UINFO(4, " Final Ranking\n");
		// Only needed to assert there are no loops in completed graph
		auto result2 = graph::alg::rank(breakGraph, followNotCutable);
		//if (dumpGraph() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
	}

	VertexBindingVec reportLoops(const Vertex& vertex, EdgeFunc func)
	{
		VertexBindingVec callTrace;
		VertexBindingMap<uint32_t> visited;
		report::vertexIterate(vertex, followEdge(func), callTrace, visited);
		return callTrace;
	}
}
