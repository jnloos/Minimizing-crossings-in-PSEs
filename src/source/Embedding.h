#ifndef PROJECT_EMBEDDING_H
#define PROJECT_EMBEDDING_H

using namespace std;
using namespace nlohmann;


class Point {
public:
    int id;
    Position pos;

    // ID of the occupying vertex
    // Default is -1 and means unoccupied
    int occupierId = -1;

    Point()
        : id(-1), pos{static_cast<double>(-1), static_cast<double>(-1)} { }

    /**
     * @param id Point-ID.
     * @param x X-coordinate.
     * @param y Y-coordinate.
     */
    Point(int const id, int const x, int const y)
        : id(id), pos{static_cast<double>(x), static_cast<double>(y)} { }

    void release() {
        occupierId = -1;
    }

    void occupy(int const vertexId) {
        occupierId = vertexId;
    }

    [[nodiscard]] bool isOccupied() const {
        return occupierId > -1;
    }

    bool operator==(const Point& other) const {
        return id == other.id;
    }

    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};


class PSE {
public:
    int width = -1;
    int height = -1;

    // PSE is a decorator of this class
    Drawing gamma;

    // Disclosure points for simple foreach iterations
    vector<Point> points;

    // Short-cut property for |E|
    long penalty = 0;

    PSE()
        :  width(0), height(0), points({}) { }

    /**
     * @param drawing Drawing of graph G(V,E).
     * @param points Point-set P.
     * @param width Specified width.
     * @param height Specified height.
     */
    PSE(Drawing drawing, vector<Point> &points, int const width, int const height)
            : width(width), height(height), gamma(std::move(drawing)), points(points) {

        milieu.resize(points.size());
        for (auto &point : points) {
            // Enables reverse access from coordinate to point
            coordinates[static_cast<int>(point.pos.x)][static_cast<int>(point.pos.y)] = point.id;

            // Create a queue of all other points, sorted by distance from the current point (or ID if same distance)
            priority_queue<pair<double, int>, vector<pair<double, int>>, greater<>> queue;
            for(auto const &otherPoint : points)
                if(point != otherPoint)
                    queue.emplace(VectorSpace::dist(point.pos, otherPoint.pos), otherPoint.id);

            for(int i=0; i < gamma.maxDeg; i++) {
                // Save the maxDeg-nearest points in a prepared list
                milieu[point.id].emplace_back(queue.top().second);
                queue.pop();
            }
        }
        penalty = static_cast<long>(gamma.vertices.size());

        // Initialize randomizer with uniform distribution
        randomPoint = NumRandomizer(0, static_cast<int>(points.size() - 1));
    }

    /**
     * Only copies dynamic values from another PSE.
     * @param other The PSE object to copy from.
     */
    void fastCopy(PSE const &other) {
        points = other.points;
        gamma.vertices = other.gamma.vertices;
        gamma.edges = other.gamma.edges;

        scoreTracker = other.scoreTracker;
        isTrackerReady = other.isTrackerReady;
    }

    /**
     * Moves a vertex to a specified position.
     * @param vertexId ID of the vertex.
     * @param pos Position in the plane.
     */
    void moveToPos(int const &vertexId, Position const &pos) {
        Vertex &vertex = gamma.getVertex(vertexId);
        vertex.moveToPos(pos);
    }

    /**
     * Moves a vertex to a specified point.
     * @param vertexId ID of the vertex.
     * @param pointId ID of the target.
     */
    void moveToPoint(int const &vertexId, int const &pointId) {
        Point &point = getPoint(pointId);
        Vertex &vertex = gamma.getVertex(vertexId);

        point.occupy(vertex.id);
        if (vertex.occupiedPoint != -1 && vertex.occupiedPoint != point.id) {
            Point &oldPoint = getPoint(vertex.occupiedPoint);
            if(oldPoint.occupierId == vertex.id)
                oldPoint.release();
        }

        moveToPos(vertex.id, point.pos);
        vertex.occupiedPoint = pointId;
    }

