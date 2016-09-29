
#include <iostream>
#include <vector>
#include <unistd.h>
#include "opencv2/features2d.hpp"
#include "opencv2/opencv.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "helper.h"

class TrackerObject
{
friend class Tracker;
private:
public:
  float xpos, ypos;
  float xvel, yvel;
  double t_start, t_lastupdate;
  bool updated;
  TrackerObject (float x, float y)
  {
    xpos = x; ypos = y;
    xvel = 0.0; yvel = 0.0;
    t_start = getTime();
    t_lastupdate = t_start;
    updated = false;
  }
  Point_<float> getPt (void)
  {
    return Point_<float> (xpos, ypos);
  }
};

class Tracker
{
private:

  float obj_timeout = 0.15; // in sec, time since last update when obj is deleted
  float obj_max_dr = pow(40,2.0); // in px^2, max distance to look for position match
  float obj_mintime = 0.25; // in sec, min time an object must persist to count
  std::vector<TrackerObject*> objs;
  std::vector<float> newx, newy;
public:
  Tracker (void);
  ~Tracker (void);
  float distanceBetween (TrackerObject *t1, TrackerObject *t2);
  void update (std::vector<KeyPoint> *keys);
  std::vector<TrackerObject*> *getPoints (void);
};
