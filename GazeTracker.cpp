#include "GazeTracker.h"
#include "EyeExtractor.h"
#include "mir.h"
#include <time.h>

fann_type *all_inputs[1000], *all_outputs[1000];
fann_type *all_inputs_left[1000], *all_outputs_left[1000];
IplImage *all_images[1000];
IplImage *all_images_left[1000];
double all_output_coords[1000][2];

static void ignore(const IplImage *) {}

int Targets::getCurrentTarget(Point point) {
    vector<double> distances(targets.size());
    //    debugtee(targets);
    transform(targets.begin(), targets.end(), distances.begin(),
	      sigc::mem_fun(point, &Point::distance));
    //    debugtee(distances);
    return min_element(distances.begin(), distances.end()) - distances.begin();
//     for(int i=0; i<targets.size(); i++)
// 	if (point.distance(targets[i]) < 30)
// 	    return i;
//     return -1;
}


CalTarget::CalTarget() {}

CalTarget::CalTarget(Point point, 
		     const IplImage* image, const IplImage* origimage, vector<int> hist_horizontal,
			      vector<int> hist_vertical):
    point(point), 
    image(cvCloneImage(image), releaseImage), 
    origimage(cvCloneImage(origimage), releaseImage)
{

	std::vector<int>::iterator it;
	it=hist_horizontal.begin();

	vector_horizontal.assign(it, hist_horizontal.end());

	it=hist_vertical.begin();

    vector_vertical.assign(it, hist_vertical.end());
}

void CalTarget::save(CvFileStorage* out, const char* name) {
    cvStartWriteStruct(out, name, CV_NODE_MAP);
    point.save(out, "point");
    cvWrite(out, "image", image.get());
    cvWrite(out, "origimage", origimage.get());
    cvEndWriteStruct(out);
}

void CalTarget::load(CvFileStorage* in, CvFileNode *node) {
    point.load(in, cvGetFileNodeByName(in, node, "point"));
    image.reset((IplImage*) cvReadByName(in, node, "image"));
    origimage.reset((IplImage*) cvReadByName(in, node, "origimage"));
}

TrackerOutput::TrackerOutput(Point gazepoint, Point target, int targetid):
    gazepoint(gazepoint), target(target), targetid(targetid)
{
}

void TrackerOutput::setActualTarget(Point actual) {
	actualTarget = actual;
}

//void TrackerOutput::setErrorOutput(bool show) {
//	outputError = show;
//}

void TrackerOutput::setFrameId(int id) {
	frameid = id;
}
 
template <class T, class S>
vector<S> getsubvector(vector<T> const& input, S T::*ptr) {
    vector<S> output(input.size());
    for(int i=0; i<input.size(); i++)
	output[i] = input[i].*ptr;
    return output;
}

// TODO ARCADI CONTINUE
// Create copy of these two functions, and modify so that they use histograms
double GazeTracker::imagedistance(const IplImage *im1, const IplImage *im2) {
    double norm = cvNorm(im1, im2, CV_L2);
    return norm*norm;
}

double GazeTracker::covariancefunction(SharedImage const& im1, 
				       SharedImage const& im2)
{
    const double sigma = 1.0;
    const double lscale = 500.0;
    return sigma*sigma*exp(-imagedistance(im1.get(),im2.get())/(2*lscale*lscale));
}

// TODO ARCADI CONTINUE
// Create copy of these two functions, and modify so that they use histograms
double GazeTracker::histDistance(vector<int> histogram1, vector<int> histogram2) {
 
    const double sigma = 1.0; // 1.0;
    const double lscale = 100.0; // 500.00;

	double norm = 0.0;

    for (int i = 0; i < histogram1.size(); i++) {
    	norm +=  pow(histogram1[i] - histogram2[i], 2); // suma de las diferencias cuadradas
    }  

    norm = sigma*sigma*exp(-norm / (2*lscale*lscale) );

    //cout << "norm: " << norm << endl;
    //cin.get();

    return norm;
}

double GazeTracker::covariancefunction_hist(vector<int> const& histogram1, vector<int> const& histogram2)
{
    return histDistance(histogram1, histogram2);
}

