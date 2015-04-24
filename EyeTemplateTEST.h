#pragma once
#include "utils.h"

#define VECTOR_SIZE 1

class EyeTemplateTEST {

    IplImage* elipse_gray;
    IplImage* irisTemplateDisk[VECTOR_SIZE];
    IplImage* irisTemplate;
    IplImage* Gaussian2D[VECTOR_SIZE];
    IplImage* Matches[VECTOR_SIZE];
    IplImage* MatchesSmoothed[VECTOR_SIZE];
    double MinVal[VECTOR_SIZE], MaxVal[VECTOR_SIZE];
    CvPoint MinLoc[VECTOR_SIZE], MaxLoc[VECTOR_SIZE];
    CvScalar color, a[VECTOR_SIZE];

public:
    void setSize(IplImage* eyesize, float, int);
    Point* ProcessToExtractFeatures(IplImage*, IplImage*, float, int, int);
    IplImage* ConstructTemplateDisk(int);
    IplImage* CreateTemplateGausian2D(IplImage*, float);

};