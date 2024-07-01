#include "ofApp.h"
#include <stdio.h>

void ofApp::allocateTexture() {

	eyeWidth = vr.getHmd().getEyeWidth();
	eyeHeight = vr.getHmd().getEyeHeight();

	//eyeWidth_eighth = vr.getHmd().getEyeWidth() / 8.f;
	eyeWidth_eighth = vr.getHmd().getEyeWidth() / 8.f;
	eyeWidth_eighth = 480;

	eyeHeight_eighth = vr.getHmd().getEyeHeight() / 8.f;
	eyeHeight_eighth = 480;


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
	mySound.setVolume(0);

}


void ofApp::createHistoryPlot(ofxHistoryPlot*& plot,ofParameter<float>& param) {
	plot = new ofxHistoryPlot(const_cast<float*>(&param.get()), param.getName(), numSample, true);
	plot->setRange(param.getMin(), param.getMax());
	plot->setColor(ofColor(255));
	plot->setBackgroundColor(ofColor(0, 220)); //custom bg color

}

void ofApp::createHistoryPlot(ofxHistoryPlot*& plot, float& val, ofParameter<int>& param) {
	plot = new ofxHistoryPlot(&val, param.getName(), numSample, true);
	plot->setRange(param.getMin(), param.getMax());
	plot->setColor(ofColor(255));
	plot->setBackgroundColor(ofColor(0, 220)); //custom bg color

}



void ofApp::setup() {

	ofSetWindowTitle("virtual hallucinogen");

	ofSetLogLevel(OF_LOG_NOTICE);
	ofLogToConsole();

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
	panel.add(isRandomDemo.set("isRandomDemo", true));
	panel.add(isDrawStereoCamera.set("isDrawStereo", true));


	panel.minimizeAll();

	getTimeStamp();
	auto sec = (int(STATUS::END) - 1) / 1000;
	sequenceTimeStr = std::to_string(int(floor(sec/60)))+" minutes "+ std::to_string(sec % 60) + " seconds";

	manual = "Press 'd' to show/hide VR view.";
	manual += "\nPress 'a' to audio check.";
	manual += "\nPress 'm' to switch play mode.";
	manual += "\nPress 'Ctrl + R' to interrupt reset.";
	manual += "\nPress 'Ctrl + E' to interrupt ending.";
	manual += "\nPress 'Ctrl + P' to pause/resume DeepDream thread.";
	manual += "\nPress 'Ctrl + Q' to exit.";
	
	vr.update();
	vive.update();


	ofLogNotice(__FUNCTION__) << "STEREO CAMERA is " << STEREO_CAMERA;


	if (STEREO_CAMERA) {
		DeepDream.setup(vive.getUndistortedTexture(0), vive.getUndistortedTexture(1));
		DeepDream.setLikeViveSRWorks(vive.getUndistortedTexture(0));
	}
	else {

		std::string path = "../../../../../addons/ofxViveSRWorks/shader/";
		bool isLoaded = texShader.load(path + "texShader");
		ofLogNotice(__FUNCTION__) << "texShader is loaded  " <<isLoaded;

		DeepDream.setup(vive.getUndistortedTexture(0));


		glm::ivec2 distortedSize, undistortedSize;
		undistortedSize.x = vive.getUndistortedTexture(0).getWidth();
		undistortedSize.y = vive.getUndistortedTexture(0).getHeight();

		float aspect = float(undistortedSize.x) / float(undistortedSize.y);

		renderRect = ofMesh::plane(8.0, 8.0 / aspect, 2, 2);
		renderRect.clearTexCoords();
		renderRect.addTexCoord(glm::vec2(0, undistortedSize.y));
		renderRect.addTexCoord(glm::vec2(undistortedSize));
		renderRect.addTexCoord(glm::vec2(0));
		renderRect.addTexCoord(glm::vec2(undistortedSize.x, 0));
	
	}

	DeepDream.getParameters().getFloat("black").set(BLACK);
	DeepDream.getParameters().getFloat("blend").set(0);
	//DeepDream.setblack(black);


	//ofLog() << "Parameter Group: " << DeepDream.getParameters().getName();
	for (const auto& param : DeepDream.getParameters()) {
		ofLog() << param->getName() << ": " << param->toString();
	}

	createHistoryPlot(plots[0], currentLayer, DeepDream.getDeepDreamGroup().getInt("layerlevel"));
	createHistoryPlot(plots[1], DeepDream.getParameters().getGroup("DeepDreamModule").getFloat("norm_str"));
	createHistoryPlot(plots[2], DeepDream.getParameters().getGroup("DeepDreamModule").getFloat("lr"));
	createHistoryPlot(plots[3], DeepDream.getParameters().getGroup("DeepDreamModule").getFloat("octave_scale"));
	createHistoryPlot(plots[4], currentOctave, DeepDream.getDeepDreamGroup().getInt("octave_num"));
	createHistoryPlot(plots[5], currentItr, DeepDream.getDeepDreamGroup().getInt("iteration"));

	createHistoryPlot(plots[6], DeepDream.getParameters().getFloat("blend_weight"));
	createHistoryPlot(plots[7], DeepDream.getParameters().getFloat("blend"));
	createHistoryPlot(plots[8], DeepDream.getParameters().getFloat("black"));

}

