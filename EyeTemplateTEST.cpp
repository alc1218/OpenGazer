#include "EyeTemplateTEST.h"
#include <math.h>
#define HORIZONTAL_BIN_SIZE 128
#define VERTICAL_BIN_SIZE 64
#define ExtraSize 0
//#define SmoothBlockSize 29

static std::vector<int> vector_static_horizontal(HORIZONTAL_BIN_SIZE, 0);
static std::vector<int> vector_static_vertical(VERTICAL_BIN_SIZE, 0);

void EyeTemplateTEST::setSize(IplImage* eyesize, float sigma, int sizeImageDisk)
{

	IplImage* elipse_rgb = (IplImage*) cvLoadImage("./elipse.jpg");
	elipse_gray = cvCreateImage(cvGetSize(elipse_rgb),IPL_DEPTH_8U,1);
	cvCvtColor(elipse_rgb,elipse_gray,CV_RGB2GRAY);

	for (int j=0; j<size(irisTemplateDisk); j++){
		irisTemplateDisk[j] = ConstructTemplateDisk(sizeImageDisk + j);

		Gaussian2D[j] = cvCreateImage(cvSize(eyesize->width - irisTemplateDisk[j]->width + 1, eyesize->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		/* Gaussian2D[j] = TODO ONUR UNCOMMENTED, NOT NECESSARY */
		CreateTemplateGausian2D(Gaussian2D[j], sigma);
	}
}

Point* EyeTemplateTEST::ProcessToExtractFeatures(	IplImage* Temporary, IplImage* TemporaryColor, float sigma, int SmoothBlockSize, int sizeImageDisk){

	setSize(Temporary, sigma, sizeImageDisk);

	IplImage* Temporary_elipsed = cvCreateImage(cvGetSize(Temporary),IPL_DEPTH_8U,1);

	//cout << "Size of Temporary_elipsed: " << Temporary_elipsed->width << ", " << Temporary_elipsed->height << endl;


	//cvRectangle(Temporary_elipsed, cvPoint(0,0), cvPoint(Temporary_elipsed->width,Temporary_elipsed->height), cvScalar(100,100,100), -1, 8, 0);
	//cvCopy(Temporary, Temporary_elipsed, elipse_gray);

	cvCopy(Temporary, Temporary_elipsed);
	
	int j;

	int comparison_method = CV_TM_CCOEFF_NORMED; //CV_TM_CCOEFF_NORMED;


	int dist = 1000;

	int winner = -1;

	//cout << "Trying for different disk sizes" << endl;
	for (j=0; j<size(irisTemplateDisk); j++){

	//cout << "Size: " << j  << endl;
		Matches[j] = cvCreateImage(cvSize(Temporary_elipsed->width - irisTemplateDisk[j]->width + 1, Temporary_elipsed->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		MatchesSmoothed[j] = cvCreateImage(cvSize(Matches[j]->width - (SmoothBlockSize - 1)/2, Matches[j]->height - (SmoothBlockSize - 1)/2), IPL_DEPTH_32F, 1);

		cvMatchTemplate(Temporary_elipsed, irisTemplateDisk[j], Matches[j], comparison_method);

		cvMul(Matches[j], Gaussian2D[j], Matches[j]);

		cvSmooth(Matches[j], Matches[j], CV_BLUR_NO_SCALE, SmoothBlockSize);

		cvSetImageROI(Matches[j], cvRect((SmoothBlockSize - 1)/2, (SmoothBlockSize - 1)/2, Matches[j]->width - (SmoothBlockSize - 1)/2, Matches[j]->height - (SmoothBlockSize - 1)/2));

		CvSize p = cvGetSize(Matches[j]);

	    cvCopy(Matches[j], MatchesSmoothed[j]);

		cvResetImageROI(Matches[j]);

		cvMinMaxLoc(MatchesSmoothed[j], &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

		if ( dist > sqrt(abs(218 * 128/400 - MaxLoc[j].x + (SmoothBlockSize - 1)/2) * abs(218 * 128/400 - MaxLoc[j].x + (SmoothBlockSize - 1)/2) + abs((155 - 50) * 64/200 - MaxLoc[j].y + (SmoothBlockSize - 1)/2) * abs((155 - 50) * 64/200 - MaxLoc[j].y + (SmoothBlockSize - 1)/2))) {

			dist = sqrt(abs(218 * 128/400 - MaxLoc[j].x + (SmoothBlockSize - 1)/2) * abs(218 * 128/400 - MaxLoc[j].x + (SmoothBlockSize - 1)/2) + abs((155 - 50) * 64/200 - MaxLoc[j].y + (SmoothBlockSize - 1)/2) * abs((155 - 50) * 64/200 - MaxLoc[j].y + (SmoothBlockSize - 1)/2));
			winner = j;
		}

	}

	//cout << "WINNER: " << winner + 1 << ", " << MaxLoc[winner].x + (SmoothBlockSize - 1)/2 << ", " << MaxLoc[winner].y + (SmoothBlockSize - 1)/2 << endl;
	//cout << "DIST: " << dist << endl;

	double maxProbability = (double) MaxVal[0];

	double tmp;

	int i = 0;
	for (j=1; j<size(irisTemplateDisk); j++){

		tmp = (double) MaxVal[j] * (double) (1+j*j/300.0);

		if (tmp > maxProbability){
			
			maxProbability = tmp;
			i = j;

		}
	}





	/*

	double max = -100;
	double min = 100;

    for(int row = 0; row < MatchesSmoothed[i]->height; row++)
    {
        for(int col = 0; col < MatchesSmoothed[i]->width; col++)
        {

        	if (max < cvGet2D(MatchesSmoothed[i], row, col).val[0]) {
        		max = cvGet2D(MatchesSmoothed[i], row, col).val[0];
        	}

        	if (min > cvGet2D(MatchesSmoothed[i], row, col).val[0]) {
        		min = cvGet2D(MatchesSmoothed[i], row, col).val[0];
        	}
        }
    }


    for(int row = 0; row < MatchesSmoothed[i]->height; row++)
    {
        for(int col = 0; col < MatchesSmoothed[i]->width; col++)
        {

        	double aux = cvGet2D(MatchesSmoothed[i], row, col).val[0];

			CvScalar tmp_scalar;
			tmp_scalar.val[0] = ((aux + abs(min)) / (abs(min) + max) ) * 255;
			cvSet2D(MatchesSmoothed[i], row, col, tmp_scalar);

            //printf("%f, ", cvGet2D(MatchesSmoothed[i], row, col).val[0]);
        }
        //printf("\n");
    }

	//cout << "VALOR DE I: " << i << endl;

	static int frame_number_detect_eye = 0;

	string file_detect_eye;
    char buffer_detect_eye [100];


    //cout << "SAVING IMAGES Probabilities" << endl;
    file_detect_eye=sprintf (buffer_detect_eye, "imgs/Probabilities/%d.jpg", frame_number_detect_eye);

    cvSaveImage(buffer_detect_eye, MatchesSmoothed[i]);

    frame_number_detect_eye++;

    */


    /*

    for(int row = 0; row < MatchesSmoothed[i]->height; row++)
    {
        for(int col = 0; col < MatchesSmoothed[i]->width; col++)
        {
            printf("%f, ", cvGet2D(MatchesSmoothed[i], row, col).val[0]);
        }
        printf("\n");
    }
    

	cin.get();

	*/

	return new Point(MaxLoc[i].x + (SmoothBlockSize - 1)/2, MaxLoc[i].y + (SmoothBlockSize - 1)/2 + ExtraSize);

	//return new Point(MaxLoc[i].x + irisTemplateDisk[i]->width/2, MaxLoc[i].y + irisTemplateDisk[i]->height/2 + ExtraSize);
}

IplImage* EyeTemplateTEST::ConstructTemplateDisk(int sizeDisk) {

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

IplImage* EyeTemplateTEST::CreateTemplateGausian2D(IplImage* Gaussian2D, float sigma) {

	CvPoint center;
	center.x = floor(Gaussian2D->width/2);
	center.y = floor(Gaussian2D->height/2);

	float tmp = 0;
	//float sigma = 150;	// With this sigma, the furthest eye pixel (corners) have around 0.94 probability
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