#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/box.h>

#include "GazeTracker.h"
#include "utils.h"
#include "GraphicalPointer.h"

class ImageArea: public Gtk::DrawingArea {
    //friend class GameWindowText;
 public:

    ImageArea();
    virtual ~ImageArea();
	void showImage(IplImage *image); 
};

class GameWindowText: public Gtk::Window {
 protected:
  //Member widgets:
    //Gtk::Button calibratebutton, loadbutton, savebutton, clearbutton, choosebutton;
    Gtk::VBox vbox2;
    //Gtk::HBox buttonbar;

 public:
	//scoped_ptr<IplImage> canvas;
    ImageArea picture2;
	GameWindowText();
	void showImage(IplImage *image);
};

class GameArea: public Gtk::DrawingArea {
    friend class GameWindow;
 public:
	TrackerOutput* output;
	IplImage* current;
	WindowPointer* calibrationPointer;
	IplImage* repositioningImage;
	IplImage* canvas;
	GameWindowText* secondWindow;

    GameArea(TrackerOutput* op);
    virtual ~GameArea();
	void showContents();
	void calculateNewFrogPosition();
    void clearLastUpdatedRegion();
	void displayImageCentered(IplImage* image); 
	
 protected:
    IplImage* orig_image;
	IplImage* background;
	IplImage* interface;
	IplImage* canvas_resized;
	IplImage* picture_resized;
	IplImage* text_resized;
	IplImage* interface2;
	IplImage* picture_resized2;
	IplImage* text_resized2;
	IplImage* frog;
	IplImage* target;
	//IplImage* black;
	IplImage* frog_mask;
	IplImage* gaussian_mask;
	IplImage* clearing_image;
	int frog_x;
	int frog_y;
	int frog_counter;
	int game_area_x;
	int game_area_y;
	int game_area_width;
	int game_area_height;
	long start_time;
	long future_time;
	long temp_time;
	CvScalar background_color;
    bool is_window_initialized;

    CvRect* last_updated_region;
	
    virtual bool on_expose_event(GdkEventExpose *event);
    virtual bool on_button_press_event(GdkEventButton *event);
	virtual bool on_button_release_event(GdkEventButton *event);
    bool on_idle();
};


class GameWindow: public Gtk::Window {
 protected:
  //Member widgets:
    //Gtk::Button calibratebutton, loadbutton, savebutton, clearbutton, choosebutton;
    Gtk::VBox vbox;
    //Gtk::HBox buttonbar;

 public:
	int gray_level;
	//scoped_ptr<IplImage> canvas;
    GameArea picture;
	GameWindow(TrackerOutput* op);
    virtual ~GameWindow();
	IplImage* get_current();
	void setCalibrationPointer(WindowPointer* pointer);
	void setRepositioningImage(IplImage* image);
    void changeWindowColor(double illuminationLevel);
};

