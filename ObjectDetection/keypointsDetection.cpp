// g++ keypointsDetection.cpp objectdetection.hpp -o kd -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_features2d 

#include "objectdetection.hpp"

int main() {	
	// defining vectors used for storing the paths of the objects and scenes images
	vector<string> objs;
	vector<string> scenes;
	scenes.push_back("ObjectDetection/license_plate.jpg");

	// building object and scene paths
	for (int i = 0; i < 5; i++) {
		stringstream obj_filepath;
		obj_filepath << "keys/" << i+1 << ".jpg";
		string obj_path = obj_filepath.str();
		objs.push_back(obj_path);
	}

	int obj_max = 7;
	int scene_max = 1;
	
	// object detection for each pair (object, scene) inside the dataset(t+1)
	for (int i = 0; i < obj_max; i++) {
		for (int j = 0; j < scene_max; j++) {
			cout << "Obj: " << i+1 << "; Scene: " << j+1 << ";" << endl;
			// create the ObjectDetection class
			ObjectDetection detector (objs[i], scenes[j]);
			// compute the keypoints and the descriptors of the object and scene images
			detector.compute();
			// compute the matches between the two images
			detector.match();
			// refine the matches between the two images
			detector.refine(4);
			
			// detect the object image inside the scene image and draw a pink line around (square)
			detector.draw(1); // 0 not to show the matches, any other number otherwise
		}
	}

	return 0;
}