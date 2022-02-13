#include "ofApp.h"
#include <stdio.h>

void ofApp::allocateTexture() {

	eyeWidth = vr.getHmd().getEyeWidth();
	eyeHeight = vr.getHmd().getEyeHeight();

	eyeWidth_eighth = vr.getHmd().getEyeWidth() / 8.f;
	eyeHeight_eighth = vr.getHmd().getEyeHeight() / 8.f;

	// Let's prepare fbos to be renderred for each eye
	ofFboSettings s;
	s.width = eyeWidth;   // Actual rendering resolution is higer than display(HMD) spec has,
	s.height = eyeHeight; // because VR system will distort images like barrel distortion and fit them through lens
	s.maxFilter = GL_LINEAR;
	s.minFilter = GL_LINEAR;
	s.numSamples = 4; // MSAA enabled. Anti-Alising is much important for VR experience
	s.textureTarget = GL_TEXTURE_2D; // Can't use GL_TEXTURE_RECTANGLE_ARB which is default in oF
	s.internalformat = GL_RGBA8;
	s.useDepth = true;
	s.useStencil = true;

	ofBackground(0);

	for (auto& f : eyeFbo) {
		f.allocate(s);
	}
}

void ofApp::setupAudio() {

	mySound.load("bi-naural_beats_L380R370_48kHz.wav",false);
	//mySound.setLoop(true);
	mySound.setVolume(0);

}

void ofApp::setup() {

	ofSetWindowTitle("generic hallucinogen");

	ofSetLogLevel(OF_LOG_VERBOSE);
	ofLogToConsole();
	//ofLogToFile("myLogFile.txt", true);

	ofSetVerticalSync(false);
	vr.setup();
	vive.init(false);

	setupAudio();
	allocateTexture();

	panel.setup();
	panel.setPosition(760,0);
	//panel.add(vive.getParameters());
	panel.add(DeepDream.getParameters());
	panel.add(isDrawVrView.set("isDrawVrView", true));
	panel.add(isUseTranspose.set("isUseTranspose", false));

	getTimeStamp();
	auto sec = (int(STATUS::END) - 1) / 1000;
	sequenceTimeStr = std::to_string(int(floor(sec/60)))+" minutes "+ std::to_string(sec % 60) + " seconds";
	
	vr.update();
	vive.update();
	DeepDream.setup(vive.getUndistortedTexture(0), vive.getUndistortedTexture(1));
	DeepDream.setLikeViveSRWorks(vive.getUndistortedTexture(0));
	DeepDream.setblack(black);

}

void ofApp::update(){

	ofSoundUpdate();
	vr.update();
	vive.update();
	updateSequence();
	DeepDream.update(vive.getUndistortedTexture(0), vive.getUndistortedTexture(1));

	for (int i = 0; i < 2; i++) {
		
		eyeFbo[i].begin();
		ofClear(0);

		if (isUseTranspose) {
			vr.beginEye(vr::Hmd_Eye(i));
			DeepDream.drawLikeViveSRWorks(i, vive.getTransform(i));
		}
		else {
			ofPushMatrix();
			ofPushView();
			ofEnableDepthTest();
			ofSetMatrixMode(OF_MATRIX_PROJECTION);
			ofLoadMatrix(vr.getHmd().getProjectionMatrix(vr::Hmd_Eye(i)));
			ofSetMatrixMode(OF_MATRIX_MODELVIEW);
			ofLoadMatrix(vr.getHmd().getEyeTransformMatrix(vr::Hmd_Eye(i)));
			DeepDream.drawLikeViveSRWorks(i);
		}
	
		vr.endEye();
		eyeFbo[i].end();

		 //Submit texture to VR!
		vr.submit(eyeFbo[i].getTexture(), vr::EVREye(i));
	}

}


void ofApp::drawVRView() {
	//ofPushMatrix();
	//ofTranslate(0, eyeWidth_eighth);
	//ofScale(1, -1); // Flipping
	//DeepDream.mergeFbo.getTexture().draw(eyeWidth_eighth, 0, eyeWidth_eighth, eyeWidth_eighth);
	//DeepDream.getTexture().draw(0, 0, eyeWidth_eighth, eyeWidth_eighth);
	//eyeFbo[vr::Eye_Left].draw(0, 0, eyeWidth_eighth, eyeWidth_eighth);
	// 
	//eyeFbo[vr::Eye_Left].draw(0, eyeWidth_eighth, eyeWidth_eighth, -eyeWidth_eighth);
	//eyeFbo[vr::Eye_Right].draw(eyeWidth_eighth, eyeWidth_eighth, eyeWidth_eighth, -eyeWidth_eighth);


	int w = 960;
	int h = 1080;

	//1150 x 750
	//ofDisableAlphaBlending();
	//vive.getUndistortedTexture(0).draw(0, 0, w, h);
	//vive.getUndistortedTexture(1).draw(w, 0, w, h);
	//ofEnableAlphaBlending();

	eyeFbo[vr::Eye_Left].draw(0, h, w, -h);
	eyeFbo[vr::Eye_Right].draw(w, h, w, -h);
	
	//eyeWidth_eighth
	//ofPopMatrix();
	//DeepDream.getTexture().draw(0, 0);

}

