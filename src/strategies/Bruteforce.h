#ifndef PROJECT_BRUTEFORCE_H
#define PROJECT_BRUTEFORCE_H

#include "../source/dependencies.h"

using namespace std;
using namespace chrono;


class Bruteforce final : public Strategy {
public:
    explicit Bruteforce() : Strategy() { }

    /**
     **************
     * Bruteforce *
     **************
     */
    PSE run(Executor &exec) override {
        importConfig("bruteforce.json");
        bool useTracker = conf["useTracker"];

        PSE &emb = exec.emb;

        int const cVertices = static_cast<int>(emb.gamma.vertices.size());
        int const cPoints = static_cast<int>(emb.points.size());
        auto variations = VariationIterator(cVertices, cPoints);

        // Create a valid initial layout
        for(int i=0; i<emb.gamma.vertices.size(); i++) {
            if(emb.getPoint(i).isOccupied())
                emb.moveToPoint(emb.getPoint(i).occupierId, emb.getPoint(i).occupierId);
            emb.moveToPoint(i, i);
        }

        long minScore = emb.lazyScore();
        PSE minEmb = emb;

        while (variations.hasNext && exec.inTime()) {
            vector<int> variation = variations.next();

            int score;
            if(useTracker) {
                // Rearrange with tracked scoring
                for (Vertex &vertex : emb.gamma.vertices) {
                    Point& point = emb.getPoint(variation[vertex.id]);
                    if(vertex.pos != point.pos)
                        emb.trackedRuthlessMoveToPoint(vertex.id, variation[vertex.id]);
                }
                score = emb.lazyScore();
            }
            else {
                // Rearrange with naive scoring
                for (Vertex const &vertex : emb.gamma.vertices)
                    emb.ruthlessMoveToPoint(vertex.id, variation[vertex.id]);
                score = emb.score();
            }

            if (score < minScore) {
                minEmb = emb;
                minScore = score;

                exec.save(score, minEmb, 3);
            }

            exec.cIter++;
        }

        return minEmb;
    }
};

#endif
