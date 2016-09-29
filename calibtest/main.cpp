#include <unistd.h>
#include <iostream>
#include <vector>
#include "opencv2/features2d.hpp"
#include "opencv2/opencv.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudawarping.hpp"
#include "opencv2/cudafilters.hpp"
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "helper.h"

#define DELAY(T) {t0 = getTime(); while (getTime() - t0 < T){cap >> img_capture;if (waitKey(1)==27) return 0;}}

#define MODE_AUTOMATIC 0
#define MODE_MANUAL 1

#define IMGW 1600
#define IMGH 896
#define NMEASX 20
#define NMEASY 16

using namespace cv;

int lastx, lasty;
double lasttime;

double getTime ()
{
  return ((double)getTickCount())/((double)getTickFrequency());
}

void mouseCallback (int event, int x, int y, int flags, void *params)
{
  if(event==CV_EVENT_LBUTTONDOWN)
  {
    lasttime = getTime();
    lastx = x;
    lasty = y;
  }
}

int main (int argc, char *argv[])
{
  int ii, ij, ind;
  double t0;
  float xval, yval;
  double min, max;
  unsigned char maxval;
  bool valid[NMEASX*NMEASY];
  int mode = MODE_AUTOMATIC;
  float xpos[NMEASX*NMEASY], ypos[NMEASX*NMEASY];
  VideoCapture cap(0);

  // remap
  Mat mapping_x_full(IMGH,IMGW,CV_32FC1);
  Mat mapping_y_full(IMGH,IMGW,CV_32FC1);
  Mat mapping_x_meas(NMEASY,NMEASX,CV_32FC1);
  Mat mapping_y_meas(NMEASY,NMEASX,CV_32FC1);

  // probing and measuring
  Mat img_capture(IMGH,IMGW,CV_8UC3);
  Mat img_probe(IMGH,IMGW,CV_8UC3);
  Mat img_background(IMGH,IMGW,CV_8UC1);
  Mat img_meas(IMGH,IMGW,CV_8UC1);
  Mat img_resid(IMGH,IMGW,CV_8UC1);
  Moments m;
  
  // remapping
  Mat img_source(IMGH,IMGW,CV_8UC3);
  Mat img_remap(IMGH,IMGW,CV_8UC3);
  FileStorage fs;


  // blob detection
  SimpleBlobDetector::Params par;
  par.filterByArea = true;
  par.minArea = 50;
  par.maxArea = 1000;
  par.filterByColor = true;
  par.blobColor = 255;
  par.filterByCircularity = false;
  par.filterByConvexity = false;
  par.filterByInertia = false;
  Ptr<SimpleBlobDetector> sbd = SimpleBlobDetector::create(par);
  std::vector<KeyPoint> keys;

  // drawing test
  Mat draw(IMGH,IMGW,CV_8UC3);
  Scalar draw_color;

  /*
  // test interpolation
  Mat mat1(3,6,CV_32FC1);
  Mat mat2(5,12,CV_32FC1);
  mat1 = 0.0;
  mat2 = 0.0;
  mat1.at<float>(0,0) = 1.0;
  mat1.at<float>(1,0) = 2.0;
  mat1.at<float>(2,0) = 3.0;
  resize_safe(&mat1, &mat2);
  std::cout << mat1 << std::endl;
  std::cout << mat2 << std::endl;
  return 0;
  */

  // Set up camera capture
  cap.set(CV_CAP_PROP_FRAME_WIDTH,IMGW);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT,IMGH);
  cap.set(CV_CAP_PROP_FPS, 30);

  camera_visible();

  // set up window
  cvNamedWindow("Name", CV_WINDOW_NORMAL);
  cvSetWindowProperty("Name", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
  setMouseCallback("Name", mouseCallback, &img_resid);

  // alignment
  while (1)
  {
    cap >> img_capture;
    imshow("Name", img_capture);
    if (waitKey(1)==27) break;
  }

  // clear screen
  img_probe = 0;
  imshow("Name", img_probe);
  t0 = getTime();
  while (getTime() - t0 < 1.0)
  {
    cap >> img_capture;
    if (waitKey(1)==27) return 0;
  }


  // Step 1: measure mapping
  for (ii=0; ii<NMEASX; ii++)
  {
    for (ij=0; ij<NMEASY; ij++)
    {

      // show blank, capture background
      img_probe = 0;
      imshow("Name", img_probe);
      t0 = getTime();
      while (getTime() - t0 < 0.3)
      {
        cap >> img_capture;
        if (waitKey(1)==27) return 0;
      }
      cvtColor(img_capture, img_capture, COLOR_BGR2GRAY);
      img_background = img_capture;



      // show dot, capture
      xval = (ii*1.0*IMGW)/(NMEASX*1.0 - 1.0) + 0.0*IMGW;
      yval = (ij*1.0*IMGH)/(NMEASY*1.0 - 1.0) + 0.0*IMGH;
      circle(img_probe, Point(xval,yval), 20, Scalar(255,255,255), -1);
      imshow("Name", img_probe);
      t0 = getTime();
      while (getTime() - t0 < 0.3)
      {
        cap >> img_capture;
        if (waitKey(1)==27) return 0;
      }
      cvtColor(img_capture, img_capture, COLOR_BGR2GRAY);
      img_meas = img_capture;



      // subtract and analyze
      img_resid = img_meas - img_background;
      if (mode == MODE_AUTOMATIC)
      {
        minMaxLoc(img_resid, &min, &max);
        maxval = (unsigned char) max;
        threshold(img_resid,img_resid, maxval-15,255,3);


        m = moments(img_resid);
        xpos[ii*NMEASY+ij] = m.m10/m.m00;
        ypos[ii*NMEASY+ij] = m.m01/m.m00;
        if (m.m00 > 0)
          valid[ii*NMEASY+ij] = true;
        else
          valid[ii*NMEASY+ij] = false;
      } else {
        imshow("Name", img_resid);
        lastx = -1;
        lasty = -1;
        while (lastx == -1)
        {
          cap >> img_capture;
          if (waitKey(1)==27) return 0;
        }
        xpos[ii*NMEASY+ij] = lastx;
        ypos[ii*NMEASY+ij] = lasty;
      }
      std::cout << ii << "," << ij << " - " << xval << "," << yval << 
        " - " << m.m00 << " - " << xpos[ii*NMEASY+ij] << ", " << ypos[ii*NMEASY+ij] << std::endl;

    }
  }

  // post-process 


  for (ii=0; ii<NMEASX; ii++)
  {
    for (ij=0; ij<NMEASY; ij++)
    {
      mapping_x_meas.at<float>(ij,ii) = xpos[ii*NMEASY+ij];
      mapping_y_meas.at<float>(ij,ii) = ypos[ii*NMEASY+ij];
    }
  }
  

  // Step 2: scale measurements to full mapping arrays
  resize_safe(&mapping_x_meas, &mapping_x_full);
  resize_safe(&mapping_y_meas, &mapping_y_full);
  
  // save file
  fs.open("remap", cv::FileStorage::WRITE);
  fs << "MapX" << mapping_x_full;
  fs << "MapY" << mapping_y_full;
  fs.release();
  //FileStorage filey("remap_y", FileStorage::WRITE);
  //filey << mapping_y_full;


  while (1)
  {
    cap >> img_source;
    remap(img_source, img_remap, mapping_x_full, mapping_y_full, INTER_LINEAR);
    imshow("Name", img_remap);
    if (waitKey(1) == 27) return 0; // esc
  }
  
  return 0;
}