void ofApp::draw(){
	
	if (isDrawVrView) {
		drawVRView();
	}
	//panel.draw();
	//drawSequenceBar();
	
}


void ofApp::drawWorld() {

	cam.begin();
	ofEnableDepthTest();
	ofScale(200.f);
	ofTranslate(0, -1.f, 0);
	{
		//drawScene();
		//vive.drawSeeThrough(0);
		//vive.drawSeeThrough(1);
		vr.debugDraw();
		vive.drawMesh(OF_MESH_WIREFRAME);
	}
	ofDisableDepthTest();
	cam.end();
}
void ofApp::getTimeStamp() {

	//timeStamp = std::to_string(ofGetYear())+"/"+ ofToString(ofGetMonth(), 2, '0')+"/" +ofToString(ofGetDay(), 2, '0')+" "
	//	+ ofToString(ofGetHours(), 2, '0')+ ":" + ofToString(ofGetMinutes(), 2, '0')+ ":" + ofToString(ofGetSeconds(),2,'0');

	std::string tmpTimestampFormat = "%F %T";

	timeStamp = ofGetTimestampString(tmpTimestampFormat);
	std::stringstream str;
	auto now = std::chrono::system_clock::now() + std::chrono::milliseconds(int(STATUS::FADE_OUT));
	auto t = std::chrono::system_clock::to_time_t(now);
	std::chrono::duration<double> s = now - std::chrono::system_clock::from_time_t(t);
	int ms = s.count() * 1000;
	auto tm = *std::localtime(&t);
	constexpr int bufsize = 256;
	char buf[bufsize];
	ofStringReplace(tmpTimestampFormat, "%i", ofToString(ms, 3, '0'));

	if (strftime(buf, bufsize, tmpTimestampFormat.c_str(), &tm) != 0) {
		str << buf;
	}
	//auto ret = str.str();
	endTimeStamp = str.str();

}

void ofApp::reset() {

	state = STATUS::READY;
	stateStr = "READY";
	mySound.stop();
	mySound.setVolume(0);
	DeepDream.setblend(0);
	DeepDream.setblack(black);
	timer = 0;
}

void ofApp::interruptReset() {

	bTimerReached = true;
	reset();
	DeepDream.pauseDeepDreamThread();
	ofLogNotice(__FUNCTION__) << "Interrupt Reset";

}

void ofApp::keyPressed(int key) {

	if (key == ' ') {
		if (state == STATUS::READY) {
			state = STATUS::START;
			bTimerReached = false;
			DeepDream.resumeDeepDreamThread();
			DeepDream.setblack(black);
			startTime = ofGetElapsedTimeMillis();  // get the start time
			getTimeStamp();
			mySound.setVolume(0);

			ofLogNotice(__FUNCTION__) << boolalpha << mySound.isLoaded();

			mySound.play();

			stateStr = "START";
			ofLogNotice(__FUNCTION__) << "spacebar : STATUS::START";
		}
		else {
			ofLogWarning(__FUNCTION__) <<"spacebar : state is not READY. press R to reset.";
		}
	}

	if (key == 'd') {
		bool b = !isDrawVrView;
		isDrawVrView.set(b);
		ofLogNotice(__FUNCTION__) << "d : change DrawVrView";

	}

	if (key == 'a') {
		if (state == STATUS::READY) {

			if (mySound.isPlaying()) {
				mySound.stop();
				mySound.setVolume(0);
				ofLogNotice(__FUNCTION__) << "a : check audio stop";
			}
			else {
				mySound.setVolume(1);
				mySound.play();
				ofLogNotice(__FUNCTION__) << "a : check audio start";
			}
		}
		else {
			ofLogWarning(__FUNCTION__) << "a : state is not READY.";
		}

		bool b = !isDrawVrView;
		isDrawVrView.set(b);
	}

	if (key == 'r') {
		if (state == STATUS::END) {

			reset();
			ofLogNotice(__FUNCTION__) << "STATUS::READY";

		}else {
			ofLogWarning(__FUNCTION__) << "R : state is not END. press Ctrl+R to interrupt.";
		}
	}

	//Ctrl + r
	if (key == 18) {
		interruptReset();
		ofLogNotice(__FUNCTION__) << "STATUS::READY";
		ofLogNotice(__FUNCTION__) << "Ctrl+R : interrupt reset.";
	}

	//Ctrl + C
	if (key == 3) {
		ofLogWarning(__FUNCTION__) << "Ctrl+C : exit.";
		exit();
	}

	//Ctrl + P
	if (key == 16) {
		if (DeepDream.getDeepDreamThread().isThreadRunning()) {
			DeepDream.pauseDeepDreamThread();
		}
		else {
			DeepDream.resumeDeepDreamThread();
		}
		ofLogNotice(__FUNCTION__) << "Ctrl+P : pause/resume deepdream thread.";
	}

	//Ctrl + E
	if (key == 5) {
	
		if (state == STATUS::RUN) {

			startTime = ofGetElapsedTimeMillis() - int(STATUS::RUN);
			state = STATUS::FADE_OUT;
			stateStr = "FADE_OUT";
			ofLogNotice(__FUNCTION__) << "STATUS::FADE_OUT" << int(STATUS::RUN);
			ofLogNotice(__FUNCTION__) << "Ctrl+E : interrupt ending.";
		}

	}

}

