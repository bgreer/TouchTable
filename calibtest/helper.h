#ifndef HELPER_H
#define HELPER_H

#include "opencv2/opencv.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;

void camera_ir (void);
void camera_visible (void);
void resize_safe (Mat *in, Mat *out);


#endif
