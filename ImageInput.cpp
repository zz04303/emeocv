/*
 * ImageInput.cpp
 *
 */

#include <ctime>
#include <string>
#include <list>
#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>

#include "ImageInput.h"

#include "unistd.h"     /* zz04303 usleep */

#include "Config.h"     /* zz04303 */

ImageInput::~ImageInput() {
}

cv::Mat& ImageInput::getImage() {
    return _img;
}

time_t ImageInput::getTime() {
    return _time;
}

void ImageInput::setOutputDir(const std::string & outDir) {
    _outDir = outDir;
}

void ImageInput::saveImage() {
    struct tm date;
    localtime_r(&_time, &date);
    char filename[PATH_MAX];
    strftime(filename, PATH_MAX, "/%Y%m%d-%H%M%S.png", &date);
    std::string path = _outDir + filename;
    if (cv::imwrite(path, _img)) {
        log4cpp::Category::getRoot() << log4cpp::Priority::INFO << "Image saved to " + path;
    }
}

DirectoryInput::DirectoryInput(const Directory& directory) :
        _directory(directory) {
    _filenameList = _directory.list();
    _filenameList.sort();
    _itFilename = _filenameList.begin();
}

bool DirectoryInput::nextImage() {

//  ophalen config start
    Config config;
    config.loadConfig();
//  ophalen config: einde

    if (_itFilename == _filenameList.end()) {
      std::cout << " end of file list" << std::endl;
      return false;
    }

    std::string path = _directory.fullpath(*_itFilename);

    if (!config.getOneFile()) {
      std::cout << *_itFilename << " "  << std::flush;   // flush used due to learning mode
    }

    while(1) {                                        /* zz04303 */
    /* sometimes misformed image from file/webcam */
      _img = cv::imread(path.c_str());
      if (_img.rows == config.getImgH() && _img.cols == config.getImgW() ) {    /* zz04303 */
          break;                                      /* zz04303 */
      }                                               /* zz04303 */
      std::cout << "Unexpected: _img.rows " << _img.rows << " _img.cols " << _img.cols << std::endl;    /* zz04303 */
      std::cout << "Write image unchanged and wait 10sec" << std::endl;    /* zz04303 */
      // save copy of image if requested
      if (!_outDir.empty()) {
        saveImage();
      }
      usleep(10000000);   /* wait in miliseconden */  /* zz04303 */
    }

    cv::Rect myROI(config.getCoordX(), config.getCoordY(), config.getCoordW(), config.getCoordH() );  /* zz04303 */

    cv::Mat _img_crop = _img(myROI);
    _img = _img_crop;

    time(&_time);

    if (!config.getOneFile()) {

      std::string path = _directory.fullpath(*_itFilename);

      _img = cv::imread(path.c_str());

      // read time from file name
      struct tm date;
      memset(&date, 0, sizeof(date));
      date.tm_year = atoi(_itFilename->substr(0, 4).c_str()) - 1900;
      date.tm_mon = atoi(_itFilename->substr(4, 2).c_str()) - 1;
      date.tm_mday = atoi(_itFilename->substr(6, 2).c_str());
      date.tm_hour = atoi(_itFilename->substr(9, 2).c_str());
      date.tm_min = atoi(_itFilename->substr(11, 2).c_str());
      date.tm_sec = atoi(_itFilename->substr(13, 2).c_str());
      _time = mktime(&date);
    }


    log4cpp::Category::getRoot() << log4cpp::Priority::INFO << "Processing " << *_itFilename << " of " << ctime(&_time);

    // save copy of image if requested
    if (!_outDir.empty()) {
        saveImage();
    }

    if (!config.getOneFile()) {
      _itFilename++;
    }
    return true;
}

CameraInput::CameraInput(int device) {
    _capture.open(device);
}

bool CameraInput::nextImage() {
    time(&_time);
    // read image from camera
    bool success = _capture.read(_img);

    log4cpp::Category::getRoot() << log4cpp::Priority::INFO << "Image captured: " << success;

    // save copy of image if requested
    if (success && !_outDir.empty()) {
        saveImage();
    }

    return success;
}