void ofApp::updatePrameter(float prog) {

	float _timer = timer / 100000.0;
	float s = sin(prog * PI);

	auto lr = DeepDream.getDeepDreamGroup().getFloat("lr");
	float rand = ofMap(ofNoise(_timer, 0.5), 0.0, 1.0, lr.getMin(), lr.getMax(), false);
	lr.set(ofMap(s, 0.0, 1.0, DeepDream.getDeepDreamThread().init_lr, rand));

	auto norm_str = DeepDream.getDeepDreamGroup().getFloat("norm_str");
	rand = ofMap(ofNoise(_timer, 2.3), 0.0, 1.0, norm_str.getMin(), norm_str.getMax(), false);
	norm_str.set(ofMap(s, 0.0, 1.0, DeepDream.getDeepDreamThread().init_norm_str, rand));

	auto octave_scale = DeepDream.getDeepDreamGroup().getFloat("octave_scale");
	rand = ofMap(ofNoise(0.5, _timer), 0.0, 1.0, octave_scale.getMin(), octave_scale.getMax(), true);
	octave_scale.set(ofMap(s, 0.0, 1.0, DeepDream.getDeepDreamThread().init_oc_scale, rand));

	auto octave_num = DeepDream.getDeepDreamGroup().getInt("octave_num");
	rand = ofMap(ofNoise(_timer, 0.9), 0.0, 1.0, octave_num.getMin(), octave_num.getMax(), true);
	octave_num.set(int(ofMap(s, 0.0, 1.0, DeepDream.getDeepDreamThread().init_oc_num, rand)));

	auto iteration = DeepDream.getDeepDreamGroup().getInt("iteration");
	rand = ofMap(ofNoise(_timer, 2.0), 0.0, 1.0, iteration.getMin(), iteration.getMax(), true);
	iteration.set(int(ofMap(s, 0.0, 1.0, DeepDream.getDeepDreamThread().init_itr, rand)));

	auto layer = DeepDream.getDeepDreamGroup().getInt("layerlevel");
	rand = ofMap(ofNoise(_timer, 2.0), 0.0, 1.0, layer.getMin(), layer.getMax(), true);
	layer.set(int(ofMap(s, 0.0, 1.0, DeepDream.getDeepDreamThread().init_layer, rand)));

	auto blend_weight = DeepDream.getParameters().getFloat("blend_weight");
	rand = ofMap(ofNoise(0.5, _timer), 0.0, 1.0, 0.05, 0.2, true);
	blend_weight.set(ofMap(s, 0.0, 1.0, 0.1, rand));

}

