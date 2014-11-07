#pragma once
#include "utils.h"

class ExtractEyeFeaturesSegmentation {

	int sizeImageDisk;
    IplImage* irisTemplateDisk[10];
    IplImage* irisTemplate;
    IplImage* Gaussian2D[10];
    double MinVal[10], MaxVal[10];
    CvPoint MinLoc[10], MaxLoc[10];
    CvScalar color, a[10];

public:
    ExtractEyeFeaturesSegmentation(CvSize);
    void processToExtractFeatures(IplImage*, IplImage*, IplImage*, IplImage*, std::vector<int>*, std::vector<int>*);
    IplImage* constructTemplateDisk(int);
    IplImage* Segmentation(IplImage*);
    IplImage* CreateTemplateGausian2D(IplImage*);
    IplImage* Histogram(IplImage*, IplImage*, IplImage*, std::vector<int>*, std::vector<int>*);
    ~ExtractEyeFeaturesSegmentation(void);

};