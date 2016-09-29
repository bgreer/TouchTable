#include <iostream>
#include <vector>
#include <unistd.h>
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
#include "capture.h"
#include "tracker.h"

#define IMGW 1600
#define IMGH 896

using namespace cv;



int main (int argc, char *argv[])
{
  Mat remapx, remapy; // for remapping
  Mat frame_gray(IMGH,IMGW,CV_8UC1); // gray image
  Mat frame_remap(IMGH,IMGW,CV_8UC1); // gray, remapped
  Mat canvas(IMGH,IMGW,CV_8UC3); // for drawing
  Mat frame_output(IMGH,IMGW,CV_8UC1); // download from gpu
  FileStorage fs;
  Capture cap(IMGW,IMGH);
  Tracker t;
  std::vector<TrackerObject*> *touch_pts;
  std::string *idstr;
  float eps = 3e-3;
  long index = 0;
  double t_lastframe, t_thisframe, t0, t1;
  double avgfps, min, max;
  cuda::GpuMat gpu_input(IMGH,IMGW,CV_8U);
  cuda::GpuMat gpu_raw_small(IMGH/4,IMGW/4,CV_8U);
  cuda::GpuMat gpu_blur_small(IMGH/4,IMGW/4,CV_8U);
  cuda::GpuMat gpu_raw(IMGH,IMGW,CV_8U);
  cuda::GpuMat gpu_remapped(IMGH,IMGW,CV_8U);
  cuda::GpuMat gpu_blur(IMGH,IMGW,CV_8U);
  cuda::GpuMat gpu_unsharp(IMGH,IMGW,CV_8U);
  cuda::GpuMat gpu_binary(IMGH,IMGW,CV_8U);
  cuda::GpuMat gpu_frame_back(IMGH,IMGW,CV_8UC1); // background image
  cuda::GpuMat gpu_remapx(IMGH,IMGW,CV_32FC1), gpu_remapy(IMGH,IMGW,CV_32FC1); // remapping
  cuda::Stream stream;

  // blob detector
  SimpleBlobDetector::Params par;
  par.filterByArea = true;
  par.minArea = 40;
  par.maxArea = 1000;
  par.filterByColor = true;
  par.blobColor = 255;
  par.filterByCircularity = false;
  par.filterByConvexity = false;
  par.filterByInertia = false;
  Ptr<SimpleBlobDetector> sbd = SimpleBlobDetector::create(par);
  std::vector<KeyPoint> keys;

  // input param
  if (argc < 2)
  {
    std::cout << "supply input param!" << std::endl;
    return -1;
  }
  int displaymode = atoi(argv[1]);

  // create window
  cvNamedWindow("Name", CV_WINDOW_NORMAL);
  cvSetWindowProperty("Name", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
  canvas = Scalar(0,0,0);
  imshow("Name", canvas);

  // read remapping from file
  fs.open("remap", cv::FileStorage::READ);
  fs["MapX"] >> remapx;
  fs["MapY"] >> remapy;
  fs.release();


  // create gaussian blur filter
  Ptr<cuda::Filter> filter = 
    cuda::createGaussianFilter(gpu_raw_small.type(), gpu_blur_small.type(), Size(17,17), 5);


  // upload remapping to gpu
  gpu_remapx.upload(remapx);
  gpu_remapy.upload(remapy);


  // initial background capture
  t0 = getTime();
  while (getTime() - t0 < 1.0)
  {
    cap.getFrame(&frame_gray);
    usleep(10000);
  }
  gpu_frame_back.upload(frame_gray);


  
  std::cout << "Running.." << std::endl;
  t_lastframe = getTime();
  avgfps = 0.0;
  while (1)
  {
    
    // upload to gpu
    gpu_input.upload(frame_gray, stream);
    // accumulate background image
    cuda::addWeighted(gpu_frame_back, (1.0-eps), gpu_input, eps, 0.0, gpu_frame_back, -1, stream);
    // subtract background
    cuda::subtract(gpu_frame_back, gpu_input, gpu_raw, noArray(), -1, stream);

    // remap
    cuda::remap(gpu_raw, gpu_remapped, gpu_remapx, gpu_remapy, INTER_LINEAR, 
        BORDER_CONSTANT, Scalar(0), stream);
    // blur
    cuda::resize(gpu_remapped, gpu_raw_small, Size(IMGW/4,IMGH/4), 0, 0, INTER_LINEAR, stream);
    filter->apply(gpu_raw_small, gpu_blur_small, stream);
    cuda::resize(gpu_blur_small, gpu_blur, Size(IMGW,IMGH), 0, 0, INTER_LINEAR, stream);
    // subtract
    cuda::subtract(gpu_remapped, gpu_blur, gpu_unsharp, noArray(), -1, stream);
    // threshold
    cuda::threshold(gpu_unsharp, gpu_binary, 10, 255, THRESH_BINARY, stream);
    // download from gpu
    switch (displaymode)
    {
      case 0:
        gpu_input.download(frame_output, stream);
        break;
      case 1:
        gpu_frame_back.download(frame_output, stream);
        break;
      case 2:
        gpu_raw.download(frame_output, stream);
        break;
      case 3:
        gpu_remapped.download(frame_output, stream);
        break;
      case 4:
        gpu_blur.download(frame_output, stream);
        break;
      case 5:
        gpu_unsharp.download(frame_output, stream);
        break;
      case 6:
        gpu_binary.download(frame_output, stream);
        break;
      case 7:
        gpu_binary.download(frame_output, stream);
        break;
      case 8:
        gpu_binary.download(frame_output, stream);
        break;
    }


    // grab next frame from camera and make gray
    cap.getFrame(&frame_gray);

    stream.waitForCompletion();
   
    // blob detect
    sbd->detect(frame_output, keys);
    t.update(&keys);

    // draw blobs
    if (displaymode == 7)
    {
      touch_pts = t.getPoints();
      for (int ii=0; ii<touch_pts->size(); ii++)
      {
        circle(frame_output, (*touch_pts)[ii]->getPt(), 30, Scalar(255), 2);
        idstr = new std::string("ID: " + std::to_string(ii));
        putText(frame_output, idstr->c_str(), (*touch_pts)[ii]->getPt() + Point_<float>(30,-30), 
            FONT_HERSHEY_PLAIN, 2.0, Scalar(255));
        delete idstr;
      }
      delete touch_pts;
    } else if (displaymode == 8) {
      touch_pts = t.getPoints();
      for (int ii=0; ii<touch_pts->size(); ii++)
        circle(canvas, (*touch_pts)[ii]->getPt(), 15, 
            Scalar(127*(sin((*touch_pts)[ii]->t_start*10)+0.5)+50,
              127*(sin((*touch_pts)[ii]->t_start*10+1.05)+0.5)+50,
              127*(sin((*touch_pts)[ii]->t_start*10+2.1)+0.5)+50), -1);
      frame_output = canvas;
    }

    
    // display
    imshow("Name", frame_output);
    t_thisframe = getTime();
    avgfps += (1./(t_thisframe-t_lastframe));
    t_lastframe = t_thisframe;
    if (index % 30 == 0 && index > 0)
    {
      //std::cout << "fps: " << avgfps/30. << std::endl;
      avgfps = 0.0;
    }
    if (waitKey(1) == 27) break; // esc
    index ++;
  }

  
  return 0;
}
