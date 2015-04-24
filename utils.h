#pragma once

#define BOOST_FILESYSTEM_VERSION 3

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <gdkmm.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <fann.h>

//#define EXPERIMENT_MODE
//#define DEBUG


// Tracker status constants
#define STATUS_IDLE	1			// App is not doing anything
#define STATUS_CALIBRATED	2		// App is not doing anything but is calibrated

#define STATUS_CALIBRATING	11	// App is calibrating
#define STATUS_TESTING	12		// App is testing
#define STATUS_PAUSED	13		// App is paused

#ifndef _UTILS_H
#define _UTILS_H
extern int tracker_status;
extern bool is_tracker_calibrated;
extern int dwelltime_parameter;
extern int test_dwelltime_parameter;
extern int sleep_parameter;
extern CvRect* face_rectangle;
#endif


using namespace std;
using namespace boost;

using std::vector;

#define xforeach(iter,container) \
    for(typeof(container.begin()) iter = container.begin();	\
	iter != container.end(); iter++)

#define xforeachback(iter,container) \
    for(typeof(container.rbegin()) iter = container.rbegin();	\
	iter != container.rend(); iter++)

struct QuitNow: public std::exception
  {
  QuitNow() { }
  virtual ~QuitNow() throw() { }
  virtual const char* what() throw()
    {
    return "QuitNow: request normal termination of program.\n"
           "(You should not see this message. Please report it if you do.)";
    }
  };

template <class T>
inline T square(T a) {
    return a * a;
}

template <class T> 
ostream& operator<< (ostream& out, vector<T> const& vec) {
    out << vec.size() << endl;
    xforeach(iter, vec)
	out << *iter << endl;
    return out;
}

template <class T>
istream& operator>> (istream& in, vector<T> &vec) {
    int size;
    T element;

    vec.clear();
    in >> size;
    for(int i=0; i<size; i++) {
	in >> element;
	vec.push_back(element);
    }
    return in;
}


template <class T>
T teefunction(T source, char* prefix, char *postfix="\n") {
    cout << prefix << source << postfix;
    return source;
}

#define debugtee(x) teefunction(x, #x ": ")

template <class T>
void savevector(CvFileStorage *out, const char* name, vector<T>& vec) {
    cvStartWriteStruct(out, name, CV_NODE_SEQ);
    for(int i=0; i<vec.size(); i++) 
	vec[i].save(out);
    cvEndWriteStruct(out);
}

template <class T>
vector<T> loadvector(CvFileStorage *in, CvFileNode *node) {
    CvSeq *seq = node->data.seq;
    CvSeqReader reader;
    cvStartReadSeq(seq, &reader, 0);
    vector<T> result(seq->total);
    for(int i=0; i<seq->total; i++) {
	CvFileNode *item = (CvFileNode*) reader.ptr;
	result[i].load(in, item);
	CV_NEXT_SEQ_ELEM(seq->elem_size, reader);       
    }
    return result;
}

#include <gtkmm.h>

#include "Point.h"

template <class From, class To>
void convert(const From& from, To& to) {
    to = from;
}

template <class From, class To>
void convert(const vector<From> &from, vector<To> &to) {
    to.resize(from.size());
    for(int i=0; i< (int) from.size(); i++)
	convert(from[i], to[i]);
}

class ConstancyDetector {
    int value;
    int counter; 
    int maxcounter;
public:
    ConstancyDetector(int maxcounter) : 
	value(-1), counter(0), maxcounter(maxcounter) {}

    bool isStable(void) {
	return counter >= maxcounter;
    }

    bool isStableExactly(void) {
	return counter == maxcounter;
    }

    bool observe(int newvalue) {
	if (newvalue != value || newvalue < 0) 
	    counter = 0;
	else
	    counter++;
	value = newvalue;
	return isStable();
    }

    void reset(void) {
	counter = 0;
	value = -1;
    }
};

// #define output(X) { cout << #X " = " << X << endl; }


template <class T> 
int maxabsindex(T const& vec, int size) {
    int maxindex = 0;
    for(int i=0; i<size; i++)
	if (fabs(vec[i]) > fabs(vec[maxindex]))
	    maxindex = i;
    return maxindex;
}

namespace boost {
    template <>
	void checked_delete(IplImage *image);
}

void releaseImage(IplImage *image);
boost::shared_ptr<IplImage> createImage(const CvSize &size, int depth, int channels);
void mapToFirstMonitorCoordinates(Point monitor2point, Point& monitor1point);
void mapToVideoCoordinates(Point monitor2point, double resolution, Point& videoPoint, bool reverse_x = true);
void mapToNeuralNetworkCoordinates(Point point, Point& nnpoint);
void mapFromNeuralNetworkToScreenCoordinates(Point nnpoint, Point& point);
string getUniqueFileName(string directory, string base_file_name);

typedef boost::shared_ptr<const IplImage> SharedImage;

void normalizeGrayScaleImage(IplImage *image, double standard_mean = 127, double standard_std = 50);
void normalizeGrayScaleImage2(IplImage *image, double standard_mean = 127, double standard_std = 50);

void printMat(CvMat* mat);

void printVectorOfVectors(vector<vector<int> > v) ;