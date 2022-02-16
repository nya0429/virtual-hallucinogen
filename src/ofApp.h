#pragma once

#include "ofMain.h"
#include "ofxOpenVrUtil.h"
#include "ofxViveSRWorks.h"
#include "ofxGui.h"
#include "ofxDeepDreamThread.h"
#include "ofxSpout.h"


class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();
		
	void exit();
	void keyPressed(int key);

	//milli sec
	static const int sequenseDulation = 900000;
	//static const int fadeinDulation = 240000;
	static const int fadeinDulation = 5000;
	static const int fadeoutDulation = 60000;

	static const int demoFadeDulation = 5000;

	bool isDebug = true;

	enum class STATUS {
		READY = -1,
		START = 0,
		FADE_IN = fadeinDulation,
		RUN = sequenseDulation-fadeoutDulation,
		FADE_OUT = sequenseDulation,
		END = FADE_OUT + 1,
	};

	enum class PLAY_MODE {
		DEMO = 0,
		SEQUENSE = 1,
	};


private:

	STATUS state = STATUS::READY;
	PLAY_MODE mode = PLAY_MODE::SEQUENSE;


	void allocateTexture();
	void setupAudio();

	int eyeWidth;
	int eyeHeight;
	int eyeWidth_eighth;
	int eyeHeight_eighth;
	void drawVRView();
	void drawWorld();

	void drawSequenceInfo();
	void updateSequence();
	void updateDemo();
	void updatePrameter(float prog);
	void getTimeStamp();
	void start();
	void reset();

	//key function
	void audioCheck();
	void changeDrawVRView();
	void switchPlayMode();
	void switchDeepDreamThreadProcess();
	void interruptReset();
	void interruptEnding();

	//demo mode
	void switchDemoModeStatus();

	std::string timeStamp = "";
	std::string endTimeStamp = "";
	std::string stateStr = "STATUS::READY";
	std::string modeStr = "PLAY_MODE::SEQUENSE";
	std::string sequenceTimeStr = "";
	std::string elapsedTimeStr = "";

	bool  bTimerReached = true; // used as a trigger when we hit the timer
	float barWidth = 360;
	float startTime = ofGetElapsedTimeMillis(); // store when we start time timer
	float timer = 0;
	float velocityScalar = 0.0;
	float angularVelocityScalar = 0.0;

	float velocityBlend = 1.0;
	float black = 0.3;
	float prog = 0.0;

	ofxDeepDream::ofxDeepDreamThread DeepDream;
	ofxOpenVrUtil::Interface vr;
	ofxViveSRWorks::Interface vive;
	std::array<ofFbo, 2> eyeFbo;

	ofEasyCam cam;
	ofxPanel panel;
	ofSoundPlayer mySound;

	ofParameter<bool> isDrawVrView;
	ofParameter<bool> isUseTranspose;
	ofParameter<int> SequenceMinutes;

};
