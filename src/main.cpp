#include "source/dependencies.h"

// TODO: Add all dependencies here!
#include "strategies/Bruteforce.h"
#include "strategies/ForceDirected.h"
#include "strategies/Greedy.h"
#include "strategies/SimulatedAnnealing.h"
#include "strategies/Analysis.h"

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

void process(Executor &exec, const cxxopts::ParseResult& opt) {
    exec.maxTime = opt["time"].as<int>();

    // TODO: Add all supported strategies here!
    unordered_map<string, function<unique_ptr<Strategy>()>> algos = {
        {"bruteforce", []() { return make_unique<Bruteforce>(); }},
        {"fda[fr]", []() { return make_unique<ForceDirected>(repelFR, attractFR, coolFR); }},
        {"fda[spring]", []() { return make_unique<ForceDirected>(repelSpring, attractSpring, coolSpring); }},
        {"greedy", []() { return make_unique<Greedy>(); }},
        {"analysis", []() { return make_unique<Analysis>(); }},
        {"sa[walk]", []() { return make_unique<SimulatedAnnealing>(randomWalk, coolExponential); }},
        {"sa[rebuild]", []() { return make_unique<SimulatedAnnealing>(rebuildNeighbourhood, coolExponential); }},
        {"sa[hybrid]", []() { return make_unique<SimulatedAnnealing>(hybrid, coolExponential); }},
        // {"sa[walk,lin]", []() { return make_unique<SimulatedAnnealing>(randomWalk, coolLinear); }},
        // {"sa[rebuild,lin]", []() { return make_unique<SimulatedAnnealing>(rebuildNeighbourhood, coolLinear); }},
        // {"sa[hybrid,lin]", []() { return make_unique<SimulatedAnnealing>(hybrid, coolLinear); }},
    };

    string seq = opt["strategy"].as<std::string>();
    transform(seq.begin(), seq.end(), seq.begin(), ::tolower);
    vector<string> strategies = split(seq, '+');

    for (string const &strategy : strategies) {
        string trimmed = strategy;
        auto it = algos.find(trimmed);
        if (it != algos.end()) {
            unique_ptr<Strategy> algo = it->second();
            exec.run(*algo);
        }
        else throw runtime_error("Unimplemented strategy recognized.");
    }
}

int main(int argc, char* argv[]) {
    try {
        cxxopts::Options options("Minimizing Crossing in PointSet Embeddings");
        options.add_options()
                ("i,inputPath", "Input directory or file path", cxxopts::value<string>())
                ("o,outputPath", "Output directory", cxxopts::value<string>())
                ("c,configPath", "Config directory", cxxopts::value<string>()->default_value("../config/"))
                ("s,strategy", "Sequence of strategies to be applied (+-seperated)", cxxopts::value<string>())
                ("m,multiple", "Enable multiple file mode", cxxopts::value<bool>()->default_value("false"))
                ("t,time", "Maximal time limit in minutes", cxxopts::value<int>()->default_value("50"))
                ("h,help", "Display help message");

        auto input = options.parse(argc, argv);

        if (input.count("help")) {
            cout << options.help() << endl;
            return 0;
        }

        if (!input.count("inputPath") || !input.count("outputPath") || !input.count("strategy")) {
            cout << options.help() << endl;
            return 1;
        }

        auto pathIn = input["inputPath"].as<string>();
        auto pathOut = input["outputPath"].as<string>();
        auto pathConf = input["configPath"].as<string>();
        bool multipleFiles = input["multiple"].as<bool>();

        pathIn = fs::absolute(pathIn).string();
        pathOut = fs::absolute(pathOut).string();
        Strategy::confDir = fs::absolute(pathConf).string();

        if (multipleFiles) {
            /**
             * In multiple file mode, each algorithm is started in a own thread.
             * The main-thread waits for all child-threads and tries to join them.
             */

            InputOutput IO(pathIn, pathOut);

            vector<fs::directory_entry> entries;
            for (const auto& entry : fs::directory_iterator(pathIn))
                if (entry.path().extension() == ".json")
                    entries.push_back(entry);

            vector<thread> threads;
            for (const auto& entry : entries) {
                string fileName = entry.path().filename().string();
                threads.emplace_back([&, fileName]() {
                    Executor exec(fileName, IO);
                    process(exec, input);
                });
            }

            for (auto& thread : threads)
                if (thread.joinable())
                    thread.join();

        } else {
            /**
             * In single file mode, the input path must be interpreted as file path.
             * The algorithms execution runs in the main-thread.
             */

            fs::path filePath(pathIn);
            string fileName = filePath.filename().string();
            string fileDir = filePath.parent_path().string();
            InputOutput IO(fileDir, pathOut);
            Executor exec(fileName, IO);
            process(exec, input);
        }

        cout << endl << "All threads terminated successfully." << endl << endl;
    }

    catch (const cxxopts::exceptions::parsing &e) {
        cerr << "Error parsing options: " << e.what() << endl;
        return 1;
    }
    return 0;
}
