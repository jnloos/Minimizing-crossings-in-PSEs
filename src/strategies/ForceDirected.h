#ifndef PROJECT_FORCE_DIRECTED_H
#define PROJECT_FORCE_DIRECTED_H

#include "../source/dependencies.h"

using namespace std;
using namespace chrono;


class ForceDirected final : public Strategy {
public:
    explicit ForceDirected(
            function<vector<double>(PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf)> const &repel,
            function<vector<double>(PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf)> const &attract,
            function<vector<double>(PSE &emb, vector<double> force, double temp, vector<double> &runConf)> const &sprain
    ) : funcRepel(repel), funcAttract(attract), funcCool(sprain) {};

    vector<double> runConf;
    enum Param {
        replSpring = 0,
        attrSpring = 1,
        lenSpring = 2,
        lenFR = 3,
    };

    PSE run(Executor &exec) override {
        importConfig("FDA.json");

        // Prepare the runtime configuration
        runConf.resize(8);
        runConf[replSpring] = conf["spring"]["repl"];
        runConf[attrSpring] = conf["spring"]["attr"];
        runConf[lenSpring] = conf["spring"]["len"];
        runConf[lenFR] = conf["fruchtrhein"]["len"];

        PSE &emb = exec.emb;

        // Conditions for termination
        int const maxIter = conf["maxIter"];
        double const maxDiff = conf["maxDiff"];

        // Temperature and cooling constant
        double temp = 1;
        double const cool = conf["cool"];

        int currIter = 0;
        double maxForce = maxDiff + 1;

        while(currIter < maxIter && maxForce > maxDiff) {
            maxForce = numeric_limits<double>::min();

            PSE copy = emb;
            for(Vertex const &vertex: copy.gamma.vertices) {
                vector<double> force = {0, 0};

                for(Vertex const &otherVertex : copy.gamma.vertices) {
                    if(vertex != otherVertex) {
                        vector<double> repl = funcRepel(copy, vertex.pos, otherVertex.pos, runConf);
                        force[0] += repl[0];
                        force[1] += repl[1];

                        if(copy.gamma.existsEdge(vertex.id, otherVertex.id)) {
                            vector<double> attr = funcAttract(copy, vertex.pos, otherVertex.pos, runConf);
                            force[0] += attr[0];
                            force[1] += attr[1];
                        }
                    }
                }

                force = funcCool(copy, force, temp, runConf);
                double const norm = VectorSpace::len(force[0], force[1]);
                if(maxForce < norm)
                    maxForce = norm;

                double x = vertex.pos.x + force[0];
                double y = vertex.pos.y + force[1];

                emb.moveToPos(vertex.id, {x, y});
            }

            temp *= cool;
            currIter += 1;
        }
        
        return normalize(emb);
    }

protected:

    /**
     *************************
     * Min-Max-Normalization *
     *************************
     * @reference https://databasecamp.de/ki/minmax-scaler
     */
    static PSE normalize(PSE &emb) {
        Drawing gamma = emb.gamma;
        Vertex const &root = gamma.getVertex(0);

        double minX = root.pos.x;
        double minY = root.pos.y;
        double maxX = root.pos.x;
        double maxY = root.pos.y;

        for(Vertex const &vertex: gamma.vertices) {
            minX = min(minX, vertex.pos.x);
            maxX = max(maxX, vertex.pos.x);
            minY = min(minY, vertex.pos.y);
            maxY = max(maxY, vertex.pos.y);
        }

        for(Vertex const &vertex : gamma.vertices) {
            Position pos = vertex.pos;

            if (maxX != minX)
                // Scales x coordinates to the interval [0,emb.width]
                pos.x = (pos.x - minX) * (static_cast<double>(emb.width) / (maxX - minX));
            else pos.x = 0;

            if (maxY != minY)
                // Scales y coordinates to the interval [0,emb.height]
                pos.y = (pos.y - minY) * (static_cast<double>(emb.height) / (maxY - minY));
            else pos.y = 0;

            emb.moveToPos(vertex.id, pos);
        }

        return emb;
    }

    function<vector<double>(PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf)> funcRepel;
    function<vector<double>(PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf)> funcAttract;
    function<vector<double>(PSE &emb, vector<double> force, double temp, vector<double> &runConf)> funcCool;
};


/**
 ****************************
 * Spring Embedder by Eades *
 ****************************
 * @reference https://algo.uni-trier.de/demos/forceDirected.html
 * @reference https://www.cs.ubc.ca/~will/536E/papers/Eades1984.pdf
 * @coauthor Jun. Prof. Phillip Kindermann, University of Trier
 */

