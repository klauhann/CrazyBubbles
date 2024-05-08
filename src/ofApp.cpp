#include "ofApp.h"
#include <iostream>


//setup
int amountOfPlayers = 4;
const int roundAmount = 2; 
const int roundTime = 4; //in seconds

const int nearThreshold = 205;
const int farThreshold = 175;
const float minBlobSize = 800;
const float maxBlobSize = 20000.0;
const bool drawKinect = false;
const bool noKinect = true; //set this to true if testing without a kinect (and test with mouse clicks)

//global variables
int amountCorrect = 0;
int frame = 0;
std::chrono::seconds duration(roundTime);
auto startTime = std::chrono::steady_clock::now();

int score = 0;
bool newRound = false;
int numberOfPeople;
int amountOfCircles = 0;
int rounds = 1;

float myMouseX = -1;
float myMouseY = -1;

//--------------------------------------------------------------
void ofApp::setup() {
    gameState = gameLoop;
    ofSetLogLevel(OF_LOG_VERBOSE);

    setupKinect();
    setupAssets();
    startGame();
}

void ofApp::startGame() {
    newRound = true;
    if (noKinect) {
        amountOfPlayers = 1;
    }
    numberOfPeople = amountOfPlayers;

}

void ofApp::setupKinect() {
    // enable depth->video image calibration
    kinect.setRegistration(true);
    kinect.init();
    kinect.open();

    // print the intrinsic IR sensor values
    if (kinect.isConnected()) {
        ofLogNotice() << "sensor-emitter dist: " << kinect.getSensorEmitterDistance() << "cm";
        ofLogNotice() << "sensor-camera dist:  " << kinect.getSensorCameraDistance() << "cm";
        ofLogNotice() << "zero plane pixel size: " << kinect.getZeroPlanePixelSize() << "mm";
        ofLogNotice() << "zero plane dist: " << kinect.getZeroPlaneDistance() << "mm";
    }

    colorImg.allocate(kinect.width, kinect.height);
    grayImage.allocate(kinect.width, kinect.height);
    grayThreshNear.allocate(kinect.width, kinect.height);
    grayThreshFar.allocate(kinect.width, kinect.height);

    ofSetFrameRate(60);

    // zero the tilt on startup
    angle = 0;
    kinect.setCameraTiltAngle(angle);
}

void ofApp::setupAssets() {
    //load assets
    font.load("impact.ttf", 50);
    headerFont.load("impact.ttf", 100);

    correct.load("correct.wav");
    correct.setLoop(false);
    incorrect.load("incorrect2.mp3");
    incorrect.setLoop(false);
    outro.load("outro.wav");
    outro.setLoop(false);
    background.load("background.wav");
    background.setLoop(true);
    background.play();

    ofSeedRandom();
}

//--------------------------------------------------------------
void ofApp::update() {
    if (gameState == gameLoop) {
        ofApp::updateCircles();
        ofApp::updateKinect();
        ofApp::updateContours();
    }
}

void ofApp::updateContours() {
    if (noKinect) {
        contourFinder.nBlobs = 1;
    }
    for (int i = 0; i < contourFinder.nBlobs; i++) {
        vector<float> coordinates = ofApp::findBlobs(i);

        if (coordinates.at(0) >= 0) {
            for (Circle& circle : circles) {
                float xStartCircle = circle.x - circle.radius;
                float yStartCircle = circle.y - circle.radius;
                float xEndCircle = circle.x + circle.radius;
                float yEndCircle = circle.y + circle.radius;

                if (isPointInCircle(coordinates.at(0), coordinates.at(1), circle.x, circle.y, circle.radius)) {
                    circle.currentAmount++;
                    if (circle.expectedAmount == circle.currentAmount) {
                        amountCorrect++;
                    }
                }
            }
        }
    }
}

bool ofApp::isPointInCircle(double x, double y, double x_center, double y_center, double radius) {
    double distance = sqrt(pow(x - x_center, 2) + pow(y - y_center, 2));
    return distance <= radius;
}

