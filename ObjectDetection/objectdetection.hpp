#include "objectdetection.h"

using namespace cv;
using namespace std;

// Create the ObjectDetection object, initializing the object and the scene images we want to use
// The string arguments are the path of the two images
ObjectDetection::ObjectDetection(String obj, String scene) {
	obj_img = imread(obj);
	resize(obj_img, obj_img, Size(80,120));
	// obj_img = ~obj_img;
	Size sz = obj_img.size();
	if (!(sz.height  > 0 || sz.width)) {
		cout << "ERROR LOADING IMAGE " << obj << "." << endl;
		exit(1);
	}
	cvtColor(obj_img, obj_img, CV_BGR2GRAY);
	double light = 0;	// sum of all pixels
	int counter = 0;	// number of pixels
	for (int i = 0; i < obj_img.cols; i++) {
		for (int j = 0; j < obj_img.rows; j++) {
			counter++;
			light += obj_img.at<uchar>(j,i);
		}
	}
	light /= counter;
	threshold(obj_img, obj_img, light, 255, THRESH_BINARY);
	imshow("obj", obj_img);
	scene_img = imread(scene);
	resize(scene_img, scene_img, Size(600, 150));
	sz = scene_img.size();
	if (!(sz.height  > 0 || sz.width)) {
		cout << "ERROR LOADING IMAGE " << scene << "." << endl;
		exit(2);
	}
	cvtColor(scene_img, scene_img, CV_BGR2GRAY);
	light = 0;	// sum of all pixels
	counter = 0;	// number of pixels
	for (int i = 0; i < scene_img.cols; i++) {
		for (int j = 0; j < scene_img.rows; j++) {
			counter++;
			light += scene_img.at<uchar>(j,i);
		}
	}
	light /= counter;
	threshold(scene_img, scene_img, light, 255, THRESH_BINARY);
	scene_img = ~scene_img;
	imshow("scene", scene_img);
	waitKey(0);
}

// Compute the keypoints and descriptors from the object and the scene image using ORB 
// Extract ORB features
void ObjectDetection::compute() {
	Ptr<ORB> orb = ORB::create(1000, 1.2f, 8, 21, 0, 2, ORB::HARRIS_SCORE, 21, 20);
	orb->detectAndCompute(obj_img, noArray(), obj_key, obj_desc);
	orb->detectAndCompute(scene_img, noArray(), scene_key, scene_desc);
}

// Compute matches between the object and scene descriptors
void ObjectDetection::match() {
	BFMatcher bfm (NORM_HAMMING, false);
	bfm.match(obj_desc, scene_desc, matches);
	cout << "MATCHES " << matches.size() << endl;
}

// Refine the matches found previously
// Mandatory method in order to use the draw() method
void ObjectDetection::refine(float ratio) {
	double min_dist = 100;
	// min distance among keypoints --> used to add a threshold to the keypoints
	for( int i = 0; i < obj_desc.rows; i++ ) { 
		double dist = matches[i].distance;
		if( dist < min_dist ) min_dist = dist;
	}

	cout << "Min dist: " << min_dist << endl << endl;

	// Vector which contains only the matches below the threshold
	vector< DMatch > good_matches;
	for( int i = 0; i < obj_desc.rows; i++ ) { 
		if( matches[i].distance < ratio*min_dist ) { 	
	 		good_matches.push_back(matches[i]); 
	 	}
	}
	matches = good_matches;

	// Localize the object inside the scene image
	for( int i = 0; i < matches.size(); i++ ) {
		// Get the keypoints from the new matches
		obj.push_back(obj_key[matches[i].queryIdx].pt);
		scene.push_back(scene_key[matches[i].trainIdx].pt);
	}
}

// Draw the bounding box of the object instance in the scene image, if the object has been found. 
// It uses cv::findHomography() to find the corners of the object in the scene and
// to draw the bounding box it uses cv::perspectiveTransform()
void ObjectDetection::draw(int show_matches) {
	// Image showing the matches
	Mat img_matches;
	if (show_matches != 0) {
		drawMatches(obj_img, obj_key, scene_img, scene_key, matches, img_matches);
	} else {
		vector<DMatch> empty;
		drawMatches(obj_img, obj_key, scene_img, scene_key, empty, img_matches);
	}

	// Define the corners of the object image
	vector<Point2f> obj_corners(4);
	obj_corners[0] = Point2f (0,0); 
	obj_corners[1] = Point2f (obj_img.cols, 0);
	obj_corners[2] = Point2f (obj_img.cols, obj_img.rows); 
	obj_corners[3] = Point2f (0, obj_img.rows);

	// Detect the corners of the object image inside the scene image
	vector<Point2f> scene_corners(4);
	Mat H = findHomography(obj, scene, CV_RANSAC);
	// cout << H << endl;
	if (! H.empty()) { 
		perspectiveTransform(obj_corners, scene_corners, H);

		// Draw the bounding box around the object image (if found) inside the scene image
		line(img_matches, scene_corners[0] + Point2f(obj_img.cols, 0), scene_corners[1] + Point2f( obj_img.cols, 0), Scalar(255,0,255), 6);
		line(img_matches, scene_corners[1] + Point2f(obj_img.cols, 0), scene_corners[2] + Point2f( obj_img.cols, 0), Scalar(255,0,255), 6);
		line(img_matches, scene_corners[2] + Point2f(obj_img.cols, 0), scene_corners[3] + Point2f( obj_img.cols, 0), Scalar(255,0,255), 6);
		line(img_matches, scene_corners[3] + Point2f(obj_img.cols, 0), scene_corners[0] + Point2f( obj_img.cols, 0), Scalar(255,0,255), 6);

		// Show the matches between the two images and the detected object (if found)
		namedWindow("OBJECT DETECTION", WINDOW_NORMAL);
		imshow("OBJECT DETECTION", img_matches);
	} else {
		cout << "Not enough points to perform a perspective transform." << endl;
	}

	waitKey(0);
} 	