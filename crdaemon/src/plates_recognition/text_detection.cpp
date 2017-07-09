#include "text_detection.hpp"
int tess(Mat image)
{

    //cout << "IMG_W=" << image.cols << endl;
   // cout << "IMG_H=" << image.rows << endl;

    /*Text Detection*/

    // Extract channels to be processed individually
    vector<Mat> channels;

    Mat grey;
    cvtColor(image,grey,COLOR_RGB2GRAY);

    // Notice here we are only using grey channel, see textdetection.cpp for example with more channels
    channels.push_back(grey);
    channels.push_back(255-grey);

    double t_d = (double)getTickCount();
    // Create ERFilter objects with the 1st and 2nd stage default classifiers
    Ptr<ERFilter> er_filter1 = createERFilterNM1(loadClassifierNM1("/home/greeser/Work/toolkits/opencv/opencv_contrib/modules/text/samples/trained_classifierNM1.xml"),8,0.00015f,0.13f,0.2f,true,0.1f);
    Ptr<ERFilter> er_filter2 = createERFilterNM2(loadClassifierNM2("/home/greeser/Work/toolkits/opencv/opencv_contrib/modules/text/samples/trained_classifierNM2.xml"),0.5);

    vector<vector<ERStat> > regions(channels.size());
    // Apply the default cascade classifier to each independent channel (could be done in parallel)
    for (int c=0; c<(int)channels.size(); c++)
    {
        er_filter1->run(channels[c], regions[c]);
        er_filter2->run(channels[c], regions[c]);
    }
  //  cout << "TIME_REGION_DETECTION = " << ((double)getTickCount() - t_d)*1000/getTickFrequency() << endl;

    Mat out_img_decomposition= Mat::zeros(image.rows+2, image.cols+2, CV_8UC1);
    vector<Vec2i> tmp_group;
    for (int i=0; i<(int)regions.size(); i++)
    {
        for (int j=0; j<(int)regions[i].size();j++)
        {
            tmp_group.push_back(Vec2i(i,j));
        }
        Mat tmp= Mat::zeros(image.rows+2, image.cols+2, CV_8UC1);
        er_draw(channels, regions, tmp_group, tmp);
        if (i > 0)
            tmp = tmp / 2;
        out_img_decomposition = out_img_decomposition | tmp;
        tmp_group.clear();
    }

    double t_g = (double)getTickCount();
    // Detect character groups
    vector< vector<Vec2i> > nm_region_groups;
    vector<Rect> nm_boxes;
    erGrouping(image, channels, regions, nm_region_groups, nm_boxes,ERGROUPING_ORIENTATION_HORIZ);
   // cout << "TIME_GROUPING = " << ((double)getTickCount() - t_g)*1000/getTickFrequency() << endl;



//    /*Text Recognition (OCR)*/

    double t_r = (double)getTickCount();
    Ptr<OCRTesseract> ocr = OCRTesseract::create();
   // cout << "TIME_OCR_INITIALIZATION = " << ((double)getTickCount() - t_r)*1000/getTickFrequency() << endl;
    string output;

    Mat out_img;
    Mat out_img_detection;
    Mat out_img_segmentation = Mat::zeros(image.rows+2, image.cols+2, CV_8UC1);
    image.copyTo(out_img);
    image.copyTo(out_img_detection);
    float scale_img  = 600.f/image.rows;
    float scale_font = (float)(2-scale_img)/1.4f;
    vector<string> words_detection;

    t_r = (double)getTickCount();

    for (int i=0; i<(int)nm_boxes.size(); i++)
    {

        rectangle(out_img_detection, nm_boxes[i].tl(), nm_boxes[i].br(), Scalar(0,255,255), 3);

        Mat group_img = Mat::zeros(image.rows+2, image.cols+2, CV_8UC1);
        er_draw(channels, regions, nm_region_groups[i], group_img);
        Mat group_segmentation;
        group_img.copyTo(group_segmentation);
        //image(nm_boxes[i]).copyTo(group_img);
        group_img(nm_boxes[i]).copyTo(group_img);
        copyMakeBorder(group_img,group_img,15,15,15,15,BORDER_CONSTANT,Scalar(0));

        vector<Rect>   boxes;
        vector<string> words;
        vector<float>  confidences;
        ocr->run(group_img, output, &boxes, &words, &confidences, OCR_LEVEL_WORD);

        output.erase(remove(output.begin(), output.end(), '\n'), output.end());
        //cout << "OCR output = \"" << output << "\" length = " << output.size() << endl;
        if (output.size() < 3)
            continue;

        for (int j=0; j<(int)boxes.size(); j++)
        {
            boxes[j].x += nm_boxes[i].x-15;
            boxes[j].y += nm_boxes[i].y-15;

            //cout << "  word = " << words[j] << "\t confidence = " << confidences[j] << endl;
            if ((words[j].size() < 2) || (confidences[j] < 51) ||
                    ((words[j].size()==2) && (words[j][0] == words[j][1])) ||
                    ((words[j].size()< 4) && (confidences[j] < 60)) ||
                    isRepetitive(words[j]))
                continue;
            words_detection.push_back(words[j]);
            rectangle(out_img, boxes[j].tl(), boxes[j].br(), Scalar(255,0,255),3);
            Size word_size = getTextSize(words[j], FONT_HERSHEY_SIMPLEX, (double)scale_font, (int)(3*scale_font), NULL);
            rectangle(out_img, boxes[j].tl()-Point(3,word_size.height+3), boxes[j].tl()+Point(word_size.width,0), Scalar(255,0,255),1);
           // putText(out_img, words[j], boxes[j].tl()-Point(1,1), FONT_HERSHEY_SIMPLEX, scale_font, Scalar(255,255,255),(int)(3*scale_font));
            out_img_segmentation = out_img_segmentation | group_segmentation;
        }
      //  cout << "TIME_OCR = " << ((double)getTickCount() - t_r)*1000/getTickFrequency() << endl;

       // for(auto it=words.begin(); it != words.end(); it++)
           // std::cout<<*it<<std::endl;

    }




    //resize(out_img_detection,out_img_detection,Size(image.cols*scale_img,image.rows*scale_img));
    //imshow("detection", out_img_detection);
    //imwrite("detection.jpg", out_img_detection);
    //resize(out_img,out_img,Size(image.cols*scale_img,image.rows*scale_img));
    if(nm_boxes.size()>0)
    {
       // namedWindow("recognition",WINDOW_NORMAL);

       // imshow("recognition", out_img);
      //  waitKey(1);
        return 1;
    }
    //imwrite("recognition.jpg", out_img);
    //imwrite("segmentation.jpg", out_img_segmentation);
    //imwrite("decomposition.jpg", out_img_decomposition);

    return 0;
}

