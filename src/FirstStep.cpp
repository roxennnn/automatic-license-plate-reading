// g++ FirstStep.cpp -o FirstStep -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core

#include <iostream>
#include <fstream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace std;

// detect the license plate in the source image
void getFirstCut(Mat src, Mat &dst, RotatedRect &cropped_plate);

// if not found with the getFirstCut function, apply a different method to detect the license plate
void getAlternativeFirstCut(Mat src, Mat &dst, RotatedRect &cropped_plate);

// refine the previously found license plate, removing noise
void refineCut(Mat src, Mat &dst);

// find and save the license plate keys (in order to be used by the python script)
void findKeys(Mat src);

// crop function:
// mode 0: crop the specified area, considering the largest side as the width
// mode 1: crop the specified area, considering the largest side as the height
void crop(Mat src, Mat &crop, RotatedRect rect, int mode); 

// main function
int main(int argc, char** argv) {

	// read the image given as argument
	Mat src = imread(argv[1]);
	if (src.cols < 1) {
		cout << "No valid argument passed: please retry passing an image path." << endl;
		cout << "Exiting..." << endl;
		exit(1);
	}
	
	Mat license_plate;			// where to save the cropped license plate detected from src
	RotatedRect cropped_plate;	// where to save the rect containing the license plate detected

	// detect license plate
	getFirstCut(src, license_plate, cropped_plate);
	
	// if no license plate is found:
	if (license_plate.rows < 1) {
		cout << "No license plate found: trying alternative method:" << endl;
		getAlternativeFirstCut(src, license_plate, cropped_plate); // try again to detect the plate
		if (license_plate.rows < 1) {
			cout << "No license plate found." << endl << endl;
			cout << "Exiting..." << endl;
			exit(1);
		}
	}

	// saving the rect corners in a .txt file, in order to pass it later to the last script
	Point2f rect_points[4]; 
	cropped_plate.points( rect_points );	// get the corner points from cropped_plate
	ofstream myfile;
	myfile.open ("temp/rect.txt");
	for( int j = 0; j < 4; j++ ) {
	    myfile << rect_points[j].x << endl;	// x coord of the corner
	    myfile << rect_points[j].y << endl;	// y coord of the corner
	}
	myfile.close();

	// resizing image: licence plate has an average ratio of 4:1
	resize(license_plate, license_plate, Size(600,150));	

	// refine the license plate detected:
	Mat refined;	
	refineCut(license_plate, refined);

	if (refined.rows < 1) {
		cout << "No possible refinement." << endl;
		refined = license_plate.clone();
	}

	// find the plate keys, saving them in the folder 'keys'
	// the key images will be used in the python script
	findKeys(refined);

	return 0;
}

// UTILITY FUNCTIONS

void getFirstCut(Mat src, Mat &dst, RotatedRect &cropped_plate) {
	
	// grayscale image
	Mat gray;
	cvtColor(src, gray, CV_BGR2GRAY);

	// apply a median filter to the grayscale image
	// median filter --> preserves edges while removing noise
	Mat median;
	medianBlur(gray, median, 1);

	// threshold to have binary image
	adaptiveThreshold(median, median, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 55, 5);

	// apply open morphological operator to further remove noise
	// open --> erode followed by dilate
	Mat element = getStructuringElement( MORPH_RECT, Size(2, 2));
	morphologyEx(median, median, MORPH_OPEN, element);

	// finding contours and rectangles around it
	vector<vector<Point> > contours;	// store contours found
	vector<Vec4i> hierarchy;			// store hierarchies of contours
	findContours(median, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); 	// RETR_TREE -> retrieves all the contours and creates a full family hierarchy list
																				// CHAIN_APPROX_SIMPLE -> saving only the corners of the contours

	// min rectangles around contours
	vector<RotatedRect> minRect( contours.size() );
	for( int i = 0; i < contours.size(); i++ ) { 
	 	minRect[i] = minAreaRect( Mat(contours[i]) );
	}

	// iterate through the contours
	for( int i = 0; i < contours.size(); i++ ) {

		// this code needs to fix the angle problem of the rectangles
		float height = minRect[i].size.height;
		float width = minRect[i].size.width;
		float angle = minRect[i].angle;
		// we need rectangles with width larger than height --> searching license plates
		if (height > width) {	// switch sizes if height > width
			float temp = width;
			width = height;
			height = temp;
		} 
		float ratio = width/height; // useful to detect the right rectangle containing the license plate
		
		// the following if statement filters out lots of rectangles --> rectangles which cannot be licence plates
		if (ratio > 1.8 && ratio < 6 && height > 20 && width > 90 && height < 90) {	
   		
	   		int counter = 0;			// number of "key" rectangles found inside the current rectangle
	   		int k = hierarchy[i][2];	// searching through the children of the current rectangle
	   		if (k > 0) {				// current rectangle has children (at least 1 child)
	   			do {
	   				Rect rex = boundingRect(contours.at(k));	// rectangle bounding a contour
	   				// the following if statement filters out rectangles which cannot be plate keys
	   				if (rex.width > 10 && rex.height > 10 && (float)rex.width/rex.height < 0.75 && (float)rex.width/rex.height > 0.3) {
	   					counter++;
	   				}
	   				k = hierarchy[k][0]; // next rectangle in the same layes
	   			} while (k>0);
	   		}
	   		if (counter > 4) {	// requirement: a license plate has at least 5 plate keys
	   			cropped_plate = minRect[i];
				crop(src, dst, minRect[i], 0);
				// it is not needed to go further --> it is unlikely this is not the license plate
				return;
	   		}
	   	} 
	}
}

