#include <thread>
#include <mutex>
#include <unistd.h>
#include "opencv2/features2d.hpp"
#include "opencv2/opencv.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;

class Capture
{
private:
  Mat curr_frame;
  std::mutex framelock;
  int width, height;
  std::thread *capture_thread;
  bool capture_running;
  void capture_loop();
public:
  Capture (int width, int height);
  ~Capture ();
  void getFrame (Mat *frame);
};



