/*
 * battery.cpp
 *
 *  Created on: Apr 4, 2017
 *      Author: Ryan Owens
 */

#include "wallaby/aruco.hpp"
/*
 * Aruco
 *
 * Class for detecting Aruco markers and getting Pose Estimation
 *
 */
Aruco::Aruco(int dictionaryId) {
  std::string calib_file = this->calibrationFile;
  this->dictionaryId = dictionaryId;
  // if dictioniary ID < 0 use custom dictionary file
  if (dictionaryId < 0)
    this->getCustomDictionary();
  else
    this->dictionary = cv::aruco::getPredefinedDictionary(
        cv::aruco::PREDEFINED_DICTIONARY_NAME(this->dictionaryId));
  this->detectorParams = cv::aruco::DetectorParameters::create();
  this->detectorParams->doCornerRefinement = true;
  if (access(this->newCalibrationFile.c_str(), F_OK) != -1)
    calib_file = this->newCalibrationFile;
  this->readCameraCalibration(calib_file);
}

/*
 * Set Camera Calibration
 *
 * Updates the Camera Distortion Matrixes from the file
 *
 */
bool Aruco::setCameraCalibration(std::string filename) {
  if (access(filename.c_str(), F_OK) == -1)
    return false;
  return this->readCameraCalibration(filename);
}

/*
 * Read Camera Calibration
 *
 * Reads in the Camera Callibration for use with Pose Estimation
 *
 */
bool Aruco::readCameraCalibration(std::string filename) {
  cv::FileStorage fs(filename, cv::FileStorage::READ);
  if (!fs.isOpened())
    return false;
  fs["camera_matrix"] >> this->cameraMatrix;
  fs["distortion_coefficients"] >> this->distortionCoefficients;
  fs.release();
  return true;
}

/*
 * Get Custom Dictionary
 *
 * Sets the Dictionary to a custom dictionary
 *
 */
bool Aruco::getCustomDictionary() {
  int markerSize, maxCorrectionBits;
  cv::Mat bytesList;
  cv::FileStorage fs(this->customDictionaryFile, cv::FileStorage::READ);
  if (!fs.isOpened())
    return false;
  fs["MarkerSize"] >> markerSize;
  fs["MaxCorrectionBits"] >> maxCorrectionBits;
  fs["BytesList"] >> bytesList;
  this->dictionary =
      new cv::aruco::Dictionary(bytesList, markerSize, maxCorrectionBits);
  return true;
}

/*
 * Vector Contains
 *
 * Helper function to check if a particular integer Vector Contains
 * a value
 *
 */
bool Aruco::vectorContains(std::vector<int> vec, int val) {
  if (find(vec.begin(), vec.end(), val) != vec.end())
    return true;
  return false;
}

/*
 * Get Pose
 *
 * Get the X, Y, and Z Rotation and Translation vectors for a
 * particular Aruco Marker in the View
 *
 * returns all zeros when not in View
 * returns as TX TY TZ RX RY RZ
 *
 */
std::vector<double> Aruco::getPose(int arucoId) {
  std::vector<double> rottransvec;
  rottransvec.assign(6, 0.0);
  // TODO setup using Libwallaby Camera
  // if(!this->isCameraOpen()) return rottransvec;

  // TODO setup using Libwallaby Camera
  // cv::Mat img = this->getFrame();
  cv::Mat img;
  std::vector<int> ids;
  std::vector<std::vector<cv::Point2f>> corners, rejected;
  std::vector<cv::Vec3d> rvecs, tvecs;

  // detect markers and estimate pose
  cv::aruco::detectMarkers(img, this->dictionary, corners, ids, detectorParams,
                           rejected);
  if (ids.size() > 0 && this->vectorContains(ids, arucoId)) {
    cv::aruco::estimatePoseSingleMarkers(corners, this->arucoSquareSize,
                                         cameraMatrix, distortionCoefficients,
                                         rvecs, tvecs);

    size_t index = find(ids.begin(), ids.end(), arucoId) - ids.begin();
    cv::Vec3d translation = tvecs[index];
    cv::Vec3d rotation = rvecs[index];
    for (size_t i = 0; i < translation.rows; i++) {
      rottransvec.push_back(translation[i]);
    }

    for (size_t i = 0; i < rotation.rows; i++) {
      rottransvec.push_back(rotation[i]);
    }
  }
  return rottransvec;
}

/*
 * Markers in View
 *
 * Get an array (vector) of all Aruco Markers in the current view
 *
 */
std::vector<int> Aruco::arucoMarkersInView() {
  std::vector<int> ids;
  std::vector<std::vector<cv::Point2f>> corners, rejected;
  // TODO Setup using Libwallaby Camera
  // if(!this->isCameraOpen()) return ids;
  // TODO Setup using Libwallaby Camera
  // Mat img = this->getFrame();
  cv::Mat img;
  cv::aruco::detectMarkers(img, this->dictionary, corners, ids, detectorParams,
                           rejected);
  return ids;
}

