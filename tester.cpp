#include <gtkmm.h>
#include <iostream>
#include "utils.h"
#include "ExtractEyeFeaturesSegmentation.h"

#define EYE_WIDTH	128
#define EYE_HEIGHT	64
#define MAX_FILES	2250

void cmp(const IplImage *mask, const IplImage *segmented_imag, int& tp, int& fp, int& fn, int& tn){

	for(int i=0; i<EYE_WIDTH; i++) {
		for(int j=0; j<EYE_HEIGHT; j++) {
			bool segmented = false;
			bool ground_truth = false;

			// Completely green pixels are considered filled (segmented)
			if( ((uchar *)(segmented_imag->imageData + i*segmented_imag->widthStep))[j*segmented_imag->nChannels + 0] == 0 &&
				((uchar *)(segmented_imag->imageData + i*segmented_imag->widthStep))[j*segmented_imag->nChannels + 1] == 255 &&
				((uchar *)(segmented_imag->imageData + i*segmented_imag->widthStep))[j*segmented_imag->nChannels + 2] == 0)
				segmented = true;

			// Completely green pixels are considered filled (segmented)
			if( ((uchar *)(mask->imageData + i*mask->widthStep))[j*mask->nChannels + 0] == 255 &&
				((uchar *)(mask->imageData + i*mask->widthStep))[j*mask->nChannels + 1] == 255 &&
				((uchar *)(mask->imageData + i*mask->widthStep))[j*mask->nChannels + 2] == 255)
				ground_truth = true;

	        if( ground_truth && segmented ){
	            tp++;
	        }else if( ground_truth ){ //implied pa == 0, else first if
	            fn++;
	        }else if( segmented ){ //implied pb == 0, else first if
	            fp++;
	        } else {  // both == 0
	            tn++;
	        }
		}
	}
}

int main(int argc, char **argv)
{
	try {
		cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
		cvMoveWindow("mainWin", EYE_WIDTH, EYE_HEIGHT);

		double aprecision = 0, arecall = 0, afmeasure = 0;
		int counter = 0;
		boost::filesystem::path dataset_path("/Users/arcadillanzacarmona/Desktop/dataset/images");
		boost::filesystem::path masks_path("/Users/arcadillanzacarmona/Desktop/dataset/mask");

		boost::filesystem::directory_iterator end_itr;

		// Segmentation object
		ExtractEyeFeaturesSegmentation segmentation(cvSize(EYE_WIDTH, EYE_HEIGHT));
		IplImage *histograma_hor = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 1);
		IplImage *histograma_ver = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 1);
		std::vector<int> *vector_hor = new vector<int>(EYE_WIDTH,0);
		std::vector<int> *vector_ver = new vector<int>(EYE_HEIGHT,0);


		IplImage *mask_orig = cvCreateImage(cvSize(400, 200), 8, 3);
		IplImage *input_orig = cvCreateImage(cvSize(400, 200), 8, 3);

		IplImage *mask = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 3);
		IplImage *input = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 3);
		IplImage *input_gray = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 1);


		for( boost::filesystem::directory_iterator mask_image(masks_path); mask_image != end_itr; ++mask_image ) {
			// Skip if not a file or if not the right type of file
			if( !boost::filesystem::is_regular_file( mask_image->status() ) ) continue;
			if( mask_image->path().extension() != ".tiff" ) continue;


			//cout << mask_image->path().filename().string();

			// Find corresponding dataset image
			string dataset_image_path = dataset_path.string() + "/" + mask_image->path().filename().string().substr(10);

			if( !boost::filesystem::exists( dataset_image_path ) )
				cout << "\tCORRESPONDING FILE (" + dataset_image_path + ") DOES NOT EXIST!!!" << endl;
			
			cout << counter + 1 << ")\t" << mask_image->path().filename().string().substr(10) << endl;

			// Load images
			IplImage *mask_image_file = (IplImage*) cvLoadImage(mask_image->path().string().c_str());
			IplImage *dataset_image_file = (IplImage*) cvLoadImage(dataset_image_path.c_str());

			cvSetImageROI(mask_image_file, cvRect(0, 50, 400, 200));
			cvSetImageROI(dataset_image_file, cvRect(0, 50, 400, 200));

			cvCopy(mask_image_file, mask_orig);
			cvCopy(dataset_image_file, input_orig);

			cvResize(mask_orig, mask);
			cvResize(input_orig, input);
			cvCvtColor(input, input_gray, CV_RGB2GRAY);
			cvSmooth(input_gray, input_gray, CV_MEDIAN, 9);

			cvShowImage("mainWin", input_gray);
			cvWaitKey(0);

			segmentation.processToExtractFeatures(input_gray, input, histograma_hor, histograma_ver, vector_hor, vector_ver);

			cvShowImage("mainWin", input);
			cvWaitKey(0);

			int tp = 0, fp=0, fn=0,tn=0;
			cmp(mask, input, tp, fp, fn, tn);
			double precision = (tp+fp == 0) ? 0 : ((double) tp) / (double)(tp+fp);
			double recall = (tp+fn == 0) ? 0 : ((double)tp) / (double)(tp+fn);
            double f1 = ( precision + recall ==0) ? 0 : (2*recall*precision/(recall+precision));

			cout << "Image recall = " << 100*recall << "%, precision = " << 100*precision << "%, f_1 measure=" << 100*f1 << "%" << endl << endl;

            aprecision += precision; 
			arecall += recall;
            afmeasure += f1;
            counter++;

            //if(counter > 5) {
            //	break;
            //}

			// Release the used images
			cvReleaseImage(&mask_image_file);
			cvReleaseImage(&dataset_image_file);
		}

		if(counter > 0) {
	        aprecision /= counter; 
			arecall /= counter; 
	        afmeasure /= counter; 
		
			cout << "Average recall = " << 100*arecall << " % precision = " << 100*aprecision << " % f_1 measure=" << 100*afmeasure << " %" << endl;
		}

		cvReleaseImage(&mask_orig);
		cvReleaseImage(&input_orig);
		cvReleaseImage(&mask);
		cvReleaseImage(&input);
		cvReleaseImage(&input_gray);

	}
  	catch (QuitNow)
    {
    	cout << "Caught it!\n";
    }
    
    return 0;
}