void getAlternativeFirstCut(Mat src, Mat &dst, RotatedRect &cropped_plate) {
	
	// grayscale image
	Mat gray;
	cvtColor(src, gray, CV_BGR2GRAY);

	// filtered grayscale image
	// using gaussian blur filter --> to reduce noise 
	Mat gaussian;
	GaussianBlur( gray, gaussian, Size(5, 5), 0);

	// filtering using sobel filter to emphasize edges
	// in this case: sobel used to detect vertical edges.
	Mat sobel;
	Sobel( gaussian, sobel, -1, 1, 0 );

	// threshold to have binary image
	threshold(sobel, sobel, 80, 255, THRESH_BINARY);

	// applying morpological operator close --> to better define the plate zone
	// close: first dilate then erode
	// useful to close small holes inside the objects
	Mat morph;
	Mat element = getStructuringElement( MORPH_RECT, Size(16, 16));
	morphologyEx(sobel, morph, MORPH_CLOSE, element);

	// finding contours and rectangles around them
	vector<vector<Point> > contours;	// store contours found
	vector<Vec4i> hierarchy;			// store hierarchies of contours
	findContours(morph, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);	// RETR_EXTERNAL ->  all child contours left behind
																					// CHAIN_APPROX_SIMPLE -> saving only the corners of the contours
	// min rectangle around contours
	vector<RotatedRect> minRect( contours.size() );	
	for( int i = 0; i < contours.size(); i++ ) { 
		minRect[i] = minAreaRect( Mat(contours[i]) );
	}

	int index = -1;				// index of the candidate rectangle
	float edge_density = 0;		// to filter out all the noise given by rectangles which are not the license plate
	Rect box(0,0,1,1);			// roi used later to crop the plate from the src; it will be updated
	
	for( int i = 0; i < contours.size(); i++ ) {	// iterate through the contours
		// this code needs to fix the angle problem of the rects
		float height = minRect[i].size.height;
		float width = minRect[i].size.width;
		// we need rectangles with width larger than height --> searching license plates
		if (height > width) {	// switch sizes if height > width
			float temp = width;
			width = height;
			height = temp;
		} 
		float ratio = width/height; // useful to detect the right rectangle containing the license plate

		Point2f center = minRect[i].center;		// center of the rectangle
		
		// the following if statement filters out lots of rectangles --> rectangles which cannot be licence plates
		if ((ratio < 1.5 || ratio > 5 || width < 30 || height < 16)) {
			// skip
	   	} else {
	   		// rotated rectangle
			Point2f rect_points[4]; 
			minRect[i].points( rect_points );
	   		// building roi to crop
	   		Rect roi;
	   		if (width < 1 || height < 1) {continue;}
	   		roi.x = minRect[i].center.x-width/2;
	   		if (roi.x < 0) { roi.x = 0; }	// check if roi is inside the image
			roi.y = minRect[i].center.y-height/2;
	   		if (roi.y < 0) { roi.y = 0; }	// check if roi is inside the image
	   		if (roi.x + width > src.cols) { roi.width = src.cols-roi.x; }	// check if roi is inside the image
	   		else { roi.width = width; }
	   		if (roi.y + height > src.rows) { roi.height = src.rows-roi.y; }	// check if roi is inside the image
	   		else { roi.height = height; }

	   		// compute the edge_density (white pixels) inside each rect which survived so far
	   		Mat crop = morph(roi);
	   		int white = 0;		// number of white pixels
	   		int count = 0;		// total number of pixels
	   		for (int k = 0; k < crop.rows; k++) {
	   			for (int m = 0; m < crop.cols; m++) {
	   				if (crop.at<uchar>(k,m) > 250) { white++; }
	   				count++;
	   			}
	   		}

	   		// edge density of the current rectangle
	   		float density = (float)white/count;	

	   		// keeping only one rect: the one with the highest density
	   		if (density > edge_density) {
	   			edge_density = density;	// max density so far
	   			index = i;				// updating the index of the candidate
	   			box = roi;				// storing the roi of the candidate
	   		} 
	  	}
	}

	// increasing the width a bit (12 pixels), in order to be sure to have the license plate
	// it will be refined later
	box.width += 12;		
	if (box.width > src.cols) { 	// check if roi is inside the image
		box.width = src.cols-box.width;
	}

	// crop the src image, based on the stored roi
	dst = src(box);
	// min rectangle containing the license plate
	cropped_plate = minRect[index];
}

