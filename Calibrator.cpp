#include "Calibrator.h"

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
    vectorOfVectors_vertical(new vector<vector<int> >), 
    vectorOfVectors_horizontal_left(new vector<vector<int> >), 
    vectorOfVectors_vertical_left(new vector<vector<int> >)

{
    trackingsystem->gazetracker.clear();
    // todo: remove all calibration points
}


void Calibrator::process() {
    static int dummy = 0;
    
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

            for (int j = 0; j < vectorOfVectors_horizontal->operator[](getPointNo()).size(); j++) {

                vectorOfVectors_horizontal->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_horizontal->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
                vectorOfVectors_horizontal_left->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_horizontal_left->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
            }

            for (int j = 0; j < vectorOfVectors_vertical->operator[](getPointNo()).size(); j++) {

                vectorOfVectors_vertical->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_vertical->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
                vectorOfVectors_vertical_left->operator[](getPointNo()).operator[](j) = floor(vectorOfVectors_vertical_left->operator[](getPointNo()).operator[](j) / (sizeVectorOfVectors - getPointNo()));
            
            }

            /*
                std::vector<int> result;

                it = vectorOfVectors_horizontal->operator[](i).begin();

                std::transform(vectorOfVectors_horizontal->operator[](getPointNo()).begin(), vectorOfVectors_horizontal->operator[](getPointNo()).end(), vectorOfVectors_horizontal->operator[](i).begin(), 
                   std::back_inserter(result), std::plus<int>());

                vectorOfVectors_horizontal->operator[](getPointNo()).assign(it, vectorOfVectors_horizontal->operator[](i).end());

                vectorOfVectors_horizontal->pop_back();

            }

            */
            
/*

            for (int j = 0; j < vectorOfVectors_horizontal->operator[](0).size(); j++){
                
                cout << "vectorOfVectors_horizontal[" << j << "]: " << vectorOfVectors_horizontal->operator[](getPointNo()).operator[](j) << endl;
                cout << "vectorOfVectors_horizontal_left[" << j << "]: " << vectorOfVectors_horizontal_left->operator[](getPointNo()).operator[](j) << endl;
                
            }
            
            cin.get();

            for (int j = 0; j < vectorOfVectors_vertical->operator[](0).size(); j++){
                
                cout << "vectorOfVectors_vertical[" << j << "]: " << vectorOfVectors_vertical->operator[](getPointNo()).operator[](j) << endl;
                cout << "vectorOfVectors_vertical_left[" << j << "]: " << vectorOfVectors_vertical_left->operator[](getPointNo()).operator[](j) << endl;
                
            }
            
            cin.get();

*/


            //trackingsystem->gazetracker.regression.AddSample(points[id], vectorOfVectors_horizontal->operator[](getPointNo()), vectorOfVectors_vertical->operator[](getPointNo()));

            trackingsystem->gazetracker.
            addExemplar(points[id], averageeye->getMean().get(),
                    trackingsystem->eyex.eyegrey.get(), 
                    vectorOfVectors_horizontal->operator[](getPointNo()), vectorOfVectors_vertical->operator[](getPointNo()));


            //trackingsystem->gazetracker.regression.AddSample(points[id], vectorOfVectors_horizontal_left->operator[](getPointNo()), vectorOfVectors_vertical_left->operator[](getPointNo()));

            // ONUR DUPLICATED CODE
            trackingsystem->gazetracker.
            addExemplar_left(points[id], averageeye_left->getMean().get(),
                    trackingsystem->eyex.eyegrey_left.get(), 
                    vectorOfVectors_horizontal_left->operator[](getPointNo()), vectorOfVectors_vertical_left->operator[](getPointNo()));

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
    MovingTarget::process();
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
