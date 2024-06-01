#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxKinect.h"
#include "ofxGui.h"
#include "ofxCvBlob.h"

#include <vector>
#include <cmath>
#include <chrono>

class Circle {
public:
    Circle(float x_, float y_, float radius_, ofColor color_, int expectedAmount_) : x(x_), y(y_), radius(radius_), color(color_), expectedAmount(expectedAmount_) {}

    float x;
    float y;
    float radius;
    ofColor color;
    int expectedAmount;
    int currentAmount = 0;
};

class ofApp : public ofBaseApp {
public:

    void setup();
    void update();
    void draw();
    void exit();

    void keyPressed(int key);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void writeToFile(int score);
    int getHighScoreFromFile();

    void setupKinect();
    void setupAssets();
    void setupMainMenu();
    void setupEndScreen();
    void startGame();
    void updateEndScreen();
    void updateMainMenu();
    void updateCircles();
    void updateKinect();
    void updateContours();
    void drawKinectImages();
    void drawGameLoop();
    void drawMainMenu();
    void drawEndScreen();
    void drawCircles();
    std::vector<std::vector<float>> findBlobs();
    ofColor generateRandomColor(float minBrightness, float maxBrightness);
    bool isPointInCircle(double x, double y, double x_center, double y_center, double radius);
    void setupNewRound();

    //fonts
    ofTrueTypeFont title;
    ofTrueTypeFont font;
    ofTrueTypeFont headerFont;

    //sounds
    ofSoundPlayer correct;
    ofSoundPlayer incorrect;
    ofSoundPlayer background;
    ofSoundPlayer outro;


    //circles
    vector<Circle> circles;

    enum gameStateEnum
    {
        mainMenu = 0,
        gameLoop = 1,
        endScreen = 2
    };
    gameStateEnum gameState = mainMenu;


    ofxKinect kinect;

    ofxCvColorImage colorImg;

    ofxCvGrayscaleImage grayImage; // grayscale depth image
    ofxCvGrayscaleImage grayThreshNear; // the near thresholded image
    ofxCvGrayscaleImage grayThreshFar; // the far thresholded image

    ofxCvContourFinder contourFinder;

    bool bThreshWithOpenCV;
    int angle;

    // used for viewing the point cloud
    ofEasyCam easyCam;

};
