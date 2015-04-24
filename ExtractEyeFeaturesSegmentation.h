#pragma once
#include "utils.h"

#define VECTOR_SIZE 2

class ExtractEyeFeaturesSegmentation {

	int sizeImageDisk;
    IplImage* elipse_gray;
    IplImage* irisTemplateDisk[VECTOR_SIZE];
    IplImage* irisTemplate;
    IplImage* Gaussian2D[VECTOR_SIZE];
    double MinVal[VECTOR_SIZE], MaxVal[VECTOR_SIZE];
    CvPoint MinLoc[VECTOR_SIZE], MaxLoc[VECTOR_SIZE];
    CvScalar color, a[VECTOR_SIZE];

public:
    ExtractEyeFeaturesSegmentation(CvSize);
    IplImage* processToExtractFeatures(IplImage*, IplImage*, IplImage*, IplImage*, std::vector<int>*, std::vector<int>*, std::vector<std::vector<int> >*);
    IplImage* processToExtractFeaturesWITHOUTVECTORS(IplImage*, IplImage*);
    IplImage* constructTemplateDisk(int);
    IplImage* Segmentation(IplImage*);
    int CalculateOptimalThreshold(IplImage*);
    int Otsu(std::vector<int>, int);
    IplImage* CreateTemplateGausian2D(IplImage*);
    void Histogram(IplImage*, IplImage*, IplImage*, std::vector<int> *, std::vector<int> *, std::vector<std::vector<int> >*);
    void SortHistogram(std::vector<int> *, std::vector<int> *, std::vector<std::vector<int> >*);

};