void ofApp::update(){

	ofSoundUpdate();
	vr.update();
	vive.update();

	if (mode == PLAY_MODE::SEQUENSE) {
		updateSequence();	
	}
	else {
		updateDemo();
	}


	if (STEREO_CAMERA) {
		DeepDream.update(vive.getUndistortedTexture(0), vive.getUndistortedTexture(1));

		for (int i = 0; i < 2; i++) {

			eyeFbo[i].begin();
			ofClear(0);

			if (isUseTranspose) {
				vr.beginEye(vr::Hmd_Eye(i));
				if (isDrawStereoCamera) {
					DeepDream.drawLikeViveSRWorks(i, vive.getTransform(i));
				}
				else {
					DeepDream.drawLikeViveSRWorks(0, vive.getTransform(0));
				}
			}
			else {
				ofPushMatrix();
				ofPushView();
				ofEnableDepthTest();
				ofSetMatrixMode(OF_MATRIX_PROJECTION);
				ofLoadMatrix(vr.getHmd().getProjectionMatrix(vr::Hmd_Eye(i)));
				ofSetMatrixMode(OF_MATRIX_MODELVIEW);
				ofLoadMatrix(vr.getHmd().getEyeTransformMatrix(vr::Hmd_Eye(i)));
				if (isDrawStereoCamera) {
					DeepDream.drawLikeViveSRWorks(i);
				}
				else {
					DeepDream.drawLikeViveSRWorks(0);
				}
			}

			vr.endEye();
			eyeFbo[i].end();

			//Submit texture to VR!
			vr.submit(eyeFbo[i].getTexture(), vr::EVREye(i));
		}
	}
	else {


		DeepDream.update(vive.getUndistortedTexture(0));
		for (int i = 0; i < 2; i++) {

			eyeFbo[i].begin();
			ofClear(0);

			if (isUseTranspose) {
				vr.beginEye(vr::Hmd_Eye(i));
				{
					ofDisableDepthTest();
					ofPushMatrix();
					ofMultMatrix(glm::scale(glm::vec3(1.f, 1.f, -1.f)) * vive.getTransform(i));
					ofTranslate(0, 0, 2.f); // Translate image plane to far away

					texShader.begin();
					texShader.setUniformTexture("tex", DeepDream.getTexture(), 0);
					renderRect.draw();
					texShader.end();

					ofPopMatrix();
					ofEnableDepthTest();
				}
				vr.endEye();
			}
			else {
			
				ofPushMatrix();
				ofPushView();
				ofEnableDepthTest();
				ofSetMatrixMode(OF_MATRIX_PROJECTION);
				ofLoadMatrix(vr.getHmd().getProjectionMatrix(vr::Hmd_Eye(i)));
				ofSetMatrixMode(OF_MATRIX_MODELVIEW);
				ofLoadMatrix(vr.getHmd().getEyeTransformMatrix(vr::Hmd_Eye(i)));

				ofDisableDepthTest();
				ofPushMatrix();
				ofMultMatrix(glm::scale(glm::vec3(1.f, 1.f, -1.f)));
				ofTranslate(0, 0, 2.f); // Translate image plane to far away

				texShader.begin();
				texShader.setUniformTexture("tex", DeepDream.getTexture(), 0);
				renderRect.draw();
				texShader.end();

				ofPopMatrix();
				ofEnableDepthTest();


				vr.endEye();

			
			}


			eyeFbo[i].end();

			vr.submit(eyeFbo[i].getTexture(), vr::EVREye(i));
		}
	
	
	}

	currentLayer = float(DeepDream.getDeepDreamGroup().getInt("layerlevel"));
	currentItr = float(DeepDream.getDeepDreamGroup().getInt("iteration"));
	currentOctave = float(DeepDream.getDeepDreamGroup().getInt("octave_num"));

}