void refineCut(Mat src, Mat &dst) {
	// clone the src, we do not want the source to change
	Mat plate = src.clone();
	cvtColor(plate, plate, CV_BGR2GRAY);	// plate in grayscale

	// threshold the grayscale image to get binary image
	adaptiveThreshold(plate, plate, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 55, 5);

	// getting contours of the cropped images
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	// find contours of cropped image
	findContours(plate, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);	// RETR_TREE -> retrieves all the contours and creates a full family hierarchy list
																				// CHAIN_APPROX_SIMPLE -> saving only the corners of the contours
	vector<RotatedRect> rects( contours.size() );	// rectangles around contours

	// find the rect with the biggest dimensions --> in order to crop better the license plate --> further remove noise
	double max_wid = 0;		// max width of rects
	double max_hei = 0;		// max height of rects
	int ind = 0;			// index of biggest rect
	for( int i = 0; i < contours.size(); i++ ) { 
	 	rects[i] = minAreaRect( Mat(contours[i]));		// get min area rect around contours
	 	Point2f pts[4]; 
	 	rects[i].points( pts );			// points of the rect

		// compute max width and height
		Size s = rects[i].size;
		double wid = s.width;			// current width
		double hei = s.height;			// current height
		if (hei > wid) {				// fixing the angle problem of rects --> we need width > height
			float temp = wid;
			wid = hei;
			hei = temp;
		} 
		// update the index of the biggest square
		if (wid >= max_wid && hei >= max_hei && hei > 20) {
			max_wid = wid;
			max_hei = hei;
			ind = i;
		}
	}
	
	// cropping the license plate with better precision, reducing noise
	crop(src, dst, rects[ind], 0);
	// save refined license plate image to test keypoints object detection on it
	stringstream filepath;
	filepath << "ObjectDetection/license_plate.jpg";
	string obj_path = filepath.str();
	imwrite(obj_path, dst);
}

