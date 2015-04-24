#include "Calibrator.h"
#include <fstream>

/*
std::vector<int> magic_function(std::vector<int> res, std::vector<int> vectorX) {

    for (int i = 0; i < vectorX.size(); i++){
        res[i] += vectorX[i];
    }

    return res;

}
*/


Calibrator::~Calibrator() {
#ifdef DEBUG
    cout << "Destroying calibrator" << endl;
#endif
}

FrameFunction::~FrameFunction() {
#ifdef DEBUG
    cout << "Destroying framefunction" << endl;
#endif
}

MovingTarget::MovingTarget(const int &frameno, 
			   const vector<Point>& points, 
               const boost::shared_ptr<WindowPointer> &pointer,
			   int dwelltime):
    FrameFunction(frameno), 
    points(points), dwelltime(dwelltime), pointer(pointer)
{    
};

MovingTarget::~MovingTarget() {
    int id = getFrame() / dwelltime;
}

void MovingTarget::process() {
    if (getPointNo() != points.size() && active()) {
        int id = getPointNo();
        
        if (getPointFrame() == 1) 
            pointer->setPosition((int)points[id].x, (int)points[id].y); 
    }
    else {
        if(getPointNo() == points.size() && tracker_status == STATUS_TESTING) {
            tracker_status = STATUS_CALIBRATED;
        }
        detach();
    }
}

bool MovingTarget::active() {
    if(parent == NULL) 
        return false;
    
    return getPointNo() < (int) points.size();
}

bool MovingTarget::isLast() {
    return getPointNo() == ((int) points.size()) - 1;
}

int MovingTarget::getPointNo() {
    return getFrame() / dwelltime;
}

int MovingTarget::getPointFrame() {
    return getFrame() % dwelltime;
}

int MovingTarget::getDwellTime() {
    return dwelltime;
}

Point MovingTarget::getActivePoint() {
    int id = getPointNo();
    
    return points[id];
}

Calibrator::Calibrator(const int &framecount, 
               const boost::shared_ptr<TrackingSystem> &trackingsystem,
               const vector<Point>& points, 
               const boost::shared_ptr<WindowPointer> &pointer,
               int dwelltime): 
    MovingTarget(framecount, points, pointer, dwelltime),
    trackingsystem(trackingsystem),

    vectorOfVectors_horizontal(new vector<vector<int> >),
    vectorOfVectors_horizontal_left(new vector<vector<int> >),
    vectorOfVectors_vertical(new vector<vector<int> >),
    vectorOfVectors_vertical_left(new vector<vector<int> >),

    histPositionSegmentedPixels(new vector<vector<vector<int> > >),
    histPositionSegmentedPixels_left(new vector<vector<vector<int> > >)

{
    trackingsystem->gazetracker.clear();
    // todo: remove all calibration points
}


