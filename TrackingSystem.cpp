#include "TrackingSystem.h"
#include "FeatureDetector.h"
#include "BlinkDetector.h"

static double quadraticminimum(double xm1, double x0, double xp1) {
//     cout << "values:" << xm1 << " " << x0 << " " << xp1 << endl;
    return (xm1 - xp1) / (2*(xp1 + xm1 - 2*x0));
}

static Point subpixelminimum(const IplImage *values) {
    CvPoint maxpoint;
    cvMinMaxLoc(values, NULL, NULL, &maxpoint);
//     cout << "max: " << maxpoint.x << " " << maxpoint.y << endl;
    
    int x = maxpoint.x;
    int y = maxpoint.y;

    Point p(x, y);

    if (x > 0 && x < 6)
	p.x += quadraticminimum(cvGetReal2D(values, y, x-1),
				cvGetReal2D(values, y, x+0),
				cvGetReal2D(values, y, x+1));
    
    if (y > 0 && y < 4)
	p.y += quadraticminimum(cvGetReal2D(values, y-1, x),
				cvGetReal2D(values, y+0, x),
				cvGetReal2D(values, y+1, x));

    return p;
}


TrackingSystem::TrackingSystem(CvSize size):
    tracker(size), headtracker(tracker), headcomp(headtracker), eyex(tracker) 
{}


void TrackingSystem::doprocessing(const IplImage *frame, 
				  IplImage *image) 
{
	if(tracker_status != STATUS_PAUSED) {
    tracker.track(frame, 2);
    if (tracker.countactivepoints() < 4) {
	tracker.draw(image);
	throw TrackingException();
    }

    headtracker.updatetracker();
    eyex.extractEye(frame);	// throws Tracking Exception
    gazetracker.update(eyex.eyefloat.get(), eyex.eyegrey.get());
    gazetracker.update_left(eyex.eyefloat_left.get(), eyex.eyegrey_left.get());

    displayeye(image);
    tracker.draw(image);
    headtracker.draw(image);

}
}

