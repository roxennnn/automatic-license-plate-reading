#ifndef OBJECT_DETECTION_H
#define OBJECT_DETECTION_H

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/ccalib.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>

class ObjectDetection {
	public:
		// Constructor
		// Create the ObjectDetection object, initializing the object and the scene images we want to use
		// The string arguments are the path of the two images
		ObjectDetection(cv::String obj, cv::String scene);

		// Compute the keypoints and descriptors from the object and the scene image using ORB 
		// Extract ORB features
		void compute();

		// Compute matches between the object and scene descriptors
		void match();

		// Refine the matches found previously
		// Mandatory method in order to use the draw() method
		void refine(float ratio);

		// Draw the bounding box of the object instance in the scene image, if the object has been found. 
		// It uses cv::findHomography() to find the corners of the object in the scene and
		// to draw the bounding box it uses cv::perspectiveTransform()
		// 0 not to show the matches, any other number otherwise
		void draw(int show_matches);

	private:
		// object image and its keypoints, descriptor
		cv::Mat obj_img;
		std::vector<cv::KeyPoint> obj_key;
		cv::Mat obj_desc;

		// scene image
		cv::Mat scene_img;
		std::vector<cv::KeyPoint> scene_key;
		cv::Mat scene_desc;

		// matches computed between the two images
		std::vector<cv::DMatch> matches;

		// points used to draw the bounding box  
		std::vector<cv::Point2f> obj;
		std::vector<cv::Point2f> scene;
};

#endif // OBJECT_DETECTION_H