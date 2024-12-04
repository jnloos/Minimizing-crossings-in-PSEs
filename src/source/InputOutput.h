#ifndef PROJECT_INPUT_OUTPUT_H
#define PROJECT_INPUT_OUTPUT_H

#include "dependencies.h"

using namespace std;
using namespace nlohmann;
namespace fs = std::filesystem;

class PSE;


class InputOutput {
public:
    string inputDir;
    string outputDir;

    /**
      * @param input Input directory path.
      * @param output Output directory path.
      * @throws runtime_error if the output directory cannot be created.
      */
    InputOutput(string input, string output)
        : inputDir(std::move(input)), outputDir(std::move(output)) {

        // Exactly one trailing separator is allowed
        while (!inputDir.empty() && inputDir.back() == '/')
            inputDir.pop_back();
        if (!inputDir.empty())
            inputDir += '/';

        // Exactly one trailing separator is allowed
        while (!outputDir.empty() && outputDir.back() == '/')
            outputDir.pop_back();
        if (!outputDir.empty())
            outputDir += '/';

        if (!fs::exists(outputDir))
            if (!fs::create_directories(outputDir))
                throw runtime_error("Failed to create output directory: " + outputDir);
    }

    /**
     * Loads a PSE from a specified file within the input directory.
     * @param name The file or relative dir-path to be loaded.
     * @throws runtime_error if the file cannot be opened.
     */
    PSE load(string name) {
        // Remove all leading separators from the name
        while (!name.empty() && name.front() == fs::path::preferred_separator)
            name.erase(0, 1);
        ifstream inputFile(inputDir + name);

        if (!inputFile.is_open())
            throw runtime_error("File is not existing.");

        stringstream buffer;
        buffer << inputFile.rdbuf();
        string fileContent = buffer.str();
        inputFile.close();

        return parse(fileContent);
    }

    /**
     * Parses a string of data into a PSE. The string must comply with the json-format of the GDC 2024.
     * More information regarding the format: https://mozart.diei.unipg.it/gdcontest/2024/live/.
     * @param data The string data of the PSE.
     * @throws json::parse_error if the JSON data is invalid or the format is invalid.
     */
    PSE parse(string& data) {
        jsonData = json::parse(data);

        vector<Point> points;
        points.resize(jsonData["points"].size());
        for (const auto& point : jsonData["points"])
            points[point["id"]] = {point["id"], point["x"], point["y"]};

        vector<Vertex> vertices;
        vertices.resize(jsonData["nodes"].size());
        for (const auto& vertex : jsonData["nodes"])
            vertices[vertex["id"]] = {vertex["id"], vertex["x"], vertex["y"]};

        vector<Edge> edges;
        for (const auto& edge : jsonData["edges"]) {
            Vertex sourceVertex = vertices[edge["source"]];
            Vertex targetVertex = vertices[edge["target"]];
            edges.emplace_back(edges.size(), sourceVertex, targetVertex);
        }

        int const drawingWidth = jsonData.contains("width") ? jsonData["width"].get<int>() : 1000000;
        int const drawingHeight = jsonData.contains("height") ? jsonData["height"].get<int>() : 1000000;

        auto const inputGraph = Drawing(vertices, edges);
        return PSE{inputGraph, points, drawingWidth, drawingHeight};
    }

    /**
     * Saves a PSE to a specified file within the output directory.
     * @param emb The PSE object to save.
     * @param path The file or relative dir-path to be saved in.
     * @throws runtime_error if the file cannot be opened or deleted.
     */
    void save(PSE& emb, string path) {
        // Remove all leading separators from the name
        while (!path.empty() && path.at(0) == '/')
            path.erase(0, 1);
        string filePath = outputDir + path;

        ifstream file(filePath);
        if (file.is_open()) {
            file.close();
            remove(filePath.c_str());
        }

        ofstream outputFile(filePath);
        if (!outputFile.is_open())
            throw runtime_error("Can not open file: " + filePath);

        outputFile << stringify(emb);
        outputFile.close();
    }

    /**
     * Stringifies the PSE into the for the GDC valid json-format.
     * Only modifications of the vertices' positions are considered.
     * @param emb The PSE object to convert.
     */
    string stringify(PSE& emb) {
        jsonData["nodes"] = {};
        for (Vertex& vertex : emb.gamma.vertices)
            jsonData["nodes"].push_back(vertex.toJson());
        return jsonData.dump(4);
    }

private:
    json jsonData;
};

#endif
