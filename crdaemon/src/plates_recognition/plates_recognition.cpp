#include "plates_recognition.hpp"
enum mode { PICTURE, CAMERA };
int morph_size_r = 1;
int morph_size_l = 4;
int sobel_kernel = 3; // 1 - 3 - 5 - 7
cv::Size gauss_kernel = cv::Size(5,5);
std::unique_ptr<alpr::Alpr> openalpr;

// Initialize the library using United States style license plates.
// You can use other countries/regions as well (for example: "eu", "au", or "kr")
void init_alpr()
{
    if (!openalpr)
    {
        openalpr.reset(new alpr::Alpr("eu","/home/greeser/Work/toolkits/openalpr-2.3.0/config/openalpr.conf.defaults", "/home/greeser/Work/toolkits/openalpr-2.3.0/runtime_data"));

        openalpr->setTopN(20);

        std::cerr << "openalpr initialized" << std::endl;
    }

#ifdef _DEBUG
//  alpr::AlprResults results = openalpr->recognize("./20170705_122304.jpg");
    alpr::AlprResults results = openalpr->recognize("./image10.png");

    std::cout << "alpr: " << results.plates.size() << " results" << std::endl;

    for (auto & plate : results.plates)
    {
        std::cout << "plate: " << plate.topNPlates.size() << " results" << std::endl;

        for (auto & candidate : plate.topNPlates)
        {
            std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
            std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
        }
    }
#endif
}

std::string exec(const char* cmd) {
  //std::cout<<cmd<<std::endl;
  char buffer[128];
  std::string result = "";
  FILE* pipe = popen(cmd, "r");
  if (!pipe) throw std::runtime_error("popen() failed!");
  try {
      while (!feof(pipe)) {
          if (fgets(buffer, 128, pipe) != NULL)
              result += buffer;
      }
  } catch (...) {
      pclose(pipe);
      throw;
  }
  pclose(pipe);
  return result;
}

void printPlate(cv::Mat& plate)
{
    std::string filename="/home/greeser/plate.png";
    cv::imwrite(filename, plate);
    std::string command = "alpr -c eu " + filename;
    std::string res = exec(command.c_str());
    smatch sm, smp;
    regex e("(-\\s\\S{6})");
    regex p(R"((?:^|\s)([+-]?[[:digit:]]*\.+[[:digit:]]+)(?=$|\s))");
    regex_search(res,sm,e);
    regex_search(res,smp,p);
    try {
    for(int i=1; i< sm.size(); i++)
    {
        std::string conf = smp.str(1);
        float fconf = stof(conf);
        if(fconf > 90.0)
        {
            std::string res=sm.str(1);
            std::cout<<res<<std::endl;
            
        }
    }
    }
    catch(...)
    {
        return ;
    }
}

void printPlateAlpr(cv::Mat& plate)
{
    std::string filename="/home/greeser/plate.png";
    cv::imwrite(filename, plate);
    alpr::AlprResults results = openalpr->recognize("/home/greeser/plate.png");

    std::cout << "alpr: " << results.plates.size() << " results" << std::endl;

    for (auto & plate : results.plates)
    {
        std::cout << "plate: " << plate.topNPlates.size() << " results" << std::endl;

        for (auto & candidate : plate.topNPlates)
        {
            std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
            std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
        }
    }
}

void showPlate(cv::RotatedRect& mr, cv::Mat& image)
{
    cv::Mat M, rotated, cropped;
    float angle = mr.angle;
    cv::Size rect_size = mr.size;
    if (mr.angle < -45.)
    {
                angle += 90.0;
                cv::swap(rect_size.width, rect_size.height);
    }
    M = cv::getRotationMatrix2D(mr.center, angle, 1.0);
    cv::warpAffine(image, rotated, M, image.size(), cv::INTER_CUBIC);
    getRectSubPix(rotated, rect_size, mr.center, cropped);
   // cv::imshow("plate", cropped);
   // cv::waitKey(1);
}

void cropPlate(cv::RotatedRect& mr, cv::Mat& image)
{

    cv::Mat target(mr.size.height, mr.size.width, CV_8UC1);
    //cv::Mat target(134, 94, CV_8UC3);
    std::vector<cv::Point2f> target_points;
    target_points.push_back(cv::Point2f(0, (float)(target.rows - 1)));
    target_points.push_back(cv::Point2f(0, 0));
    target_points.push_back(cv::Point2f((float)(target.cols - 1), 0));
    target_points.push_back(cv::Point2f((float)(target.cols - 1), (float)(target.rows - 1)));

    cv::Point2f ps[4];
    mr.points(ps);
    std::vector<cv::Point2f> points;

    for(int i=0; i< 4;i++)
    {
        switch(i)
        {
        case 0:
            ps[i] += cv::Point2f(-20,20);
            break;
        case 1:
            ps[i] += cv::Point2f(-20,-20);
            break;
        case 2:
            ps[i] += cv::Point2f(20,-20);
            break;
        case 3:
            ps[i] += cv::Point2f(20,20);
            break;

        }

        points.push_back(ps[i]);

         //std::cout<<ps[i]<<"=="<<target_points[i]<<std::endl;

    }

    cv::Mat const trans_mat = cv::getPerspectiveTransform(points, target_points);
    cv::warpPerspective(image, target, trans_mat, target.size());

    if(tess(target)>0)
    {
        cv::Mat plate=image(mr.boundingRect());
        printPlateAlpr(plate);
       // cv::imshow("perspPlate", target);
       // cv::waitKey(100);
    }
    else
    {
        return;
    }


}

