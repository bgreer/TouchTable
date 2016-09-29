#include "helper.h"

double getTime ()
{
  return ((double)getTickCount())/((double)getTickFrequency());
}

// set up camera for visible-light calibration
void camera_visible (void)
{
  int ret;
  ret = system("v4l2-ctl -c exposure_auto=0");
  ret = system("v4l2-ctl -c exposure_absolute=100");
  ret = system("v4l2-ctl -c exposure_auto_priority=0");
  ret = system("v4l2-ctl -c focus_auto=0");
  ret = system("v4l2-ctl -c focus_absolute=90");
  ret = system("v4l2-ctl -c led1_mode=0");
}

// set up camera for ir-light use
void camera_ir (void)
{
  int ret;
  ret = system("v4l2-ctl -c exposure_auto=0");
  ret = system("v4l2-ctl -c exposure_absolute=500");
  ret = system("v4l2-ctl -c exposure_auto_priority=0");
  ret = system("v4l2-ctl -c focus_auto=0");
  ret = system("v4l2-ctl -c focus_absolute=90");
  ret = system("v4l2-ctl -c led1_mode=0");
}


// because opencv resize is completely wrong
void resize_safe (Mat *in, Mat *out)
{
  int ii, ij;
  int x0, x1, y0, y1;
  float xpos, ypos, dx, dy;
  float val0, val1, val;
  Size insize, outsize;

  insize = in->size();
  outsize = out->size();

  for (ii=0; ii<outsize.width; ii++)
  {
    xpos = ((float)ii) * (insize.width-1.0) / (outsize.width-1.0);
    x0 = (int)floor(xpos);
    x1 = (int)ceil(xpos);
    dx = xpos-x0;
    for (ij=0; ij<outsize.height; ij++)
    {
      ypos = ((float)ij) * (insize.height-1.0) / (outsize.height-1.0);
      y0 = (int)floor(ypos);
      y1 = (int)ceil(ypos);
      dy = ypos-y0;

      // interpolate
      if (x0 == x1)
      {
        if (y0 == y1)
        {
          val = in->at<float>(y0,x0);
        } else {
          val0 = in->at<float>(y0,x0);
          val1 = in->at<float>(y1,x0);
          val = (1.0-dy)*val0 + dy*val1;
        }
      } else {
        if (y0 == y1)
        {
          val = (1.0-dx)*(in->at<float>(y0,x0)) + (dx)*(in->at<float>(y0,x1));
        } else {
          val0 = (1.0-dx)*(in->at<float>(y0,x0)) + (dx)*(in->at<float>(y0,x1));
          val1 = (1.0-dx)*(in->at<float>(y1,x0)) + (dx)*(in->at<float>(y1,x1));
          val = (1.0-dy)*val0 + (dy)*val1;
        }
      }

      out->at<float>(ij,ii) = val;
    }
  }
}
