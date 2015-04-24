#include <gtkmm.h>
#include <iostream>
#include "utils.h"
#include "EyeTemplate.h"

#define EYE_WIDTH	128
#define EYE_HEIGHT	64
#define MAX_FILES	652
#define MAX_DIST 	25

void euclideanDistance(Point point1, Point point2, double& x_singleImage_dist, double& y_singleImage_dist, double& xy_singleImage_dist) {

	x_singleImage_dist = double (point1.x-point2.x);
	y_singleImage_dist = double (point1.y-point2.y);
	xy_singleImage_dist = double (sqrt(pow(x_singleImage_dist,2)+pow(y_singleImage_dist,2)));

}



int main(int argc, char **argv)
{
	try {

		EyeTemplate eyeTemplate;

		vector<double> XY_DISTANCE, X_DISTANCE, Y_DISTANCE;

		vector<Point> EyeCenterPoints;


		cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
		cvMoveWindow("mainWin", EYE_WIDTH, EYE_HEIGHT);

		int good = 0, bad = 0;

		double xy_singleImage_dist = 0, x_singleImage_dist = 0, y_singleImage_dist = 0, xy_total_dist = 0, x_total_dist = 0, y_total_dist = 0;
		int counter = 0;
		boost::filesystem::path dataset_path("/Users/arcadillanzacarmona/Desktop/dataset/images");

		boost::filesystem::directory_iterator end_itr;

		// Object
		IplImage *input_orig = cvCreateImage(cvSize(400, 200), 8, 3);

		IplImage *input = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 3);
		IplImage *input_gray = cvCreateImage(cvSize(EYE_WIDTH, EYE_HEIGHT), 8, 1);


		int i = 0;
	   	char line[100];

	   	float x, y;

	   	FILE *infile;

		infile = fopen ("/Users/arcadillanzacarmona/Desktop/dataset/centerPosition652.txt", "rt");
			while(i < 652 && fgets(line, sizeof(line), infile) != NULL) {
			 	sscanf(line, "%f %f", &x, &y);

			 	x = x * 128/400;
			 	y = (y - 50) * 64/200;

				EyeCenterPoints.push_back(Point(x,y));
			 	i++;
		   	}
		fclose(infile);


		for( boost::filesystem::directory_iterator dataset_image(dataset_path); dataset_image != end_itr; ++dataset_image ) {
			if (counter <652) {
				// Skip if not a file or if not the right type of file
				if( !boost::filesystem::is_regular_file( dataset_image->status() ) ) continue;
				if( dataset_image->path().extension() != ".tiff" ) continue;

				//cout << dataset_image->path().filename().string();

				// Find corresponding dataset image
				string dataset_image_path = dataset_path.string() + "/" + dataset_image->path().filename().string();

				if( !boost::filesystem::exists( dataset_image_path ) )
					cout << "\tCORRESPONDING FILE (" + dataset_image_path + ") DOES NOT EXIST!!!" << endl;
				
				cout << counter + 1 << ")\t" << dataset_image->path().filename().string() << endl;

				// Load images
				IplImage *dataset_image_file = (IplImage*) cvLoadImage(dataset_image_path.c_str());

				cvSetImageROI(dataset_image_file, cvRect(0, 50, 400, 200));
				cvSetImageROI(dataset_image_file, cvRect(0, 50, 400, 200));

				cvCopy(dataset_image_file, input_orig);

				cvResize(input_orig, input);
				cvCvtColor(input, input_gray, CV_RGB2GRAY);
				cvSmooth(input_gray, input_gray, CV_MEDIAN, 9);

				//cvShowImage("mainWin", input_gray);
				//cvWaitKey(0);

				Point calculatedPoint = *(eyeTemplate.ProcessToExtractFeatures(input_gray, input));

				euclideanDistance(EyeCenterPoints.operator[](counter), calculatedPoint, x_singleImage_dist, y_singleImage_dist, xy_singleImage_dist);





				if (xy_singleImage_dist < MAX_DIST) {

	            	good++;

					cvRectangle(
					  input,
					  cvPoint(calculatedPoint.x-1, calculatedPoint.y-1),
					  cvPoint(calculatedPoint.x+1, calculatedPoint.y+1),
					  CV_RGB(0, 255, 0),
					  2, 8, 0
					);
				} else {

	            	bad++;

					cvRectangle(
					  input,
					  cvPoint(calculatedPoint.x-1, calculatedPoint.y-1),
					  cvPoint(calculatedPoint.x+1, calculatedPoint.y+1),
					  CV_RGB(255, 0, 0),
					  2, 8, 0
					);
				}

				cvRectangle(
				  input,
				  cvPoint(EyeCenterPoints.operator[](counter).x-1, EyeCenterPoints.operator[](counter).y-1),
				  cvPoint(EyeCenterPoints.operator[](counter).x+1, EyeCenterPoints.operator[](counter).y+1),
				  CV_RGB(0, 0, 255),
				  2, 8, 0
				);


				static int frame_number_detect_eye = 0;

				string file_detect_eye;
			    char buffer_detect_eye [100];


			    //cout << "SAVING IMAGES Template Left" << endl;
			    file_detect_eye=sprintf (buffer_detect_eye, "imgs/Eye/%d.jpg", counter);

			    cvSaveImage(buffer_detect_eye, input);






	            XY_DISTANCE.push_back(xy_singleImage_dist);
	            X_DISTANCE.push_back(x_singleImage_dist);
	            Y_DISTANCE.push_back(y_singleImage_dist);

				xy_total_dist += xy_singleImage_dist;
	            x_total_dist += x_singleImage_dist; 
				y_total_dist += y_singleImage_dist;
	            counter++;


				// Release the used images
				cvReleaseImage(&dataset_image_file);
				cvReleaseImage(&dataset_image_file);

			}
		}


		if(counter > 0) {
	        x_total_dist /= counter; 
			y_total_dist /= counter;
			xy_total_dist /= counter;
		
			cout << "xy_total_dist = " << xy_total_dist << " x_total_dist = " << x_total_dist << " y_total_dist = " << y_total_dist << endl;
			cout << "Good = " << good << " Bad = " << bad << endl;

			string file = getUniqueFileName("../dataset/Output_Test", "testerEyeCenterPositionIris");


			FILE *pf; 
			pf = fopen(file.c_str(), "w");

			fprintf(pf,"IMAGE_NUMBER) XY_DISTANCE, X_DISTANCE, Y_DISTANCE\n\n");

			for(int j = 0; j < XY_DISTANCE.size(); j++) {
				fprintf(pf,"%d) %f %f %f\n", j, XY_DISTANCE[j], X_DISTANCE[j], Y_DISTANCE[j]);
			}

			fprintf(pf,"\nAverage\n\n");
			fprintf(pf,"xy_total_dist: %f \t x_total_dist: %f \t y_total_dist: %f \n", xy_total_dist, x_total_dist, y_total_dist);

			fprintf(pf,"\nStatiscs\n\n");
			fprintf(pf,"Good: %u \t Bad: %u \n", good, bad);

			fclose(pf);
			

		}

		cvReleaseImage(&input_orig);
		cvReleaseImage(&input);
		cvReleaseImage(&input_gray);

	}
  	catch (QuitNow)
    {
    	cout << "Caught it!\n";
    }
    
    return 0;
}