void ofApp::drawVRView() {
	
	ofDisableAlphaBlending();
	eyeFbo[vr::Eye_Left].draw(0, eyeWidth_eighth, eyeWidth_eighth, -eyeWidth_eighth);
	eyeFbo[vr::Eye_Right].draw(eyeWidth_eighth, eyeWidth_eighth, eyeWidth_eighth, -eyeWidth_eighth);
	ofEnableAlphaBlending();


	//for capture HMD
	//int w = 960;
	//int h = 1080;
	//eyeFbo[vr::Eye_Left].draw(0, h, w, -h);
	//eyeFbo[vr::Eye_Right].draw(w, h, w, -h);

	//1150 x 750 source draw
	
	//vive.getUndistortedTexture(0).draw(0, 0, w, h);
	//vive.getUndistortedTexture(1).draw(w, 0, w, h);
	//ofEnableAlphaBlending();
}
void ofApp::draw(){
	
	if (isDrawVrView) {
		drawVRView();
	}
	drawSequenceInfo();
	panel.draw();

	for (int i = 0; i<9; i++) {
		plots[i]->draw(20 + 240*int(i/6), 300 + 35 * int(i%6), 200, 30);
	}

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
	endTimeStamp = str.str();

}

void ofApp::reset() {

	state = STATUS::READY;
	stateStr = "READY";
	mySound.stop();
	mySound.setVolume(0);
	DeepDream.getParameters().getFloat("blend").set(0);
	DeepDream.getParameters().getFloat("black").set(BLACK);

	timer = 0;
}

void ofApp::start() {

	state = STATUS::START;
	stateStr = "START";
	bTimerReached = false;
	DeepDream.resumeDeepDreamThread();
	DeepDream.getParameters().getFloat("black").set(BLACK);

	startTime = ofGetElapsedTimeMillis();  // get the start time
	getTimeStamp();
	mySound.setVolume(0);
	mySound.play();

}

void ofApp::interruptReset() {

	bTimerReached = true;
	reset();
	DeepDream.pauseDeepDreamThread();
	ofLogNotice(__FUNCTION__) << "STATUS::READY";

}

void ofApp::interruptEnding() {
	if (state == STATUS::RUN) {

		if (mode == PLAY_MODE::SEQUENSE) {
			startTime = int(ofGetElapsedTimeMillis()) - int(STATUS::RUN);
		}
		else {
			startTime = ofGetElapsedTimeMillis();
		}

		state = STATUS::FADE_OUT;
		stateStr = "FADE_OUT";
		ofLogNotice(__FUNCTION__) << "STATUS::FADE_OUT";
	}
	else {
	
		ofLogWarning(__FUNCTION__) << "interruptEnding only use STATUS::RUN.";

	}
}

void ofApp::audioCheck() {

	if (state == STATUS::READY) {

		if (mySound.isPlaying()) {
			mySound.stop();
			mySound.setVolume(0);
			ofLogNotice(__FUNCTION__) << "audio stop";
		}
		else {
			mySound.setVolume(1);
			mySound.play();
			ofLogNotice(__FUNCTION__) << "audio start";
		}
	}
	else {
		ofLogWarning(__FUNCTION__) << "state is not READY.";
	}
}

void ofApp::changeDrawVRView() {
	bool b = !isDrawVrView;
	isDrawVrView.set(b);
}

void ofApp::switchPlayMode() {


	if (state == STATUS::READY) {
		if (mode == PLAY_MODE::DEMO) {

			mode = PLAY_MODE::SEQUENSE;
			modeStr = "PLAY_MODE::SEQUENSE";
			ofLogNotice(__FUNCTION__) << "PLAY_MODE::SEQUENSE";
		}
		else {
			mode = PLAY_MODE::DEMO;
			modeStr = "PLAY_MODE::DEMO";
			ofLogNotice(__FUNCTION__) << "PLAY_MODE::DEMO";
		}
	}
	else {
		ofLogWarning(__FUNCTION__) << "Can't switch play mode because the sequence state is not ready. Please set the sequence state to ready.";
	}

}

void ofApp::switchDemoModeStatus() {

	if (mode == PLAY_MODE::DEMO) {
		if (state == STATUS::READY) {
			start();
			ofLogNotice(__FUNCTION__) << "change to STATUS::START";
		}
		else if (state == STATUS::RUN) {
			interruptEnding();
		}
		else {
			ofLogWarning(__FUNCTION__) << "please try again.";
		}
	}
	else {
		ofLogWarning(__FUNCTION__) << "status is not Demo.";
	}
}