void Calibrator::process() {
    static int dummy = 0;

    cout << "Calibrator process()" << endl;
    
    if (active()) {
        int id = getPointNo();
        int frame = getPointFrame();
        if (frame == 1) {// start
            averageeye.reset(new FeatureDetector(EyeExtractor::eyesize));
            averageeye_left.reset(new FeatureDetector(EyeExtractor::eyesize));
        }
        if (frame >= 11) { // middle    ONUR dwelltime/2 changed to 11
            if(!trackingsystem->eyex.isBlinking()) {

                vectorOfVectors_horizontal->push_back(*trackingsystem->eyex.vector_horizontal);
                vectorOfVectors_vertical->push_back(*trackingsystem->eyex.vector_vertical);
                vectorOfVectors_horizontal_left->push_back(*trackingsystem->eyex.vector_horizontal_left);
                vectorOfVectors_vertical_left->push_back(*trackingsystem->eyex.vector_vertical_left);

                // TODO ARCADI 23/12

                averageeye->addSample(trackingsystem->eyex.eyeGraySegmented);
                averageeye_left->addSample(trackingsystem->eyex.eyeGraySegmented_left);

                //averageeye->addSample(trackingsystem->eyex.eyefloat.get());
                //averageeye_left->addSample(trackingsystem->eyex.eyefloat_left.get());
                
                // Neural network 
                //if(dummy % 8 == 0) {  // Only add samples on the 11-19-27-35 frames
                //for(int i=0; i<1000; i++) {   // Train 100 times with each frame
                    trackingsystem->gazetracker.
                    addSampleToNN(points[id], trackingsystem->eyex.eyefloat.get(), trackingsystem->eyex.eyegrey.get());
                    trackingsystem->gazetracker.
                    addSampleToNN_left(points[id], trackingsystem->eyex.eyefloat_left.get(), trackingsystem->eyex.eyegrey_left.get());
                    
                    dummy++;
                //}
            }
            else {
                cout << "Skipped adding sample!!!!" << endl;
            }
        }
    
        if (frame == dwelltime-1) { // end

            /*

            std::accumulate(vectorOfVectors_horizontal->begin()+getPointNo()+1, vectorOfVectors_horizontal->end(), vectorOfVectors_horizontal->begin()+getPointNo(), 
                                        magic_function);

            */

    cout << "Calibrator process() dwelltime-1" << endl;
            int sizeVectorOfVectors = vectorOfVectors_horizontal->size();

            for (int i = sizeVectorOfVectors-1; i > getPointNo(); i--) {

                int sizeVector = vectorOfVectors_horizontal->operator[](i).size();

                for (int j = 0; j < sizeVector; j++) {

                    vectorOfVectors_horizontal->operator[](getPointNo()).operator[](j) += vectorOfVectors_horizontal->operator[](i).operator[](j);
                    vectorOfVectors_horizontal_left->operator[](getPointNo()).operator[](j) += vectorOfVectors_horizontal_left->operator[](i).operator[](j);
                    
                }

                sizeVector = vectorOfVectors_vertical->operator[](i).size();

                for (int j = 0; j < sizeVector; j++) {

                    vectorOfVectors_vertical->operator[](getPointNo()).operator[](j) += vectorOfVectors_vertical->operator[](i).operator[](j);
                    vectorOfVectors_vertical_left->operator[](getPointNo()).operator[](j) += vectorOfVectors_vertical_left->operator[](i).operator[](j);
                    
                }

                vectorOfVectors_horizontal->pop_back();
                vectorOfVectors_vertical->pop_back();
                vectorOfVectors_horizontal_left->pop_back();
                vectorOfVectors_vertical_left->pop_back();

            }
    cout << "Calibrator process() for 2" << endl;

            for (int j = 0; j < vectorOfVectors_horizontal->operator[](getPointNo()).size(); j++) {

                vectorOfVectors_horizontal->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_horizontal->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
                vectorOfVectors_horizontal_left->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_horizontal_left->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
            }

    cout << "Calibrator process() for 3" << endl;
            for (int j = 0; j < vectorOfVectors_vertical->operator[](getPointNo()).size(); j++) {

                vectorOfVectors_vertical->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_vertical->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
                vectorOfVectors_vertical_left->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_vertical_left->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
            
            }

    cout << "Calibrator process() resto" << endl;
            std::vector<std::vector<int> > AUX_POSITION_VECTOR;

    cout << "Calibrator process() resto 2" << endl;
            trackingsystem->eyex.extractFeatures.SortHistogram(&(vectorOfVectors_horizontal->operator[](getPointNo())), &(vectorOfVectors_vertical->operator[](getPointNo())), &AUX_POSITION_VECTOR);

    cout << "Calibrator process() resto 3" << endl;
            histPositionSegmentedPixels->push_back(AUX_POSITION_VECTOR);

    cout << "Calibrator process() resto 4" << endl;
            std::vector<std::vector<int> > AUX_POSITION_VECTOR_LEFT;

    cout << "Calibrator process() resto 5" << endl;
            trackingsystem->eyex.extractFeatures.SortHistogram(&(vectorOfVectors_horizontal_left->operator[](getPointNo())), &(vectorOfVectors_vertical_left->operator[](getPointNo())), &AUX_POSITION_VECTOR_LEFT);

    cout << "Calibrator process() resto 6" << endl;
            histPositionSegmentedPixels_left->push_back(AUX_POSITION_VECTOR_LEFT);

            /*
            trackingsystem->gazetracker.
            addExemplar(points[id], averageeye->getMean().get(),
                    trackingsystem->eyex.eyegrey.get(), 
                    vectorOfVectors_horizontal->operator[](getPointNo()), vectorOfVectors_vertical->operator[](getPointNo()));
            */

    cout << "Calibrator process() add exemplar" << endl;
            trackingsystem->gazetracker.
            addExemplar(points[id], averageeye->getMean().get(),
                    trackingsystem->eyex.eyegrey.get(), 
                    histPositionSegmentedPixels->operator[](getPointNo()));

            // ONUR DUPLICATED CODE
            /*
            trackingsystem->gazetracker.
            addExemplar_left(points[id], averageeye_left->getMean().get(),
                    trackingsystem->eyex.eyegrey_left.get(), 
                    vectorOfVectors_horizontal_left->operator[](getPointNo()), vectorOfVectors_vertical_left->operator[](getPointNo()));
            */

            trackingsystem->gazetracker.
            addExemplar_left(points[id], averageeye_left->getMean().get(),
                    trackingsystem->eyex.eyegrey_left.get(), 
                    histPositionSegmentedPixels_left->operator[](getPointNo()));

            cout << "Puntos: " << points[getPointNo()].x << ", " << points[getPointNo()].y << endl;

            //trackingsystem->gazetracker.regression.CalculateMedian(points[id], vectorOfVectors_horizontal_left->operator[](getPointNo()), vectorOfVectors_vertical_left->operator[](getPointNo()));
            //trackingsystem->gazetracker.regression.CalculateStandardDeviation(vectorOfVectors_horizontal_left->operator[](getPointNo()), vectorOfVectors_vertical_left->operator[](getPointNo()));


            //trackingsystem->gazetracker.regression.AddSample(points[id], vectorOfVectors_horizontal_left->operator[](getPointNo()), vectorOfVectors_vertical_left->operator[](getPointNo()));          

            //trackingsystem->gazetracker.regression.CalculateRegressionTranning();

            if(id == points.size()-1) {
                tracker_status = STATUS_CALIBRATED;
                is_tracker_calibrated = true;
                
                //trackingsystem->gazetracker.trainNN();
                //trackingsystem->gazetracker.calculateTrainingErrors();
            }
        
            // If we have processed the last target
            // Calculate training error and output on screen
            //if(isLast()) {
            //  trackingsystem->gazetracker.calculateTrainingErrors();
            //}
        }
    }
    cout << "Calibrator process() end" << endl;
    MovingTarget::process();
    cout << "MovingTarget process() end" << endl;
}

