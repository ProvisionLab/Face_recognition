// This file includes functions dealing with bounding box in image processing.
//
// Code exploited by Feng Wang <feng.wff@gmail.com>
//
// This Source Code Form is subject to the terms of the BSD lisence.
//
// calcRect, modernPosit are copied from AFLW toolkit, which is available for non-commercial research purposes only.
#ifndef BOUNDING_BOX_INC_H
#define BOUNDING_BOX_INC_H

#pragma once
#include <opencv2/opencv.hpp>


using namespace cv;
typedef Rect_<double> Rect2d;
static Scalar DEFAULT_SCALAR;
#undef assert
#define assert(_Expression) if(!(_Expression)) printf("error: %s %d, %s\n", __FILE__, __LINE__, #_Expression)
namespace FaceInception {

  struct FaceAndPoints {
    Mat image;
    std::vector<Point2d> points;
  };

  static std::vector<cv::Point3f> worldPts;

  inline double IoU(Rect2d rect1, Rect2d rect2) {
    double left = std::max<double>(rect1.x, rect2.x);
    double top = std::max<double>(rect1.y, rect2.y);
    double right = std::min<double>(rect1.x + rect1.width, rect2.x + rect2.width);
    double bottom = std::min<double>(rect1.y + rect1.height, rect2.y + rect2.height);
    double overlap = max<double>(right - left, 0) * max<double>(bottom - top, 0);
    return overlap / (rect1.width * rect1.height + rect2.width * rect2.height - overlap);
  }

  inline Rect2d BoundingBoxRegressionTarget(Rect2d data_rect, Rect2d ground_truth) {
    return Rect2d((ground_truth.x - data_rect.x) / data_rect.width,
                  (ground_truth.y - data_rect.y) / data_rect.height,
                  log(ground_truth.width / data_rect.width),
                  log(ground_truth.height / data_rect.height));
  }

  void modernPosit(cv::Mat &rot, cv::Point3f &trans, std::vector<cv::Point2d> imagePts, std::vector<cv::Point3f> worldPts, double focalLength, cv::Point2d center = cv::Point2d(0.0, 0.0), int maxIterations = 100);

  cv::Point2d project(cv::Point3f pt, double focalLength, cv::Point2d imgCenter);

  cv::Point3f transform(cv::Point3f pt, cv::Mat rot, cv::Point3f trans);

  bool calcCenterScaleAndUp(cv::Mat faceData, std::vector<cv::Point2d> imagePts, double normEyeDist, cv::Point2d &center, double &scale, cv::Point2d &upv);

  Rect2d calcRect(cv::Mat faceData, std::vector<cv::Point2d> imagePts);

  bool strict_weak_ordering(const std::pair<Rect2d, float> a, const std::pair<Rect2d, float> b);

  void NMS(std::vector<std::pair<Rect2d, float>>& rects, double nms_threshold);

  enum IoU_TYPE {
    IoU_UNION,
    IoU_MIN,
    IoU_MAX
  };

  std::vector<int> nms_max(std::vector<std::pair<Rect2d, float>>& rects, double overlap, IoU_TYPE type = IoU_UNION);

  std::vector<int> nms_avg(std::vector<std::pair<Rect2d, float>>& rects, double overlap);

  inline bool checkRect(Rect2d rect, Size image_size) {
    if (rect.x < 0 || rect.y < 0 || rect.width <= 0 || rect.height <= 0 ||
        (rect.x + rect.width) > (image_size.width - 1) || (rect.y + rect.height) > (image_size.height - 1))
      return false;
    else return true;
  }
  inline bool fixRect(Rect2d& rect, Size image_size, bool only_center = false) {
    if (rect.width <= 0 || rect.height <= 0) return false;
    if (only_center) {
      Point2d center = Point2d(rect.x + rect.width / 2, rect.y + rect.height / 2);
      center.x = max<double>(center.x, 0.0);
      center.y = max<double>(center.y, 0.0);
      center.x = min<double>(center.x, image_size.width - 1);
      center.y = min<double>(center.y, image_size.height - 1);
      rect.x = center.x - rect.width / 2;
      rect.y = center.y - rect.height / 2;
    }
    else {
      rect.x = max<double>(rect.x, 0.0);
      rect.y = max<double>(rect.y, 0.0);
      rect.width = min<double>(rect.width, image_size.width - 1 - rect.x);
      rect.height = min<double>(rect.height, image_size.height - 1 - rect.y);
    }
    return true;
  }

  inline void make_rect_square(Rect2d& rect) {
    double max_len = max(rect.width, rect.height);
    rect.x += rect.width / 2 - max_len / 2;
    rect.y += rect.height / 2 - max_len / 2;
    rect.width = rect.height = max_len;
  }

  double IoU(Rect2d rect, RotatedRect ellip, Size image_size = Size(0,0));

  /** @brief Applies an affine transformation to an image.

  The function warpAffine transforms the source image using the specified matrix:

  \f[\texttt{dst} (x,y) =  \texttt{src} ( \texttt{M} _{11} x +  \texttt{M} _{12} y +  \texttt{M} _{13}, \texttt{M} _{21} x +  \texttt{M} _{22} y +  \texttt{M} _{23})\f]

  when the flag WARP_INVERSE_MAP is set. Otherwise, the transformation is first inverted
  with cv::invertAffineTransform and then put in the formula above instead of M. The function cannot
  operate in-place.

  @param src input image.
  @param dst output image that has the size dsize and the same type as src .
  @param M \f$2\times 3\f$ transformation matrix.
  @param dsize size of the output image.
  @param flags combination of interpolation methods (see cv::InterpolationFlags) and the optional
  flag WARP_INVERSE_MAP that means that M is the inverse transformation (
  \f$\texttt{dst}\rightarrow\texttt{src}\f$ ).
  @param borderMode pixel extrapolation method (see cv::BorderTypes); when
  borderMode=BORDER_TRANSPARENT, it means that the pixels in the destination image corresponding to
  the "outliers" in the source image are not modified by the function.
  @param borderValue value used in case of a constant border; by default, it is 0.

  @sa  warpPerspective, resize, remap, getRectSubPix, transform
  */
  Mat cropImage(Mat& input_image, Rect2d roi, Size2d target_size, int flags = 1, int borderMode = 0, Scalar& borderValue = DEFAULT_SCALAR);

  //Only support 2 scaling
  Mat getPyramidStitchingImage2(Mat& input_image, std::vector<std::pair<Rect, double>>& location_and_scale, double scaling = 0.707,
                                Scalar background_color = Scalar(0,0,0), int min_side = 12, int interval = 2);

}

#endif //BOUNDING_BOX_INC_H