    /**
     * Moves a vertex to a specified point and tracks the score.
     * @param vertexId ID of the vertex.
     * @param pointId ID of the target.
     */
    void trackedMoveToPoint(int const &vertexId, int const &pointId) {
        prepareTracker();
        Vertex &vertex = gamma.getVertex(vertexId);
        if(vertex.occupiedPoint == pointId)
            return;

        long const oldScore = pen(vertex.id, TrackerMode::before);
        moveToPoint(vertex.id, pointId);
        long const newScore = pen(vertex.id, TrackerMode::after);

        scoreTracker += (newScore - oldScore);
    }

    /**
     * Moves a vertex to a specified point. Ignores the occupation status.
     * @param vertexId ID of the vertex.
     * @param pointId ID of the target.
     */
    void ruthlessMoveToPoint(int const &vertexId, int const &pointId) {
        Point const &point = getPoint(pointId);
        moveToPos(vertexId, point.pos);
    }

    /**
     * Moves a vertex to a specified point and tracks the score. Ignores the occupation status.
     * @param vertexId ID of the vertex.
     * @param pointId ID of the target.
     */
    void trackedRuthlessMoveToPoint(int const &vertexId, int const &pointId) {
        prepareTracker();

        long const oldScore = pen(vertexId, TrackerMode::before);
        ruthlessMoveToPoint(vertexId, pointId);
        long const newScore = pen(vertexId, TrackerMode::after);

        scoreTracker += (newScore - oldScore);
    }

    /**
     * Exchanges the occupied points of two vertices are positioned.
     * @param aVertexId ID of the vertex.
     * @param bVertexId ID of the other vertex.
     */
    void exchangePoints(int const &aVertexId, int const &bVertexId) {
        if (aVertexId == bVertexId)
            return;

        Vertex const &aVertex = gamma.getVertex(aVertexId);
        Vertex const &bVertex = gamma.getVertex(bVertexId);

        Point &aPoint = getPoint(aVertex.occupiedPoint);
        Point &bPoint = getPoint(bVertex.occupiedPoint);

        // Modify positions
        aPoint.release();
        moveToPoint(bVertex.id, aPoint.id);
        moveToPoint(aVertex.id, bPoint.id);
    }

    /**
     * Exchanges the occupied points of two vertices and tracks the score.
     * @param aVertexId ID of the vertex.
     * @param bVertexId ID of the other vertex.
     */
    void trackedExchangePoints(int const &aVertexId, int const &bVertexId) {
        prepareTracker();
        if (aVertexId == bVertexId)
            return;

        Vertex const &aVertex = gamma.getVertex(aVertexId);
        Vertex &bVertex = gamma.getVertex(bVertexId);

        bVertex.ignored = true;
        long const aOldCrossings = pen(aVertex.id, TrackerMode::before);
        bVertex.ignored = false;
        long const bOldCrossings = pen(bVertex.id, TrackerMode::before);

        exchangePoints(aVertexId, bVertexId);

        bVertex.ignored = true;
        long const aNewCrossings = pen(aVertex.id, TrackerMode::after);
        bVertex.ignored = false;
        long const bNewCrossings = pen(bVertex.id, TrackerMode::after);

        // New version
        scoreTracker += (aNewCrossings - aOldCrossings);
        scoreTracker += (bNewCrossings - bOldCrossings);
    }

    /**
     * Checks the occupation status of the target point. If it is free, the vertex is moved to the point.
     * In other case, the two involved vertexes exchange their points.
     * @param vertexId ID of the vertex.
     * @param pointId ID of the target.
     */
    void moveOrSwap(int const &vertexId, int const &pointId) {
        Point const &point = getPoint(pointId);
        if (point.isOccupied())
            exchangePoints(vertexId, point.occupierId);
        else moveToPoint(vertexId, point.id);
    }