void GazeTracker::updateGPs(void) {
    vector<double> xlabels;
    vector<double> ylabels;
		
    for(int i=0; i<caltargets.size(); i++) {
	    xlabels.push_back(caltargets[i].point.x);
	    ylabels.push_back(caltargets[i].point.y);
    }

    vector<SharedImage> images = 
	getsubvector(caltargets, &CalTarget::image);


    vector<vector<int> > vector_horizontals = 
	getsubvector(caltargets, &CalTarget::vector_horizontal);


    vector<vector<int> > vector_verticals = 
	getsubvector(caltargets, &CalTarget::vector_vertical);
	
	/*
cout << "INSIDE updateGPs" << endl;
cout << "labels size: " << xlabels.size();
cout << "images size: " << images.size();
*/
    gpx.reset(new ImProcess(images, xlabels, covariancefunction, 0.01));
    gpy.reset(new ImProcess(images, ylabels, covariancefunction, 0.01));  


// ---------------------------------------------- ARCADI PROCESS ----------------------------------------------

    histx.reset(new HistProcess(vector_horizontals, xlabels, covariancefunction_hist, 0.01));
    histy.reset(new HistProcess(vector_verticals, ylabels, covariancefunction_hist, 0.01));  

/*
    vector<vector<int> > vectors_horizontals_and_verticals =
	getsubvector(caltargets, &CalTarget::vector_horizontal);


	copy( vector_verticals[vector_horizontals.size()-1].begin(), vector_verticals[vector_horizontals.size()-1].end(), back_inserter(vectors_horizontals_and_verticals[vector_horizontals.size()-1]));


    histx.reset(new HistProcess(vectors_horizontals_and_verticals, xlabels, covariancefunction_hist, 0.01));
    histy.reset(new HistProcess(vectors_horizontals_and_verticals, ylabels, covariancefunction_hist, 0.01));  
*/
    // TODO ARCADI MIR.CPP
    
// ------------------------------------------------------------------------------------------------------------

   // TODO ARCADI CONTINUE Create new type of GaussianProcesses (HistProcess) (with different covariance func)
    targets.reset(new Targets(getsubvector(caltargets, &CalTarget::point)));

}

void GazeTracker::updateGPs_left(void) {
    vector<double> xlabels;
	vector<double> ylabels;

    for(int i=0; i<caltargets_left.size(); i++) {
	    xlabels.push_back(caltargets_left[i].point.x);
	    ylabels.push_back(caltargets_left[i].point.y);
    }

    vector<SharedImage> images = 
	getsubvector(caltargets_left, &CalTarget::image);


    vector<vector<int> > vector_horizontals = 
	getsubvector(caltargets_left, &CalTarget::vector_horizontal);


    vector<vector<int> > vector_verticals = 
	getsubvector(caltargets_left, &CalTarget::vector_vertical);


    gpx_left.reset(new ImProcess(images, xlabels, covariancefunction, 0.01));
    gpy_left.reset(new ImProcess(images, ylabels, covariancefunction, 0.01));  

// ---------------------------------------------- ARCADI PROCESS ----------------------------------------------

    histx_left.reset(new HistProcess(vector_horizontals, xlabels, covariancefunction_hist, 0.01));
    histy_left.reset(new HistProcess(vector_verticals, ylabels, covariancefunction_hist, 0.01));  

/*
    vector<vector<int> > vectors_horizontals_and_verticals =
	getsubvector(caltargets_left, &CalTarget::vector_horizontal);

	copy( vector_verticals[vector_horizontals.size()-1].begin(), vector_verticals[vector_horizontals.size()-1].end(), back_inserter(vectors_horizontals_and_verticals[vector_horizontals.size()-1]));

    histx_left.reset(new HistProcess(vectors_horizontals_and_verticals, xlabels, covariancefunction_hist, 0.01));
    histy_left.reset(new HistProcess(vectors_horizontals_and_verticals, ylabels, covariancefunction_hist, 0.01));  
*/

// ------------------------------------------------------------------------------------------------------------

    // TODO ARCADI CONTINUE Create new type of GaussianProcesses (HistProcess) (with different covariance func)
    //targets_left.reset(new Targets(getsubvector(caltargets_left, &CalTarget::point)));
}

