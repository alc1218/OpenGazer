#pragma once
#include "utils.h"
#include "PointTracker.h"
#include "BlinkDetector.h"
#include "ExtractEyeFeaturesSegmentation.h"


class EyeExtractor {
    const PointTracker &tracker; /* dangerous */
    scoped_ptr<IplImage> eyefloat2;
    scoped_ptr<IplImage> eyefloat2_left;
	BlinkDetector blinkdet;
	BlinkDetector blinkdet_left;
	bool blink;
    void processEye(void);

public:
    bool saveImage;
    static const int eyedx;
    static const int eyedy;
    static const CvSize eyesize;

    ExtractEyeFeaturesSegmentation extractFeatures;

    scoped_ptr<IplImage> eyegrey, eyefloat, eyeimage;
    scoped_ptr<IplImage> eyegrey_left, eyefloat_left, eyeimage_left;
    IplImage* histogram_horizontal, * histogram_vertical, * histogram_horizontal_left, * histogram_vertical_left;
    scoped_ptr<std::vector<int> > vector_horizontal, vector_vertical, vector_horizontal_left, vector_vertical_left;
    IplImage* eyeGraySegmented, * eyeGraySegmented_left;

    EyeExtractor(const PointTracker &tracker);
    void extractEye(const IplImage *origimage) throw (TrackingException);
    void extractLeftEye(const IplImage *origimage,double x0, double y0, double x1, double y1) throw (TrackingException);
	bool isBlinking();
    ~EyeExtractor(void);
};