    /**
     * Checks the occupation status of the target point. If it is free, the vertex is moved to the point.
     * In other case, the two involved vertexes exchange their points.
     * @param vertexId ID of the vertex.
     * @param pointId ID of the target.
     */
    void trackedMoveOrSwap(int const &vertexId, int const &pointId) {
        Point const &point = getPoint(pointId);
        if (point.isOccupied())
            trackedExchangePoints(vertexId, point.occupierId);
        else trackedMoveToPoint(vertexId, point.id);
    }

    Point &getPoint(int const &pointId) {
        return points[pointId];
    }

    Point &getRandomPoint() {
        return getPoint(randomPoint.pull());
    }

    Point &getPointOnPos(Position const &pos) {
        return getPoint(coordinates[static_cast<int>(pos.x)][static_cast<int>(pos.y)]);
    }

    /**
     * Retrieves the n nearest points to a given point.
     * @param pointId ID of the target.
     * @param n Number to retrieve.
     */
    vector<int> nNearestPoints(int const &pointId, int const &n) {
        // Try to use the cached nearest points first
        if(n <= gamma.maxDeg)
            return {milieu[pointId].begin(), milieu[pointId].begin() + n};

        // Create a queue of all other points, sorted by distance from the considered point (or ID if same distance)
        Point const &point = getPoint(pointId);
        priority_queue<pair<double, int>, vector<pair<double, int>>, greater<>> queue;
        for(Point &otherPoint : points)
            if(point != otherPoint)
                queue.emplace(VectorSpace::dist(point.pos, otherPoint.pos), otherPoint.id);

        // Store n-nearest points in a vector
        vector<int> nearest;
        for(int i=0; i<n; i++) {
            nearest.emplace_back(queue.top().second);
            queue.pop();
        }

        return nearest;
    }

    /**
     * Calculates the embeddings' total score.
     * @coauthor Jun. Prof. Dr. Philipp Kindermann, University of Trier
     */
    long score() {
        long crossings = 0;

        // Sum all penalties of all edges (without duplications)
        for (auto aEdge = gamma.edges.begin(); aEdge != gamma.edges.end(); ++aEdge)
            for (auto bEdge = next(aEdge); bEdge != gamma.edges.end(); ++bEdge)
                crossings += cross(*aEdge, *bEdge);

        return crossings;
    }

    /**
     * Retrieves the tracked score and prepares the tracker at the first call.
     * @coauthors Alexander Kutscheid and Jun. Prof. Dr. Philipp Kindermann, University of Trier
     */
    long lazyScore() {
        // Ensures that the tracker is initialised
        if(!isTrackerReady)
            prepareTracker();
        return scoreTracker;
    }

protected:
    NumRandomizer<int> randomPoint;

    map<int, map<int, int>> coordinates;
    vector<vector<int>> milieu;

    // Currently tracked score
    long scoreTracker = 0;

    // Is the tracker initialized?
    bool isTrackerReady = false;

    /**
     * Evaluates a cross for two edges.
     * @param aEdge The first edge.
     * @param bEdge The second edge.
     */
    long cross(Edge const &aEdge, Edge const &bEdge) {
        if(aEdge != bEdge){
            Position const &aStart = gamma.getVertex(aEdge.aVertexId).pos;
            Position const &aEnd = gamma.getVertex(aEdge.bVertexId).pos;
            Position const &bStart = gamma.getVertex(bEdge.aVertexId).pos;
            Position const &bEnd = gamma.getVertex(bEdge.bVertexId).pos;
            return VectorSpace::evalSegments(aStart, aEnd, bStart, bEnd, penalty);
        }
        return 0;
    }