void GazeTracker::calculateTrainingErrors() {
	int num_of_monitors = Gdk::Screen::get_default()->get_n_monitors();
	Gdk::Rectangle monitorgeometry;
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();

	// Geometry of main monitor
	screen->get_monitor_geometry(num_of_monitors - 1, monitorgeometry);
	
	vector<Point> points = getsubvector(caltargets, &CalTarget::point);
	
	int j = 0;
	
	//cout << "Input count: " << input_count;
	//cout << ", Target size: " << caltargets.size() << endl;
	
	for(int i=0; i<caltargets.size(); i++) {
		double x_total = 0;
		double y_total = 0;
		double sample_count = 0;
		
		//cout << points[i].x << ", " << points[i].y << " x " << all_output_coords[j][0] << ", " << all_output_coords[j][1] << endl;
		
		while(j < input_count && points[i].x == all_output_coords[j][0] && points[i].y == all_output_coords[j][1]) {
			double x_estimate = (gpx->getmean(SharedImage(all_images[j], &ignore)) + gpx_left->getmean(SharedImage(all_images_left[j], &ignore))) / 2;
			double y_estimate = (gpy->getmean(SharedImage(all_images[j], &ignore)) + gpy_left->getmean(SharedImage(all_images_left[j], &ignore))) / 2;
			
			//cout << "i, j = (" << i << ", " << j << "), est: " << x_estimate << "("<< gpx->getmean(SharedImage(all_images[j], &ignore)) << ","<< gpx_left->getmean(SharedImage(all_images_left[j], &ignore)) << ")" << ", " << y_estimate << "("<< gpy->getmean(SharedImage(all_images[j], &ignore)) <<","<< gpy_left->getmean(SharedImage(all_images_left[j], &ignore)) << ")"<< endl;
			
			x_total += x_estimate;
			y_total += y_estimate;
			sample_count++;
			j++;
		}
		
		x_total /= sample_count;
		y_total /= sample_count;
	
		*output_file << "TARGET: (" << caltargets[i].point.x << "\t, " << caltargets[i].point.y << "\t),\tESTIMATE: ("<< x_total << "\t, " << y_total <<")" << endl;
		//cout << "TARGET: (" << caltargets[i].point.x << "\t, " << caltargets[i].point.y << "\t),\tESTIMATE: ("<< x_total << "\t, " << y_total <<"),\tDIFF: ("<< fabs(caltargets[i].point.x- x_total) << "\t, " << fabs(caltargets[i].point.y - y_total) <<")" << endl;
		
		// Calibration error removal
		xv[i][0] = x_total;		// Source
		xv[i][1] = y_total;
		
		// Targets
		fv_x[i] = caltargets[i].point.x;
		fv_y[i] = caltargets[i].point.y;
		sigv[i] = 0;
		
		int targetId = getTargetId(Point(x_total, y_total));
		
		if(targetId != i) {
			cout << "Target id is not the expected one!! (Expected: "<< i<< ", Current: "<< targetId << ")" << endl;
		}
		
	}
	
	// Add the corners of the monitor as 4 extra data points. This helps the correction for points that are near the edge of monitor
	xv[caltargets.size()][0] = monitorgeometry.get_x();
	xv[caltargets.size()][1] = monitorgeometry.get_y();
	fv_x[caltargets.size()] = monitorgeometry.get_x()-40;
	fv_y[caltargets.size()] = monitorgeometry.get_y()-40;
	
	xv[caltargets.size()+1][0] = monitorgeometry.get_x() + monitorgeometry.get_width();
	xv[caltargets.size()+1][1] = monitorgeometry.get_y();
	fv_x[caltargets.size()+1] = monitorgeometry.get_x() + monitorgeometry.get_width() + 40;
	fv_y[caltargets.size()+1] = monitorgeometry.get_y() - 40;
	
	xv[caltargets.size()+2][0] = monitorgeometry.get_x() + monitorgeometry.get_width();
	xv[caltargets.size()+2][1] = monitorgeometry.get_y() + monitorgeometry.get_height();
	fv_x[caltargets.size()+2] = monitorgeometry.get_x() + monitorgeometry.get_width() + 40;
	fv_y[caltargets.size()+2] = monitorgeometry.get_y() + monitorgeometry.get_height() + 40;
	
	xv[caltargets.size()+3][0] = monitorgeometry.get_x();
	xv[caltargets.size()+3][1] = monitorgeometry.get_y() + monitorgeometry.get_height();
	fv_x[caltargets.size()+3] = monitorgeometry.get_x() - 40;
	fv_y[caltargets.size()+3] = monitorgeometry.get_y() + monitorgeometry.get_height() + 40;
	
	int point_count = caltargets.size() + 4;
    int N = point_count;
    N = binomialInv(N, 2) - 1;
	
	// Find the best beta and gamma parameters for interpolation
    mirBetaGamma(1, 2, point_count, (double*)xv, fv_x, sigv, 0, NULL, NULL, NULL,
                 N, 2, 50.0, &beta_x, &gamma_x);
    mirBetaGamma(1, 2, point_count, (double*)xv, fv_y, sigv, 0, NULL, NULL, NULL,
                 N, 2, 50.0, &beta_y, &gamma_y);
	
	*output_file << endl << endl;
	cout << endl << endl;
	
	output_file->flush();
	
	
	cout << "ERROR CALCULATION FINISHED. BETA = " << beta_x << ", " << beta_y << ", GAMMA IS " << gamma_x << ", " << gamma_y << endl;
	for(int j=0; j<point_count; j++) {
			cout << xv[j][0] << ", " << xv[j][1] << endl;
	}
	

    //checkErrorCorrection();
}


void GazeTracker::printTrainingErrors() {
	int num_of_monitors = Gdk::Screen::get_default()->get_n_monitors();
	Gdk::Rectangle monitorgeometry;
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();
	
	//return;

	// Geometry of main monitorGazeTracker.cpp:233:136: error: expected ‘;’ before string constant

	screen->get_monitor_geometry(num_of_monitors - 1, monitorgeometry);
	
	vector<Point> points = getsubvector(caltargets, &CalTarget::point);
	
	int j = 0;
	
	/*
	cout << "PRINTING TRAINING ESTIMATIONS: " << endl;
	for(int i=0; i<15; i++) {
		int image_index = 0;
		
		while(j < input_count && points[i].x == all_output_coords[j][0] && points[i].y == all_output_coords[j][1]) {
			cout << "X, Y: '" << gpx->getmean(SharedImage(all_images[j], &ignore)) << ", " << gpy->getmean(SharedImage(all_images[j], &ignore)) << "' and '" << gpx_left->getmean(SharedImage(all_images_left[j], &ignore)) << ", " << gpy_left->getmean(SharedImage(all_images_left[j], &ignore)) << "' "<< endl;
			
			image_index++;
			j++;
		}
		
	}
	* */
}


