#ifndef PROJECT_TOOLBOX_H
#define PROJECT_TOOLBOX_H

#include "dependencies.h"

using namespace std;
using namespace chrono;


struct Position {
    double x;
    double y;

    bool operator==(Position const &other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(Position const &other) const {
        return !(*this == other);
    }

    bool operator<(Position const &other) const {
        if(y == other.y)
            return x < other.x;
        return y < other.y;
    }

    friend ostream& operator<<(std::ostream &out, Position const &pos) {
        out << "(" << pos.x << ", " << pos.y << ")";
        return out;
    }
};


class VectorSpace {
public:

    /**
     * Evaluates a cross for two segments.
     * @param aStart The first segment's start.
     * @param aEnd  The first segment's end.
     * @param bStart The second segment's start.
     * @param bEnd The second segment's end.
     * @param pen Penalty value for endpoints on segments.
     */
    static long evalSegments(const Position &aStart, const Position &aEnd, const Position &bStart, const Position &bEnd, int const pen) {
        if(! ((aStart == bStart && aEnd == bEnd) || (aStart == bEnd && aEnd == bStart))){
            // Do the curves share no common endpoint?
            if (dist(aStart, bStart) > EPS && dist(aEnd, bEnd) > EPS &&
                dist(aStart, bEnd) > EPS && dist(aEnd, bStart) > EPS) {

                // Is a endpoint of aSegment on bSegment or vice versa?
                if (onSegment(aStart, bStart, bEnd) || onSegment(aEnd, bStart, bEnd) ||
                    onSegment(bStart, aStart, aEnd) || onSegment(bEnd, aStart, aEnd)) {

                    return pen;
                }

                // Do the segments intersect?
                if(doCross(aStart, aEnd, bStart, bEnd))
                    return 1;
            }
            else {
                // Is the unshared endpoint of aEdge on bEdge or vice versa?
                if (dist(aStart, bStart) < EPS)
                    if (onSegment(aEnd, bStart, bEnd) || onSegment(bEnd, aStart, aEnd))
                        return pen;

                if(dist(aEnd, bStart) < EPS)
                    if (onSegment(aStart, bStart, bEnd) || onSegment(bEnd, aStart, aEnd))
                        return pen;

                if (dist(aStart, bEnd) < EPS)
                    if (onSegment(aEnd, bStart, bEnd) || onSegment(bStart, aStart, aEnd))
                        return pen;

                if(dist(aEnd, bEnd) < EPS)
                    if (onSegment(aStart, bStart, bEnd) || onSegment(bStart, aStart, aEnd))
                        return pen;
            }
        }
        return 0;
    }

    /**
     * Is the position on the segment?
     * @param pos The position.
     * @param start The segment's start.
     * @param end The segment's end.
     */
    static bool onSegment(const Position &pos, const Position &start, const Position &end) {
        // Check x interval
        if (start.x < pos.x && end.x < pos.x)
            return false;
        if (start.x > pos.x && end.x > pos.x)
            return false;

        // Check y interval
        if (start.y < pos.y && end.y < pos.y)
            return false;
        if (start.y > pos.y && end.y > pos.y)
            return false;

        // Vertical line case
        if (start.x == pos.x)
            return end.x == pos.x;

        // Horizontal line case
        if (start.y == pos.y)
            return end.y == pos.y;

        // Match the gradients
        return (start.x - pos.x) * (end.y - pos.y) == (pos.x - end.x) * (pos.y - start.y);
    }

    /**
     * Do the segments cross?
     * @param aStart The first segment's start.
     * @param aEnd  The first segment's end.
     * @param bStart The second segment's start.
     * @param bEnd The second segment's end.
     */
    static bool doCross(const Position &aStart, const Position &aEnd, const Position &bStart, const Position &bEnd) {
        // Check for coinciding vertices
        if ((aStart == bStart && aEnd == bEnd) || (aStart == bEnd && aEnd == bStart))
            return false;

        // Calculate the orientation for points (A, B, C) and (A, B, D)
        double const orientABC = orient(aStart, aEnd, bStart);
        double const orientABD = orient(aStart, aEnd, bEnd);

        // Check if C and D are on the same side of line AB
        if (orientABC == orientABD)
            return false;

        // Calculate the orientation for points (C, D, A) and (C, D, B)
        double const orientCDA = orient(bStart, bEnd, aStart);
        double const orientCDB = orient(bStart, bEnd, aEnd);

        // Check if A and B are on the same side of line CD
        if (orientCDA == orientCDB)
            return false;

        return true;
    }

