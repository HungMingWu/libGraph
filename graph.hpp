#pragma once
#include "list.hpp"

#include <functional>
#include <list>
#include <map>
#include <memory>
namespace graph::core
{
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
		constexpr bool operator<(const Ref& rhs) const { return pointer < rhs.pointer; }
		constexpr bool operator==(const Ref& rhs) const { return pointer == rhs.pointer; }
	};

	class Edge : public list_element<Forward>, public list_element<Reverse> {
		Vertex& m_from;
		Vertex& m_to;
		int m_weight;
		friend class Vertex;
		friend class Graph;

	public:
		Edge(Vertex& from, Vertex& to, int weight);
		~Edge() = default;
		Vertex& from();
		const Vertex& from() const;
		Vertex& to();
		const Vertex& to() const;
		void weight(int);
		int weight() const;
	};

	class Vertex : public list_element<> {
		intrusive_list<Edge, Reverse> m_in;
		intrusive_list<Edge, Forward> m_out;
		friend class Graph;
		friend class Edge;

	public:
		Vertex() = default;
		~Vertex() = default;
		intrusive_list<Edge, Reverse>& inEdges() { return m_in; }
		const intrusive_list<Edge, Reverse>& inEdges() const { return m_in; }
		intrusive_list<Edge, Forward>& outEdges() { return m_out; }
		const intrusive_list<Edge, Forward>& outEdges() const { return m_out; }
	};

	class Graph {
	protected:
		intrusive_list<Vertex> m_vertices;
		friend class Vertex;
		friend class Edge;

	public:
		Graph() = default;
		~Graph();
		Graph(const Graph&) = delete;
		Graph(Graph&&) = default;
		Vertex& newVertex();
		Edge& newEdge(Vertex& from, Vertex& to, int weight);
		intrusive_list<Vertex>& vertices() { return m_vertices; }
		const intrusive_list<Vertex>& vertices() const { return m_vertices; }
		void remove(Vertex&);
		void remove(Edge&);
		virtual bool cutable(const Edge& edge) const { return true; }
		virtual void cutable(const Edge& edge, bool cutable) {}
	};

	template <typename T>
	using VertexBindingMap = std::map<Ref<const Vertex>, T>;

	template <typename T>
	using EdgeBindingMap = std::map<Ref<const Edge>, T>;

	using VertexBindingVec = std::vector<Ref<const Vertex>>;

	using EdgeFunc = std::function<bool(const Edge&)>;
}