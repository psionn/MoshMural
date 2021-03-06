/*
 *  Flock.cpp
 *  flock
 *
 *  Created by Jeffrey Crouse on 3/30/10.
 *  Copyright 2010 Eyebeam. All rights reserved.
 *
 */

#include "Flock.h"
#include <iostream>

Flock::Flock(ofColor fcolor) {
    
    color = fcolor;
  
    
    
}

void Flock::update(myBlob* blob) {
    
    while(boids.size() < 500){
        
        addBoid(blob->cen);
    }
    
	for(int i=0; i<boids.size(); i++) {
		boids[i]->update(boids);
        boids[i]->intersects(blob, boids);
        
	}
}

void Flock::draw(myBlob* blob, float rad) {
    
    
	
    
    for(int i=0; i<boids.size(); i++) {
		boids[i]->draw(color, rad);
        
        if(boids[i]->debug){
           
                
                ofPushMatrix();
                ofTranslate(blob->cen.x, blob->cen.y);
                ofSetColor(0, 255, 0);
                ofEllipse(0, 0, 10, 10);
                ofPopMatrix();
                
                
            
            
        }
        

	}
    
 

    
    
}

void Flock::addBoid(ofPoint centroid) {
	boids.push_back( new Boid(centroid) );
}