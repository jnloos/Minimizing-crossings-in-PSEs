#ifndef PROJECT_SIMULATED_ANNEALING_H
#define PROJECT_SIMULATED_ANNEALING_H

#include <utility>
#include "dependencies.h"

class SimulatedAnnealing final : public Strategy {
public:
    explicit SimulatedAnnealing(
        const function<PSE(PSE& emb, vector<double> &runConf)>& refactor,
        const function<double(double temp, long cIter, PSE& emb, vector<double> &runConf)>& cooling
    ) : funcRefactor(refactor), funcCooling(cooling) {};

    vector<double> runConf;
    enum Param {
        initTemp = 0,
        distribExp = 1,

        expBase = 2,
        linFact = 3,
        chooseFar = 4,

        loopTime = 5,
        nextMethod = 6,
        lastImp = 7
    };

    /**
     ***********************
     * Simulated Annealing *
     ***********************
     * @reference: https://link.springer.com/chapter/10.1007/0-306-48056-5_10
     * @reference: https://pure.tue.nl/ws/portalfiles/portal/2116564/338267.pdf
     * @coauthor: Alexander Kutscheid
     */
    PSE run(Executor &exec) override {
        importConfig("SA.json");

        // Prepare the runtime configuration
        runConf.resize(8);
        runConf[Param::initTemp] = conf["initTemp"];
        runConf[Param::distribExp] = conf["distribExp"];
        runConf[Param::expBase] = conf["exponential"]["base"];
        runConf[Param::linFact] = conf["linear"]["factor"];
        runConf[Param::chooseFar] = conf["rebuild-neighbours"]["chooseFar"];
        runConf[Param::loopTime] = conf["loopTime"];
        runConf[Param::nextMethod] = 0.0;
        runConf[Param::lastImp] = 0.0;

        PSE &emb = exec.emb;
        PSE minEmb = emb;
        PSE copy = emb;

        long minScore = minEmb.lazyScore();

        // int lastExport = 0;
        while(exec.inTime()) {

            /*
    	    if(exec.consumed<minutes>() >= lastExport) {
                string const name = exec.name + "-min-" + to_string(lastExport) + "-" + to_string(minScore) + ".json";
                exec.IO.save(minEmb, name);
                lastExport += 5;
            }
            */

            long currIter = 0;
            double temp = runConf[Param::initTemp];
            emb.fastCopy(minEmb);

            long const start = exec.consumed<seconds>();
            while((exec.consumed<seconds>() - start) < runConf[loopTime] && exec.inTime()) {
                copy.fastCopy(emb);
                funcRefactor(copy, runConf);

                long const newScore = copy.lazyScore();
                long const oldScore = emb.lazyScore();
                double const prob = exp((oldScore - newScore) / temp) * 100;

                if(newScore < oldScore) {
                    emb.fastCopy(copy);

                    if(newScore < minScore) {
                        minScore = newScore;
                        minEmb.fastCopy(copy);

                        runConf[Param::lastImp] = 0;
                        exec.save(minScore, minEmb, 2);
                    }
                }
                else if(randPercent.pull() <= prob)
                    emb.fastCopy(copy);

                currIter += 1;
                exec.cIter += 1;

                temp = funcCooling(temp, currIter, copy, runConf);
            }

            runConf[Param::lastImp] += 1;
        }

        return minEmb;
    }

protected:
    function<PSE(PSE& emb, vector<double> &runConf)> funcRefactor;
    function<double(double temp, long cIter, PSE& emb, vector<double> &runConf)> funcCooling;
};


/**
 **********************
 * Cooling Techniques *
 **********************
 * @reference https://www.fys.ku.dk/~andresen/BAhome/ownpapers/perm-annealSched.pdf
 */

inline auto coolExponential =
    [](double temp, long cIter, PSE& emb, vector<double> &runConf) {
        double const base = runConf[SimulatedAnnealing::Param::expBase];
        return temp * base;
    };

// NOT CONSIDERED IN THE THESIS
inline auto coolLinear =
    [](double temp, long cIter, PSE& emb, vector<double> &runConf) {
        double const initTemp = runConf[SimulatedAnnealing::Param::initTemp];
        double const fact = runConf[SimulatedAnnealing::Param::linFact];
        return initTemp - fact * cIter;
};


/**
 **************************
 * Refactoring Techniques *
 **************************
 */

inline auto randomWalk =
        [](PSE& emb, vector<double> &runConf){
            Vertex vertex;
            int exp = static_cast<int>(runConf[SimulatedAnnealing::Param::distribExp]);
            vertex = emb.gamma.getRandomVertex(exp);

            Point const &point = emb.getRandomPoint();
            emb.trackedMoveOrSwap(vertex.id, point.id);
            return emb;
        };

inline auto rebuildNeighbourhood =
        [](PSE& emb, vector<double> &runConf){
            Vertex vertex;
            int exp = static_cast<int>(runConf[SimulatedAnnealing::Param::distribExp]);
            vertex = emb.gamma.getRandomVertex(exp);

            // Get the nearest points
            vector<int> neighbours = emb.gamma.getNeighbours(vertex.id);
            vector<int> nearest = emb.nNearestPoints(vertex.occupiedPoint, vertex.deg);

            neighbours.push_back(vertex.id);
            nearest.push_back(vertex.occupiedPoint);

            // Shuffle the nearest points
            shuffle(nearest.begin(), nearest.end(), default_random_engine(randPercent.pull()));

            // Allow points in a far distance
            double probFar = runConf[SimulatedAnnealing::Param::chooseFar];
            for(int i=0; i<neighbours.size(); i++) {
                if(randPercent.pull() < (probFar * 100))
                    emb.trackedMoveOrSwap(neighbours[i], emb.getRandomPoint().id);
                else emb.trackedMoveOrSwap(neighbours[i], nearest[i]);
            }

            return emb;
        };

// NOT CONSIDERED IN THE THESIS
inline auto hybrid =
        [](PSE& emb, vector<double> &runConf){
            // Execute random walk once the "switch" is triggered
            if(static_cast<int>(runConf[SimulatedAnnealing::Param::nextMethod]) == 1)
                return randomWalk(emb, runConf);

            // Trigger "switch" if the last improvement was before two minutes ago
            double lastImp = runConf[SimulatedAnnealing::Param::lastImp];
            double tolerance = ceil(120 / runConf[SimulatedAnnealing::Param::loopTime]);
            runConf[SimulatedAnnealing::Param::nextMethod] = (lastImp < tolerance) ? 0.0 : 1.0;

            // Execute rebuild neighbourhood
            return rebuildNeighbourhood(emb, runConf);
        };
#endif