const Point Calibrator::defaultpointarr[] = {Point(0.5, 0.5), 
					     Point(0.1, 0.5), Point(0.9, 0.5),
					     Point(0.5, 0.1), Point(0.5, 0.9), 
					     Point(0.1, 0.1), Point(0.1, 0.9), 
					     Point(0.9, 0.9), Point(0.9, 0.1), 
					     Point(0.3, 0.3), Point(0.3, 0.7), 
					     Point(0.7, 0.7), Point(0.7, 0.3)};

vector<Point> 
Calibrator::defaultpoints(Calibrator::defaultpointarr, 
			  Calibrator::defaultpointarr+
			  (sizeof(Calibrator::defaultpointarr) / 
			   sizeof(Calibrator::defaultpointarr[0])));

vector<Point> Calibrator::loadpoints(istream& in) {
    vector<Point> result;

    for(;;) {
	double x, y;
	in >> x >> y;
	if (in.rdstate()) break; // break if any error
	result.push_back(Point(x, y));
    }

    return result;
}

vector<Point> Calibrator::scaled(const vector<Point> &points,
				      double x, double y) 
{
//     double dx = x > y ? (x-y)/2 : 0.0;
//     double dy = y > x ? (y-x)/2 : 0.0;
//     double scale = x > y ? y : x;

    vector<Point> result;

    xforeach(iter, points)
	result.push_back(Point(iter->x * x, iter->y * y));
// 	result.push_back(Point(iter->x * scale + dx, iter->y * scale + dy));

    return result;
}

vector<Point> Calibrator::scaled(const vector<Point> &points,
                          int x, int y, double width, double height) 
{
    vector<Point> result;

    xforeach(iter, points) {
    result.push_back(Point(iter->x * width + x, iter->y * height + y));
        //cout << "ADDED POINT (" << iter->x * width + x << ", " << iter->y * height + y << ")" << endl;
    }
    return result;
}
