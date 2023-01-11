#include <math.h>
#include "util.h"

int32_t util::computeDistance(Point p1, Point p2) {
    return (int32_t)ceil(sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2)));
}