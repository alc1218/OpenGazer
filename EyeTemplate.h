#pragma once
#include "utils.h"

class EyeTemplate {

	int sizeImageDisk;
    IplImage* irisTemplateDisk[10];
    IplImage* irisTemplate;
    IplImage* Gaussian2D[10];
    double MinVal[10], MaxVal[10];
    CvPoint MinLoc[10], MaxLoc[10];
    CvScalar color, a[10];

public:
    void setSize(IplImage* eyesize);
    Point* ProcessToExtractFeatures(IplImage*, IplImage*);
    IplImage* ConstructTemplateDisk(int);
    IplImage* Segmentation(IplImage*);
    IplImage* CreateTemplateGausian2D(IplImage*);

};