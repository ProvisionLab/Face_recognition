#include "plates_recognition.hpp"

int morph_size_r = 1;
int morph_size_l = 4;
int sobel_kernel = 3; // 1 - 3 - 5 - 7
cv::Size gauss_kernel = cv::Size(5,5);

Ptr<ERFilter> PlateRecognizer::er_filter1;  
Ptr<ERFilter> PlateRecognizer::er_filter2;
Ptr<OCRTesseract> PlateRecognizer::ocr;
std::unique_ptr<alpr::Alpr> PlateRecognizer::openalpr;
std::string current_res = "";

PlateRecognizer::PlateRecognizer()
{
    if ( !er_filter1 || !er_filter2 || !ocr || !openalpr)
    {
        std::cerr<<"PlateRecognizer havn't been initialized"<<std::endl;
        exit(1);
    }
}

PlateRecognizer::~PlateRecognizer()
{
   
}

void PlateRecognizer::init()
{
    if (!openalpr)
    {
        openalpr.reset(new alpr::Alpr("eu","openalpr.conf"));

        openalpr->setTopN(20);

        std::cerr << "Alpr init" << std::endl;
    }   


    if(!er_filter1 || !er_filter2)
    {
        er_filter1 = createERFilterNM1(loadClassifierNM1("./modules/text/samples/trained_classifierNM1.xml"),8,0.00015f,0.13f,0.2f,true,0.1f);
        er_filter2 = createERFilterNM2(loadClassifierNM2("./modules/text/samples/trained_classifierNM2.xml"),0.5);
        std::cerr << "Filters init" << std::endl;
    } 
    if (!ocr)
    {
        ocr = OCRTesseract::create();
        std::cerr << "Tesseract init" << std::endl;
    }

    std::cerr << "PlateRecognizer init successfull" << std::endl;
}


std::vector<std::pair<std::string, cv::Mat>> PlateRecognizer::recognize(cv::Mat const & image)
{

    image_ = image.clone();

    cv::Mat processed_image = processImage();

    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(processed_image, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    std::vector<cv::RotatedRect> rects;
    
    auto itc = contours.begin();
    while (itc != contours.end())
    {
        auto mr = cv::minAreaRect(cv::Mat(*itc));

        float area = fabs(cv::contourArea(*itc));
        float bbArea=mr.size.width * mr.size.height;
        float ratio = area/bbArea;
        float long_ratio = (float)mr.size.height / (float)mr.size.width;

        if( (ratio < 0.45) || (bbArea < 500) || long_ratio > 0.4 || bbArea > 5000  )
        {
            itc= contours.erase(itc);
        }
        else
        {
            ++itc;
            rects.push_back(mr);
            processPlate(mr);

          
#ifdef DRAW_PLATES
            cv::Rect_<double> plate_rect = mr.boundingRect();
            cv::Size_<double> deltaSize( plate_rect.width * 0.3f, plate_rect.height * 0.9f );
            cv::Point_<double> offset( deltaSize.width/2, deltaSize.height/2);
            plate_rect += deltaSize;
            plate_rect -= offset;
            cv::putText(image,std::to_string(bbArea),cv::Point(plate_rect.x,plate_rect.y+plate_rect.height/2),
                CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(0, 0 , 0),1, CV_AA);
            cv::rectangle(image,plate_rect,cv::Scalar(255,0,0),2);
#endif
        }
    }

    return results_;
}

cv::Mat PlateRecognizer::processImage()
{
    cv::Mat gray_image;
    cv::cvtColor(image_,gray_image,CV_BGR2GRAY);

    //cv::threshold(gray_image, binary_image, 170, 255, CV_THRESH_BINARY );

    cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 2*morph_size_l + 1, 2*morph_size_r+1 ),
                                                 cv::Point( morph_size_l, morph_size_r ) );

    /// Apply the specified morphology operation
    cv::Mat sobel_image;
    cv::morphologyEx( gray_image, sobel_image, 5, element, cv::Point(-1,-1),1);

    cv::Mat median_image;
    cv::medianBlur(sobel_image, median_image, 5);

    cv::Mat dilotate_image;
    cv::dilate(median_image, dilotate_image, element,cv::Point(-1,-1),1);

    cv::Mat erode_image;
    cv::erode(dilotate_image,erode_image,element,cv::Point(-1,-1),1 );