void ofApp::updateSequence() {

	prog = 0.0;
	if (state != STATUS::READY && state != STATUS::END) {

		timer = ofGetElapsedTimeMillis() - startTime;
		velocityScalar = glm::length(vr.getHmd().getVelocityVector());
		angularVelocityScalar = glm::length(vr.getHmd().getAngularVelocityVector()) * 0.1;
		float attenuation = (velocityScalar + angularVelocityScalar) / 2.0;

		switch (state) {

			case STATUS::START:
				DeepDream.getParameters().getFloat("blend_weight").set(0.1);
				state = STATUS::FADE_IN;
				stateStr = "FADE_IN";
				ofLogNotice(__FUNCTION__) << "STATUS::FADE_IN";
				break;

			case STATUS::FADE_IN:
				prog = ofMap(timer, 0.0, (float)STATUS::FADE_IN, 0.0, 1.0, true);
				DeepDream.setblend(prog);
				DeepDream.setblack(ofMap(prog, 0.0, 1.0, black, 0.0, false));
				mySound.setVolume(ofMap(prog, 0.01, 1.0, 0.0, 1.0, true));
				if (prog >= 1.0) {
					state = STATUS::RUN;
					stateStr = "RUN";
					ofLogNotice(__FUNCTION__) << "STATUS::RUN";
				}
				break;

			case STATUS::RUN:
				prog = ofMap(timer, (float)STATUS::FADE_IN, (float)STATUS::RUN, 0.0, 1.0, true);
				updatePrameter(prog);

				DeepDream.getParameters().getFloat("blend_weight") += attenuation;
				DeepDream.setblend(1.0 - attenuation);
				if (prog >= 1.0) {
					DeepDream.getParameters().getFloat("blend_weight").set(0.1);
					state = STATUS::FADE_OUT;
					stateStr = "FADE_OUT";
					ofLogNotice(__FUNCTION__) << "STATUS::FADE_OUT";
				}
				break;
			case STATUS::FADE_OUT:
				prog = ofMap(timer, (float)STATUS::RUN, (float)STATUS::FADE_OUT, 0.0, 1.0, true);
				DeepDream.setblend(1.0 - prog);
				DeepDream.setblack(prog);
				mySound.setVolume((1.0 - prog));
				if (prog >= 1.0) {
					state = STATUS::END;
					stateStr = "END";
					bTimerReached = true;
					DeepDream.pauseDeepDreamThread();
					ofLogNotice(__FUNCTION__) << "STATUS::END";
				}
				break;
		}

	}

}

void ofApp::drawSequenceBar() {

	int bar_x = 20;
	int bar_y = 300;

	// the background to the progress bar
	ofSetColor(100);
	ofDrawRectangle(bar_x, bar_y, barWidth, 20);

	// get the percentage of the timer
	float pct = ofMap(timer, 0.0, float(STATUS::END), 0.0, 1.0, true);
	ofSetHexColor(0xf02589);
	ofDrawRectangle(bar_x, bar_y, barWidth * pct, 20);

	// draw the percentage
	ofSetColor(20);
	auto percentage = std::to_string(int(pct * 100));
	ofSetColor(255);
	ofDrawBitmapString(percentage + "%", bar_x + barWidth + 10, bar_y + 20);

	// some information about the timer
	string  info = "FPS:        " + std::to_string(ofGetFrameRate()) + "\n";
	info += "Start Time: " + timeStamp + "\n";
	info += "End Time:   " + endTimeStamp + "\n";

	info += "Sequence:   " + sequenceTimeStr + "\n";
	int sec = int(timer / 1000);
	info += "Timer:      " + std::to_string(int(floor(sec / 60))) + " minutes " + std::to_string(sec % 60) + " seconds" + "\n";
	info += "Status:     " + stateStr + " "+ std::to_string(int(prog*100))+ "%\n";
	info += "Velocity:   " + std::to_string(velocityScalar) + "\n";
	info += "Angular:    " + std::to_string(angularVelocityScalar) + "\n";
	info += "Volume:   " + std::to_string(mySound.getVolume()) + "\n";

	info += "Percentage: " + percentage + "%\n";
	info += "\nPress ' ' to start.";
	info += "\nPress 'r' to ready.";
	info += "\nPress 'd' to show/hide VR view.";
	info += "\nPress 'a' to audio check.";
	info += "\nPress 'Ctrl+r' to interrupt reset.";
	info += "\nPress 'Ctrl+p' to pause/resume DeepDream thread.";
	info += "\nPress 'Ctrl+c' to exit.";
	info += "\nPress 'Ctrl+e' to interrupt ending.";

	ofSetColor(255);
	ofDrawBitmapString(info, 20, 20);

}

void ofApp::exit() {

	vr.exit();
	ofLogVerbose(__FUNCTION__) << "VR Exit.";

	vive.exit();
	ofLogVerbose(__FUNCTION__) << "VIVE Exit.";

	mySound.stop();
	mySound.setVolume(0);
	ofSoundStopAll();
	ofSoundShutdown();
	ofLogVerbose(__FUNCTION__) << "ofSound Exit.";

	DeepDream.pauseDeepDreamThread();
	DeepDream.exit();

	ofSleepMillis(5000);
	ofLogVerbose(__FUNCTION__) << "ofSleep end and Exit.";

}