    static double orient(const Position &aPos, const Position &bPos, const Position &cPos) {
        // Calculates the orientation and returns the signum
        return copysign(1.0, (bPos.x * cPos.y - cPos.x * bPos.y) + (cPos.x * aPos.y - aPos.x * cPos.y) + (aPos.x * bPos.y - bPos.x * aPos.y));
     }

    static double dist(const Position &aPos, const Position &bPos) {
        // Creates vector and returns its length
        double const xDist = aPos.x - bPos.x;
        double const yDist = aPos.y - bPos.y;
        return len(xDist, yDist);
    }

    static double len(double const &xMove, double const &yMove) {
        // Calculates euclidean length of the vector
        double norm = sqrt(xMove * xMove + yMove * yMove);
        if(isnan(norm))
            return numeric_limits<double>::quiet_NaN();
        return norm;
    }
};


class VariationIterator {
public:
    bool hasNext = true;

    /**
     * @param k Size of variations to be pulled.
     * @param n Number of available keys.
     */
    VariationIterator(int const k, int const n)
            : n(n), k(k), selectors(n, 0), variation(k) {

        for (int i = 0; i < k; ++i)
            selectors[i] = 1;
        std::sort(selectors.begin(), selectors.end());
        reselect();
    }

    /**
     * Pulls a variation of k elements from the set [0, (n-1)].
     */
    vector<int> next() {
        vector<int> curr = variation;
        if (!next_permutation(variation.begin(), variation.end()))
            if (!next_permutation(selectors.begin(), selectors.end()))
                hasNext = false;
            else reselect();
        return curr;
    }

private:
    int n;
    int k;

    // Next variation to be pulled.
    vector<int> variation;

    // Entries indicate if keys are used or not.
    vector<int> selectors;

    /**
     * Updates the keys within the recent variation.
     */
    void reselect() {
        int key = 0;
        for (int value = 0; value < n; ++value) {
            if (selectors[value]) {
                variation[key++] = value;
            }
        }
    }
};


template<typename Num>
class NumRandomizer {
private:
    // Thread-local generator and distribution
    thread_local static std::mt19937 gen;
    uniform_int_distribution<Num> distrib;

public:
    NumRandomizer() : distrib(0, 99) {
        initialize_gen();
    }

    /**
     * @param start Start of the range.
     * @param end End of the range.
     */
    NumRandomizer(Num const start, Num const end) : distrib(start, end) {
        initialize_gen();
    }

    /**
     * Returns a random value within the range.
     */
    Num pull() {
        return distrib(gen);
    }

private:
    static void initialize_gen() {
        static thread_local bool initialized = false;
        if (!initialized) {
            gen.seed(std::chrono::system_clock::now().time_since_epoch().count() + std::hash<std::thread::id>{}(std::this_thread::get_id()));
            initialized = true;
        }
    }
};

template<typename Num>
thread_local std::mt19937 NumRandomizer<Num>::gen;


/**
 * Converts a duration to a pretty string.
 * @param ms The duration in ms
 */
inline string prettyTime(long ms) {
    auto const total = ms;
    auto const h = total / 3600000;
    auto const min = (total % 3600000) / 60000;
    auto const sec = (total % 60000) / 1000;
    ms = total % 1000;

    ostringstream oss;
    oss << setw(2) << setfill('0') << h << "h:"
        << setw(2) << setfill('0') << min << "m:"
        << setw(2) << setfill('0') << sec << "s:"
        << setw(3) << setfill('0') << ms << "ms";

    return oss.str();
}

/**
 * Splits a string into a vector of substrings.
 * @param str String to split
 * @param delimiter Delimiter
 */
inline vector<string> split(const string& str, char const delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string item;
    while (getline(ss, item, delimiter))
        tokens.push_back(item);
    return tokens;
}

#endif