void GazeTracker::clear() {
    caltargets.clear();
    caltargets_left.clear();
    
    beta_x = -1;
    gamma_x = -1;

	ANN = fann_create_standard(2, nn_eyewidth * nn_eyeheight, 2);
	fann_set_activation_function_output(ANN, FANN_SIGMOID);
	
	ANN_left = fann_create_standard(2, nn_eyewidth * nn_eyeheight, 2);
	fann_set_activation_function_output(ANN_left, FANN_SIGMOID);
    // updateGPs()
}

void GazeTracker::addExemplar(Point point, 
			      const IplImage *eyefloat, 
			      const IplImage *eyegrey,
			      vector<int> hist_horizontal,
			      vector<int> hist_vertical) // TODO ARCADI CONTINUE, Add histogram parameters
{

	cout << "RIGHT" << endl;
    caltargets.push_back(CalTarget(point, eyefloat, eyegrey, hist_horizontal, hist_vertical)); // TODO ARCADI CONTINUE, Add histograms to caltarget structure
    updateGPs();
}

void GazeTracker::addExemplar_left(Point point, 
			      const IplImage *eyefloat, 
			      const IplImage *eyegrey,
			      vector<int> hist_horizontal_left,
			      vector<int> hist_vertical_left) 	 // TODO ARCADI CONTINUE, Add histogram parameters
{

	cout << "LEFT" << endl;
    caltargets_left.push_back(CalTarget(point, eyefloat, eyegrey, hist_horizontal_left, hist_vertical_left)); // TODO ARCADI CONTINUE, Add histograms to caltarget structure
    updateGPs_left();
}

// Neural network
void GazeTracker::addSampleToNN(Point point, 
				const IplImage *eyefloat,
  				const IplImage *eyegrey)
{
	// Save the entire grey image for later use
	IplImage *savedimage = cvCreateImage(EyeExtractor::eyesize, IPL_DEPTH_32F, 1);
	cvCopy(eyefloat, savedimage);
	
	all_images[input_count] = savedimage;
	all_output_coords[input_count][0] = point.x;
	all_output_coords[input_count][1] = point.y;
	
	// Resize image to 16x8 and equalize histogram
	cvResize(eyegrey, nn_eye);
	//cvEqualizeHist(nn_eye, nn_eye);
	
	// Convert image to interval [0, 1]
	fann_type* inputs = new fann_type[nn_eyewidth * nn_eyeheight];
	for (int i = 0; i < nn_eyewidth * nn_eyeheight; ++i) 
	{ 
		inputs[i] = (float)(nn_eye->imageData[i] + 129) / 257.0f;
		
		if(inputs[i] <0 || inputs[i] > 1) {
			cout << "IMPOSSIBLE INPUT!" << endl;
		}
//		if(((int) eyegrey->imageData[i] >= 127) || ((int) eyegrey->imageData[i] <= -127))
//			cout << "INPUT[" << i << "] = " << inputs[i] << ", image data = " << (int) eyegrey->imageData[i] << endl;
	}
	
	// Convert coordinates to interval [0, 1]
	Point nnpoint;
	mapToNeuralNetworkCoordinates(point, nnpoint);
	
	fann_type* outputs = new fann_type[2];
	outputs[0] = nnpoint.x;
	outputs[1] = nnpoint.y;
	
	all_outputs[input_count] = &(outputs[0]);
	all_inputs[input_count] = &(inputs[0]);
	input_count++;
	
	//cout << "Added sample # " << input_count << endl;
	//for(int j=0; j<100; j++)
	//fann_train(ANN, inputs, outputs);	// Moved training to batch
}


void GazeTracker::addSampleToNN_left(Point point, 
				const IplImage *eyefloat,
  				const IplImage *eyegrey)
{
	// Save the entire grey image for later use
	IplImage *savedimage = cvCreateImage(EyeExtractor::eyesize, IPL_DEPTH_32F, 1);
	cvCopy(eyefloat, savedimage);
	
	all_images_left[input_count_left] = savedimage;
	
	// Resize image to 16x8
	cvResize(eyegrey, nn_eye);
	//cvEqualizeHist(nn_eye, nn_eye);

	// Convert image to interval [0, 1]
	fann_type* inputs = new fann_type[nn_eyewidth * nn_eyeheight];
	for (int i = 0; i < nn_eyewidth * nn_eyeheight; ++i)
	{ 
		inputs[i] = (float)(nn_eye->imageData[i] + 129) / 257.0f;
		
		if(inputs[i] <0 || inputs[i] > 1) {
			cout << "IMPOSSIBLE INPUT!" << endl;
		}
	}
	
	// Convert coordinates to interval [0, 1]
	Point nnpoint;
	mapToNeuralNetworkCoordinates(point, nnpoint);
	
	fann_type* outputs = new fann_type[2];
	outputs[0] = nnpoint.x;
	outputs[1] = nnpoint.y;
	
	all_outputs_left[input_count_left] = outputs;
	all_inputs_left[input_count_left] = inputs;
	input_count_left++;
	
	//cout << "(Left) Added sample # " << input_count_left << endl;
	//for(int j=0; j<100; j++)
	//fann_train(ANN_left, inputs, outputs);	// Moved training to batch
}

