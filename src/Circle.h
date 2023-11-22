#ifndef Circle_h
#define Circle_h


#endif /* Circle_h */

class Circle {
public:
    Circle(float x, float y, float radius, ofColor color)
        : x(x), y(y), radius(radius), color(color) {}
    
    float x;
    float y;
    float radius;
    ofColor color;
};