void processImage(cv::Mat& image, cv::Mat& processed_image)
{
    cv::Mat gray_image;
    cv::cvtColor(image,gray_image,CV_BGR2GRAY);

    cv::Mat dilotate_image;
    //cv::threshold(gray_image, binary_image, 170, 255, CV_THRESH_BINARY );

    cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 2*morph_size_l + 1, 2*morph_size_r+1 ),
                                                 cv::Point( morph_size_l, morph_size_r ) );

    /// Apply the specified morphology operation
      cv::Mat sobel_image;
      cv::morphologyEx( gray_image, sobel_image, 5, element, cv::Point(-1,-1),1);
//    cv::dilate(gray_image, dilotate_image, element,cv::Point(-1,-1),1);

//    cv::Mat erode_image;
//    cv::erode(dilotate_image,erode_image,element,cv::Point(-1,-1),1 );

//    cv::Mat substract_image;
//    cv::subtract(erode_image, gray_image, substract_image);

//    cv::Mat sobel_image;
//    cv::Sobel(substract_image, sobel_image, substract_image.depth(),1,0,sobel_kernel);

    cv::Mat gaussain_image;
    cv::GaussianBlur(sobel_image, gaussain_image,gauss_kernel,0);

    cv::Mat closing_image;
    cv::morphologyEx(gaussain_image, closing_image,cv::MORPH_CLOSE, element);

    cv::Mat otsu_image;
    cv::threshold(closing_image, otsu_image, 0, 255, CV_THRESH_OTSU);

#ifdef STEPS
    cv::imshow("image", image);
    cv::imshow("gray", gray_image);
//    cv::imshow("dilotate", dilotate_image);
//    cv::imshow("erode", erode_image);
//    cv::imshow("substr", substract_image);
    cv::imshow("sobel", sobel_image);
    cv::imshow("gaussain", gaussain_image);
    cv::imshow("closing", closing_image);
    cv::imshow("otsu", otsu_image);
    cv::waitKey(0);
    cv::destroyAllWindows();
#endif
    processed_image = otsu_image;
}

std::vector<std::string> plate_recognition(cv::Mat& image)
{
    std::vector<std::string> results;
    cv::Mat otsu_image;
    processImage(image, otsu_image);
    std::cout<<" IMAGE PROCESSED"<<std::endl;
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(otsu_image, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    cv::Mat target(50, 200, CV_8UC1);
    //cv::Mat target(134, 94, CV_8UC3);
    std::vector<cv::Point2f> target_points;
    target_points.push_back(cv::Point2f(0, 0));
    target_points.push_back(cv::Point2f((float)(target.cols - 1), 0));
    target_points.push_back(cv::Point2f((float)(target.cols - 1), (float)(target.rows - 1)));
    target_points.push_back(cv::Point2f(0, (float)(target.rows - 1)));

    std::vector<cv::RotatedRect> rects;
    auto itc = contours.begin();
    while (itc != contours.end())
    {
        auto mr = cv::minAreaRect(cv::Mat(*itc));

        float area = fabs(cv::contourArea(*itc));
        float bbArea=mr.size.width * mr.size.height;
        float ratio = area/bbArea;
        float long_ratio = (float)mr.size.height / (float)mr.size.width;

        if( (ratio < 0.45) || (bbArea < 400) || long_ratio > 0.4){
            itc= contours.erase(itc);
        }else{
            ++itc;
            rects.push_back(mr);
            cv::Rect_<double> plate_rect = mr.boundingRect();
            cv::Size_<double> deltaSize( plate_rect.width * 0.3f, plate_rect.height * 0.9f );
            cv::Point_<double> offset( deltaSize.width/2, deltaSize.height/2);
            plate_rect += deltaSize;
            plate_rect -= offset;
           // cv::putText(image,std::to_std::string(plate_rect.area()),cv::Point(plate_rect.x,plate_rect.y+plate_rect.height/2),CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8,
               //         cv::Scalar(0, 0 , 0),1, CV_AA);
            cv::rectangle(image,plate_rect,cv::Scalar(255,0,0),2);
            cv::Point2f ps[4];
            mr.points(ps);

            cropPlate(mr, image);

            results.push_back("lol");

            std::cout<<" PLATE CROPPED"<<std::endl;

        }

    }
    return results;
}