/*
 * Marker In View
 *
 * Check if a particular Aruco Marker is in the current view
 *
 */
bool Aruco::arucoMarkerInView(int arucoId) {
  std::vector<int> ids;
  std::vector<std::vector<cv::Point2f>> corners, rejected;
  // TODO Setup using Libwallaby Camera
  // if(!this->isCameraOpen()) return false;
  // TODO Setup using Libwallaby Camera
  // Mat img = this->getFrame();
  cv::Mat img;
  cv::aruco::detectMarkers(img, this->dictionary, corners, ids, detectorParams,
                           rejected);
  if (find(ids.begin(), ids.end(), arucoId) != ids.end())
    return true;
  return false;
}

/*
 * Calculate Chess Board Position
 *
 * Determines the chess board Position
 *
 */
void Aruco::calculateChessBoardPosition(std::vector<cv::Point3f> &corners) {
  for (size_t i = 0; i < (size_t)this->chessBoardDimensions.height; i++) {
    for (size_t j = 0; j < (size_t)this->chessBoardDimensions.width; j++) {
      // X = j * square size, Y = i * square size, Z = 0.0
      corners.push_back(cv::Point3f(j * this->chessBoardSquareSize,
                                    i * this->chessBoardSquareSize, 0.0f));
    }
  }
}

/*
 * Calculate Chess Board Corners
 *
 * Determines all the chess board corner positions
 *
 */
void Aruco::calculateChessBoardCornersFromImages(
    std::vector<std::vector<cv::Point2f>> &foundCorners) {
  for (std::vector<cv::Mat>::iterator iter = this->images.begin();
       iter != this->images.end(); iter++) {
    std::vector<cv::Point2f> pointBuf;
    bool chessBoardFound = findChessboardCorners(
        *iter, this->chessBoardDimensions, pointBuf, this->chessBoardFlags);
    if (chessBoardFound) {
      foundCorners.push_back(pointBuf);
    }
  }
}

/*
 * Get Images From Camera
 *
 * Grab images from a camera and store for calibration
 *
 */

void Aruco::getImagesFromCamera() {
  cv::Mat frame, drawToFrame, oefficients,
      cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
  std::vector<cv::Mat> savedImages;
  std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCorners;

  // TODO setup using Libwallaby Camera
  // if (!this->inputVideo.isOpened()) {
  //   return;
  // }
  usleep(5 * 1000000);
  while (true) {
    // TODO Setup using LibWallaby Camera
    // if (!this->inputVideo.read(frame))
    //   break;

    std::vector<cv::Vec2f> foundPoints;
    bool found = false;
    found = findChessboardCorners(frame, this->chessBoardDimensions,
                                  foundPoints, this->chessBoardFlags);

    if (this->images.size() >= this->numImagesForCalibration)
      return;
    if (found) {
      cv::Mat temp;
      frame.copyTo(temp);
      this->images.push_back(temp);
      // TODO Display to User to positon for next Frame...
      usleep(5 * 1000000);
    } else {
      // TODO Display to User to reposition camera...
    }
  }
}

/*
 * Get Images From Camera
 *
 * Grab images from a camera and store for calibration
 *
 */
bool Aruco::calibrate() {
  this->getImagesFromCamera();
  std::vector<std::vector<cv::Point2f>> imageSpacePoints;
  this->calculateChessBoardCornersFromImages(imageSpacePoints);
  std::vector<std::vector<cv::Point3f>> worldCornerPoints(1);
  this->calculateChessBoardPosition(worldCornerPoints[0]);
  worldCornerPoints.resize(imageSpacePoints.size(), worldCornerPoints[0]);

  std::vector<cv::Mat> rVecs, tVecs;
  this->distortionCoefficients = cv::Mat::zeros(8, 1, CV_64F);
  // Performs the Calibration
  calibrateCamera(worldCornerPoints, imageSpacePoints,
                  this->chessBoardDimensions, this->cameraMatrix,
                  this->distortionCoefficients, rVecs, tVecs);
  if (this->saveCalibration())
    return true;
  else
    return false;
}

bool Aruco::saveCalibration() {
  cv::FileStorage fs(this->newCalibrationFile, cv::FileStorage::WRITE);
  if (!fs.isOpened())
    return false;
  time_t tm;
  time(&tm);
  struct tm *t2 = localtime(&tm);
  char buf[1024];
  strftime(buf, sizeof(buf), "%c", t2);

  fs << "calibration_time" << buf;
  fs << "camera_matrix" << this->cameraMatrix;
  fs << "distortion_coefficients" << this->distortionCoefficients;
  fs.release();
  this->setCameraCalibration(this->newCalibrationFile);
  return true;
}
