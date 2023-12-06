#include "ofApp.h"

#include <iostream>


float blobX;
float blobY;
int amountCorrect = 0;
const int amountOfPlayer = 4;
int frame = 0;
std::chrono::seconds duration(7);
auto start_time = std::chrono::steady_clock::now();
int score = 0;
bool newRound = true;
int numberOfPeople;
int amountOfCircles = 0;
int rounds = 1;

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetLogLevel(OF_LOG_VERBOSE);

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

    nearThreshold = 205;
    farThreshold = 175;
    bThreshWithOpenCV = true;

    ofSetFrameRate(60);

    // zero the tilt on startup
    angle = 0;
    kinect.setCameraTiltAngle(angle);

    // start from the front
    bDrawPointCloud = false;

    font.load("impact.ttf", 50);
    headerFont.load("impact.ttf", 100);
    ofSeedRandom();

    numberOfPeople = amountOfPlayer;
}

//--------------------------------------------------------------
void ofApp::update() {
    ofApp::updateCircles();
    ofApp::updateKinect();
    ofApp::updateContours();
}

void ofApp::updateContours() {

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
        if (bThreshWithOpenCV) {
            grayThreshNear = grayImage;
            grayThreshFar = grayImage;
            grayThreshNear.threshold(nearThreshold, true);
            grayThreshFar.threshold(farThreshold);
            cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);
        }
        else {

            // or we do it ourselves - show people how they can work with the pixels
            ofPixels& pix = grayImage.getPixels();
            int numPixels = pix.size();
            for (int i = 0; i < numPixels; i++) {
                if (pix[i] < nearThreshold && pix[i] > farThreshold) {
                    pix[i] = 255;
                }
                else {
                    pix[i] = 0;
                }
            }
        }

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
            //int numberOfPeople = 1;
            if (!overlaps) {
                numberOfPeople -= thisCircle;

                circles.push_back(Circle(new_x, new_y, new_radius, randomColor, thisCircle));
                start_time = std::chrono::steady_clock::now();
                amountOfCircles++;
            }
        }
    }


}

std::vector<float> ofApp::findBlobs(int i) {
    float minBlobSize = 800;
    float maxBlobSize = 20000.0;

    // Loop through all contours found
    // Get the current contour
    ofxCvBlob blob = contourFinder.blobs[i];

    float blobSize = blob.area;

    if (blobSize >= minBlobSize && blobSize <= maxBlobSize) {
        ofPoint centroid = blob.centroid;

        // Greifen Sie auf die Koordinaten des j-ten Punktes der Kontur zu
        blobX = centroid.x * (1920 / 640);
        blobY = centroid.y * (1080 / 480);

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

//--------------------------------------------------------------
void ofApp::draw() {
    if (rounds > 10) {
        ofSleepMillis(2000);
        newRound = false;
        circles.clear();
        ofBackground(0, 0, 100);
        headerFont.drawString("Score: " + ofToString(score), ofGetWidth() / 2 - 300, ofGetHeight() / 2);

    }
    else {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();

        if (amountCorrect == amountOfCircles && amountOfCircles != 0) {
            score += 10 * (duration.count() - elapsed_time);
            ofColor col;
            col.set(0, 255, 0);
            ofClear(col);
            ofBackground(0, 255, 0);
            frame = ofGetFrameNum();
            circles.clear();
            newRound = true;
            numberOfPeople = amountOfPlayer;
            amountOfCircles = 0;
            rounds++;
        }
        else if (ofGetFrameNum() == frame + 1) {
            ofSleepMillis(2000);

        }
        else if (elapsed_time >= duration.count()) {
            frame = ofGetFrameNum();
            circles.clear();
            ofBackground(255, 0, 0);
            newRound = true;
            numberOfPeople = amountOfPlayer;
            amountOfCircles = 0;
            rounds++;
        }
        else {
            ofBackground(0, 0, 0);
        }


        ofApp::drawCircles();

        ofSetColor(255, 255, 255);
        font.drawString("Time: " + ofToString(duration.count() - elapsed_time), ofGetWidth() - 300, 100);
        font.drawString("Score: " + ofToString(score), ofGetWidth() - 300, 200);
        font.drawString("Round: " + ofToString(rounds), ofGetWidth() - 300, 300);

        for (int i = 0; i < contourFinder.nBlobs; i++) {
            vector<float> blobCoordinates = ofApp::findBlobs(i);

            if (blobCoordinates.at(0) >= 0) {
                ofColor white(255, 255, 255);
                ofSetColor(white);
                ofDrawCircle(blobCoordinates.at(0), blobCoordinates.at(1), 15);
            }
        }

        ofSetColor(255, 255, 255);

        if (bDrawPointCloud) {
            easyCam.begin();
            drawPointCloud();
            easyCam.end();
        }
        else {
            // draw from the live kinect
            kinect.drawDepth(10, 10, 400, 300);
            kinect.draw(420, 10, 400, 300);

            grayImage.draw(10, 320, 400, 300);
            contourFinder.draw(10, 320, 400, 300);
        }

        for (Circle& circle : circles) {
            circle.currentAmount = 0;
        }
        amountCorrect = 0;
    }
}

void ofApp::drawPointCloud() {
    int w = 640;
    int h = 480;
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_POINTS);
    int step = 2;
    for (int y = 0; y < h; y += step) {
        for (int x = 0; x < w; x += step) {
            if (kinect.getDistanceAt(x, y) > 0) {
                mesh.addColor(kinect.getColorAt(x, y));
                mesh.addVertex(kinect.getWorldCoordinateAt(x, y));
            }
        }
    }
    glPointSize(3);
    ofPushMatrix();
    // the projected points are 'upside down' and 'backwards' 
    ofScale(1, -1, -1);
    ofTranslate(0, 0, -1000); // center the points a bit
    ofEnableDepthTest();
    mesh.drawVertices();
    ofDisableDepthTest();
    ofPopMatrix();
}

void ofApp::drawCircles() {
    for (const Circle& circle : circles) {
        ofSetColor(circle.color);
        ofDrawCircle(circle.x, circle.y, circle.radius);

        ofSetColor(0);
        font.drawString(ofToString(circle.expectedAmount), circle.x - 15, circle.y + 25);
        font.drawString(ofToString(circle.currentAmount), circle.x - 15, circle.y - 30);
        font.drawString(ofToString(amountCorrect), circle.x - 15, circle.y - 80);

    }
}


//--------------------------------------------------------------
void ofApp::exit() {
    kinect.setCameraTiltAngle(0); // zero the tilt on exit
    kinect.close();
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
