#ifndef PROJECT_DRAWING_H
#define PROJECT_DRAWING_H

using namespace std;
using namespace nlohmann;

struct Vertex {
    int id;
    int deg = 0;

    Position pos{};

    // ID of the occupied point
    // Default is -1 and means not occupying
    int occupiedPoint = -1;

    // Should the vertex be ignored during tracking?
    bool ignored = false;

    // Tracked temperature aka summed up penalties
    long temp = 0;

    Vertex()
        : id(-1), pos({-1, -1}) { }

    /**
     * @param id Vertex-ID.
     * @param xPos X-coordinate.
     * @param yPos Y-coordinate.
     */
    Vertex(int const id, double const xPos, double const yPos)
        : id(id), pos({xPos, yPos}) { }

    [[nodiscard]] json toJson() const {
        return json{{"id", id}, {"x", static_cast<double>(pos.x)}, {"y", static_cast<double>(pos.y)} };

        // Use integer cast during the GDC (for safety's sake)
        // return json{{"id", id}, {"x", static_cast<int>(pos.x)}, {"y", static_cast<int>(pos.y)} };
    }

    void moveToPos(Position const &position) {
        pos = position;
    }

    [[nodiscard]] bool isOccupying() const {
        return occupiedPoint > -1;
    }

    bool operator==(Vertex const &other) const {
        return id == other.id;
    }

    bool operator!=(Vertex const &other) const {
        return !(*this == other);
    }
};


struct Edge {
    int id;

    int aVertexId; // References a vertex id
    int bVertexId; // References a vertex id

    Edge()
        : id(-1), aVertexId(-1), bVertexId(-1) { }

    /**
     * @param id Edge-ID.
     * @param aVertex First adjacent vertex.
     * @param bVertex Second adjacent vertex.
     */
    Edge(int const id, Vertex const &aVertex, Vertex const &bVertex)
        : id(id), aVertexId(aVertex.id), bVertexId(bVertex.id) { }

    bool operator==(const Edge& other) const {
        return (aVertexId == other.aVertexId && bVertexId == other.bVertexId) ||
               (aVertexId == other.bVertexId && bVertexId == other.aVertexId);
    }

    bool operator!=(const Edge& other) const {
        return !(*this == other);
    }
};


class Drawing {
public:
    // Disclosure edges and vertices for simple foreach iterations
    vector<Vertex> vertices;
    vector<Edge> edges;

    // Max degree of a vertex
    long maxDeg = 0;

    Drawing()
        : vertices({}), edges({}) { }

    /**
     * @param vertices The set of vertices.
     * @param edges The set of edges.
     */
    Drawing(vector<Vertex>& vertices, vector<Edge>& edges)
            : vertices(vertices), edges(edges) {

        // Initialize the adjacency matrix and list
        adjacencyMatrix.resize(vertices.size());
        for (auto const &aVertex : vertices) {
            adjacencyMatrix[aVertex.id].resize(vertices.size());
            for (auto const &bVertex : vertices)
                adjacencyMatrix[aVertex.id][bVertex.id] = -1;
        }

        // Prepare the adjacency-lists, penalties, and -matrix
        adjacencyList.resize(vertices.size());
        for (auto const &edge : edges) {
            Vertex &aVertex = getVertex(edge.aVertexId);
            Vertex &bVertex = getVertex(edge.bVertexId);

            aVertex.deg += 1;
            bVertex.deg += 1;

            if (aVertex.deg > maxDeg)
                maxDeg = aVertex.deg;
            if (bVertex.deg > maxDeg)
                maxDeg = bVertex.deg;

            adjacencyMatrix[aVertex.id][bVertex.id] = edge.id;
            adjacencyMatrix[bVertex.id][aVertex.id] = edge.id;
            adjacencyList[aVertex.id].push_back(bVertex.id);
            adjacencyList[bVertex.id].push_back(aVertex.id);
        }

        // Initialize randomizer with uniform distribution
        randomVertex = NumRandomizer(0, static_cast<int>(vertices.size() - 1));
    }

    Vertex &getVertex(int const &vertexId) {
        return vertices[vertexId];
    }

    Vertex &getRandomVertex(int const &exp) {
        // Uniform distributions has its own randomizer
        if(exp == 0)
            return getVertex(randomVertex.pull());

        long globTemp = 0;
        for (auto &vertex : vertices)
            globTemp += static_cast<long>(pow(vertex.temp, exp));

        // Generates zero division otherwise
        if (globTemp < 1)
            return getRandomVertex(0);

        NumRandomizer randomizer = NumRandomizer<long>(0, globTemp - 1);
        long val = randomizer.pull();

        for (auto &vertex : vertices) {
            val -= static_cast<long>(pow(vertex.temp, exp));
            if (val < 0)
                return vertex;
        }

        // Fallback case is the uniform distribution
        return getRandomVertex(0);
    }

    Edge &getEdge(int const &edgeId) {
        return edges[edgeId];
    }

    Edge &getEdge(int const &aVertexId, int const &bVertexId) {
        return getEdge(adjacencyMatrix[aVertexId][bVertexId]);
    }

    vector<int> &getNeighbours(int const &vertexId) {
        return adjacencyList[vertexId];
    }

    [[nodiscard]] bool existsVertex(int const &vertexId) const {
        return  0 <= vertexId < vertices.size();
    }

    [[nodiscard]] bool existsEdge(int const &aVertexId, int const &bVertexId) const {
        // Considered graphs are simple
        if(aVertexId == bVertexId)
            return false;

        // Corresponding vertices must exist
        if(!existsVertex(aVertexId) || !existsVertex(bVertexId))
            return false;

        return adjacencyMatrix[aVertexId][bVertexId] != -1;
    }

protected:
    vector<vector<int>> adjacencyMatrix;
    vector<vector<int>> adjacencyList;

    NumRandomizer<int> randomVertex;
};

#endif
