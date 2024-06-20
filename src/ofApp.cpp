#include "ofApp.h"
#include <iostream>

// setup
int amountOfPlayers = 4;
const int roundAmount = 5;
const int roundTime = 3; // in seconds
int waitTime = 2;       // time to stay in circle before game starts (in frames)

const bool drawKinect = true;
const bool noKinect = false; // set this to true if testing without a kinect (and test with mouse clicks)

// global variables
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

int framesInCircle = 0;

int highscore = -1;

//--------------------------------------------------------------
void ofApp::setup()
{
    ofLog() << "Setup";
    if (noKinect)
    {
        waitTime = 3;
        amountOfPlayers = 1;
    }
    ofSetLogLevel(OF_LOG_VERBOSE);

    setupKinect();
    setupGui();
    setupAssets();
    setupMainMenu();
    ofLog() << "Setup Complete" << endl;
}

void ofApp::startGame()
{
    circles.clear();
    amountCorrect = 0;
    frame = 0;
    score = 0;
    amountOfCircles = 0;
    rounds = 1;
    numberOfPeople = amountOfPlayers;
    gameState = gameLoop;
    newRound = true;
    startTime = std::chrono::steady_clock::now();
}

void ofApp::setupKinect()
{
    // enable depth->video image calibration
    kinect.setRegistration(true);
    kinect.init();
    kinect.open();

    // print the intrinsic IR sensor values
    if (kinect.isConnected())
    {
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

void ofApp::setupGui()
{
    gui.setup();
    gui.add(nearThreshold.setup("Near Threshold", 0, 0, 255));
    gui.add(farThreshold.setup("Far Threshold", 255, 0, 255));
    gui.add(minBlobSize.setup("Min Blob Size", 0, 0, 76800));
    gui.add(maxBlobSize.setup("Max Blob Size", 76800, 0, 76800));

    gui.add(translateX.setup("Translate X", 0.0, -1.0 * ofGetWidth() / 2, ofGetWidth() / 2));
    gui.add(translateY.setup("Translate Y", 0.0, -1.0 * ofGetHeight() / 2, ofGetHeight() / 2));
    gui.add(rotateAngle.setup("Rotation Angle", 0.0, -180.0, 180.0));
    gui.add(scaleX.setup("Scale X", 1.0, 0.5, 2.0));
    gui.add(scaleY.setup("Scale Y", 1.0, 0.5, 2.0));

    nearThreshold.setSize(500, 50);
    farThreshold.setSize(500, 50);
    minBlobSize.setSize(500, 50);
    maxBlobSize.setSize(500, 50);
    translateX.setSize(500, 50);
    translateY.setSize(500, 50);
    rotateAngle.setSize(500, 50);
    scaleX.setSize(500, 50);
    scaleY.setSize(500, 50);

    gui.setSize(600, 500);
    ofxGuiSetFont("assets/impact.ttf", 20);
    gui.loadFromFile("kinect_settings.json");

}

void ofApp::setupAssets()
{
    // load assets
    title.load("assets/RammettoOne.ttf", 110);
    font.load("assets/impact.ttf", 50);
    headerFont.load("assets/RammettoOne.ttf", 80);

    correct.load("assets/correct.wav");
    correct.setLoop(false);
    incorrect.load("assets/incorrect2.mp3");
    incorrect.setLoop(false);
    outro.load("assets/outro.wav");
    outro.setLoop(false);
    background.load("assets/background.wav");
    background.setLoop(true);
    background.play();

    ofSeedRandom();
}

void ofApp::setupMainMenu()
{
    background.setVolume(1);
    framesInCircle = 0;
    gameState = mainMenu;
    circles.clear();
    ofColor bigCircleColor = generateRandomColor(100, 200);
    circles.push_back(Circle(ofGetWidth() / 2, ofGetHeight() / 2 - 100, 400, bigCircleColor, -2));

    int numCircles = 8;
    float circleDiameter = 200;
    float spacing = 50;
    float totalWidth = numCircles * circleDiameter + (numCircles - 3) * spacing;
    float startX = (ofGetWidth() - totalWidth) / 2;

    for (int i = 0; i < numCircles; i++) {
        ofColor randomColor = generateRandomColor(200, 255);
        float x = startX + i * (circleDiameter + spacing);
        float y = ofGetHeight() - 200;
        circles.push_back(Circle(x, y, circleDiameter / 2, randomColor, i + 1));
    }
}

void ofApp::setupEndScreen()
{
    background.setVolume(0.2);
    outro.play();
    framesInCircle = 0;
    gameState = endScreen;
    circles.clear();
    ofColor randomColor = generateRandomColor(100, 200);
    circles.push_back(Circle(ofGetWidth() / 2, ofGetHeight() - 650, 360, randomColor, -3));
    highscore = getHighScoreFromFile();
    if (score > highscore)
    {
        highscore = -1;
    }
    writeToFile(score);
}
//--------------------------------------------------------------
void ofApp::update()
{
    updateKinect();
    if (gameState == gameLoop)
    {
        //ofLog() << "update game loop";
        ofApp::updateCircles();
        ofApp::updateContours();
        //ofLog() << "update complete";
    }
    else if (gameState == mainMenu)
    {
        //ofLog() << "update main menu";
        ofApp::updateMainMenu();
        //ofLog() << "update complete";
    }
    else if (gameState == endScreen)
    {
        //ofLog() << "update end screen";
        ofApp::updateEndScreen();
        //ofLog() << "update complete";
    }
    myMouseX = -1;
    myMouseY = -1;
}

void ofApp::updateEndScreen()
{
    if (circles.size() > 0)
    {
        if (isPointInCircle(myMouseX, myMouseY, circles[0].x, circles[0].y, circles[0].radius) == true)
        {
            framesInCircle++;
            if (framesInCircle >= waitTime)
            {
                setupMainMenu();
            }
        }
    }
    
}

void ofApp::updateMainMenu()
{
    if (circles.size() > 0)
    {
        for (int j = 1; j < circles.size(); j++)
        {
            if (isPointInCircle(myMouseX, myMouseY, circles[j].x, circles[j].y, circles[j].radius) == true)
            {
                amountOfPlayers = circles[j].expectedAmount;
                ofLog() << "amountOfPlayers: " << amountOfPlayers;
            }
        }

        if (isPointInCircle(myMouseX, myMouseY, circles[0].x, circles[0].y, circles[0].radius) == true)
        {
            framesInCircle++;
            if (framesInCircle >= waitTime)
            {
                ofLog() << "Starting game...";
                startGame();
            }
         }
    }
}

void ofApp::updateContours()
{
    auto blobs = ofApp::findBlobs();
    for (int i = 0; i < blobs.size(); i++)
    {
        vector<float> coordinates = blobs[i];
        if (coordinates.at(0) >= 0)
        {
            for (Circle &circle : circles)
            {
                if (isPointInCircle(coordinates.at(0), coordinates.at(1), circle.x, circle.y, circle.radius))
                {
                    circle.currentAmount++;
                    if (circle.expectedAmount == circle.currentAmount)
                    {
                        amountCorrect++;
                    }
                }
            }
        }
    }
}

bool ofApp::isPointInCircle(double x, double y, double x_center, double y_center, double radius)
{
    double distance = sqrt(pow(x - x_center, 2) + pow(y - y_center, 2));
    return distance <= radius;
}

void ofApp::updateKinect()
{
    kinect.update();

    // there is a new frame and we are connected
    if (kinect.isFrameNew())
    {

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
        contourFinder.findContours(grayImage, minBlobSize, maxBlobSize, 10, false);
    }
}

void ofApp::updateCircles()
{
    if (newRound)
    {
        newRound = false;
        while (numberOfPeople > 0)
        {
            int thisCircle = ofRandom(1, numberOfPeople + 1);
            float new_radius = ofRandom(150 + 100 * thisCircle, 200 + 100 * thisCircle);
            float new_x = ofRandom(new_radius + 20, ofGetWidth() - (new_radius + 20));
            float new_y = ofRandom(new_radius + 20, ofGetHeight() - (new_radius + 20));
            ofColor randomColor = generateRandomColor(200, 255);

            bool overlaps = false;

            for (const Circle &circle : circles)
            {
                float distance = ofDist(new_x, new_y, circle.x, circle.y);
                if (distance < new_radius + circle.radius + 20)
                {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps)
            {
                numberOfPeople -= thisCircle;
                circles.push_back(Circle(new_x, new_y, new_radius, randomColor, thisCircle));
            }
        }
        amountOfCircles = circles.size();
    }
}

std::vector<vector<float>> ofApp::findBlobs()
{
    vector<vector<float>> blobs = {};

    blobs.push_back({ myMouseX, myMouseY });

    if (noKinect)
    {
        contourFinder.nBlobs = 0;
    }
    // Loop through all contours found
    // Get the current contour
    if (contourFinder.nBlobs > 0) {
        //ofLog() << "Amount of Blobs found: " << std::to_string(contourFinder.nBlobs);
        for (int i = 0; i < contourFinder.nBlobs; i++)
        {
            ofxCvBlob blob = contourFinder.blobs[i];
            float blobSize = blob.area;

            if (blobSize >= minBlobSize && blobSize <= maxBlobSize)
            {
                ofPoint centroid = blob.centroid;

                // Transfrom blobs based on beamer size and angle
                float blobX = centroid.x * (1920 / 640);
                float blobY = centroid.y * (1080 / 480);

                blobX *= scaleX;
                blobY *= scaleY;


                blobX += translateX;
                blobY += translateY;

                float finalBlobX = blobX * cos(ofDegToRad(rotateAngle)) - blobY * sin(ofDegToRad(rotateAngle));
                float finalBlobY = blobX * sin(ofDegToRad(rotateAngle)) + blobY * cos(ofDegToRad(rotateAngle));


                blobs.push_back({ finalBlobX, finalBlobY });
            }
        }
    }

    /*ofLog() << "blobs size: " << blobs.size();
    for (int i = 0; i < blobs.size(); i++)
    {
        ofLog() << "blob " << i << ": x = " << blobs[i].at(0) << ", y = " << blobs[i].at(1);
    }*/
    return blobs;
}

ofColor ofApp::generateRandomColor(float minBrightness, float maxBrightness) {
    float hue = ofRandom(0, 255);
    float saturation = ofRandom(100, 255);
    float brightness = ofRandom(minBrightness, maxBrightness);
    return ofColor::fromHsb(hue, saturation, brightness);
}

void ofApp::setupNewRound()
{
    frame = ofGetFrameNum();
    circles.clear();
    newRound = true;
    amountOfCircles = 0;
    amountCorrect = 0;
    background.setVolume(0.2);
    numberOfPeople = amountOfPlayers;
    highscore = -1;
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofBackground(0, 0, 0);

    if (gameState == gameLoop)
    {
        //ofLog() << "draw game loop";
        drawGameLoop();
        //ofLog() << "done";
    }
    else if (gameState == mainMenu)
    {
        //ofLog() << "draw main menu";
        drawMainMenu();
        //ofLog() << "done";
    }
    else if (gameState == endScreen)
    {
        //ofLog() << "draw end screen";
        drawEndScreen();
        //ofLog() << "done";
    }

    drawBlobs();
    if (drawKinect) {
        drawKinectImages();
        gui.draw();
    }
}

void ofApp::drawEndScreen()
{
    newRound = false;
    std::string highscoreText = "";

    if (highscore == -1)
    {
        highscoreText = "New Highscore!";
    }
    else
    {
        highscoreText = "Highscore: " + ofToString(highscore);
    }

    ofSetColor(255);

    std::string scoreText = "Your score: " + ofToString(score);
    ofRectangle boundingBox = title.getStringBoundingBox(scoreText, 0, 0);
    title.drawString(scoreText, (ofGetWidth() - boundingBox.width) / 2, 500);

    boundingBox = headerFont.getStringBoundingBox(highscoreText, 0, 0);
    headerFont.drawString(highscoreText, (ofGetWidth()-boundingBox.width) / 2, 700);
    drawCircles();
}

void ofApp::drawMainMenu()
{
    ofSetColor(255, 255, 255);
    std::string text = "Welcome to Crazy Bubbles!";
    ofRectangle boundingBox = title.getStringBoundingBox(text, 0, 0);
    title.drawString(text, (ofGetWidth() - boundingBox.width) / 2, 250);

    text = "Amount of players: " + ofToString(amountOfPlayers);
    boundingBox = font.getStringBoundingBox(text, 0, 0);
    font.drawString(text, (ofGetWidth() - boundingBox.width) / 2, ofGetHeight() - 400);
    drawCircles();
}

void ofApp::drawGameLoop()
{
    if (rounds > roundAmount)
    { // game is over
        setupEndScreen();
    }
    else
    {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - startTime).count();

        if (ofGetFrameNum() == frame + 1 && ofGetFrameNum() > 1)
        { // frame after a new round
            ofSleepMillis(2000);
            background.setVolume(1);
            startTime = std::chrono::steady_clock::now();
            rounds++;
        }
        else if (amountCorrect == amountOfCircles && amountOfCircles != 0)
        {
            score += 50 * (duration.count() - elapsed_time);
            setupNewRound();
            ofBackground(0, 255, 0);
            correct.play();
        }
        else if (elapsed_time >= duration.count())
        {
            setupNewRound();
            ofBackground(255, 0, 0);
            incorrect.play();
        }
        else
        {
            ofBackground(0, 0, 0);
            ofApp::drawCircles();

            // draw info
            ofSetColor(255, 255, 255);
            font.drawString("Time: " + ofToString(duration.count() - elapsed_time), ofGetWidth() - 300, 100);
            font.drawString("Score: " + ofToString(score), ofGetWidth() - 300, 200);
            font.drawString("Round: " + ofToString(rounds), ofGetWidth() - 300, 300);

            for (Circle &circle : circles)
            {
                circle.currentAmount = 0;
            }
            amountCorrect = 0;
        }
    }
}

void ofApp::drawBlobs()
{
    vector<vector<float>> blobs = ofApp::findBlobs();
    for (int i = 0; i < blobs.size(); i++)
    {
        vector<float> blobCoordinates = blobs[i];
        if (blobCoordinates.at(0) >= 0)
        {
            ofColor white(255, 255, 255);
            ofSetColor(white);
            ofDrawCircle(blobCoordinates.at(0), blobCoordinates.at(1), 15);
        }
    }
}

void ofApp::drawKinectImages()
{
    ofSetColor(255, 255, 255);
    // draw from the live kinect
    kinect.drawDepth(10, 10, 800, 600);
    kinect.draw(820, 10, 800, 600);

    grayImage.draw(10, 620, 800, 600);
    contourFinder.draw(820, 620, 800, 600);
}

void ofApp::drawCircles()
{
    for (const Circle &circle : circles)
    {
        ofSetColor(circle.color);
        ofDrawCircle(circle.x, circle.y, circle.radius);

        if (circle.expectedAmount > 0)
        {
            ofSetColor(0);
            std::string circleText = ofToString(circle.expectedAmount);
            ofRectangle boundingBox = font.getStringBoundingBox(circleText, 0, 0);
            font.drawString(circleText, circle.x - boundingBox.width / 2, circle.y + boundingBox.height / 2);
        }
        else 
        {
            vector<std::string> texts = {};
            if (circle.expectedAmount == -2) {
                texts.push_back("Start");
                texts.push_back("Game");
            }
            else if (circle.expectedAmount == -3) {
                texts.push_back("Play");
                texts.push_back("again");
            }

            ofSetColor(255);
            for (int i = 0; i < texts.size(); i++)
            {
                ofRectangle boundingBox = headerFont.getStringBoundingBox(texts[i], 0, 0);
                headerFont.drawString(texts[i], circle.x - boundingBox.width / 2,(circle.y - 80) + (i*150) + boundingBox.height / 2);
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::exit()
{
    ofLog() << std::to_string(minBlobSize);
    gui.saveToFile("kinect_settings.json");
    kinect.setCameraTiltAngle(0); // zero the tilt on exit
    kinect.close();
    ofLog() << "Exit";
}

//--------------------------------------------------------------

void ofApp::writeToFile(int score)
{
    std::string filePath = ofToDataPath("scores_" + std::to_string(amountOfPlayers) + "_" + std::to_string(roundAmount) + ".txt");
    std::ofstream outputFile(filePath, std::ios::app);

    if (outputFile.is_open())
    {
        outputFile << to_string(score) << std::endl;
        outputFile.close();
    }
    else
    {
        ofLogError() << "Fehler beim oeffnen der Datei zum Schreiben.";

    }
}

int ofApp::getHighScoreFromFile()
{
    std::string filePath = ofToDataPath("scores_" + std::to_string(amountOfPlayers) + "_" + std::to_string(roundAmount) + ".txt");
    // Pr�fe, ob die Datei existiert, und erstelle sie, falls nicht
    std::ofstream outputFile(filePath, std::ios::app); // �ffnen im Anh�ngemodus erstellt die Datei, wenn sie nicht existiert
    outputFile.close(); // Sofort wieder schlie�en

    std::ifstream inputFile(filePath);
    int highscore = 0;

    if (inputFile.is_open())
    {
        std::string line;
        while (std::getline(inputFile, line))
        {
            try {
                int score = std::stoi(line);
                if (score > highscore)
                {
                    highscore = score;
                }
            }
            catch (const std::invalid_argument&) {
                ofLogError() << "Ungueltige Zeile in der Datei: " << line;
            }
            catch (const std::out_of_range&) {
                ofLogError() << "Wert ausserhalb des gueltigen Bereichs in der Datei: " << line;
            }
        }
        inputFile.close();
    }
    else
    {
        ofLogError() << "Fehler beim oeffnen der Datei zum Lesen.";
    }
    return highscore;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    if (key == 'p') {
        setupEndScreen();
    }
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
void ofApp::mouseEntered(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
}