inline auto repelSpring =
    [](PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf){
        double const dist = VectorSpace::dist(aPos, bPos);
        double const xMove = aPos.x - bPos.x;
        double const yMove = aPos.y - bPos.y;

        // Avoids that vertices are freezing at a shared position.
        if(dist < EPS) {
            Point const &point = emb.getRandomPoint();
            double const xSign = pow(-1, (static_cast<int>(point.pos.x) % 2));
            double const ySign = pow(-1, (static_cast<int>(point.pos.y) % 2));
            return vector{xSign * emb.width, ySign * emb.height};
        }

        double const repl = runConf[ForceDirected::Param::replSpring];
        double xRepel = (repl / pow(dist, 2)) * (xMove / dist);
        double yRepel = (repl / pow(dist, 2)) * (yMove / dist);

        if(isnan(xRepel))
            xRepel = numeric_limits<double>::quiet_NaN();
        if(isnan(yRepel))
            yRepel = numeric_limits<double>::quiet_NaN();

        return vector{xRepel, yRepel};
    };

inline auto attractSpring =
    [](PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf) {
        double const dist = VectorSpace::dist(bPos, aPos);
        double const xMove = bPos.x - aPos.x;
        double const yMove = bPos.y - aPos.y;

        if(dist < EPS)
            return vector<double>{0, 0};

        double len = runConf[ForceDirected::Param::lenSpring];
        double const attr = runConf[ForceDirected::Param::attrSpring];

        vector<double> const repelForce = repelSpring(emb, aPos, bPos, runConf);
        double xAttract = ((attr * log(dist / len)) * (xMove / dist)) - repelForce[0];
        double yAttract = ((attr * log(dist / len)) * (yMove / dist)) - repelForce[1];

        if(isnan(xAttract))
            xAttract = numeric_limits<double>::quiet_NaN();
        if(isnan(yAttract))
            yAttract = numeric_limits<double>::quiet_NaN();

        return vector{xAttract, yAttract};
    };

inline auto coolSpring =
    [](PSE &emb, vector<double> force, double temp, vector<double> &runConf) {
        force[0] *= temp;
        force[1] *= temp;
        return force;
    };


/**
 ****************************
 * Fruchtermann & Rheingold *
 ****************************
 * @reference https://algo.uni-trier.de/demos/forceDirected.html
 * @reference http://www.mathe2.uni-bayreuth.de/axel/papers/reingold:graph_drawing_by_force_directed_placement.pdf
 * @coauthor Jun. Prof. Phillip Kindermann, University of Trier
 */

inline auto repelFR =
        [](PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf){
            double const dist = VectorSpace::dist(aPos, bPos);
            double const xMove = aPos.x - bPos.x;
            double const yMove = aPos.y - bPos.y;

            // Avoids that vertices are freezing at a shared position.
            if(dist < EPS) {
                Point const &point = emb.getRandomPoint();
                double const xSign = pow(-1, (static_cast<int>(point.pos.x) % 2));
                double const ySign = pow(-1, (static_cast<int>(point.pos.y) % 2));
                return vector{xSign * emb.width, ySign * emb.height};
            }

            double len = runConf[ForceDirected::Param::lenFR];
            double xRepel = (pow(len, 2) / dist) * (xMove / dist);
            double yRepel = (pow(len, 2) / dist) * (yMove / dist);

            if(isnan(xRepel))
                xRepel = numeric_limits<double>::quiet_NaN();
            if(isnan(yRepel))
                yRepel = numeric_limits<double>::quiet_NaN();

            return vector{xRepel, yRepel};
        };

inline auto attractFR =
        [](PSE &emb, Position const &aPos, Position const &bPos, vector<double> &runConf){
            double const dist = VectorSpace::dist(bPos, aPos);
            double const xMove = bPos.x - aPos.x;
            double const yMove = bPos.y - aPos.y;

            if(dist < EPS)
                return vector<double>{0, 0};

            double len = runConf[ForceDirected::Param::lenFR];
            double xAttract = (pow(dist, 2) / len) * (xMove / dist);
            double yAttract = (pow(dist, 2) / len) * (yMove / dist);

            if(isnan(xAttract))
                xAttract = numeric_limits<double>::quiet_NaN();
            if(isnan(yAttract))
                yAttract = numeric_limits<double>::quiet_NaN();

            return vector{xAttract, yAttract};
        };

inline auto coolFR =
    [](PSE &emb, vector<double> force, double temp, vector<double> &runConf) {
        double len = runConf[ForceDirected::Param::lenFR];

        double maxLen = temp * len * 2;
        len = VectorSpace::len(force[0], force[1]);
        if(len > maxLen) {
            force[0] = (force[0] / len) * maxLen;
            force[1] = (force[1] / len) * maxLen;
        }

        return force;
    };

#endif
