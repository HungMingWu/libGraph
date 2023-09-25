#include "graph.hpp"

namespace graph::core
{
	Edge::Edge(Vertex& from, Vertex& to, int weight)
		: m_from(from)
		, m_to(to)
		, m_weight(weight)
	{
		m_from.m_out.push_back(*this);
		m_to.m_in.push_back(*this);
	}

	Vertex& Edge::from() { return m_from; }
	const Vertex& Edge::from() const { return m_from; }
	Vertex& Edge::to() { return m_to; }
	const Vertex& Edge::to() const { return m_to; }

	void Edge::weight(int weight) { m_weight = weight; }
	int Edge::weight() const { return m_weight; }

	Graph::~Graph() {
		for (auto it = m_vertices.begin(); it != m_vertices.end(); ++it)
			remove(*it);
	}

	Vertex& Graph::newVertex() {
		auto newVertex = new Vertex();
		m_vertices.push_back(*newVertex);
		return *newVertex;
	}

	Edge& Graph::newEdge(Vertex& from, Vertex& to, int weight) {
		Edge* newEdge = new Edge(from, to, weight);
		from.m_out.push_back(*newEdge);
		to.m_in.push_back(*newEdge);
		return *newEdge;
	}

	void Graph::remove(Vertex& vtx) {
		for (auto& edge : vtx.m_in)
			remove(edge);
		for (auto& edge : vtx.m_out)
			remove(edge);
		delete& vtx;
	}

	void Graph::remove(Edge& edge) {
		edge.m_from.m_out.erase(edge);
		edge.m_to.m_in.erase(edge);
		delete& edge;
	}
}