void findKeys(Mat src) {
	// grayscale plate; src is cloned because we do not want it to change
	Mat gray_refined = src.clone();
	cvtColor(src, gray_refined, CV_BGR2GRAY);
	
	// threshold to get binary image of plate
	adaptiveThreshold(gray_refined, gray_refined, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 55, 5);

	// find contours inside the detected license plate
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	// find contours of cropped image
	findContours(gray_refined, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);	// RETR_TREE -> retrieves all the contours and creates a full family hierarchy list
																						// CHAIN_APPROX_SIMPLE -> saving only the corners of the contours
	vector<RotatedRect> digichar( contours.size() );	// all rectangles around contours
	for( int i = 0; i < contours.size(); i++ ) { 
	 	digichar[i] = minAreaRect( Mat(contours[i]));		// get min area rect around contours
	}

	vector<Mat> keys;			// license plate keys (need to be sorted)
	vector<double> x_centers;	// x coord of centers of key rectangles	(used to sort keys)
	// Showing the digichar in the plate
	for (int i = 0; i < digichar.size(); i++) {
		Point2f center = digichar[i].center;
		Size size = digichar[i].size;
		
		// in this case we want rectangles to have larger height than width
		if (size.width > size.height) {
			size = Size(size.height, size.width);
		}

		float ratio = (float)size.height/size.width;	// useful information to detect keys of the current rectangle

		// if statements to filter out rectangles that are not plate keys
		if (size.width <= 25 || size.height <= 75 || size.height > 180) {continue;}
		if (ratio < 1.25 || ratio > 4.4) {continue;}	
	
		// crop candidate key from license plate and store it
		Mat candidate;		
		crop(src, candidate, digichar[i], 1);
		keys.push_back(candidate);
		x_centers.push_back(center.x);
	}
	vector<Mat> sorted_keys;		// used to store sorted keys
	int iters = keys.size();
	// if no keys were found --> exit
	if (x_centers.size() < 1) {
		cout << "No keys found in detected license plate." << endl;
		cout << "Ending..." << endl;
		exit(1);
	}
	double prev = x_centers.at(0);	// key at previous iteration
	for (int i = 0; i < iters; i++) {
		double x = x_centers.at(0);	// always get the first one --> later keys will be removed from here when added to the sorted vector	
		int index = 0;				// used to sort keys properly
		// for loop to get next key to add in the sorted vector
		for (int j = 1; j < x_centers.size(); j++) {
			double y = x_centers.at(j);
			if (y < x) {	// keeping track of the most to the left remaining key
				x = y;
				index = j;
			}
		}
		if (x < prev+5 && i != 0) {	// this rectangle is inside the previous key --> ignored and removed
			prev = x; 
			keys.erase(keys.begin() + index); 
			x_centers.erase(x_centers.begin() + index);
		}
		else {						// add the key in the sorted list, then removed from the keys list
			Mat temp = keys.at(index);
			sorted_keys.push_back(temp);
			keys.erase(keys.begin() + index);
			x_centers.erase(x_centers.begin() + index);
			prev = x;
		}
	}

	// grayscale license plate image
	cvtColor(src, src, CV_BGR2GRAY);	
	double light = 0;	// average pixel value
	int counter = 0;	// number of pixels
	// summing up all pixel values of the grayscale license plate and then divide by the total number of pixel
	// --> to get best possibile value for thresholding
	for (int i = 0; i < src.cols; i++) {
		for (int j = 0; j < src.rows; j++) {
			counter++;
			light += src.at<uchar>(j,i);
		}
	}
	light /= counter;

	// padding values
	int top = (int) (0.3*28); 
	int bottom = (int) (0.3*28);
	int left = (int) (0.3*28); 
	int right = (int) (0.3*28);

	// processing of keys
	// thresholding, resizing and padding the keys --> to better resemble the dataset used to train the CNN
	for (int i = 0; i < sorted_keys.size(); i++) {
		Mat temp = sorted_keys.at(i);
		cvtColor(temp, temp, CV_BGR2GRAY);
		threshold(temp, temp, light, 255, THRESH_BINARY); 
		resize(temp, temp, Size(28,28));
		Mat inverted = ~temp;
		copyMakeBorder( inverted, inverted, top, bottom, left, right, BORDER_CONSTANT, 0 );
		resize(inverted, inverted, Size(28,28));

		// Mat element = getStructuringElement( MORPH_RECT, Size(2,2));
		// erode(inverted, inverted, element);

		// save in the 'keys' folder the processed key images
		stringstream filepath;
		filepath << "keys/" << i+1 << ".jpg";
		string obj_path = filepath.str();
		imwrite(obj_path, inverted);
	}
}

void crop(Mat src, Mat &crop, RotatedRect rect, int mode) {
	// get center, angle and size of rect
	Point2f center = rect.center;
	double angle = rect.angle;
	Size size = rect.size;
	if (mode == 0) {
		// we need width > height
		if (size.height > size.width) {
			angle += 90;
			size = Size(size.height, size.width);
		}
	} else if (mode == 1) {
		// we need height > width
		if (size.width > size.height) {
			angle += 90;
			size = Size(size.height, size.width);
		}
	} else {	// wrong mode
		cout << "Wrong 'mode' crop paramter" << endl;
		cout << "Exiting..." << endl;
		exit(1);
	}

	// compute rotation matrix
	Mat m = getRotationMatrix2D(center, angle, 1);
	// apply affine transformation
	warpAffine(src, crop, m, src.size());
	// retrieve rectangle from an image with sub-pixel accuracy
	getRectSubPix(crop, size, center, crop);
}