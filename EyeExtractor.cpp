#include "utils.h"
#include "EyeExtractor.h"
#include "ExtractEyeFeaturesSegmentation.h"

bool saveImage = true;
const int EyeExtractor::eyedx = 32*2;
const int EyeExtractor::eyedy = 16*2;
const CvSize EyeExtractor::eyesize = cvSize(eyedx*2, eyedy*2);

void EyeExtractor::processEye(void) {

    normalizeGrayScaleImage(eyegrey.get(), 127, 50);
    cvConvertScale(eyegrey.get(), eyefloat2.get());
    // todo: equalize it somehow first!
    // TODO ONUR REMOVED cvSmooth(eyefloat2.get(), eyefloat.get(), CV_GAUSSIAN, 3);
    //TODO INCLUDE cvEqualizeHist(eyegrey.get(), eyegrey.get());


    // ONUR DUPLICATED CODE FOR LEFT EYE
    normalizeGrayScaleImage(eyegrey_left.get(), 127, 50);
    cvConvertScale(eyegrey_left.get(), eyefloat2_left.get());
    // todo: equalize it somehow first!
    // TODO ONUR REMOVED cvSmooth(eyefloat2_left.get(), eyefloat_left.get(), CV_GAUSSIAN, 3);
    //TODO INCLUDE cvEqualizeHist(eyegrey_left.get(), eyegrey_left.get());
    
	// Blink detection trials
	//scoped_ptr<IplImage> temp(cvCreateImage(eyesize, IPL_DEPTH_32F, 1));
	//scoped_ptr<IplImage> temp2(cvCreateImage(eyesize, IPL_DEPTH_32F, 1));
	//cvConvertScale(eyegrey.get(), temp.get());
	blinkdet.update(eyefloat);

    eyeGraySegmented = extractFeatures.processToExtractFeatures(   eyegrey.get(), eyeimage.get(), histogram_horizontal, 
                                                histogram_vertical, vector_horizontal.get(), vector_vertical.get(), histPositionSegmentedPixels);

    
    cout << "Size of vector[4]: " << histPositionSegmentedPixels->size() << endl;

	//cvConvertScale(eyegrey_left.get(), temp2.get());
	blinkdet_left.update(eyefloat_left);
	
    eyeGraySegmented_left = extractFeatures.processToExtractFeatures(   eyegrey_left.get(), eyeimage_left.get(), histogram_horizontal_left, 
                                                histogram_vertical_left, vector_horizontal_left.get(), vector_vertical_left.get(), histPositionSegmentedPixels_left);

	if(blinkdet.getState() >= 2 && blinkdet_left.getState() >= 2) {
		blink = true;
		//cout << "BLINK!! RIGHT EYE STATE: " << blinkdet.getState() << "LEFT EYE STATE: " << blinkdet_left.getState() <<endl;
	}
	else {
		blink = false;
	}

}

bool EyeExtractor::isBlinking() {
	return blink;
}


EyeExtractor::EyeExtractor(const PointTracker &tracker):
    tracker(tracker), 
    eyefloat2(cvCreateImage( eyesize, IPL_DEPTH_32F, 1 )),
    eyegrey(cvCreateImage( eyesize, 8, 1 )),
    eyefloat(cvCreateImage( eyesize, IPL_DEPTH_32F, 1 )),
    eyeimage(cvCreateImage( eyesize, 8, 3 )),
    histogram_horizontal(cvCreateImage( eyesize, 8, 3 )),
    histogram_vertical(cvCreateImage( cvSize(eyesize.height, eyesize.width), 8, 3 )),
    vector_horizontal(new vector<int> (eyesize.width,0)),
    vector_vertical(new vector<int> (eyesize.height,0)),
    eyeGraySegmented(cvCreateImage( eyesize, IPL_DEPTH_32F, 1 )),
 	// ONUR DUPLICATED CODE FOR LEFT EYE
    eyefloat2_left(cvCreateImage( eyesize, IPL_DEPTH_32F, 1 )),
    eyegrey_left(cvCreateImage( eyesize, 8, 1 )),
    eyefloat_left(cvCreateImage( eyesize, IPL_DEPTH_32F, 1 )),
    eyeimage_left(cvCreateImage( eyesize, 8, 3 )),
    histogram_horizontal_left(cvCreateImage( eyesize, 8, 3 )),
    histogram_vertical_left(cvCreateImage( cvSize(eyesize.height, eyesize.width), 8, 3 )),
    vector_horizontal_left(new vector<int> (eyesize.width,0)),
    vector_vertical_left(new vector<int> (eyesize.height,0)),
    eyeGraySegmented_left(cvCreateImage( eyesize, IPL_DEPTH_32F, 1 )),
	blink(false),

    histPositionSegmentedPixels (new vector<vector<int> >),
    histPositionSegmentedPixels_left (new vector<vector<int> >),

    extractFeatures(eyesize)
{
}