void ofApp::updateKinect() {
    kinect.update();

    // there is a new frame and we are connected
    if (kinect.isFrameNew()) {

        // load grayscale depth image from the kinect source
        grayImage.setFromPixels(kinect.getDepthPixels());

        // we do two thresholds - one for the far plane and one for the near plane
        // we then do a cvAnd to get the pixels which are a union of the two thresholds
        grayThreshNear = grayImage;
        grayThreshFar = grayImage;
        grayThreshNear.threshold(nearThreshold, true);
        grayThreshFar.threshold(farThreshold);
        cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);

        // update the cv images
        grayImage.flagImageChanged();

        // find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
        // also, find holes is set to true so we will get interior contours as well....
        contourFinder.findContours(grayImage, 10, (kinect.width * kinect.height) / 2, 20, false);
    }
}

void ofApp::updateCircles() {
    if (newRound) {
        newRound = false;
        while (numberOfPeople > 0) {
            int thisCircle = ofRandom(1, numberOfPeople + 1);
            float new_radius = ofRandom(150 + 100 * thisCircle, 200 + 100 * thisCircle);
            float new_x = ofRandom(new_radius + 20, ofGetWidth() - (new_radius + 20));
            float new_y = ofRandom(new_radius + 20, ofGetHeight() - (new_radius + 20));
            ofColor randomColor(ofRandom(100, 255), ofRandom(100, 255), ofRandom(100, 255));

            bool overlaps = false;

            for (const Circle& circle : circles) {
                float distance = ofDist(new_x, new_y, circle.x, circle.y);
                if (distance < new_radius + circle.radius + 20) {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps) {
                numberOfPeople -= thisCircle;
                circles.push_back(Circle(new_x, new_y, new_radius, randomColor, thisCircle));
                amountOfCircles++;
            }
        }
    }
}

std::vector<float> ofApp::findBlobs(int i) {
    if (noKinect) {
        return { myMouseX, myMouseY };
    }
    // Loop through all contours found
    // Get the current contour
    ofxCvBlob blob = contourFinder.blobs[i];
    float blobSize = blob.area;

    if (blobSize >= minBlobSize && blobSize <= maxBlobSize) {
        ofPoint centroid = blob.centroid;

        // Transfrom blobs based on beamer size and angle
        float blobX = centroid.x * (1920 / 640);
        float blobY = centroid.y * (1080 / 480);

        blobX *= 1.4;
        blobY *= 1.55;

        std::vector<float> coordinates = { blobX, blobY };
        return coordinates;
    }
    else {
        std::vector<float> coordinates = { -1, -1 };
        return coordinates;
    }

}

void ofApp::setupNewRound() {
    frame = ofGetFrameNum();
    circles.clear();
    newRound = true;
    amountOfCircles = 0;
    amountCorrect = 0;
    background.setVolume(0.2);
    numberOfPeople = amountOfPlayers;
}

bool outroPlaying = false;
bool scoreWritten = false;
bool newHighscore = false;
//--------------------------------------------------------------
void ofApp::draw() {
    if (gameState == gameLoop) {
        drawGameLoop();
    }
}

void ofApp::drawGameLoop() {
    if (rounds > roundAmount) { // game is over
        if (!outroPlaying) {
            background.stop();
            outro.play();
            outroPlaying = true;
        }
        newRound = false;
        circles.clear();
        ofBackground(0, 0, 100); //blue
        int highscore = getHighScoreFromFile();

        if (!scoreWritten) {
            if (score > highscore) {
                newHighscore = true;
            }
            writeToFile(score);
            scoreWritten = true;
        }

        if (newHighscore) {
            ofSetColor(255);
            headerFont.drawString("New Highscore!", ofGetWidth() / 2 - 450, ofGetHeight() / 2 - 200);
        }
        else {
            font.drawString("Highscore: " + ofToString(highscore), ofGetWidth() / 2 - 225, ofGetHeight() / 2 + 150);
        }

        headerFont.drawString("Score: " + ofToString(score), ofGetWidth() / 2 - 300, ofGetHeight() / 2);
    }
    else {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - startTime).count();

        if (ofGetFrameNum() == frame + 1 && ofGetFrameNum() > 1) { //frame after a new round
            ofSleepMillis(2000);
            background.setVolume(1);
            startTime = std::chrono::steady_clock::now();
            rounds++;
        }
        else if (amountCorrect == amountOfCircles && amountOfCircles != 0) {
            score += 50 * (duration.count() - elapsed_time);
            setupNewRound();
            ofBackground(0, 255, 0);
            correct.play();
        }
        else if (elapsed_time >= duration.count()) {
            setupNewRound();
            ofBackground(255, 0, 0);
            incorrect.play();
        }
        else {
            ofBackground(0, 0, 0);
            ofApp::drawCircles();

            // draw info
            ofSetColor(255, 255, 255);
            font.drawString("Time: " + ofToString(duration.count() - elapsed_time), ofGetWidth() - 300, 100);
            font.drawString("Score: " + ofToString(score), ofGetWidth() - 300, 200);
            font.drawString("Round: " + ofToString(rounds), ofGetWidth() - 300, 300);

            // draw blobs
            for (int i = 0; i < contourFinder.nBlobs; i++) {
                vector<float> blobCoordinates = ofApp::findBlobs(i);

                if (blobCoordinates.at(0) >= 0) {
                    ofColor white(255, 255, 255);
                    ofSetColor(white);
                    ofDrawCircle(blobCoordinates.at(0), blobCoordinates.at(1), 15);
                }
            }
            for (Circle& circle : circles) {
                circle.currentAmount = 0;
            }
            amountCorrect = 0;
        }
        if (noKinect) {
            myMouseX = -1.0;
            myMouseY = -1.0;
        }

        if (drawKinect) {
            void drawKinectImages();
        }
    }
}

