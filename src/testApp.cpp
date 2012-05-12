#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    listener    = new BlobListener();
    img         = NULL;
    kinect.setRegistration(true);
    
	kinect.init();
	kinect.open();

    mesh = kinectMesh(kinect);

    nearThreshold           = 230;
    farThreshold            = 90;
    blobCount               = 0;
    vAdj                    = 0;
    hAdj                    = 0;
    vZoom                   = 2.0f;
    hZoom                   = 1.6f;
    
    grayImage.allocate(kinect.width, kinect.height);
	grayThreshNear.allocate(kinect.width, kinect.height);
	grayThreshFar.allocate(kinect.width, kinect.height);
    ofSetFrameRate(60);
    
    meshing     = false;
    cracking    = false;
    flocking    = false;
    frequency   = false;
    volume      = false;
    drawImage   = false;
    debug       = false;
    
    tracker.setListener(listener);
    
    serial.enumerateDevices();

    serial.setup("/dev/cu.usbmodem621", 9600);
    
//    srand((unsigned int)time((time_t *)NULL));
    
    // 0 output channels, 
    // 2 input channels
    // 44100 samples per second
    // BUFFER_SIZE samples per buffer
    // 4 num buffers (latency)
    
    stream.listDevices();
    stream.setDeviceID(0);
    stream.setup(this, 0, 2, 44100, BUFFER_SIZE, 4);//ofSoundStreamSetup (0,2,this, 44100,BUFFER_SIZE, 4);	
    left = new float[BUFFER_SIZE];
    right = new float[BUFFER_SIZE];
    streaming = true;
    
    for (int i = 0; i < NUM_WINDOWS; i++)
    {
        for (int j = 0; j < BUFFER_SIZE/2; j++)
        {
            freq[i][j] = 0;	
        }
    }

}

void testApp::serialHandler() {
    
    bool sw = serial.writeByte(1);

    // we want to read 8 bytes 
    int bytesRequired = 16; 
    unsigned char bytes[bytesRequired]; 
    int bytesRemaining = bytesRequired; 
    // loop until we've read everything 
    while ( bytesRemaining > 0 ) { 
        // check for data 
        if ( serial.available() > 0 ) { 
            // try to read - note offset into the bytes[] array, this is so
            // that we don't overwrite the bytes we already have 
            int bytesArrayOffset = bytesRequired - bytesRemaining; 
            int result = serial.readBytes( &bytes[bytesArrayOffset], bytesRemaining );
            // check for error code
            if ( result == OF_SERIAL_ERROR )
            {
                // something bad happened
                ofLog( OF_LOG_ERROR, "unrecoverable error reading from serial" );
                // bail out
                break;
            }
            else if ( result == OF_SERIAL_NO_DATA )
            {
                // nothing was read, try again
            }
            else
            {   
                // we read some data!
                bytesRemaining -= result;
            }
        }
    }
    cout<<"READ:";
    for (int i = 0; i<bytesRequired; i++) cout<<bytes[i];
    cout<<endl;
}

//--------------------------------------------------------------
void testApp::update(){
    serialHandler();
    if (volume || frequency) {
        if (!streaming) {
            stream.start();
            streaming = true;
        }
        fftUpdate();
    } else {
        if (streaming) {
            stream.stop();
            streaming = false;
            max = 0; frq = 0;
        }
    }
        
    kinect.update();
    
    if (kinect.isFrameNew()) {
        if (meshing) mesh.update();
        if (flocking || debug) {
            blobHandler(true);
        }
        if (flocking) listener->update();
        if (debug) img = &kinect.getTextureReference();
    }


}

