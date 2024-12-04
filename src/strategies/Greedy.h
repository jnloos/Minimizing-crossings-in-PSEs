#ifndef PROJECT_GREEDY_H
#define PROJECT_GREEDY_H

#include "../source/dependencies.h"

class Greedy final : public Strategy {
public:
    explicit Greedy() : Strategy() { }

    PSE run(Executor &exec) override {
        importConfig("greedy.json");

        if(conf["useSlow"] && !conf["useFast"])
            return slowAssignment(exec.emb);

        if(conf["useFast"] && !conf["useSlow"])
            return fastAssignment(exec.emb);

        PSE aEmb = exec.emb;
        PSE bEmb = exec.emb;

        PSE resultFast, resultSlow;
        thread fastThread([&resultFast, aEmb]() {
            resultFast = Greedy::fastAssignment(aEmb);
        });
        thread slowThread([&resultSlow, bEmb]() {
            resultSlow = Greedy::slowAssignment(bEmb);
        });

        fastThread.join();
        slowThread.join();

        if (resultFast.lazyScore() < resultSlow.lazyScore())
            return resultFast;
        return resultSlow;
    }

private:

    /**
     **************************
     * Fast Greedy Assignment *
     **************************
     * @coauthor Alexander Kutscheid
     */
    static PSE fastAssignment(PSE emb) {
        Drawing &gamma = emb.gamma;

        for(Vertex const &vertex: gamma.vertices) {
            double minDist = numeric_limits<double>::max();
            int closest = -1;

            // Find the closest unoccupied point
            for(Point &point: emb.points) {
                if(! point.isOccupied()) {
                    double const dist = VectorSpace::dist(vertex.pos, point.pos);
                    if(dist < minDist) {
                        minDist = dist;
                        closest = point.id;
                    }
                }
            }

            // Moves the vertex to the closest point
            emb.moveToPoint(vertex.id, closest);
        }

        return emb;
    }

    /**
     **************************
     * Slow Greedy Assignment *
     **************************
     * @coauthor Alexander Kutscheid
     */
    static PSE slowAssignment(PSE emb) {
        Drawing &gamma = emb.gamma;

        int cAssigned = 0;
        while(cAssigned < gamma.vertices.size()) {
            double minDist = numeric_limits<double>::max();
            pair<int, int> assign = {-1, -1};

            // Find the best pair of unoccupied point and unassigned point
            for(Vertex const &vertex: gamma.vertices) {
                if(! vertex.isOccupying()) {
                    for(Point &point: emb.points) {
                        if(! point.isOccupied()) {
                            double const dist = VectorSpace::dist(vertex.pos, point.pos);
                            if(dist < minDist) {
                                minDist = dist;
                                assign = {vertex.id, point.id};
                            }
                        }
                    }
                }
            }

            // Assembles the optimal pair
            if(assign.second != -1) {
                emb.moveToPoint(assign.first, assign.second);
                cAssigned++;
            }
        }

        return emb;
    }
};

#endif