void ofApp::switchDeepDreamThreadProcess() {
	if (DeepDream.getDeepDreamThread().isThreadRunning()) {
		DeepDream.pauseDeepDreamThread();
		ofLogNotice(__FUNCTION__) << "pause deepdream thread.";
	}
	else {
		DeepDream.resumeDeepDreamThread();
		ofLogNotice(__FUNCTION__) << "resume deepdream thread.";
	}
};

void ofApp::keyPressed(int key) {

	if (31 < key && key < 127) {
		std::string s = std::string(1, key);
		ofLogNotice(__FUNCTION__) << s + " key pressed";
	}

	if (key == 'a') {
		audioCheck();
	}
	if (key == 'd') {
		changeDrawVRView();
		ofLogNotice(__FUNCTION__) << "change Draw VR View";
	}

	if (key == ' ') {

		if (mode == PLAY_MODE::DEMO) {
			switchDemoModeStatus();
		}
		else {
			if (state == STATUS::READY) {
				start();
				ofLogNotice(__FUNCTION__) << "spacebar : STATUS::START";
			}
			else {
				ofLogWarning(__FUNCTION__) << "spacebar : state is not READY. press R to reset.";
			}
		}
	}

	if (key == 'r') {
		if (state == STATUS::END) {
			reset();
			ofLogNotice(__FUNCTION__) << "STATUS::READY";

		}else {
			ofLogWarning(__FUNCTION__) << "state is not END. press Ctrl+R to interrupt.";
		}
	}

	if (key == 'm') {
		switchPlayMode();
	}

	//Ctrl + r
	if (key == 18) {
		ofLogNotice(__FUNCTION__) << "Ctrl+R key pressed";
		interruptReset();
	}

	//Ctrl + E
	if (key == 5) {
		ofLogNotice(__FUNCTION__) << "Ctrl+E key pressed";
		interruptEnding();
	}

	//Ctrl + P
	if (key == 16) {
		ofLogNotice(__FUNCTION__) << "Ctrl+P key pressed";
		switchDeepDreamThreadProcess();
	}

	//Ctrl + Q
	if (key == 17) {
		ofLogWarning(__FUNCTION__) << "Ctrl+Q : exit.";
		exit();
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

void ofApp::setDefaultParameters() {
	DeepDream.getDeepDreamGroup().getInt("layerlevel").set(DeepDream.getDeepDreamThread().init_layer);
	DeepDream.getDeepDreamGroup().getInt("iteration").set(DeepDream.getDeepDreamThread().init_itr);
	DeepDream.getDeepDreamGroup().getInt("octave_num").set(DeepDream.getDeepDreamThread().init_oc_num);
	DeepDream.getDeepDreamGroup().getFloat("octave_scale").set(DeepDream.getDeepDreamThread().init_oc_scale);
	DeepDream.getDeepDreamGroup().getFloat("lr").set(DeepDream.getDeepDreamThread().init_lr);
	DeepDream.getDeepDreamGroup().getFloat("norm_str").set(DeepDream.getDeepDreamThread().init_norm_str);
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
				setDefaultParameters();
				state = STATUS::FADE_IN;
				stateStr = "FADE_IN";
				ofLogNotice(__FUNCTION__) << "STATUS::FADE_IN";
				break;

			case STATUS::FADE_IN:
				prog = ofMap(timer, 0.0, (float)STATUS::FADE_IN, 0.0, 1.0, true);
				DeepDream.getParameters().getFloat("blend").set(prog - attenuation * prog);
				DeepDream.getParameters().getFloat("black").set(ofMap(prog, 0.0, 1.0, BLACK, 0.0, false));
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
				DeepDream.getParameters().getFloat("blend").set(1.0 - attenuation);

				if (prog >= 1.0) {
					DeepDream.getParameters().getFloat("blend_weight").set(0.1);
					state = STATUS::FADE_OUT;
					stateStr = "FADE_OUT";
					ofLogNotice(__FUNCTION__) << "STATUS::FADE_OUT";
				}
				break;
			case STATUS::FADE_OUT:
				prog = ofMap(timer, (float)STATUS::RUN, (float)STATUS::FADE_OUT, 0.0, 1.0, true);
				DeepDream.getParameters().getFloat("blend").set(1.0 - prog - attenuation*(1.0-prog));
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

void ofApp::updateDemo() {

	prog = 0.0;
	if (state != STATUS::READY && state != STATUS::END) {

		timer = ofGetElapsedTimeMillis() - startTime;

		velocityScalar = glm::length(vr.getHmd().getVelocityVector());
		angularVelocityScalar = glm::length(vr.getHmd().getAngularVelocityVector()) * 0.1;
		float attenuation = (velocityScalar + angularVelocityScalar) / 2.0;

		switch (state) {

		case STATUS::START:
			DeepDream.getParameters().getFloat("blend_weight").set(0.1);
			setDefaultParameters();
			state = STATUS::FADE_IN;
			stateStr = "FADE_IN";
			ofLogNotice(__FUNCTION__) << "STATUS::FADE_IN";
			break;

		case STATUS::FADE_IN:

			if (isRandomDemo) {
				updatePrameter(sin(timer / 1000000.0) * 0.5 + 0.5);
			}
			prog = ofMap(timer, 0.0, (float)demoFadeDulation, 0.0, 1.0, true);
			DeepDream.getParameters().getFloat("blend").set(prog - attenuation * prog);
			DeepDream.getParameters().getFloat("black").set(ofMap(prog, 0.0, 1.0, BLACK, 0.0, false));


			mySound.setVolume(ofMap(prog, 0.01, 1.0, 0.0, 1.0, true));
			if (prog >= 1.0) {
				state = STATUS::RUN;
				stateStr = "RUN";
				ofLogNotice(__FUNCTION__) << "STATUS::RUN";
			}
			break;

		case STATUS::RUN:

			if (isRandomDemo) {
				updatePrameter(sin(timer / 1000000.0) * 0.5 + 0.5);
			}
			DeepDream.getParameters().getFloat("blend_weight") += attenuation;
			DeepDream.getParameters().getFloat("blend").set(1.0 - attenuation);

			break;

		case STATUS::FADE_OUT:

			if (isRandomDemo) {
				updatePrameter(sin(timer / 1000000.0) * 0.5 + 0.5);
			}

			prog = ofMap(timer, 0.0, (float)demoFadeDulation, 0.0, 1.0, true);
			DeepDream.getParameters().getFloat("blend").set(1.0 - prog - attenuation * (1.0 - prog));
			DeepDream.getParameters().getFloat("black").set(ofMap(prog, 0.0, 1.0, 0.0, BLACK, false));


			mySound.setVolume((1.0 - prog));
			if (prog >= 1.0) {
				state = STATUS::END;
				stateStr = "END";
				bTimerReached = true;
				DeepDream.pauseDeepDreamThread();
				ofLogNotice(__FUNCTION__) << "STATUS::END";
				reset();
			}
			break;
		}

	}
}

void ofApp::drawSequenceInfo() {

	// some information about the timer
	string  info = "FPS:        " + std::to_string(ofGetFrameRate()) + "\n";
	info += "Play Mode:  " + modeStr + "\n";
	info += "Status:     " + stateStr + " " + std::to_string(int(prog * 100)) + "%\n";

	info += "\n";

	info += "Velocity:   " + std::to_string(velocityScalar) + "\n";
	info += "Angular:    " + std::to_string(angularVelocityScalar) + "\n";
	info += "Volume:   " + std::to_string(mySound.getVolume()) + "\n";

	ofSetColor(0,0,0,128);
	ofDrawRectangle(0, 0, 960, 300);


	if (mode == PLAY_MODE::SEQUENSE) {

		int bar_x = 20;
		int bar_y = 240;

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

		info += "\n";
		info += "Start Time: " + timeStamp + "\n";
		info += "End Time:   " + endTimeStamp + "\n";
		info += "Sequence:   " + sequenceTimeStr + "\n";
		int sec = int(timer / 1000);
		info += "Timer:      " + std::to_string(int(floor(sec / 60))) + " minutes " + std::to_string(sec % 60) + " seconds" + "\n";
		info += "Percentage: " + percentage + "%\n";
	}
	else {
	
		info += "\n";
		int sec = int(timer / 1000);
		info += "Timer:      " + std::to_string(int(floor(sec / 60))) + " minutes " + std::to_string(sec % 60) + " seconds" + "\n";
	
	}

	if (mode == PLAY_MODE::SEQUENSE) {
		info += "\nPress ' ' to start.";
		info += "\nPress 'r' to ready.";
	}
	else {
		info += "\nPress ' ' to start/stop demo.";
	
	}



	ofSetColor(255);
	//ofBackground(128, 128);
	ofDrawBitmapString(info, 20, 20);
	ofDrawBitmapString(manual, 360, 20);
	//ofBackground(0,0);





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