void testApp::fftUpdate() {
    //FFT stuff
    
    max = 0;
    frq = -1;
    static int index=0;
    float avg_power = 0.0f;	
    
    if(index < 80)
        index += 1;
    else
        index = 0;
    
    
    
    //do the FFT
    myfft.powerSpectrum(0,(int)BUFFER_SIZE/2, left,BUFFER_SIZE,&magnitude[0],&phase[0],&power[0],&avg_power);
    
    // start from 1 because mag[0] = DC component 
    // and discard the upper half of the buffer 
    for(int j=1; j < BUFFER_SIZE/2; j++) {
        freq[index][j] = magnitude[j];		
        if (magnitude[j] > max ) {
            max = magnitude[j];
            frq = j;
        }
    }
    if (max > absMax) absMax = max;
}
//--------------------------------------------------------------
void testApp::draw(){
    ofBackground(0,0,0);
    if (img != NULL && debug) img->draw(0, 0);

    if (meshing) mesh.draw(max, frq);
    if (debug) {
        cout<< "Frequency : " << frq << " Volume : " << max <<endl;
        tracker.draw(0,0);
    }
    if (flocking) (volume||frequency)?listener->draw(max, frq):listener->draw();
    
    
}

void testApp::exit() {
    cout<<"Loudest : "<<absMax<<endl;
    kinect.close();
}



void testApp::blobHandler(bool bThreshWithOpenCV = false) {
    grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
    
    
    if(bThreshWithOpenCV) {
        grayThreshNear = grayImage;
        grayThreshFar = grayImage;
        grayThreshNear.threshold(nearThreshold, true);
        grayThreshFar.threshold(farThreshold);
        cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);
    } else {
        unsigned char * pix = grayImage.getPixels();
        
        int numPixels = grayImage.getWidth() * grayImage.getHeight();
        for(int i = 0; i < numPixels; i++) {
            if(pix[i] < nearThreshold && pix[i] > farThreshold) {
                pix[i] = 255;
            } else {
                pix[i] = 0;
            }
        }
    }
    
    grayImage.flagImageChanged();
    
    
    contourFinder.findContours(grayImage, 300, (kinect.width*kinect.height)/2, 20, false);
    scalePoints(contourFinder, vZoom, hZoom, vAdj, hAdj);
    tracker.trackBlobs( contourFinder.blobs );
}

void testApp::scalePoints(ofxCvContourFinder& cf, float wScale, float hScale, int vAdjust, int hAdjust) {
    for (int i = 0; i < cf.blobs.size(); i++) {
        cf.blobs[i].centroid.x = cf.blobs[i].centroid.x * wScale + vAdjust;
        cf.blobs[i].centroid.y = cf.blobs[i].centroid.y * hScale + hAdjust;
        for (int j = 0; j < cf.blobs[i].pts.size(); j++) {
            cf.blobs[i].pts[j].x = cf.blobs[i].pts[j].x * wScale + vAdjust;
            cf.blobs[i].pts[j].y = cf.blobs[i].pts[j].y * hScale + hAdjust;
        }
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
        case 'f':
            flocking = !flocking;
            break;
        case 'c':
            cracking = !cracking;
            break;
        case 'm':
            meshing = !meshing;
            break;
        case 'v':
            volume = !volume;
            break;
        case 'd':
            debug = !debug;
            break;
        case '-':
            nearThreshold--;
            break;
        case '=':
            nearThreshold++;
            break;
        case '_':
            farThreshold--;
            break;
        case '+':
            farThreshold++;
            break;
        case OF_KEY_DOWN:
            hAdj--;
            break;
        case OF_KEY_UP:
            hAdj++;
            break;
        case OF_KEY_LEFT:
            vAdj--;
            break;
        case OF_KEY_RIGHT:
            vAdj++;
            break;
        case ',':
            vZoom -= .1;
            hZoom -= .1;
            break;
        case '.':
            vZoom += .1;
            hZoom += .1;
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
//    for (int i = 0; i < blobs.size(); i++) {
//        blobs[i].resizeWindow(w, h);
//    }
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

void testApp::audioReceived 	(float * input, int bufferSize, int nChannels){	
	// samples are "interleaved"
	for (int i = 0; i < bufferSize; i++){
		left[i] = input[i*2];
		right[i] = input[i*2+1];
	}
	bufferCounter++;
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}