void TrackingSystem::displayeye(IplImage *image) 
{
    CvSize eyesize = EyeExtractor::eyesize;
    int basex, basey, stepx, stepy;
    int eyedx = 2*EyeExtractor::eyedx;
    int eyedy = 2*EyeExtractor::eyedy;

    static IplImage *eyegreytemp = cvCreateImage( eyesize, 8, 1 );
    static FeatureDetector features(EyeExtractor::eyesize);
    static FeatureDetector features_left(EyeExtractor::eyesize);

    features.addSample(eyex.eyegrey.get());

    basex = 5; basey = 5;
    stepx = 5+eyedx; stepy = 5+eyedy;

    gazetracker.draw(image, eyedx, eyedy);

    // RIGHT EYE
    // Mean eye image
    cvConvertScale(features.getMean().get(),  eyegreytemp);
    cvSetImageROI(image, cvRect(basex, basey, eyedx, eyedy));
    cvCvtColor(eyegreytemp, image, CV_GRAY2RGB);

    // Grey eye image
    cvSetImageROI(image, cvRect(basex, basey + stepy, eyedx, eyedy));
    cvCvtColor(eyex.eyegrey.get(), image, CV_GRAY2RGB);

    // Color eye image
    cvSetImageROI(image, cvRect(basex, basey + stepy*2, eyedx, eyedy));
    cvCopy(eyex.eyeimage.get(), image); //, eyex.eyeimage.get());

    // Horizontal histogram Segmentation eye image
    cvSetImageROI(image, cvRect(basex, basey + stepy*3, eyedx, eyedy));
    cvCopy(eyex.histogram_horitzontal, image); //, eyex.eyeimage.get());

    // Vertical histogram Segmentation eye image
    cvSetImageROI(image, cvRect(basex, basey + stepy*4, eyedy, eyedx));
    cvCopy(eyex.histogram_vertical, image); //, eyex.eyeimage.get());



	// ONUR DUPLICATED CODE FOR LEFT EYE
    features_left.addSample(eyex.eyegrey_left.get());

    // Mean eye image
    cvConvertScale(features_left.getMean().get(),  eyegreytemp);
    cvSetImageROI(image, cvRect(basex + stepx, basey, eyedx, eyedy));
    cvCvtColor(eyegreytemp, image, CV_GRAY2RGB);

    // Grey eye image
    cvSetImageROI(image, cvRect(basex + stepx, basey + stepy, eyedx, eyedy));
    cvCvtColor(eyex.eyegrey_left.get(), image, CV_GRAY2RGB);

    // Color eye image
    cvSetImageROI(image, cvRect(basex + stepx, basey + stepy*2, eyedx, eyedy));
    cvCopy(eyex.eyeimage_left.get(), image); //eyex.eyeimage_left.get());

    // Horizontal histogram Segmentation eye image
    cvSetImageROI(image, cvRect(basex + stepx, basey + stepy*3, eyedx, eyedy));
    cvCopy(eyex.histogram_horitzontal_left, image); //, eyex.eyeimage.get());

    // Vertical histogram Segmentation eye image
    cvSetImageROI(image, cvRect(basex + stepx, basey + stepy*4, eyedy, eyedx));
    cvCopy(eyex.histogram_vertical_left, image); //, eyex.eyeimage.get());





// //     features.getVariance(eyegreytemp);
// //     cvSetImageROI(image, cvRect(basex, basey+stepy*2, eyedx*2, eyedy*2));
// //     cvCvtColor(eyegreytemp, image, CV_GRAY2RGB);

//     // compute the x-derivative
    
//     static IplImage *eyegreytemp1 = cvCreateImage( eyesize, IPL_DEPTH_32F, 1 );
//     static scoped_ptr<IplImage> 
// 	eyegreytemp2(cvCreateImage(eyesize, IPL_DEPTH_32F, 1));

// //     static IplImage *eyegreytemp3 = cvCreateImage( eyesize, IPL_DEPTH_32F, 1 );
//     static IplImage *eyegreytemp4 = cvCreateImage(cvSize(7,5),IPL_DEPTH_32F,1);
//     features.getMean(eyegreytemp1);
//     cvConvertScale(eyex.eyegrey, eyegreytemp2.get());
//     double distance = cvNorm(eyegreytemp1, eyegreytemp2.get(), CV_L2);
//     static BlinkDetector blinkdet;
//     blinkdet.update(eyegreytemp2);
//     cout << "distance: " << distance 
// 	 << " blink: " << blinkdet.getState() <<endl;
    
//     cvSetImageROI(eyegreytemp1, cvRect(2,2,eyedx*2-6,eyedy*2-4));
//     cvMatchTemplate(eyegreytemp2.get(), eyegreytemp1, eyegreytemp4, CV_TM_SQDIFF);
//     cvResetImageROI(eyegreytemp1);

//     for(int i=0; i<5; i++) {
// 	cout << endl;
// 	for(int j=0; j<7; j++)
// 	    cout << cvGetReal2D(eyegreytemp4, i, j)/1e6 << " ";
//     }
//     cout << endl;

//     CvPoint maxpoint;
//     cvMinMaxLoc(eyegreytemp4, NULL, NULL, &maxpoint);
//     cout << "max: " << maxpoint.x << " " << maxpoint.y << endl;

//     cvSetImageROI(eyex.eyegrey, 
// 		  cvRect(maxpoint.x, maxpoint.y, eyedx*2-6, eyedy*2-4));
//     cvSetImageROI(image, cvRect(basex, basey+stepy*3, eyedx*2-6, eyedy*2-4));
//     cvCvtColor(eyex.eyegrey, image, CV_GRAY2RGB);
//     cvResetImageROI(eyex.eyegrey);

//     Point mxpoint = subpixelminimum(eyegreytemp4);
//     cout << "max: " << mxpoint.x << " " << mxpoint.y << endl;

//     tracker.currentpoints[0].x += 0.4 * (mxpoint.x - 3.0);
//     tracker.currentpoints[0].y += 0.4 * (mxpoint.y - 2.0);

//     cvSub(eyex.eyefloat, eyegreytemp1, eyegreytemp3);
//     cvSetImageROI(eyegreytemp1, cvRect(0,0,eyedx*2-1,eyedy*2));
//     cvSetImageROI(eyegreytemp2, cvRect(1,0,eyedx*2-1,eyedy*2));
//     cvCopy(eyegreytemp1, eyegreytemp2);
//     cvResetImageROI(eyegreytemp1);
//     cvResetImageROI(eyegreytemp2);
//     cvAddS(eyegreytemp1, cvScalar(128.0), eyegreytemp1);
//     cvSub(eyegreytemp1, eyegreytemp2, eyegreytemp1);
    
//     cvSetImageROI(image, cvRect(basex, basey+stepy*2, eyedx*2, eyedy*2));
//     cvConvertScale(eyegreytemp1, eyegreytemp);

// //     cvMul(eyegreytemp3, eyegreytemp1, eyegreytemp3);
// //     cout << "x movement: " << cvAvg(eyegreytemp3).val[0] << endl;
    

//     cvCvtColor(eyegreytemp, image, CV_GRAY2RGB);

    

    cvResetImageROI(image);
}
