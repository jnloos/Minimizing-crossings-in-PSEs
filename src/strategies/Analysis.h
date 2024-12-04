#ifndef PROJECT_ANALYSIS_H
#define PROJECT_ANALYSIS_H

#include "../source/dependencies.h"

using namespace std;
using namespace chrono;


class Analysis final : public Strategy {
public:
    explicit Analysis() : Strategy() { }

    PSE run(Executor &exec) override {
        console.lock();

        cout << endl;
        cout << "Analysis report of " << exec.name << endl;
        cout << "|V| = " << exec.emb.gamma.vertices.size() << endl;
        cout << "|E| = " << exec.emb.gamma.edges.size() << endl;
        cout << "|P| = " << exec.emb.points.size() << endl;

        double avgDeg = 0;
        int maxDeg = numeric_limits<int>::min();
        int minDeg = numeric_limits<int>::max();
        for(Vertex const &vertex : exec.emb.gamma.vertices) {
            avgDeg += vertex.deg;
            maxDeg = max(vertex.deg, maxDeg);
            minDeg = min(vertex.deg, minDeg);
        }
        avgDeg = static_cast<double>(avgDeg) / static_cast<double>(exec.emb.gamma.vertices.size());

        cout << "minDegree = " << minDeg << endl;
        cout << "maxDegree = " << maxDeg << endl;
        cout << "avgDegree = " << avgDeg << endl;
        cout << "size = " << exec.emb.width << "x" << exec.emb.height << endl;
        cout << endl;

        console.unlock();
        return exec.emb;
    }
};

#endif