#include "graph.hpp"

namespace graph::core
{
	Edge::Edge(Graph& graph, Vertex& from, Vertex& to, int weight)
		: m_graph(graph)
		, m_from(from)
		, m_to(to)
		, m_weight(weight)
	{
		m_from.m_out.push_back(*this);
		m_to.m_in.push_back(*this);
	}

	const Vertex& Edge::from() const { return m_from; }

	const Vertex& Edge::to() const { return m_to; }

	Vertex::Vertex(Graph& graph)
		: m_graph(graph) {
		m_graph.active_vertices.insert(m_graph.active_vertices.end(), *this);
	}

	void Vertex::removeEdges() {
		for (Edge& inEdge : m_in) inEdge.remove();
		for (Edge& outEdge : m_out) outEdge.remove();
	}

	void Vertex::remove() {
		removeEdges();
		m_graph.active_vertices.erase(*this);
	}

	int Edge::weight() const { return m_weight; }

	void Edge::remove() {
		m_from.m_out.erase(*this);
		m_to.m_in.erase(*this);
	}

	Graph::~Graph() {
		active_vertices.clear();
		allocated_vertices.clear();
		allocated_edges.clear();
	}

	Vertex& Graph::newVertex() {
		auto iter = allocated_vertices.emplace(allocated_vertices.end(), *this);
		return *iter;
	}

	Edge& Graph::newEdge(Vertex& from, Vertex& to, int weight) {
		return allocated_edges.emplace_back(*this, from, to, weight);
	}
}