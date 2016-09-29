#include "capture.h"
#include "helper.h"

Capture::Capture (int w, int h)
{
  width = w;
  height = h;

  framelock.lock();
  curr_frame = Mat(height,width,CV_8UC3);
  framelock.unlock();

  capture_running = true;
  capture_thread = new std::thread(&Capture::capture_loop, this);
}

Capture::~Capture (void)
{
  capture_running = false;
  capture_thread->join();
}


void Capture::capture_loop (void)
{
  Mat frame_cap(height,width,CV_8UC3);
  VideoCapture cap(0);
  usleep(1000);
  // initialize camera capture
  cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
  usleep(1000);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT,height);
  usleep(1000);
  cap.set(CV_CAP_PROP_FPS, 30);
  usleep(1000);
  camera_ir();

  while (capture_running)
  {
    // capture frame
    cap >> frame_cap;
    cvtColor( frame_cap, frame_cap, CV_BGR2GRAY );
    // copy to global
    framelock.lock();
    curr_frame = frame_cap;
    framelock.unlock();
  }
}

void Capture::getFrame (Mat *img)
{
  framelock.lock();
  (*img) = curr_frame;
  framelock.unlock();
}

