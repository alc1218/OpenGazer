#include "ExtractEyeFeaturesSegmentation.h"
#include <math.h>
#include <fstream>
#define HORIZONTAL_BIN_SIZE 128
#define VERTICAL_BIN_SIZE 64

#define GRAY_LEVEL 127

static std::vector<int> vector_static_horizontal(HORIZONTAL_BIN_SIZE, 0);
static std::vector<int> vector_static_vertical(VERTICAL_BIN_SIZE, 0);

ExtractEyeFeaturesSegmentation::ExtractEyeFeaturesSegmentation(CvSize eyesize)
{

	IplImage* elipse_rgb = (IplImage*) cvLoadImage("./elipse.jpg");
	elipse_gray = cvCreateImage(cvGetSize(elipse_rgb),IPL_DEPTH_8U,1);
	cvCvtColor(elipse_rgb,elipse_gray,CV_RGB2GRAY);

	sizeImageDisk = 42;
	for (int j=0; j<size(irisTemplateDisk); j++){
		irisTemplateDisk[j] = constructTemplateDisk(sizeImageDisk+j);

		Gaussian2D[j] = cvCreateImage(cvSize(eyesize.width - irisTemplateDisk[j]->width + 1, eyesize.height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		/* Gaussian2D[j] = TODO ONUR UNCOMMENTED, NOT NECESSARY */
		CreateTemplateGausian2D(Gaussian2D[j]);
	}
}

IplImage* ExtractEyeFeaturesSegmentation::processToExtractFeatures(	IplImage* Temporary, 
																	IplImage* TemporaryColor, 
																	IplImage* histograma_horizontal, 
																	IplImage* histograma_vertical, 
																	std::vector<int>* vector_horizontal, 
																	std::vector<int>* vector_vertical,
																	std::vector<std::vector<int> >* histPositionSegmentedPixels){

	/*// BORRAR

	float a = 0;
	float b = 255;
	IplImage* MatchesSS;
	CvScalar new_value;

	static int frame_number = 0;

	string fileSS;
    char bufferSS [100];

    cout << "SAVING IMAGES" << endl;
    fileSS=sprintf (bufferSS, "imgs/Eye_Image_%d.jpg", frame_number);

    cvSaveImage(bufferSS, TemporaryColor);

    // HASTA AQUI*/

	IplImage* Temporary_elipsed = cvCreateImage(cvGetSize(Temporary),IPL_DEPTH_8U,1);
	cvRectangle(Temporary_elipsed, cvPoint(0,0), cvPoint(Temporary_elipsed->width,Temporary_elipsed->height), cvScalar(100,100,100), -1, 8, 0);
	//cvCopy(Temporary, Temporary_elipsed, elipse_gray);

	cvCopy(Temporary, Temporary_elipsed, elipse_gray);

    static double *bestval;
    static CvPoint *bestloc;
	
	int j;
	IplImage* Matches;


	int comparison_method = CV_TM_CCOEFF_NORMED; //CV_TM_CCOEFF_NORMED;

	for (j=0; j<size(irisTemplateDisk); j++) {

		Matches = cvCreateImage(cvSize(Temporary_elipsed->width - irisTemplateDisk[j]->width + 1, Temporary_elipsed->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		cvMatchTemplate(Temporary_elipsed, irisTemplateDisk[j], Matches, comparison_method);

		cvMul(Matches, Gaussian2D[j], Matches);

		cvMinMaxLoc(Matches, &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

	}

	double maxProbability = (double) MaxVal[0];

	//cout << "Valor Probabilidad Disco: " << maxProbability << endl;

	//double maxProbability = (double) MinVal[0] * (double) (a[0].val[0]) * (double) 1 / ((double) (30+(0*2)));
	double tmp;
	//cout << "j=0" << ": " << MinVal[j] << " x " << a[0].val[0] << " = " << maxProbability << endl;
	int i = 0;
	for (j=1; j<size(irisTemplateDisk); j++){
		// TODO ONUR : Removed multiplication by disk size for now
		tmp = (double) MaxVal[j] * (1+j*j/300.0);

		//cout << "Valor Probabilidad Disco: " << tmp << endl;

		//tmp = MaxVal[j];
		//tmp = (double) MinVal[j] * (double) (a[j].val[0]) * (double) 1 / ((double) (30+(0*2)));

		if (tmp > maxProbability){
			maxProbability = tmp;
			//maxProbability = (double) MinVal[j] * (double) (a[j].val[0]) * (double) 1 / ((double) (30+(0*2)));
			i = j;
		}
	}

	cvSetImageROI(Temporary_elipsed, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	IplImage* finalIris = Segmentation(Temporary_elipsed);

	IplImage* blackAndWitheIris = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,1);

	cvRectangle(blackAndWitheIris, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0, 0, 0), -1, 8, 0);

	cvSetImageROI(blackAndWitheIris, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	cvCopy(finalIris, blackAndWitheIris);

	cvResetImageROI(blackAndWitheIris);

	Histogram(blackAndWitheIris, histograma_horizontal, histograma_vertical, vector_horizontal, vector_vertical, histPositionSegmentedPixels);

	cout << "Size of vector[3]: " << histPositionSegmentedPixels->size() << endl;

	// Iris detected


	IplImage* extraFeaturesImageSegmented = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,1);

	cvRectangle(extraFeaturesImageSegmented, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(GRAY_LEVEL, GRAY_LEVEL, GRAY_LEVEL), -1, 8, 0);

	cvSetImageROI(extraFeaturesImageSegmented, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));
	
	cvSetImageROI(Temporary, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	cvCopy(Temporary, extraFeaturesImageSegmented, finalIris);

	cvResetImageROI(Temporary);
	
	cvResetImageROI(extraFeaturesImageSegmented);


	cvSetImageROI(TemporaryColor, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	IplImage* aux = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,3);

	cvRectangle(aux, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0,255,0), -1, 8, 0);

	cvCopy(aux, TemporaryColor, finalIris);

	cvResetImageROI(Temporary_elipsed);
	cvResetImageROI(TemporaryColor);

	return extraFeaturesImageSegmented;

}

IplImage* ExtractEyeFeaturesSegmentation::processToExtractFeaturesWITHOUTVECTORS(	IplImage* Temporary, IplImage* TemporaryColor){

	IplImage* Temporary_elipsed = cvCreateImage(cvGetSize(Temporary),IPL_DEPTH_8U,1);
	cvRectangle(Temporary_elipsed, cvPoint(0,0), cvPoint(Temporary_elipsed->width,Temporary_elipsed->height), cvScalar(100,100,100), -1, 8, 0);
	cvCopy(Temporary, Temporary_elipsed, elipse_gray);

    static double *bestval;
    static CvPoint *bestloc;
	
	int j;
	IplImage* Matches;

	int comparison_method = CV_TM_CCOEFF_NORMED; //CV_TM_CCOEFF_NORMED;

	for (j=0; j<size(irisTemplateDisk); j++){

		Matches = cvCreateImage(cvSize(Temporary_elipsed->width - irisTemplateDisk[j]->width + 1, Temporary_elipsed->height - irisTemplateDisk[j]->height + 1), IPL_DEPTH_32F, 1);

		cvMatchTemplate(Temporary_elipsed, irisTemplateDisk[j], Matches, comparison_method);

		cvMul(Matches, Gaussian2D[j], Matches);
		
		cvMinMaxLoc(Matches, &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

	}

	double maxProbability = (double) MaxVal[0];
	double tmp;
	int i = 0;
	for (j=1; j<size(irisTemplateDisk); j++){
		tmp = (double) MaxVal[j] * (1+j*j/300.0);

		if (tmp > maxProbability){
			maxProbability = tmp;
			i = j;
		}
	}

	cvSetImageROI(Temporary_elipsed, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	IplImage* finalIris = Segmentation(Temporary_elipsed);

	IplImage* blackAndWitheIris = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,1);

	cvRectangle(blackAndWitheIris, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0, 0, 0), -1, 8, 0);

	cvSetImageROI(blackAndWitheIris, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	cvCopy(finalIris, blackAndWitheIris);

	cvResetImageROI(blackAndWitheIris);

	// Iris detected


	IplImage* extraFeaturesImageSegmented = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,1);

	cvRectangle(extraFeaturesImageSegmented, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(GRAY_LEVEL, GRAY_LEVEL, GRAY_LEVEL), -1, 8, 0);

	cvSetImageROI(extraFeaturesImageSegmented, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));
	
	cvSetImageROI(Temporary, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	cvCopy(Temporary, extraFeaturesImageSegmented, finalIris);

	cvResetImageROI(Temporary);
	
	cvResetImageROI(extraFeaturesImageSegmented);



	//cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
	//cvMoveWindow("mainWin", extraFeaturesImageSegmented->width, extraFeaturesImageSegmented->height);
	//cvShowImage("mainWin", extraFeaturesImageSegmented );

	cvSaveImage("extraFeaturesImageSegmented.png", extraFeaturesImageSegmented);

	cvSetImageROI(TemporaryColor, cvRect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i]->width, irisTemplateDisk[i]->height));

	IplImage* aux = cvCreateImage(cvGetSize(TemporaryColor),IPL_DEPTH_8U,3);

	cvRectangle(aux, cvPoint(0,0), cvPoint(TemporaryColor->width,TemporaryColor->height), cvScalar(0,255,0), -1, 8, 0);

	cvCopy(aux, TemporaryColor, finalIris);

	cvResetImageROI(Temporary_elipsed);
	cvResetImageROI(TemporaryColor);

	return extraFeaturesImageSegmented;

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

	//for (int i = 0; i < 5; i++){
	//	cvSmooth(Temporary, Temporary, CV_GAUSSIAN, 3);
	//}

	cvThreshold(Temporary, im_bw, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	//cvThreshold(Temporary, im_bw, CalculateOptimalThreshold(Temporary), 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	//cvThreshold(Temporary, im_bw, CalculateOptimalThreshold(Temporary), 255, CV_THRESH_BINARY);

	cvNot(im_bw, im_bw);

	return im_bw;
}

int ExtractEyeFeaturesSegmentation::CalculateOptimalThreshold(IplImage* Temporary) {

	std::vector<int> histograma_segmentacion(255, 0);

	int pos = -1;

	for (int y = 0; y<cvGetSize(Temporary).height; y++) {
		for (int x = 0; x<cvGetSize(Temporary).width; x++) {

		histograma_segmentacion.operator[](cvGet2D(Temporary, y, x).val[0])++;

		}
	}

	int totalPixels = cvGetSize(Temporary).height * cvGetSize(Temporary).width;
	int optimalValue = Otsu(histograma_segmentacion, totalPixels);

	cout << "optimalValue: " << optimalValue << endl;

	return optimalValue;
}

int ExtractEyeFeaturesSegmentation::Otsu(std::vector<int> histogram, int total) {
    float sum = 0;
    for (int i = 1; i < 256; ++i)
        sum += i * histogram.operator[](i);
    float sumB = 0;
    int wB = 0;
    int wF = 0;
    double mB;
    double mF;
    float max = 0.0;
    double between = 0.0;
    int threshold1 = 0.0;
    int threshold2 = 0.0;
    for (int i = 0; i < 256; ++i) {
        wB += histogram.operator[](i);
        if (wB == 0)
            continue;
        wF = total - wB;
        if (wF == 0)
            break;
        sumB += i * histogram.operator[](i);
        mB = sumB / wB;
        mF = (sum - sumB) / wF;
        between = wB * wF * (mB - mF) * (mB - mF);
        if ( between >= max ) {
            threshold1 = i;
            if ( between > max ) {
                threshold2 = i;
            }
            max = between;            
        }
    }
    return ( threshold1 + threshold2 ) / 2.0;
}

IplImage* ExtractEyeFeaturesSegmentation::CreateTemplateGausian2D(IplImage* Gaussian2D) {

	CvPoint center;
	center.x = floor(Gaussian2D->width/2);
	center.y = floor(Gaussian2D->height/2);

	float tmp = 0;
	float sigma = 200;	// With this sigma, the furthest eye pixel (corners) have around 0.94 probability
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
			//cout << tmp << " ";
		}
		//cout << endl;
	}
	//cout << endl << endl;
	
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

void ExtractEyeFeaturesSegmentation::Histogram(	IplImage* blackAndWitheIris, 
														IplImage* hist_horizontal, 
														IplImage* hist_vertical, 
														std::vector<int> * blackAndWitheIris_summed_horizontal,
														std::vector<int> * blackAndWitheIris_summed_vertical,
														std::vector<std::vector<int> >* histPositionSegmentedPixels) {


	int width = blackAndWitheIris->widthStep;
	uchar* data = (uchar*) blackAndWitheIris->imageData;
	
	//std::vector<int> blackAndWitheIris_summed_horizontal (blackAndWitheIris->width,0); 
	//std::vector<int> blackAndWitheIris_summed_vertical (blackAndWitheIris->height,0); 

	//int blackAndWitheIris_summed_horizontal[blackAndWitheIris->width];
	//int blackAndWitheIris_summed_vertical[blackAndWitheIris->height];

	blackAndWitheIris_summed_horizontal->assign(HORIZONTAL_BIN_SIZE, 0);
	blackAndWitheIris_summed_vertical->assign(VERTICAL_BIN_SIZE, 0);
	//memcpy(blackAndWitheIris_summed_horizontal->operator[], array_horizontal, HORIZONTAL_BIN_SIZE*sizeof(int));
	//memcpy(blackAndWitheIris_summed_vertical->operator[], array_vertical, VERTICAL_BIN_SIZE*sizeof(int));

	for (int i=0;i<blackAndWitheIris->width;i++) {
	    for (int k=0;k<blackAndWitheIris->height;k++) {
			if ((int) data[i + k*width] == 255) {
				blackAndWitheIris_summed_horizontal->operator[](i)++;
				blackAndWitheIris_summed_vertical->operator[](k)++;
			}
	    }
	}

	//IplImage* hist_horizontal = cvCreateImage(cvSize(blackAndWitheIris->width, blackAndWitheIris->height),IPL_DEPTH_8U,1);

	cvRectangle(hist_horizontal, cvPoint(0,0), cvPoint(hist_horizontal->width, hist_horizontal->height), cvScalar(0,0,0), -1, 8, 0);

	for (int i=0;i<blackAndWitheIris->width;i++) {
	    cvLine(hist_horizontal, 
		   cvPoint(i, hist_horizontal->height),
		   cvPoint(i, hist_horizontal->height - blackAndWitheIris_summed_horizontal->operator[](i)),
		   CV_RGB(255,255,255));
	}

	//IplImage* hist_vertical = cvCreateImage(cvSize(blackAndWitheIris->height, blackAndWitheIris->width),IPL_DEPTH_8U,1);

	cvRectangle(hist_vertical, cvPoint(0,0), cvPoint(hist_vertical->width, hist_vertical->height), cvScalar(0,0,0), -1, 8, 0);

	for (int i=0;i<blackAndWitheIris->width;i++) {
	    cvLine(hist_vertical, 
		   cvPoint(i, hist_vertical->height),
		   cvPoint(i, hist_vertical->height - blackAndWitheIris_summed_vertical->operator[](i)),
		   CV_RGB(255,255,255));
	}

	SortHistogram(blackAndWitheIris_summed_horizontal, blackAndWitheIris_summed_vertical, histPositionSegmentedPixels);

	cout << "Size of vector[2]: " << histPositionSegmentedPixels->size() << endl;

	/*

	cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
	cvMoveWindow("mainWin", hist_vertical->width, hist_vertical->height);

	cvShowImage("mainWin", hist_vertical );

	cin.get();

	*/
}

void ExtractEyeFeaturesSegmentation::SortHistogram(	std::vector<int>* histHorizontalPixelsSegmented, 
													std::vector<int>* histVerticalPixelsSegmented, 
													std::vector<std::vector<int> >* histPositionSegmentedPixels) {


	/*

	static int asdf = 0;

	cout << "HISTOGRAMA_ANTIGUO_HORIZONTAL[" << asdf << "]" << endl;

    for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

    	cout << histHorizontalPixelsSegmented->operator[](j) << " ";
    }

	cout << "HISTOGRAMA_ANTIGUO_VERTICAL[" << asdf << "]" << endl;

    for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {


    	cout << histVerticalPixelsSegmented->operator[](j) << " ";
    }

    */

	int left_pos = 0, right_pos = 0, pos_mean = 0, acum = 0;

	cout << "RIGHT ACUM SORT" << endl;

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	    if ((left_pos != histHorizontalPixelsSegmented->operator[](j)) && (left_pos == 0)) {
	        left_pos = j;
	    }

	    if ((right_pos != histHorizontalPixelsSegmented->operator[](histHorizontalPixelsSegmented->size()-j)) && (right_pos == 0)) {
	        right_pos = histHorizontalPixelsSegmented->size()-j;
	    }

	    acum += histHorizontalPixelsSegmented->operator[](j);
	}

	cout << "RIGHT MEAN SORT" << endl;

	acum = acum / 2;

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	    if (acum > 0) {
	    	acum -= histHorizontalPixelsSegmented->operator[](j);
	    	if (acum <= 0) {
	    		pos_mean = j;
	    	}
	    }
	}

	cout << "RIGHT GAUSSIAN SORT" << endl;

	double sigma = 20;
	double mean = pos_mean;
	double diff = 0.0;

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {
		diff = mean-j;

		histHorizontalPixelsSegmented->operator[](j) = floor(histHorizontalPixelsSegmented->operator[](j) * exp( - diff*diff/ (2*sigma*sigma) ) );

	}











	left_pos = 0, right_pos = 0, pos_mean = 0, acum = 0;

	cout << "LEFT ACUM SORT" << endl;

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	    if ((left_pos != histVerticalPixelsSegmented->operator[](j)) && (left_pos == 0)) {
	        left_pos = j;
	    }

	    if ((right_pos != histVerticalPixelsSegmented->operator[](histVerticalPixelsSegmented->size()-j)) && (right_pos == 0)) {
	        right_pos = histVerticalPixelsSegmented->size()-j;
	    }

	    acum += histVerticalPixelsSegmented->operator[](j);
	}

	cout << "LEFT MEAN SORT" << endl;

	acum = acum / 2;

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	    if (acum > 0) {
	    	acum -= histVerticalPixelsSegmented->operator[](j);
	    	if (acum <= 0) {
	    		pos_mean = j;
	    	}
	    }
	}

	cout << "LEFT GAUSSIAN SORT" << endl;


	sigma = 20;
	mean = pos_mean;

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {
		diff = mean-j;

		histVerticalPixelsSegmented->operator[](j) = floor(histVerticalPixelsSegmented->operator[](j) * exp( - diff*diff/ (2*sigma*sigma) ) );
		
	}





	histPositionSegmentedPixels->clear();

	cout << "HIST" << endl;

	int max = -1;

	// Obtencion del maximo

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	    if (max < histHorizontalPixelsSegmented->operator[](j)) {
	        max = histHorizontalPixelsSegmented->operator[](j);
	    }
	}

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	    if (max < histVerticalPixelsSegmented->operator[](j)) {
	        max = histVerticalPixelsSegmented->operator[](j);
	    }
	}

	int aux_max = -1;

	// Colocacion de los elementos que contengan ese maximo en un vector

	std::vector<int> aux_histPosition;

	while (max > -1) {

	    for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	        if (histHorizontalPixelsSegmented->operator[](j) == max) {

	            aux_histPosition.push_back(j);

	        } else if ((histHorizontalPixelsSegmented->operator[](j) < max) && 
	            (aux_max < histHorizontalPixelsSegmented->operator[](j))) {

	            aux_max = histHorizontalPixelsSegmented->operator[](j);
	        }
	    }

	    for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	        if (histVerticalPixelsSegmented->operator[](j) == max) {

	            aux_histPosition.push_back(j + histHorizontalPixelsSegmented->size());

	        } else if ((histVerticalPixelsSegmented->operator[](j) < max) && 
	            (aux_max < histVerticalPixelsSegmented->operator[](j))) {

	            aux_max = histVerticalPixelsSegmented->operator[](j);
	        }
	    }

	    histPositionSegmentedPixels->push_back(aux_histPosition);

		aux_histPosition.clear();

	    max = aux_max;
	    aux_max = -1;

	}


	/*

	cout << "HISTOGRAMA_NUEVO_HORIZONTAL[" << asdf << "]" << endl;

    for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

    	cout << histHorizontalPixelsSegmented->operator[](j) << " ";
    }

	cout << "HISTOGRAMA_NUEVO_VERTICAL[" << asdf << "]" << endl;

    for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {


    	cout << histVerticalPixelsSegmented->operator[](j) << " ";
    }

   	asdf++;

    //cin.get();

    */

	cout << "Size of vector[1]: " << histPositionSegmentedPixels->size() << endl;

}
