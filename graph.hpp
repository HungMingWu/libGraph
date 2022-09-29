#pragma once
#include "list.hpp"

#include <functional>
#include <list>
#include <map>
#include <memory>
struct Forward;
struct Reverse;
class Graph;
class Vertex;

template <typename T>
class Ref {
	T* pointer = nullptr;

public:
	Ref() = default;
	Ref(T& ref)
		: pointer(std::addressof(ref)) {}
	operator T& () { return *pointer; }
	T* ptr() const { return pointer; }
	bool operator<(const Ref& rhs) const { return pointer < rhs.pointer; }
};

class Edge : public list_element<Forward>, public list_element<Reverse> {
	Graph& m_graph;
	Vertex& m_from;
	Vertex& m_to;
	int m_weight;
	friend class Vertex;
	friend class Graph;

public:
	Edge(Graph& graph, Vertex& from, Vertex& to, int weight);
	~Edge() = default;
	void remove();
	const Vertex& from() const;
	const Vertex& to() const;
	int weight() const;
};

class Vertex : public list_element<> {
	Graph& m_graph;
	intrusive_list<Edge, Reverse> m_in;
	intrusive_list<Edge, Forward> m_out;
	friend class Graph;
	friend class Edge;

public:
	Vertex(Graph& graph);
	~Vertex() = default;
	void removeEdges();
	void remove();
	const intrusive_list<Edge, Reverse>& inEdges() const { return m_in; }
	const intrusive_list<Edge, Forward>& outEdges() const { return m_out; }
};

class Graph {
protected:
	std::list<Vertex> allocated_vertices;
	std::list<Edge> allocated_edges;
	intrusive_list<Vertex> active_vertices;
	friend class Vertex;
	friend class Edge;

public:
	Graph() = default;
	~Graph();
	Graph(const Graph&) = delete;
	Graph(Graph&&) = default;
	Vertex& newVertex();
	Edge& newEdge(Vertex& from, Vertex& to, int weight);
	const intrusive_list<Vertex>& vertices() const { return active_vertices; }
};

template <typename T>
using VertexBindingMap = std::map<Ref<const Vertex>, T>;

template <typename T>
using EdgeBindingMap = std::map<Ref<const Edge>, T>;

using EdgeFunc = std::function<bool(const Edge&)>;
inline bool followEdge(const Edge& edge, const EdgeFunc& func) {
	return edge.weight() && func(edge);
}