void FANN_API getTrainingData(unsigned int row, unsigned int input_size, unsigned int output_size, fann_type* input, fann_type* output)
{
	//cout << "GTD: row=" << row << ", inp. size=" << input_size << ", op. size=" << output_size << endl;
	int i;
	for(i=0; i<input_size; i++)
		input[i] = all_inputs[row][i];
		
	for(i=0; i<output_size; i++) 
		output[i] = all_outputs[row][i];
		
	//memcpy(input, all_inputs[row], input_size * sizeof(fann_type));
	//memcpy(output, all_outputs[row], output_size * sizeof(fann_type));
}

void FANN_API getTrainingData_left(unsigned int row, unsigned int input_size, unsigned int output_size, fann_type* input, fann_type* output)
{
	//cout << "GTD: row=" << row << ", inp. size=" << input_size << ", op. size=" << output_size << endl;
	int i;
	for(i=0; i<input_size; i++)
		input[i] = all_inputs_left[row][i];
		
	for(i=0; i<output_size; i++) 
		output[i] = all_outputs_left[row][i];
		//memcpy(input, all_inputs_left[row], input_size * sizeof(fann_type));
		//memcpy(output, all_outputs_left[row], output_size * sizeof(fann_type));
}

void GazeTracker::trainNN()
{
	cout << "Getting data" << endl;
	struct fann_train_data* data = fann_create_train_from_callback(input_count, nn_eyewidth * nn_eyeheight, 2, getTrainingData);
	//fann_save_train(data, "data.txt");

	cout << "Getting left data" << endl;
	struct fann_train_data* data_left = fann_create_train_from_callback(input_count, nn_eyewidth * nn_eyeheight, 2, getTrainingData_left);
	//fann_save_train(data_left, "data_left.txt");

	fann_set_training_algorithm(ANN, FANN_TRAIN_RPROP);
	fann_set_learning_rate(ANN, 0.75);
	fann_set_training_algorithm(ANN_left, FANN_TRAIN_RPROP);
	fann_set_learning_rate(ANN_left, 0.75);
	
	cout << "Training" << endl;
	fann_train_on_data(ANN, data, 200, 20, 0.01);

	cout << "Training left" << endl;
	fann_train_on_data(ANN_left, data_left, 200, 20, 0.01);
	
	double mse = fann_get_MSE(ANN);
	double mse_left = fann_get_MSE(ANN_left);
	
	cout << "MSE: " << mse << ", MSE left: " << mse_left << endl;
}

// void GazeTracker::updateExemplar(int id, 
// 				 const IplImage *eyefloat, 
// 				 const IplImage *eyegrey)
// {
//     cvConvertScale(eyegrey, caltargets[id].origimage.get());
//     cvAdd(caltargets[id].image.get(), eyefloat, caltargets[id].image.get());
//     cvConvertScale(caltargets[id].image.get(), caltargets[id].image.get(), 0.5);
//     updateGPs();
// }

void GazeTracker::draw(IplImage *destimage, int eyedx, int eyedy) {
//     for(int i=0; i<caltargets.size(); i++) {
// 	Point p = caltargets[i].point;
// 	cvSetImageROI(destimage, cvRect((int)p.x - eyedx, (int)p.y - eyedy, 
// 					2*eyedx, 2*eyedy));
// 	cvCvtColor(caltargets[i].origimage, destimage, CV_GRAY2RGB);
// 	cvRectangle(destimage, cvPoint(0,0), cvPoint(2*eyedx-1,2*eyedy-1),
// 		    CV_RGB(255,0,255));
//     }
//     cvResetImageROI(destimage);
}

void GazeTracker::save(void) {
    CvFileStorage *out = 
	cvOpenFileStorage("calibration.xml", NULL, CV_STORAGE_WRITE);
    save(out, "GazeTracker");
    cvReleaseFileStorage(&out);
}

void GazeTracker::save(CvFileStorage *out, const char *name) {
    cvStartWriteStruct(out, name, CV_NODE_MAP);
    savevector(out, "caltargets", caltargets);
    cvEndWriteStruct(out);
}


void GazeTracker::load(void) {
    CvFileStorage *in = 
	cvOpenFileStorage("calibration.xml", NULL, CV_STORAGE_READ);
    CvFileNode *root = cvGetRootFileNode(in);
    load(in, cvGetFileNodeByName(in, root, "GazeTracker"));
    cvReleaseFileStorage(&in);
    updateGPs();
}