void ofApp::drawKinectImages()
{
    ofSetColor(255, 255, 255);
    // draw from the live kinect
    kinect.drawDepth(10, 10, 400, 300);
    kinect.draw(420, 10, 400, 300);

    grayImage.draw(10, 320, 400, 300);
    contourFinder.draw(10, 320, 400, 300);
}

void ofApp::drawCircles() {
    for (const Circle& circle : circles) {
        ofSetColor(circle.color);
        ofDrawCircle(circle.x, circle.y, circle.radius);

        ofSetColor(0);
        font.drawString(ofToString(circle.expectedAmount), circle.x - 15, circle.y + 25);
        //font.drawString(ofToString(circle.currentAmount), circle.x - 15, circle.y - 30);
        //font.drawString(ofToString(amountCorrect), circle.x - 15, circle.y - 80);
    }
}


//--------------------------------------------------------------
void ofApp::exit() {
    kinect.setCameraTiltAngle(0); // zero the tilt on exit
    kinect.close();
}

//--------------------------------------------------------------
void ofApp::writeToFile(int score) {
    std::string filePath = ofToDataPath("scores.txt");
    std::ofstream outputFile(filePath, std::ios::app);

    if (outputFile.is_open()) {
        outputFile << to_string(score) << std::endl;
        outputFile.close();
    }
    else {
        ofLogError() << "Fehler beim Öffnen der Datei zum Schreiben.";
    }
}

int ofApp::getHighScoreFromFile() {
    std::string filePath = ofToDataPath("scores.txt");
    std::ifstream inputFile(filePath);
    int highscore = 0;

    if (inputFile.is_open()) {
        std::string line;
        while (std::getline(inputFile, line)) {
            if (stoi(line) > highscore) {
                highscore = stoi(line);
            }
        }
        inputFile.close();
    }
    else {
        ofLogError() << "Fehler beim Öffnen der Datei zum Lesen.";
    }
    return highscore;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
    myMouseX = x * 1.0;
    myMouseY = y * 1.0;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{

}
