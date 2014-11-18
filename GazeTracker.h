#pragma once
#include "utils.h"
#include "GaussianProcess.cpp"

typedef MeanAdjustedGaussianProcess<SharedImage> ImProcess;
typedef MeanAdjustedGaussianProcess<vector<int> > HistProcess;	// TODO ARCADI CONTINUE


const int nn_eyewidth = 16;
const int nn_eyeheight = 8;

struct Targets {
    vector<Point> targets;

    Targets(void) {};
    Targets(vector<Point> const& targets): targets(targets) {}
    int getCurrentTarget(Point point);
};

struct CalTarget {
    Point point;
    SharedImage image, origimage;

    vector<int> vector_horizontal, vector_vertical;

    CalTarget();
    CalTarget(Point point, const IplImage* image, const IplImage* origimage, vector<int> vector_horizontal,
			      vector<int> vector_vertical);

    void save(CvFileStorage* out, const char* name=NULL);
    void load(CvFileStorage* in, CvFileNode *node);
};

struct TrackerOutput {
    Point gazepoint;
    Point gazepoint_left;
	// Neural network
	Point nn_gazepoint;
	Point nn_gazepoint_left;
    Point target;
	Point actualTarget;
    int targetid;
	int frameid;
	//bool outputError;

    TrackerOutput(Point gazepoint, Point target, int targetid);
	void setActualTarget(Point actual);
	//void setErrorOutput(bool show);
	void setFrameId(int id);
};

class GazeTracker {
    scoped_ptr<ImProcess> gpx, gpy;
    scoped_ptr<HistProcess> histx, histy;
    vector<CalTarget> caltargets;
    scoped_ptr<Targets> targets;

	// ONUR DUPLICATED CODE FOR LEFT EYE
    scoped_ptr<ImProcess> gpx_left, gpy_left;
    scoped_ptr<HistProcess> histx_left, histy_left;
    vector<CalTarget> caltargets_left;
    //scoped_ptr<Targets> targets_left;
    
	// Neural network
	struct fann *ANN, *ANN_left;
	int input_count, input_count_left;
	
	// Calibration error removal
	double beta_x, gamma_x, beta_y, gamma_y, sigv[100];	// Max 100 calibration points
	double xv[100][2], fv_x[100], fv_y[100];
	
	IplImage *nn_eye;
	
    static double imagedistance(const IplImage *im1, const IplImage *im2);
    static double covariancefunction(const SharedImage& im1, 
				     const SharedImage& im2);

    static double histDistance(vector<int> hist_horizontal, vector<int> hist_vertical);
    static double covariancefunction_hist(const vector<int>& hist_horizontal, const vector<int>& hist_vertical);

    void updateGPs(void);
	void updateGPs_left(void);

public:
    TrackerOutput output;
	ostream* output_file;

    GazeTracker(): targets(new Targets), 
	output(Point(0,0), Point(0,0), -1), nn_eye(cvCreateImage(cvSize(16, 8), 8, 1)),
	input_count(0), input_count_left(0) {}

    bool isActive() { return gpx.get() && gpy.get(); }

    void clear();
    void addExemplar(Point point, 
		     const IplImage *eyefloat, const IplImage *eyegrey, vector<int> vector_horizontal,
			      vector<int> vector_vertical);
    void addExemplar_left(Point point, 
		     const IplImage *eyefloat, const IplImage *eyegrey, vector<int> vector_horizontal_left,
			      vector<int> vector_vertical_left);
	// Neural network
	void addSampleToNN(Point point, 
			const IplImage *eyefloat, const IplImage *eyegrey);
	void addSampleToNN_left(Point point, 
			const IplImage *eyefloat, const IplImage *eyegrey);
	void trainNN();
	
	//Calibration error removal
	void removeCalibrationError(Point& estimate);
	void boundToScreenCoordinates(Point& estimate);
    void checkErrorCorrection();
	
    void draw(IplImage *canvas, int eyedx, int eyedy);
    void save(void);
    void save(CvFileStorage *out, const char *name);
    void load(void);
    void load(CvFileStorage *in, CvFileNode *node);
    void update(const IplImage *image, const IplImage *eyegrey, vector<int> vector_horizontal,
			      vector<int> vector_vertical);
    void update_left(const IplImage *image, const IplImage *eyegrey, vector<int> vector_horizontal_left,
			      vector<int> vector_vertical_left);
    int getTargetId(Point point);
	void calculateTrainingErrors();
	void printTrainingErrors();
	Point getTarget(int id);
};
