#ifndef PROJECT_STRATEGY_H
#define PROJECT_STRATEGY_H

using namespace std;
using namespace nlohmann;
using namespace chrono;
namespace fs = std::filesystem;

class PSE;
class Executor;


class Strategy {
public:
    static string confDir;

    explicit Strategy() : conf({}) { }
    virtual ~Strategy() = default;

    /**
     * Runs the strategy on the given PSE. An executor parameter provides helper functions.
     * This method should be overridden by derived classes to apply specific algorithms.
     * @param emb PSE to be processed.
     * @param exec Helping Executor.
     */
    virtual PSE run(Executor &exec);

protected:
    json conf;

    /**
     * Imports configurations from a specified JSON file.
     * @param file Name of the configuration file.
     * @throws runtime_error if the file cannot be opened.
     */
    void importConfig(const string& file) {
        ifstream inputFile(confDir + file);
        if (!inputFile.is_open())
            throw runtime_error("File is not existing.");

        stringstream buffer;
        buffer << inputFile.rdbuf();
        string fileContent = buffer.str();
        inputFile.close();

        conf = json::parse(buffer);
    }
};

// Usually overwritten by main
string Strategy::confDir = "../config/";


class Executor {
public:
    string name;

    PSE emb;
    InputOutput IO;

    // Maximal time consumption in minutes
    long maxTime = 50;

    // Current iteration
    long cIter = 0;

    // Timestamp of initialization
    time_point<high_resolution_clock> initTime;

    /**
     * @param filePath The path to the initial drawing.
     * @param inputOutput InputOutput for File-IO.
     */
    explicit Executor(const string& filePath, InputOutput inputOutput)
            : initTime(high_resolution_clock::now()), IO(std::move(inputOutput)) {

        emb = IO.load(filePath);
        filesystem::path pathObj(filePath);
        if (pathObj.extension() == ".json")
            name = pathObj.stem().string();
        else name = filePath;
    }

    /**
     * Runs the given strategy on the loaded PSE and saves the results.
     * @param strategy Strategy to run.
     * @return Optimized PSE.
     */
    PSE run(Strategy& strategy) {
        string strategyName = typeid(strategy).name();
        regex const digits("^\\d+");
        strategyName = regex_replace(strategyName, digits, "");

        // Execute and save the results
        emb = strategy.run(*this);
        long const score = emb.score();

        {
            lock_guard guard(console);
            cout  << endl <<"Finished execution of " << strategyName << " for " << name << "." << endl;
            cout << "Time: " << prettyTime(consumed<milliseconds>()) << ", Score: " << score;
            if(cIter > 0)
                cout << ", Executions: " << cIter;
            cout << endl << endl;
        }

        // Only keep the final drawing
        save(score, emb, 0);
        return emb;
    }

    /**
     * Saves an interim version of the PSE.
     * Keeps the specified number of best results so far and removes the others.
     * @param score Current score.
     * @param toSave PSE to save.
     * @param keepOld Number of interim results to keep.
     */
    void save(long const &score, PSE &toSave, int keepOld) {
        // Remove the worst file (usually only one iteration)
        while(interimScores.size() > keepOld) {
            try {
                long const delScore = interimScores.top();
                string const delFile = name + "-" + (to_string(delScore) + ".json");
                filesystem::remove(IO.outputDir + "/" + delFile);
                interimScores.pop();
            }  catch (const filesystem::filesystem_error& e) {}
        }

        interimScores.emplace(score);
        IO.save(toSave,  name + "-" + (to_string(score)) + ".json");
    }

    [[nodiscard]] bool inTime() const { return consumed<minutes>() < maxTime; }

    /**
     * Calculates the time consumed since the initialization.
     * @tparam durationType e.g., milliseconds, seconds, minutes
     */
    template<typename durationType>
    [[nodiscard]] long consumed() const {
        auto currentTime = high_resolution_clock::now();
        return duration_cast<durationType>(currentTime - initTime).count();
    }

protected:
    priority_queue<long> interimScores;
};

PSE Strategy::run(Executor &exec) {
    return exec.emb;
}

#endif