size_t edit_distance(const string &A, const string &B)
{
    size_t NA = A.size();
    size_t NB = B.size();

    vector< vector<size_t> > M(NA + 1, vector<size_t>(NB + 1));

    for (size_t a = 0; a <= NA; ++a)
        M[a][0] = a;

    for (size_t b = 0; b <= NB; ++b)
        M[0][b] = b;

    for (size_t a = 1; a <= NA; ++a)
        for (size_t b = 1; b <= NB; ++b)
        {
            size_t x = M[a-1][b] + 1;
            size_t y = M[a][b-1] + 1;
            size_t z = M[a-1][b-1] + (A[a-1] == B[b-1] ? 0 : 1);
            M[a][b] = min(x,y,z);
        }

    return M[A.size()][B.size()];
}

bool isRepetitive(const string &s)
{
    int count = 0;
    for (int i=0; i<(int)s.size(); i++)
    {
        if ((s[i] == 'i') ||
                (s[i] == 'l') ||
                (s[i] == 'I'))
            count++;
    }
    if (count > ((int)s.size()+1)/2)
    {
        return true;
    }
    return false;
}

void er_draw(vector<Mat> &channels, vector<vector<ERStat> > &regions, vector<Vec2i> group, Mat &segmentation)
{
    for (int r=0; r<(int)group.size(); r++)
    {
        ERStat er = regions[group[r][0]][group[r][1]];
        if (er.parent != NULL) // deprecate the root region
        {
            int newMaskVal = 255;
            int flags = 4 + (newMaskVal << 8) + FLOODFILL_FIXED_RANGE + FLOODFILL_MASK_ONLY;
            floodFill(channels[group[r][0]],segmentation,Point(er.pixel%channels[group[r][0]].cols,er.pixel/channels[group[r][0]].cols),
                    Scalar(255),0,Scalar(er.level),Scalar(0),flags);
        }
    }
}

size_t min(size_t x, size_t y, size_t z)
{
    return x < y ? min(x,z) : min(y,z);
}