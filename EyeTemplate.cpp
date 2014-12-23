#include "EyeTemplate.h"
#include <math.h>
#define HORIZONTAL_BIN_SIZE 128
#define VERTICAL_BIN_SIZE 64
#define ExtraSize 0

static std::vector<int> vector_static_horizontal(HORIZONTAL_BIN_SIZE, 0);
static std::vector<int> vector_static_vertical(VERTICAL_BIN_SIZE, 0);

void EyeTemplate::setSize(IplImage* eyesize)
{
	sizeImageDisk = 20;

	for (int j=0; j<size(irisTemplateDisk); j++){
		irisTemplateDisk[j] = ConstructTemplateDisk(sizeImageDisk+j*2);

		Gaussian2D[j] = cvCreateImage(cvSize(eyesize->width - irisTemplateDisk[j]->width + 1, eyesize->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		/* Gaussian2D[j] = TODO ONUR UNCOMMENTED, NOT NECESSARY */
		CreateTemplateGausian2D(Gaussian2D[j]);
	}
}

Point* EyeTemplate::ProcessToExtractFeatures(	IplImage* Temporary, IplImage* TemporaryColor){
	static int counter = 0;

    string file;
    char buffer [100];



	setSize(Temporary);

    static double *bestval;
    static CvPoint *bestloc;
	
	int j;
	IplImage* Matches;

	int comparison_method = CV_TM_CCOEFF_NORMED; //CV_TM_CCOEFF_NORMED;

	cout << "Trying for different disk sizes" << endl;
	for (j=0; j<size(irisTemplateDisk); j++){

	cout << "Size: " << j  << endl;
		Matches = cvCreateImage(cvSize(Temporary->width - irisTemplateDisk[j]->width + 1, Temporary->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		cvMatchTemplate(Temporary, irisTemplateDisk[j], Matches, comparison_method);

		cvMul(Matches, Gaussian2D[j], Matches);
		
		cvMinMaxLoc(Matches, &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

	}

	cout << "Finished trying for different disk sizes" << endl;
	double maxProbability = (double) MaxVal[0];
	//double maxProbability = (double) MinVal[0] * (double) (a[0].val[0]) * (double) 1 / ((double) (30+(0*2)));
	double tmp;
	//cout << "j=0" << ": " << MinVal[j] << " x " << a[0].val[0] << " = " << maxProbability << endl;
	int i = 0;
	for (j=1; j<size(irisTemplateDisk); j++){
		// TODO ONUR : Removed multiplication by disk size for now
		tmp = (double) MaxVal[j] * (1+j*j/300.0);
		//tmp = MaxVal[j];
		//tmp = (double) MinVal[j] * (double) (a[j].val[0]) * (double) 1 / ((double) (30+(0*2)));

		if (tmp > maxProbability){
			maxProbability = tmp;
			//maxProbability = (double) MinVal[j] * (double) (a[j].val[0]) * (double) 1 / ((double) (30+(0*2)));
			i = j;
		}
	}


//	cout << "Max probability: " << maxProbability << endl;

/*
//// ONUR
	double tempMinVal, tempMaxVal;
    CvPoint tempMinLoc, tempMaxLoc;
	Matches = cvCreateImage(cvSize(Temporary->width - irisTemplateDisk[i]->width + 1, Temporary->height - irisTemplateDisk[i]->height + 1), IPL_DEPTH_32F, 1);

	cvMatchTemplate(Temporary, irisTemplateDisk[i], Matches, comparison_method);
	cvMinMaxLoc(Matches, &tempMinVal, &tempMaxVal, &tempMinLoc, &tempMaxLoc);
	cvConvertScale(Matches, Matches, 1/tempMaxVal, 0);
	cvSaveImage("../1_matches.jpg", Matches);
	cvMul(Matches, Gaussian2D[i], Matches);
	cvMinMaxLoc(Matches, &tempMinVal, &tempMaxVal, &tempMinLoc, &tempMaxLoc);
	cvConvertScale(Matches, Matches, 1/tempMaxVal, 0);
	cvSaveImage("../2_matches_mult.jpg", Matches);
	//cvSaveImage("../2_gaussian.jpg", Gaussian2D[i]);
//////////
*/
/*
	cvSetImageROI(Temporary, cvRect(MaxLoc[i].x - (ExtraSize * 2), MaxLoc[i].y, irisTemplateDisk[i]->width + (ExtraSize * 4), irisTemplateDisk[i]->height + ExtraSize));

	IplImage* finalIris = Segmentation(Temporary);

	IplImage* blackAndWitheIris = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,1);

	cvRectangle(blackAndWitheIris, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0, 0, 0), -1, 8, 0);

	cvSetImageROI(blackAndWitheIris, cvRect(MaxLoc[i].x - (ExtraSize * 2), MaxLoc[i].y, irisTemplateDisk[i]->width + (ExtraSize * 4), irisTemplateDisk[i]->height + ExtraSize));

	cvCopy(finalIris, blackAndWitheIris);

	cvResetImageROI(blackAndWitheIris);

	// Iris detected

	cvSetImageROI(TemporaryColor, cvRect(MaxLoc[i].x - (ExtraSize * 2), MaxLoc[i].y, irisTemplateDisk[i]->width + (ExtraSize * 4), irisTemplateDisk[i]->height + ExtraSize));

	IplImage* aux = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,3);

	cvRectangle(aux, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0,255,0), -1, 8, 0);

	cvCopy(aux, TemporaryColor);

	cvResetImageROI(Temporary);
	cvResetImageROI(TemporaryColor);
*/
	//cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
	//cvMoveWindow("mainWin", 0, 0);

	//cvShowImage("mainWin", TemporaryColor );

	//cin.get();


	//file=sprintf (buffer, "./imgs/Eye_Image_%d_size_%d.jpg", counter, sizeImageDisk+i*2);
	//cvSaveImage(buffer, Temporary);
	//counter++;

	return new Point(MaxLoc[i].x + irisTemplateDisk[i]->width/2, MaxLoc[i].y + irisTemplateDisk[i]->height/2);
}

IplImage* EyeTemplate::ConstructTemplateDisk(int sizeDisk) {

	CvPoint center;
	center.x = floor(sizeDisk/2);
	center.y = floor(sizeDisk/2);

	CvScalar color;
	color = CV_RGB(0, 0, 0);


	IplImage* irisTemplateDisk = cvCreateImage(cvSize(sizeDisk, sizeDisk), IPL_DEPTH_8U, 1);
	cvSet(irisTemplateDisk, cvScalar(255,255,255));
	cvCircle( irisTemplateDisk, center, sizeDisk/2, color, -1);

	return irisTemplateDisk;
}

IplImage* EyeTemplate::Segmentation(IplImage* Temporary) {

	IplImage* im_bw = cvCreateImage(cvGetSize(Temporary),IPL_DEPTH_8U,1);
	cvThreshold(Temporary, im_bw, 80, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	cvNot(im_bw, im_bw);

	return im_bw;
}

IplImage* EyeTemplate::CreateTemplateGausian2D(IplImage* Gaussian2D) {

	CvPoint center;
	center.x = floor(Gaussian2D->width/2);
	center.y = floor(Gaussian2D->height/2);

	float tmp = 0;
	float sigma = 250;	// With this sigma, the furthest eye pixel (corners) have around 0.94 probability
	float max_prob = exp( - (0) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);

	for (int x = 0; x<Gaussian2D->width; x++) {
		for (int y = 0; y<Gaussian2D->height; y++) {
			float fx = abs(x - center.x);
			float fy = abs(y - center.y);

			tmp = exp( - (fx * fx + fy * fy) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);
			tmp = tmp / max_prob;	// Divide by max prob. so that values are in range [0, 1]

			CvScalar tmp_scalar;
			tmp_scalar.val[0] = tmp;
			cvSet2D(Gaussian2D, y, x, tmp_scalar);
		}
	}
    
	return Gaussian2D;
}