//    cv::Mat substract_image;
//    cv::subtract(erode_image, gray_image, substract_image);

    cv::Mat gaussain_image;
    cv::GaussianBlur(erode_image, gaussain_image,gauss_kernel,0);

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
    cv::imshow("median", median_image);
    cv::imshow("gaussain", gaussain_image);
    cv::imshow("closing", closing_image);
    cv::imshow("otsu", otsu_image);
    cv::waitKey(0);
    cv::destroyAllWindows();
#endif
    return otsu_image;
}

void PlateRecognizer::processPlate(cv::RotatedRect& mr)
{
    cv::Mat target(mr.size.height, mr.size.width, CV_8UC1);
    std::vector<cv::Point2f> target_points;
    target_points.push_back(cv::Point2f(0, (float)(target.rows - 1)));
    target_points.push_back(cv::Point2f(0, 0));
    target_points.push_back(cv::Point2f((float)(target.cols - 1), 0));
    target_points.push_back(cv::Point2f((float)(target.cols - 1), (float)(target.rows - 1)));

    cv::Point2f ps[4];
    mr.points(ps);
    std::vector<cv::Point2f> points;
    float offset = 20;
    for(int i = 0; i < 4; i++)
    {
        switch(i)
        {
        case 0:
            ps[i] += cv::Point2f(-offset,offset);
            break;
        case 1:
            ps[i] += cv::Point2f(-offset,-offset);
            break;
        case 2:
            ps[i] += cv::Point2f(offset,-offset);
            break;
        case 3:
            ps[i] += cv::Point2f(offset,offset);
            break;

        }

        points.push_back(ps[i]);

    }

    cv::Mat const trans_mat = cv::getPerspectiveTransform(points, target_points);
    cv::warpPerspective(image_, target, trans_mat, target.size());

    cv::Rect plate_rect = mr.boundingRect();
    cv::Mat plate = image_(plate_rect);
    
    definePlate(plate);
#ifdef DRAW_PLATES
    if(!res.empty())
    {
    cv::rectangle(image,plate_rect,cv::Scalar(255,255,0),2);
    cv::putText(image,res,cv::Point(plate_rect.x,plate_rect.y+plate_rect.height*1.5),
        CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(0, 255 , 0),1, CV_AA);
    cv::resize(image, image, cv::Size(800, 600));
    cv::imshow("image", image);
    cv::waitKey(0);
    }
#endif

       // cv::imshow("perspPlate", target);



}

void PlateRecognizer::definePlate(cv::Mat & plateImage)
{
    cv::cvtColor(plateImage,plateImage,CV_BGR2GRAY);

    alpr::AlprResults results = openalpr->recognize(plateImage.data, plateImage.elemSize(), plateImage.cols, plateImage.rows, rois);

    for (auto & plate : results.plates)
    {
        for (auto & candidate : plate.topNPlates)
        {
            std::string res = candidate.characters ;
            if ((float)candidate.overall_confidence > 89.0 && res.size() > 5)
            {
                std::cout << "    - " << res << "\t confidence: " << candidate.overall_confidence <<std::endl;
                results_.push_back(std::make_pair(res, plateImage));
            }
        }
    }

}

// Initialize the library using United States style license plates.
// You can use other countries/regions as well (for example: "eu", "au", or "kr")
// void init_alpr()
// {
//     if (!openalpr)
//     {
//         openalpr.reset(new alpr::Alpr("eu","/home/greeser/Work/toolkits/openalpr-2.3.0/config/openalpr.conf.defaults", "/home/greeser/Work/toolkits/openalpr-2.3.0/runtime_data"));

//         openalpr->setTopN(20);

//         std::cerr << "openalpr initialized" << std::endl;
//     }

// #ifdef _DEBUG
// //  alpr::AlprResults results = openalpr->recognize("./20170705_122304.jpg");
//     alpr::AlprResults results = openalpr->recognize("./image10.png");

//     std::cout << "alpr: " << results.plates.size() << " results" << std::endl;

//     for (auto & plate : results.plates)
//     {
//         std::cout << "plate: " << plate.topNPlates.size() << " results" << std::endl;

//         for (auto & candidate : plate.topNPlates)
//         {
//             std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
//             std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
//         }
//     }
// #endif
// }
