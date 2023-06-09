#include "yogatest.hpp"
#include "yoga/Yoga.h"
#include "Arduino.h"

void yoga_test_main() {
    auto a = micros();
    auto root = YGNodeNew();
    YGNodeStyleSetWidth(root, 320);
    YGNodeStyleSetHeight(root, 240);
    YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
    YGNodeStyleSetJustifyContent(root, YGJustifySpaceBetween);
    YGNodeStyleSetAlignItems(root, YGAlignStretch);

    auto child1 = YGNodeNew();
    YGNodeStyleSetWidth(child1, 100);
    YGNodeStyleSetHeight(child1, 100);
    YGNodeStyleSetMargin(child1, YGEdgeAll, 10);
    YGNodeStyleSetPadding(child1, YGEdgeAll, 10);
    YGNodeStyleSetBorder(child1, YGEdgeAll, 5);

    auto child2 = YGNodeNew();
    YGNodeStyleSetWidth(child2, 100);
    
    YGNodeInsertChild(root, child1, 0);
    YGNodeInsertChild(root, child2, 1);

    YGNodeCalculateLayout(root, 320, 240, YGDirectionLTR);
    auto b = micros();

    Serial.printf("child1: %f, %f, %f, %f\n", YGNodeLayoutGetLeft(child1), YGNodeLayoutGetTop(child1), YGNodeLayoutGetWidth(child1), YGNodeLayoutGetHeight(child1));
    Serial.printf("child2: %f, %f, %f, %f\n", YGNodeLayoutGetLeft(child2), YGNodeLayoutGetTop(child2), YGNodeLayoutGetWidth(child2), YGNodeLayoutGetHeight(child2));
    Serial.printf("time: %d\n", b - a);
}