    /**
     * Prepares the tracker and initializes local temperatures.
     */
    void prepareTracker() {
        if(!isTrackerReady) {
            scoreTracker = 0;

            for (auto aEdge = gamma.edges.begin(); aEdge != gamma.edges.end(); ++aEdge) {
                Vertex &aStart = gamma.getVertex(aEdge->aVertexId);
                Vertex &aEnd = gamma.getVertex(aEdge->bVertexId);

                // Skip edges with ignored vertices
                if(aStart.ignored || aEnd.ignored)
                    continue;

                for (auto bEdge = next(aEdge); bEdge != gamma.edges.end(); ++bEdge) {
                    Vertex &bStart = gamma.getVertex(bEdge->aVertexId);
                    Vertex &bEnd = gamma.getVertex(bEdge->bVertexId);

                    // Skip edges with ignored vertices
                    if(bStart.ignored || bEnd.ignored)
                        continue;

                    // Sum crossings in the score tracker
                    long const pen = cross(*aEdge, *bEdge);
                    scoreTracker += pen;

                    // Sum penalties as local temperatures
                    aStart.temp += pen;
                    aEnd.temp += pen;
                    bStart.temp += pen;
                    bEnd.temp += pen;
                }
            }

            // The tracker is now prepared
            isTrackerReady = true;
        }
    }

    enum TrackerMode {before = -1, after = +1};

    /**
     * Determines the penalty on a vertex's adjacent edges.
     * Updates the local temperatures immediately in one.
     */
    long pen(int const &vertexId, TrackerMode penSign) {
        long score = 0;

        Vertex &vertex = gamma.getVertex(vertexId);
        vector<int> neighbourIds = gamma.getNeighbours(vertex.id);

        for (int const neighbourId: neighbourIds) {
            Edge &aEdge = gamma.getEdge(vertex.id, neighbourId);
            Vertex &aStart = gamma.getVertex(aEdge.aVertexId);
            Vertex &aEnd = gamma.getVertex(aEdge.bVertexId);

            // Skip edges with ignored vertices
            if(aStart.ignored || aEnd.ignored)
                continue;

            for (auto &bEdge: gamma.edges) {
                Vertex &bStart = gamma.getVertex(bEdge.aVertexId);
                Vertex &bEnd = gamma.getVertex(bEdge.bVertexId);

                // Skip edges with ignored vertices
                if(bStart.ignored || bEnd.ignored)
                    continue;

                // Edges to neighbours must be calculated in the subsequent loop
                if(bEdge.aVertexId == vertex.id || bEdge.bVertexId == vertex.id)
                    continue;

                // Updates the local temperatures
                // Impact is subtracted before the modification and then added again
                long pen = cross(aEdge, bEdge);
                aStart.temp += penSign * pen;
                aEnd.temp += penSign * pen;
                bStart.temp += penSign * pen;
                bEnd.temp += penSign * pen;

                score += pen;
            }
        }

        // Without this separate loop for the neighbours, deviating scores occurred
        for (int i=0; i<neighbourIds.size(); i++) {
            Vertex const &neighbour = gamma.getVertex(neighbourIds[i]);
            Edge &aEdge = gamma.getEdge(vertex.id, neighbour.id);
            Vertex &aStart = gamma.getVertex(aEdge.aVertexId);
            Vertex &aEnd = gamma.getVertex(aEdge.bVertexId);

            // Skip edges with ignored vertices
            if(aStart.ignored || aEnd.ignored)
                continue;

            for(int j=i+1; j<neighbourIds.size(); j++) {
                Vertex const &otherNeighbour = gamma.getVertex(neighbourIds[j]);
                Edge &bEdge = gamma.getEdge(vertex.id, otherNeighbour.id);
                Vertex &bStart = gamma.getVertex(bEdge.aVertexId);
                Vertex &bEnd = gamma.getVertex(bEdge.bVertexId);

                // Skip edges with ignored vertices
                if(bStart.ignored || bEnd.ignored)
                    continue;

                // Updates the local temperatures
                // Impact is subtracted before the modification and then added again
                long pen = cross(aEdge, bEdge);
                aStart.temp += penSign * pen;
                aEnd.temp += penSign * pen;
                bStart.temp += penSign * pen;
                bEnd.temp += penSign * pen;

                score += pen;
            }
        }

        return score;
    }
};

#endif
