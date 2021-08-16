# Project on Automatic License Plate Reading
## Computer Vision course ch1 2018/19 UniPD

#### What you can find inside:
* **AutomaticLicensePlateReading.sh**
	This shell script calls the C++ and Python scripts to perform Automatic License Plate Reading. It requires one argument, which is the path to a car image you want to use to detect and read its license plate. It calls, in order: *FirstStep*, *SecondStep* and *ThirdStep*.
* **Folders**
	* **src:**
	Inside the this folder you can find the source codes *FirstStep.cpp*, *SecondStep.py* and *ThirdStep.cpp*, which are the ones used to detect and read license plate from an image. Furthermore, you can find a Jupyter notebook: *NoLowerCase.ipynb* - used to train the Convolutional Neural Network used to read the license plate keys.
	* **cars:**
	Sample images of cars are present in this folder, to test the script.
	* **models:**
	CNN models and various weights for this CNNs are stored in this folder. These are used by *NoLowerCase.py*.
	* **keys:**
	Temporary images of plate keys are saved inside this folder by *FirstStep.cpp* and then used by *SecondStep.py* to read key by key.
	* **temp:**
	Inside this folder are saved - temporary - *rect.txt*, in which are written the coordinates of the rectangle surrounding the detected license plate, and *license_read.txt*, in which is written the license plate read.
	* **ObjectDetection:**
	In this folder are present the C++ codes to perform object detection, using keypoints and descriptors, to test this technique to detect license plate keys.
* **EMNIST_datasets.zip**
	Inside this zip there are some EMNIST datasets, which differ in the number of sammples and classes. I downloaded these datasets from [here](https://www.kaggle.com/crawford/emnist).

#### How to perform Automatic License Plate Reading

1. Compile the C++ codes:
```
g++ src/FirstStep.cpp -o FirstStep -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core
```
```
g++ src/ThirdStep.cpp -o ThirdStep -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core
```

2. Execute the shell script:
```
./AutomaticLicensePlateReading.sh cars/x.jpg
```
where 'x' is the number which identifies the car image.

#### How to train the CNN
1. Open *JupyterLab*
2. Just run the whole script, selecting which dataset to use.

Note: the notebook can be modified in order to preprocess the dataset in a different way and/or to change the CNN.

#### How to perform Object Detection using keypoints and descriptors
1. Compile the C++ codes:
```
g++ ObjectDetection/keypointsDetection.cpp ObjectDetection/objectdetection.hpp -o kd -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_features2d 
```
```
g++ src/FirstStep.cpp -o FirstStep -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core
```
2. Run *FirstStep*, in order to save cropped license plate image and plate keys:
```
./FirstStep cars/x.jpg
```
where 'x' is the number which identifies the car image.
3. Run the *ObjectDetection* script:
```
./kd
```