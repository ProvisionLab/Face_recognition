#include "help_functions.h"

float euclidianDist(const cv::Point2f &p1, const cv::Point2f &p2)
{
    return sqrt ( pow(p1.x-p2.x,2) + pow(p1.y-p2.y,2) );
}

void faceTransformRegard(cv::Mat &face, cv::Mat &warp_dst, std::vector<cv::Point2f> &src_points,
                         cv::Rect_<double> &src_rect, cv::Mat target_mat)
{
    float h_rect = src_rect.height;
    float w_rect = src_rect.width;
    float l1 = euclidianDist(src_points[0],src_points[1]);
    float l2 = euclidianDist(src_points[2],src_points[3]);
    std::cout<<"Rect_H="<<h_rect<<" Rect_W="<<w_rect<<" Eye_D="<<l1<<" Mouth_D="<<l2<<std::endl;

    float a1 = (src_points[2].y - src_points[3].y) / (src_points[2].x - src_points[3].x);
    float b1 = src_points[3].y - src_points[3].x * a1;

    float a2 = -pow(a1,-1);
    float b2 = src_points[1].x / a1 + src_points[1].y;

    float xp = (b2 - b1) / (a1 - a2);
    float yp = a1 * xp + b1;
    cv::Point2f x_p(xp,yp);

    float l3 = euclidianDist(src_points[1],x_p);

    float ratio_l1 = l1 / w_rect;
    float ratio_l2 = l2 / w_rect;
    float ratio_l3 = l3 / h_rect;

    float res_ratio = 1;
    ratio_l3 *= res_ratio;
    std::cout<<"L1Rate="<<ratio_l1<<" L2Rate="<<ratio_l2<<" L3_Rate="<<ratio_l3<<std::endl;

    float ratio_x0 = (1 - ratio_l1) / 2;
    float ratio_x1 = 1 - ratio_x0;

    float ratio_x2 = (1-ratio_l2) / 2;
    float ratio_x3 = 1 - ratio_x2;

    float ratio_y0 = (1 - ratio_l3) / 2;
    float ratio_y1 = ratio_y0;

    float ratio_y2 = 1 - ratio_y0;
    float ratio_y3 = ratio_y2;



   // const float edge_x = 96.0;
 // const float edge_y = 112.0;
    const float edge_x =(float) target_mat.cols;
    const float edge_y =(float) target_mat.rows;
    std::cout<<edge_x<<" "<<edge_y<<std::endl;

    std::vector<cv::Point2f> target_points;
 //   cv::Mat target1(112,96, CV_32FC1);
//    cv::Mat target(224,224, CV_32FC1);

	float shift  = (float) target_mat.rows * 0.1;
    target_points.push_back(cv::Point2f(edge_x*ratio_x0, edge_y*ratio_y0+shift));
    target_points.push_back(cv::Point2f(edge_x*ratio_x1, edge_y*ratio_y1+shift));
    target_points.push_back(cv::Point2f(edge_x*ratio_x2, edge_y*ratio_y2+shift));
    target_points.push_back(cv::Point2f(edge_x*ratio_x3, edge_y*ratio_y3+shift));

    cv::Mat const trans_mat = cv::getPerspectiveTransform(src_points, target_points);
    cv::warpPerspective(face, warp_dst, trans_mat,target_mat.size());

    for(int i=0; i<4; i++)
    {
        std::cout<<"from="<<src_points[i]<<" to="<<target_points[i]<<std::endl;
     //   cv::circle(warp_dst, target_points[i], 2, cv::Scalar(0, 255, 255), -1);
    }

}

void faceTransform(cv::Mat &face, cv::Mat &warp_dst, std::vector<cv::Point2f> &src_points)
{
    cv::Mat target(112,96, CV_32FC1);
    std::vector<cv::Point2f> target_points;

    /*OLD
    target_points.push_back(cv::Point2f(50,52));
    target_points.push_back(cv::Point2f(160, 52));
    target_points.push_back(cv::Point2f(65, 175));
    target_points.push_back(cv::Point2f(147, 175));
    */

    target_points.push_back(cv::Point2f(64,85));
    target_points.push_back(cv::Point2f(146, 85));
    target_points.push_back(cv::Point2f(72, 178));
    target_points.push_back(cv::Point2f(138, 178));

    cv::Mat const trans_mat = cv::getPerspectiveTransform(src_points, target_points);
    cv::warpPerspective(face, warp_dst, trans_mat,target.size());

    for(int i=0; i<4; i++)
    {
        std::cout<<"from="<<src_points[i]<<" to="<<target_points[i]<<std::endl;
       // cv::circle(warp_dst, target_points[i], 2, cv::Scalar(0, 255, 255), -1);
    }
}