void GazeTracker::load(CvFileStorage *in, CvFileNode *node) {
    caltargets = loadvector<CalTarget>(in, cvGetFileNodeByName(in, node, 
							       "caltargets"));
}

void GazeTracker::update(const IplImage *image, const IplImage *eyegrey, vector<int> vector_horizontal,
			      vector<int> vector_vertical) {
    if (isActive()) {
		//output.gazepoint = Point(gpx->getmean(SharedImage(image, &ignore)), 
		//			 gpy->getmean(SharedImage(image, &ignore)));

		output.gazepoint = Point(histx->getmean(vector_horizontal), 
					 histy->getmean(vector_vertical));

/*
    	vector<int> vector_horizontal_and_vertical;
		
		copy( vector_horizontal.begin(), vector_horizontal.end(), back_inserter(vector_horizontal_and_vertical));
		copy( vector_vertical.begin(), vector_vertical.end(), back_inserter(vector_horizontal_and_vertical));

		output.gazepoint = Point(histx->getmean(vector_horizontal_and_vertical), 
					 histy->getmean(vector_horizontal_and_vertical));
*/
		//double* out = regression.CalculateOutput(vector_horizontal, vector_vertical);

		//output.gazepoint = Point(out[0], out[1]);

		

		output.targetid = getTargetId(output.gazepoint);
		output.target = getTarget(output.targetid);
	
		// Neural network
		// Resize image to 16x8
		cvResize(eyegrey, nn_eye);
		cvEqualizeHist(nn_eye, nn_eye);
		
		fann_type inputs[nn_eyewidth * nn_eyeheight];
		for (int i = 0; i < nn_eyewidth * nn_eyeheight; ++i) 
		{ 
			inputs[i] = (float)(nn_eye->imageData[i] + 129) / 257.0f;
		}
	
		fann_type* outputs = fann_run(ANN, inputs);
		mapFromNeuralNetworkToScreenCoordinates(Point(outputs[0], outputs[1]), output.nn_gazepoint); 
    }
}

void GazeTracker::update_left(const IplImage *image, const IplImage *eyegrey, vector<int> vector_horizontal_left,
			      vector<int> vector_vertical_left) {
    if (isActive()) {
		//output.gazepoint_left = Point(gpx_left->getmean(SharedImage(image, &ignore)), 
		//			 gpy_left->getmean(SharedImage(image, &ignore)));

		output.gazepoint_left = Point(histx_left->getmean(vector_horizontal_left), 
					 histy_left->getmean(vector_vertical_left));

/*
    	vector<int> vector_horizontal_and_vertical_left;
		
		copy( vector_horizontal_left.begin(), vector_horizontal_left.end(), back_inserter(vector_horizontal_and_vertical_left));
		copy( vector_vertical_left.begin(), vector_vertical_left.end(), back_inserter(vector_horizontal_and_vertical_left));

		output.gazepoint_left = Point(histx_left->getmean(vector_horizontal_and_vertical_left), 
					 histy_left->getmean(vector_horizontal_and_vertical_left));
*/
		// Neural network
		// Resize image to 16x8
		cvResize(eyegrey, nn_eye);
		cvEqualizeHist(nn_eye, nn_eye);

		fann_type inputs[nn_eyewidth * nn_eyeheight];
		for (int i = 0; i < nn_eyewidth * nn_eyeheight; ++i)
		{ 
			inputs[i] = (float)(nn_eye->imageData[i] + 129) / 257.0f;
		}

		fann_type* outputs = fann_run(ANN_left, inputs);
		mapFromNeuralNetworkToScreenCoordinates(Point(outputs[0], outputs[1]), output.nn_gazepoint_left);
		
		if(gamma_x != 0) {
			// Overwrite the NN output with the GP output with calibration errors removed
			output.nn_gazepoint.x = (output.gazepoint.x + output.gazepoint_left.x) / 2;
			output.nn_gazepoint.y = (output.gazepoint.y + output.gazepoint_left.y) / 2;
		
			removeCalibrationError(output.nn_gazepoint);
		
			output.nn_gazepoint_left.x = output.nn_gazepoint.x;
			output.nn_gazepoint_left.y = output.nn_gazepoint.y;
		}
    }
}

