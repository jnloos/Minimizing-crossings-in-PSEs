#ifndef PROJECT_ALL_HEADERS_H
#define PROJECT_ALL_HEADERS_H

#include <utility>
#include <unordered_set>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include <set>
#include <queue>
#include <random>
#include <mutex>
#include <thread>
#include <cstdio>
#include <regex>
#include <algorithm>

#include "external/nlohmann/json.hpp"
#include "external/cxxopts/cxxopts.hpp"

using namespace std;

inline mutex console;

inline constexpr double EPS = 1e-9;

#include "Helpers.h"
#include "Drawing.h"
#include "Embedding.h"
#include "InputOutput.h"
#include "Strategy.h"

inline auto randPercent = NumRandomizer(0, 99);

#endif