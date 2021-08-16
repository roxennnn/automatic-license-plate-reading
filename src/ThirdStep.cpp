// g++ ThirdStep.cpp -o ThirdStep -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core

#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <fstream>

using namespace cv;
using namespace std;

int main(int argc, char** argv) {
	// read points saved previously inside the rect.txt file
	Point2f rect_points[4];
	string val;
	ifstream myfile ("temp/rect.txt");
	int i = 0;
	if (myfile.is_open()) {
		while ( getline (myfile,val) ) {
		  // cout << val << endl;
		  float x = atof(val.c_str());
		  getline(myfile, val);
		  float y = atof(val.c_str());
		  Point2f pt (x,y);
		  rect_points[i++] = pt;
		}
		myfile.close();
		remove("temp/rect.txt");	// delete rect.txt
	} else {
		exit(1);
	}

	// read passed argument
	Mat src = imread(argv[1]);
	if (src.cols < 1) {			// this should not be needed because already checked in FirstStep
		cout << "No valid argument passed: please retry passing an image path." << endl;
		cout << "Exiting..." << endl;
		exit(1);
	}

	// detect the bottom left point of the rectangle which detect the license plate
	// and draw the rectangle on the source image
	double x = rect_points[0].x;
	double y = rect_points[0].y;
	for (int i = 0; i < 4; i++) {
		line( src, rect_points[i], rect_points[(i+1)%4], Scalar(0,255,0), 3, 8 );
		if (rect_points[i].y > y) {		// find the most bottom point
			y = rect_points[i].y;
		}
		if (rect_points[i].x < x) {		// find the most left point
			x = rect_points[i].x;
		}
	}

	// read the predicted license plate from .txt file
	ifstream license ("temp/license_read.txt");
	if (license.is_open()) {
		while ( getline (license,val) ) { }
		license.close();
		remove("temp/license_read.txt");	// delete license_read.txt
	} else {
		cout << "Unable to open file" << endl; 
	}

	// write down the predicted license plate read from .txt file
	putText(src, val, cvPoint(x,y+30), 4, 1, cvScalar(0,255,0), 1, CV_AA);

	// plot the result: src image with a rectangle around the detected license plate and the prediction of the license plate
	imshow("RESULT", src);
	waitKey(0);

	return 0;
}