void GazeTracker::removeCalibrationError(Point& estimate) {
	double x[1][2];
	double output[1];
	double sigma[1];
	int point_count = caltargets.size() + 4;
	
	if(beta_x == -1 && gamma_x == -1)
		return;
	
	x[0][0] = estimate.x;
	x[0][1] = estimate.y;
	/*
	cout << "INSIDE CAL ERR REM. BETA = " << beta_x << ", " << beta_y << ", GAMMA IS " << gamma_x << ", " << gamma_y << endl;
	for(int j=0; j<point_count; j++) {
			cout << xv[j][0] << ", " << xv[j][1] << endl;
	}
	*/
    int N = point_count;
    N = binomialInv(N, 2) - 1;
    
    //cout << "CALIB. ERROR REMOVAL. Target size: " << point_count << ", " << N << endl; 
	
    mirEvaluate(1, 2, 1, (double*)x, point_count, (double*)xv, fv_x, sigv,
                0, NULL, NULL, NULL, beta_x, gamma_x, N, 2, output, sigma);
                
    if(output[0] >= -100)
        estimate.x = output[0];
	
	
    mirEvaluate(1, 2, 1, (double*)x, point_count, (double*)xv, fv_y, sigv,
                0, NULL, NULL, NULL, beta_y, gamma_y, N, 2, output, sigma);

    if(output[0] >= -100)
        estimate.y = output[0];
	
	//cout << "Estimation corrected from: ("<< x[0][0] << ", " << x[0][1] << ") to ("<< estimate.x << ", " << estimate.y << ")" << endl;
	
	boundToScreenCoordinates(estimate);
	
	//cout << "Estimation corrected from: ("<< x[0][0] << ", " << x[0][1] << ") to ("<< estimate.x << ", " << estimate.y << ")" << endl;
}

void GazeTracker::boundToScreenCoordinates(Point& estimate) {
	int num_of_monitors = Gdk::Screen::get_default()->get_n_monitors();
	Gdk::Rectangle monitorgeometry;
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();

	// Geometry of main monitor
	screen->get_monitor_geometry(num_of_monitors - 1, monitorgeometry);
	
	// If x or y coordinates are outside screen boundaries, correct them
	if(estimate.x < monitorgeometry.get_x())
		estimate.x = monitorgeometry.get_x();
	
	if(estimate.y < monitorgeometry.get_y())
		estimate.y = monitorgeometry.get_y();
		
	if(estimate.x >= monitorgeometry.get_x() + monitorgeometry.get_width())
		estimate.x = monitorgeometry.get_x() + monitorgeometry.get_width();
		
	if(estimate.y >= monitorgeometry.get_y() + monitorgeometry.get_height())
		estimate.y = monitorgeometry.get_y() + monitorgeometry.get_height();
}

int GazeTracker::getTargetId(Point point) {
    return targets->getCurrentTarget(point);
}

