#pragma once

#include <string>
#include "I18n.h"

using namespace std;

enum class CardinalPoint {
    East, West, North, South
};

class CardinalPointUtils {
public:
    static string toString(CardinalPoint cardinalPoint) {
        switch (cardinalPoint) {
            case CardinalPoint::East:
                return I18n::t("dir.east");
            case CardinalPoint::West:
                return I18n::t("dir.west");
            case CardinalPoint::North:
                return I18n::t("dir.north");
            case CardinalPoint::South:
                return I18n::t("dir.south");
        }
    }
};