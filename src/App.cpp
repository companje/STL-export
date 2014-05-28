#include "ofMain.h"
#include "ofAppGlutWindow.h"
#include "ofxOpenCv.h"
#include "ofxMesh.h"
#include "ofxSTLExporter.h"

class App : public ofBaseApp {
public:
  
  ofxCvColorImage rgb;
  ofxCvGrayscaleImage grey;
  ofxCvContourFinder contours;
  ofxMesh mesh;
  ofPath path;
  ofImage bg;
  ofEasyCam cam;
  bool isDrawing;
  ofShader shader;
  
  void setup() {
    ofBackground(255);
    path.setStrokeWidth(5);
    path.setFilled(false);
    path.setStrokeColor(0);
    
    load("kinkfm450.jpg");
    shader.load("normal");
  }
  
  void clear() {
    path.clear();
//    bg = ofImage();
    bg.clear();
  }
  
  void mousePressed(int x, int y, int button) {
    isDrawing = x<600;
    if (isDrawing) path.moveTo(x,y);
  }
  
  void mouseDragged(int x, int y, int button) {
    if (isDrawing) path.lineTo(x,y);
  }
  
  void mouseReleased(int x, int y, int button) {
    trace();
  }
  
  void load(string filename) {
    bg.loadImage(filename);
    bg.setAnchorPercent(.5,.5);
    if (bg.width<600 && bg.height<600) return;
    
    
    bool landscape = bg.width > bg.height;
    float ratio = landscape ? float(bg.height) / bg.width : float(bg.width) / bg.height;
    
    if (landscape) {
      bg.resize(600, 600*ratio);
    } else {
      bg.resize(600*ratio, 600);
    }

  }
  
  void trace() {
    ofImage img;
    //img.grabScreen(0,0,ofGetWidth(),ofGetHeight());
    img.grabScreen(0,0,600,600);
    rgb.allocate(img.width,img.height);
    rgb.setFromPixels(img.getPixelsRef());
    grey = rgb;
    grey.brightnessContrast(0, .5);
    //        grey.threshold(5);
    grey.mirror(true,false);
    contours.findContours(grey, 10, img.width*img.height/2, 200, true);
    
    mesh.clear();
    float thickness=40;
    ofVec3f zOffset(0,0,thickness);
    
    ofPath path;
    path.setFilled(true);
    
    for (int i=0; i<contours.blobs.size(); i++) {
      vector<ofPoint> &pts = contours.blobs[i].pts;
      path.moveTo(pts[0]);
      for (int j=0; j<pts.size(); j++) {
          path.lineTo(pts[j]);
      }
    }
    
    mesh.clear();
    vector<ofPolyline> outline = path.getOutline();
    ofxMesh *tess = (ofxMesh*) &path.getTessellation();
    ofxMesh top = *tess; //copy
    ofxMesh bottom = top; //copy
    ofxMesh sides;
    
    for (int i=0; i<top.getVertices().size(); i++) {
      top.addNormal(ofVec3f(0,0,-1));
      bottom.addNormal(ofVec3f(0,0,1));
    }
    
//    ofxInvertIndices(top);
    
    bottom.translate(zOffset); //extrude
            
    //make side mesh by using outlines
    for (int j=0; j<outline.size(); j++) {
        
      int iMax = outline[j].getVertices().size();
      
      for (int i=0; i<iMax; i++) {
          
        ofVec3f a = outline[j].getVertices()[i];
        ofVec3f b = outline[j].getVertices()[i] + zOffset;
        ofVec3f c = outline[j].getVertices()[(i+1) % iMax] + zOffset;
        ofVec3f d = outline[j].getVertices()[(i+1) % iMax];
        
        sides.addFace(a,b,c,d);
      }
    }
    
    ofxInvertIndices(sides);
    
    mesh = ofxMesh(sides.addMesh(bottom.addMesh(top)));
    
    mesh.translate(-mesh.getCentroid()); //center
    
    //mesh.addBox(ofRectangle(-50,-50,100,100), 100);
    
  }
  
  vector<ofPolyline> ofxGetPolyLinesFromBlobs(vector<ofxCvBlob> blobs) {
    vector<ofPolyline> lines;
    for (int i=0; i<blobs.size(); i++) {
      vector<ofPoint> &pts = blobs[i].pts;
      ofPolyline line;
      for (int j=0; j<pts.size(); j++) {
          line.lineTo(pts[j].x, pts[j].y);
      }
      lines.push_back(line);
    }
    return lines;
  }
 
  
  void ofxInvertIndices(ofxMesh &m) {
    for (int i=0; i<m.getNumIndices()/2; i++) {
      ofIndexType &first = m.getIndices()[i];
      ofIndexType &last = m.getIndices()[m.getNumIndices()-1-i];
      swap(first,last);
    }
  }
  
  void update() {
      
  }
      
  void draw() {
    ofSetColor(255);
    if (bg.isAllocated()) bg.draw(300,300);
    path.draw(0,0);
    ofSetColor(0);
    ofLine(600,0,600,600);
    ofTranslate(900,300);
    ofRotateY(ofGetMouseX());
    ofRotateX(ofGetMouseY());
    ofSetColor(255);
    
    ofEnableDepthTest();
    shader.begin();

    mesh.draw();
    
    ofDrawBox(80);
    ofLine(0,0,0,70);

    shader.end();
  }
  
  void exportSTL(string filename) {
    ofxMesh mesh = this->mesh; //local copy
    mesh.scale(.25,.25);
    if (mesh.getVertices().size()<1) return; //when no trace is done yet

//    ofMesh mesh = ofBoxPrimitive(150,150,150).getMesh();

    
    ofxSTLExporter stl;
    stl.beginModel();
    
    for (int i=0; i < mesh.getIndices().size()-2; i+=3) {
      ofVec3f a = mesh.getVertices().at(mesh.getIndex(i+0));
      ofVec3f b = mesh.getVertices().at(mesh.getIndex(i+1));
      ofVec3f c = mesh.getVertices().at(mesh.getIndex(i+2));
      ofVec3f n = mesh.getNormal(mesh.getIndex(i));
      stl.addTriangle(a,b,c,n);
    }
    
    stl.useASCIIFormat(true);
    stl.saveModel(filename);
  }
  
  void keyPressed(int key) {
    if (key=='t') trace();
    if (key=='i') ofxInvertIndices(mesh);
    if (key=='e') {
//      trace();
      string filename = ofToString(ofGetFrameNum())+".stl";
      exportSTL(filename);
      string absFilename = ofFile(filename).getAbsolutePath();
      string cmd = ("open -a /Applications/UP.app " + absFilename);
      system(cmd.c_str());
    }
    if (key=='c') clear();
  }
  
  void dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.size()==1) load(dragInfo.files[0]);
  }
  
};

//========================================================================
int main( ){
  ofAppGlutWindow window;
  ofSetupOpenGL(&window, 1200, 600, OF_WINDOW);
  ofRunApp(new App());
}