Point GazeTracker::getTarget(int id) {
    return targets->targets[id];
}





    CalculateRegression::CalculateRegression():
    inputIndex(0),
    beta(-1),
    gamma(-1)
    {
    }

    //CalculateRegression::CalculateRegression(): {};

    void CalculateRegression::CalculateMedian(Point point, vector<int> sample_horizontal, vector<int> sample_vertical){

        int i, j;

        fv[inputIndex] = point.x;
		sigv[inputIndex] = 0;

        accumulate_horizontal = 0;

        for (i = 0; i < sample_horizontal.size(); i++) {
            XV2[inputIndex][0] += sample_horizontal[i] * (i + 1);
            accumulate_horizontal += sample_horizontal[i];
        }

        XV2[inputIndex][0] = XV2[inputIndex][0] / accumulate_horizontal;

        accumulate_vertical = 0;

        for (j = 0; j < sample_vertical.size(); j++) {
            XV2[inputIndex][2] += sample_vertical[j] * (j + 1);
            accumulate_vertical += sample_vertical[j];
        }

        XV2[inputIndex][2] = XV2[inputIndex][2] / accumulate_vertical;


    }

    void CalculateRegression::CalculateStandardDeviation(vector<int> sample_horizontal, vector<int> sample_vertical){

        int i, j;
        double medianIndex = XV2[inputIndex][0];

        for (i = 0; i < sample_horizontal.size(); i++) {
            XV2[inputIndex][1] += pow((i + 1) - medianIndex, 2) * sample_horizontal[i];
        }

        XV2[inputIndex][1] = XV2[inputIndex][1] / accumulate_horizontal;

        medianIndex = XV2[inputIndex][2];

        for (j = 0; j < sample_vertical.size(); j++) {
            XV2[inputIndex][3] += pow((j + 1) - medianIndex, 2) * sample_vertical[j];
        }

        XV2[inputIndex][3] = XV2[inputIndex][3] / accumulate_vertical;

        inputIndex++; // VIGILA CON EL INDICE

    }


    double CalculateRegression::CalculateMedianSingleInput(vector<int> sample){

        int i, j;
        double aux = 0;

        double accumulate = 0;

        for (i = 0; i < sample.size(); i++) {
            aux += sample[i] * (i + 1);
            accumulate += sample[i];
        }

        return aux / accumulate;

    }

    double CalculateRegression::CalculateStandardDeviationSingleInput(vector<int> sample, double medianIndex){


        int i, j;
        double aux = 0;

        double accumulate = 0;

        for (i = 0; i < sample.size(); i++) {
            aux += pow((i + 1) - medianIndex, 2) * sample[i];
            accumulate += sample[i];
        }

        aux = aux / accumulate;

        return aux;

    }

    void CalculateRegression::AddSample(Point point, vector<int> sample_horizontal, vector<int> sample_vertical){
    	// TODO UNCOMMENT
        //fv[inputIndex][0] = point.x;
        //fv[inputIndex][1] = point.y;
        fv[inputIndex] = point.x;
        
        sigv[inputIndex] = 0;

        int i, totalSize;

        for (i = 0; i < sample_horizontal.size(); i++) {
            XV[inputIndex][i] = sample_horizontal[i];
        }
        for (; i < SIZESAMPLE; i++) {
            XV[inputIndex][i] = sample_vertical[i];
        }

        inputIndex++;

    }

    void CalculateRegression::CalculateRegressionTranning(){

        for (int i = 0; i < 15; i++) {
            cout << "INPUT " << i << "= " << XV2[i][0] << " , " << XV2[i][1]<< " , " << XV2[i][2]<< " , " << XV2[i][3] << endl;
            cout << "OUTPUT " << i << "= " << fv[i] << endl;
        }

		cout << "ANTES" << endl;        

/*
Calculate the best parameters beta and gamma

Input arguments:
    nfunc   Number of functions approximated
    ndim    Dimension of the approximation space (>0)
    nv      number of value data points (>0)
    xv      ndim * nv dimensional array, value data points
    fv      nfunc * nv dimensional array, function values at value data points
    sigv    nv dimensional array, measurement error of value data points
    ng      number of gradient data points (>=0)
    xg      ndim * ng dimensional array, gradient data points
    fg      nfunc * ng * ndim dimensional array, function gradients at
            gradient data points
    sigg    ng dimensional array, measurement error of gradient data points
    N       The Taylor order parameter (>0)
    P       The polynomial exactness parameter (>=0)
    safety  The safety factor, must be greater than 0.
            Higher than 1 will produce a conservative (larger) gamma,
            lower than 1 will produce a aggressive (smaller) gamma.
Output arguments:
    beta    The magnitude parameter
    gamma   The wavenumber parameter
Return value:
    0 if ok, -1 if error.
*/


        mirBetaGamma(1, 4, inputIndex, (double*) XV2, (double*) fv, sigv, 0, NULL, NULL, NULL,
                     binomialInv(inputIndex, 2) - 1, 2, 50.0, &beta, &gamma);



		cout << "DESPUES" << endl;

        cout << "CALIBRATED, VALUES FOR beta=" << beta << ", gamma=" << gamma << endl;

    }

    double* CalculateRegression::CalculateOutput(vector<int> sample_horizontal, vector<int> sample_vertical){
        
    	double *output = new double[2];
		double sigma[1];


		if(beta == -1 && gamma == -1) {
			output[0] = 0;
			output[1] = 0;

			return output;
		}

		double x[1][4];

		x[0][0] = CalculateMedianSingleInput(sample_horizontal);
		x[0][1] = CalculateStandardDeviationSingleInput(sample_horizontal, x[0][0]);
		x[0][2] = CalculateMedianSingleInput(sample_vertical);
		x[0][3] = CalculateStandardDeviationSingleInput(sample_vertical, x[0][2]);

		cout << "INPUTS: " << x[0][0] << " " << x[0][1] << " " << x[0][2] << " " << x[0][3] << endl;
/*
Input arguments:
    nfunc   Number of functions approximated
    ndim    Dimension of the approximation space (>0)
    nx      number of approximation points
    x       ndim * nx dimensional array, the approximation points (*)
    nv      number of value data points (>0)
    xv      ndim * nv dimensional array, value data points (*)
    fv      nv * nfunc dimensional array, function values at value data points
            (**)
    sigv    nv dimensional array, measurement error of value data points
    ng      number of gradient data points (>=0)
    xg      ndim * ng dimensional array, gradient data points (*)
    fg      nfunc * ng * ndim dimensional array, function gradients at
            gradient data points (***)
    sigg    ng dimensional array, measurement error of gradient data points
    beta    The magnitude parameter (>0)
    gamma   The wavenumber parameter (>0)
    N       The Taylor order parameter (>0)
    P       The polynomial exactness parameter (>=0)
Output arguments:
    fx      nx * nfunc dimensional array, the approximation function value
            at each approximation point (**)
    sigma   nx dimensional array, estimated approximation error at each
            approximation point
Return value:
    0 if ok, -1 if error.
*/

		//clock_t launch = clock();

		// En el 4o parametro va x

        mirEvaluate(1, 4, 1, (double*) x, inputIndex, (double*) XV2, (double*) fv, sigv,
                0, NULL, NULL, NULL, beta, gamma, binomialInv(inputIndex, 2) - 1, 2, output, sigma);


		//clock_t done = clock();
		//double diff = (done - launch) / (double) CLOCKS_PER_SEC;

		//cout << "diff: " << diff << endl;
		//cin.get();


        output[1] = 100;

        return output;

    }


