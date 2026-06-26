#pragma once

#include <string>

using namespace std;

enum class CardinalPoint {
    East, West, North, South
};

class CardinalPointUtils {
public:
    static string toString(CardinalPoint cardinalPoint) {
        switch (cardinalPoint) {
            case CardinalPoint::East:
                return "東";
            case CardinalPoint::West:
                return "西";
            case CardinalPoint::North:
                return "北";
            case CardinalPoint::South:
                return "南";
        }
    }
};