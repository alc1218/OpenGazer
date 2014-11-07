#include "ExtractEyeFeaturesSegmentation.h"
#include <math.h>
#define HORIZONTAL_BIN_SIZE 128
#define VERTICAL_BIN_SIZE 64

ExtractEyeFeaturesSegmentation::ExtractEyeFeaturesSegmentation(CvSize eyesize)
{
	sizeImageDisk = 30;
	for (int j=0; j<size(irisTemplateDisk); j++){
		irisTemplateDisk[j] = constructTemplateDisk(sizeImageDisk+j*2);

		Gaussian2D[j] = cvCreateImage(cvSize(eyesize.width - irisTemplateDisk[j]->width + 1, eyesize.height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		Gaussian2D[j] = CreateTemplateGausian2D(Gaussian2D[j]);
	}
}

void ExtractEyeFeaturesSegmentation::processToExtractFeatures(	IplImage* Temporary, IplImage* TemporaryColor, 
																IplImage* histograma_horizontal, IplImage* histograma_vertical, 
																std::vector<int>* vector_horizontal, std::vector<int>* vector_vertical){
	
	int j;
	IplImage* Matches;
	for (j=0; j<size(irisTemplateDisk); j++){

		Matches = cvCreateImage(cvSize(Temporary->width - irisTemplateDisk[j]->width + 1, Temporary->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		cvMatchTemplate(Temporary, irisTemplateDisk[j], Matches, CV_TM_CCORR);

		cvMinMaxLoc(Matches, &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

		cvMul(Matches, Gaussian2D[j], Matches);
		
		cvMinMaxLoc(Matches, &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

		/*
		
		cvSetImageROI(Temporary, cvRect(MinLoc[j].x, MinLoc[j].y, MinLoc[j].x+irisTemplateDisk[j]->width, MinLoc[j].y+irisTemplateDisk[j]->height));

		a[j] = cvAvg(Temporary);

		cvResetImageROI(Temporary);

		*/

	}

	double maxProbability = (double) MinVal[0] * (double) 1 / ((double) (30+(0*2)));
	//double maxProbability = (double) MinVal[0] * (double) (a[0].val[0]) * (double) 1 / ((double) (30+(0*2)));
	double tmp;
	//cout << "j=0" << ": " << MinVal[j] << " x " << a[0].val[0] << " = " << maxProbability << endl;
	int i = 0;
	for (j=1; j<size(irisTemplateDisk); j++){

		tmp = (double) MinVal[j] * ((double) 1/ (double) (30+(j*2)));
		//tmp = (double) MinVal[j] * (double) (a[j].val[0]) * (double) 1 / ((double) (30+(0*2)));

		if (tmp < maxProbability){
			maxProbability = (double) MinVal[j] * ((double) 1/ (double) (30+(j*2)));
			//maxProbability = (double) MinVal[j] * (double) (a[j].val[0]) * (double) 1 / ((double) (30+(0*2)));
			i = j;
		}
	}

	cvSetImageROI(Temporary, cvRect(MinLoc[i].x-5, MinLoc[i].y, irisTemplateDisk[i]->width+10, irisTemplateDisk[i]->height));

	IplImage* finalIris = Segmentation(Temporary);

	IplImage* blackAndWitheIris = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,1);

	cvRectangle(blackAndWitheIris, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0, 0, 0), -1, 8, 0);

	cvSetImageROI(blackAndWitheIris, cvRect(MinLoc[i].x-5, MinLoc[i].y, irisTemplateDisk[i]->width+10, irisTemplateDisk[i]->height));

	cvCopy(finalIris, blackAndWitheIris);

	cvResetImageROI(blackAndWitheIris);

	Histogram(blackAndWitheIris, histograma_horizontal, histograma_vertical, vector_horizontal, vector_vertical);

	// Iris detected

	cvSetImageROI(TemporaryColor, cvRect(MinLoc[i].x-5, MinLoc[i].y, irisTemplateDisk[i]->width+10, irisTemplateDisk[i]->height));

	IplImage* aux = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,3);

	cvRectangle(aux, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0,255,0), -1, 8, 0);

	cvCopy(aux, TemporaryColor, finalIris);

	cvResetImageROI(Temporary);
	cvResetImageROI(TemporaryColor);

}

IplImage* ExtractEyeFeaturesSegmentation::constructTemplateDisk(int sizeDisk) {

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

IplImage* ExtractEyeFeaturesSegmentation::Segmentation(IplImage* Temporary) {

	IplImage* im_bw = cvCreateImage(cvGetSize(Temporary),IPL_DEPTH_8U,1);
	cvThreshold(Temporary, im_bw, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	IplImage* im_bw2 = cvCreateImage(cvGetSize(Temporary),IPL_DEPTH_8U,3);

	cvCvtColor(im_bw,im_bw2,CV_GRAY2RGB);

	cvInRangeS(im_bw, cvScalar(0,0,0), cvScalar(0,255,0), im_bw);

	return im_bw;
}

IplImage* ExtractEyeFeaturesSegmentation::CreateTemplateGausian2D(IplImage* Gaussian2D) {

	CvPoint center;
	center.x = floor(Gaussian2D->width/2);
	center.y = floor(Gaussian2D->height/2);

	float tmp = 0;

	for (int x = 0; x<Gaussian2D->width; x++) {
		for (int y = 0; y<Gaussian2D->height; y++) {
			float sigma = 75;
			float fx = abs(x - center.x);
			float fy = abs(y - center.y);

			tmp = 1 - 1000 * exp( - (fx * fx + fy * fy) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);

			CvScalar tmp_scalar;
			tmp_scalar.val[0] = tmp;
			cvSet2D(Gaussian2D, y, x, tmp_scalar);
		}
	}
	
	/*
    for(int row = 0; row < Gaussian2D->height; row++)
    {
        for(int col = 0; col < Gaussian2D->width; col++)
        {
            printf("%f, ", cvGet2D(Gaussian2D, row, col).val[0]);
        }
        printf("\n");
    }
    

	cin.get();
*/
	return Gaussian2D;
}

IplImage* ExtractEyeFeaturesSegmentation::Histogram(	IplImage* blackAndWitheIris, IplImage* hist_horizontal, 
														IplImage* hist_vertical, std::vector<int>* blackAndWitheIris_summed_horizontal, 
														std::vector<int>* blackAndWitheIris_summed_vertical) {

	int width = blackAndWitheIris->widthStep;
	uchar* data = (uchar*) blackAndWitheIris->imageData;
	
	//std::vector<int> blackAndWitheIris_summed_horizontal (blackAndWitheIris->width,0); 
	//std::vector<int> blackAndWitheIris_summed_vertical (blackAndWitheIris->height,0); 

	//int blackAndWitheIris_summed_horizontal[blackAndWitheIris->width];
	//int blackAndWitheIris_summed_vertical[blackAndWitheIris->height];


	//memcpy(blackAndWitheIris_summed_horizontal, array_horizontal, HORIZONTAL_BIN_SIZE*sizeof(int));
	//memcpy(blackAndWitheIris_summed_vertical, array_vertical, VERTICAL_BIN_SIZE*sizeof(int));

	for (int i=0;i<blackAndWitheIris->width;i++)
	{

	    for (int k=0;k<blackAndWitheIris->height;k++)
	    {

			if ((int) data[i + k*width] == 255) {
				cout << "valor: " << blackAndWitheIris_summed_horizontal->size() << endl;
				cin.get();
	    		//(blackAndWitheIris_summed_horizontal[i])++;
	    		//(blackAndWitheIris_summed_vertical[k])++;
			}
	    }
	}

	//IplImage* hist_horizontal = cvCreateImage(cvSize(blackAndWitheIris->width, blackAndWitheIris->height),IPL_DEPTH_8U,1);
/*
	cvRectangle(hist_horizontal, cvPoint(0,0), cvPoint(hist_horizontal->width, hist_horizontal->height), cvScalar(0,0,0), -1, 8, 0);

	for (int i=0;i<blackAndWitheIris->width;i++) {
	    cvLine(hist_horizontal, 
		   cvPoint(i, hist_horizontal->height),
		   cvPoint(i, hist_horizontal->height - blackAndWitheIris_summed_horizontal[i]),
		   CV_RGB(255,255,255));
	}

	//IplImage* hist_vertical = cvCreateImage(cvSize(blackAndWitheIris->height, blackAndWitheIris->width),IPL_DEPTH_8U,1);

	cvRectangle(hist_vertical, cvPoint(0,0), cvPoint(hist_vertical->width, hist_vertical->height), cvScalar(0,0,0), -1, 8, 0);

	for (int i=0;i<blackAndWitheIris->width;i++) {
	    cvLine(hist_vertical, 
		   cvPoint(i, hist_vertical->height),
		   cvPoint(i, hist_vertical->height - blackAndWitheIris_summed_vertical[i]),
		   CV_RGB(255,255,255));
	}
*/


	/*

	cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
	cvMoveWindow("mainWin", hist_vertical->width, hist_vertical->height);

	cvShowImage("mainWin", hist_vertical );

	cin.get();

	*/

}



ExtractEyeFeaturesSegmentation::~ExtractEyeFeaturesSegmentation(void) {
}


