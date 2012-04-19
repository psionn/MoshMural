//
//  myBlob.cpp
//  MoshMural
//
//  Created by Mark Kleback on 4/13/12.
//  Copyright (c) 2012 Kleebtronics, Inc. All rights reserved.
//

#include <iostream>
#include "myBlob.h"


myBlob::myBlob(ofPoint c, vector<ofPoint> pts, int blobNo)
{
    cen = c;
    points = pts;
    
    flockcolor = ofColor(ofRandom(20, 255), ofRandom(20,255), ofRandom(20, 255));
    
    available = true;
    blobDelete = false;
    blobId = blobNo;
    
    timer = 127;
    
}

void myBlob::update(ofxCvBlob _cv)
{
 //   points.clear();
    cen = _cv.centroid;
    points = _cv.pts;    
    
 
 

}

void myBlob::draw()
{
    ofSetColor(flockcolor);
    ofNoFill();
    ofBeginShape();
    
    for(int i = 0; i < points.size(); i++)
    {
        ofVertex(points[i].x, points[i].y);
    }
    
    ofEndShape();
}



void myBlob::countDown()
{
    timer--;
    
}

bool myBlob::dead()
{
    if(timer < 0) return true;
    return false;
    
}