void EyeExtractor::extractEye(const IplImage *origimage) 
    throw (TrackingException) 
{
    static int frame_no = 1;
    string file;
    char buffer [100];

    //if (!tracker.status[tracker.eyepoint1])
	//throw TrackingException();



    double x0 = tracker.currentpoints[tracker.eyepoint1].x;
    double y0 = tracker.currentpoints[tracker.eyepoint1].y;
    double x1 = tracker.currentpoints[tracker.eyepoint2].x;
    double y1 = tracker.currentpoints[tracker.eyepoint2].y;

    double dh = sqrt(pow((x1-x0),2) + pow((y1-y0),2)); 
    double dx = x1-x0;
    double dy = y1-y0;
    
    double alpha = atan2(dy, dx);// * 180 / PI;

    x0 -= dh/30*sin(alpha);
    y0 += dh/30*cos(alpha);
    x1 -= dh/30*sin(alpha);
    y1 += dh/30*cos(alpha);

    double factor = 0.17;
    double xfactor = 0.05;
    double yfactor = 0.20 * (x0 < x1 ? -1 : 1);
    double L = factor / eyedx;
    double LL = x0 < x1? L : -L;
    float matrix[6] = 
	{LL*(x1-x0), LL*(y0-y1), 
	 x0 + factor * ((1-xfactor)*(x1-x0) + yfactor * (y0-y1)),
	 LL*(y1-y0), LL*(x1-x0), 
	 y0 + factor * ((1-xfactor)*(y1-y0) + yfactor * (x1-x0))};
    CvMat M = cvMat( 2, 3, CV_32F, matrix );

    cvGetQuadrangleSubPix( origimage, eyeimage.get(), &M);
    cvCvtColor(eyeimage.get(), eyegrey.get(), CV_RGB2GRAY);

/*
// ------------------ Arcadi -------------------

    //if (saveImage == true){
        cout << "SAVING IMAGES" << endl;
	    file=sprintf (buffer, "../Images/Colour/Eye_Image_%d.jpg", frame_no);

	    cvSaveImage(buffer, eyeimage.get());

	    file=sprintf (buffer, "../Images/Grey/Eye_Image_%d.jpg", frame_no);

	    cvSaveImage(buffer, eyegrey.get());

        frame_no++;
	//}

// ---------------------------------------------
*/
	extractLeftEye(origimage, x0, y0, x1, y1);
	
    processEye();
}

void EyeExtractor::extractLeftEye(const IplImage *origimage, double x1, double y1, double x0, double y0) 
    throw (TrackingException) 
{
    //if (!tracker.status[tracker.eyepoint1])
	//throw TrackingException();

    double factor = 0.17;
    double xfactor = 0.05;
    double yfactor = 0.20 * (x0 < x1 ? -1 : 1);
    double L = factor / eyedx;
    double LL = x0 < x1? L : -L;
    float matrix[6] = 
	{LL*(x1-x0), LL*(y0-y1), 
	 x0 + factor * ((1-xfactor)*(x1-x0) + yfactor * (y0-y1)),
	 LL*(y1-y0), LL*(x1-x0), 
	 y0 + factor * ((1-xfactor)*(y1-y0) + yfactor * (x1-x0))};
    CvMat M = cvMat( 2, 3, CV_32F, matrix );

	float matrix2[6] = 
	{LL*(x1-x0), LL*(y0-y1), 
	 x0 + 2*64 + factor * ((1-xfactor)*(x1-x0) + yfactor * (y0-y1)),
	 LL*(y1-y0), LL*(x1-x0), 
	 y0 + factor * ((1-xfactor)*(y1-y0) + yfactor * (x1-x0))};
    
    cvGetQuadrangleSubPix( origimage, eyeimage_left.get(), &M);
    cvCvtColor(eyeimage_left.get(), eyegrey_left.get(), CV_RGB2GRAY);
}
EyeExtractor::~EyeExtractor(void) {
}


