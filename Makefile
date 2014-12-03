# Get UNAME to decide on which system we are building
UNAME := $(shell uname)

# Command prefix to suppress outputs of executed commands on the terminal
# remove the @ sign if you want to see the executed commands
CMD_PREFIX = @


######################################
# NOTHING TO CHANGE AFTER THIS POINT #
######################################
VERSION = eyetracker-1.0.0
CPPFLAGS = -Wall -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-write-strings -Wno-empty-body -Wno-c++11-extensions -g -O3

# Linux linker parameters and include directories
ifeq ($(UNAME), Linux)
	LINKER = -L/usr/local/lib -L/opt/local/lib -lm -ldl -lgthread-2.0  -lfann -lboost_filesystem -lboost_system -lgsl -lgslcblas
	INCLUDES = -I/usr/local/include
endif

# Mac OS X linker parameters and include directories
ifeq ($(UNAME), Darwin)
	LINKER = -L/opt/local/lib -lm -ldl -lgthread-2.0  -lfann -lboost_filesystem-mt -lboost_system-mt -lgsl -lgslcblas
	INCLUDES = -I/usr/local/include
endif

sources = opengazer.cpp Calibrator.cpp GazeTrackerGtk.cpp HeadTracker.cpp LeastSquares.cpp EyeExtractor.cpp GazeTracker.cpp MainGazeTracker.cpp OutputMethods.cpp PointTracker.cpp FaceDetector.cpp GazeArea.cpp TrackingSystem.cpp GtkStore.cpp Containers.cpp GraphicalPointer.cpp Point.cpp utils.cpp BlinkDetector.cpp FeatureDetector.cpp mir.cpp GameWindow.cpp ExtractEyeFeaturesSegmentation.cpp tester.cpp
objects = $(patsubst %.cpp,%.o,$(sources))

og_sources = opengazer.cpp Calibrator.cpp GazeTrackerGtk.cpp HeadTracker.cpp LeastSquares.cpp EyeExtractor.cpp GazeTracker.cpp MainGazeTracker.cpp OutputMethods.cpp PointTracker.cpp FaceDetector.cpp GazeArea.cpp TrackingSystem.cpp GtkStore.cpp Containers.cpp GraphicalPointer.cpp Point.cpp utils.cpp BlinkDetector.cpp FeatureDetector.cpp mir.cpp GameWindow.cpp ExtractEyeFeaturesSegmentation.cpp
og_objects = $(patsubst %.cpp,%.o,$(og_sources))

test_sources = tester.cpp Calibrator.cpp GazeTrackerGtk.cpp HeadTracker.cpp LeastSquares.cpp EyeExtractor.cpp GazeTracker.cpp MainGazeTracker.cpp OutputMethods.cpp PointTracker.cpp FaceDetector.cpp GazeArea.cpp TrackingSystem.cpp GtkStore.cpp Containers.cpp GraphicalPointer.cpp Point.cpp utils.cpp BlinkDetector.cpp FeatureDetector.cpp mir.cpp GameWindow.cpp ExtractEyeFeaturesSegmentation.cpp
test_objects = $(patsubst %.cpp,%.o,$(test_sources))

all: opengazer tester

opengazer: 	$(og_objects)
	$(CMD_PREFIX)g++ -o $@ $^ `pkg-config cairomm-1.0 opencv gtkmm-2.4 --libs`  $(LINKER) $(CPPFLAGS)

tester: 	$(test_objects)
	$(CMD_PREFIX)g++ -o $@ $^ `pkg-config cairomm-1.0 opencv gtkmm-2.4 --libs`  $(LINKER) $(CPPFLAGS)

%.o.depends: %.cpp
	$(CMD_PREFIX)g++ -MM $< > $@

%.o: %.cpp 
	$(CMD_PREFIX)g++ -c -o $@ $(INCLUDES) $<  `pkg-config cairomm-1.0 opencv gtkmm-2.4 --cflags` $(CPPFLAGS)

clean:
	$(CMD_PREFIX)rm -rf opengazer
	$(CMD_PREFIX)rm -rf tester
	$(CMD_PREFIX)rm -rf *.o
	$(CMD_PREFIX)rm -rf *.o.depends

