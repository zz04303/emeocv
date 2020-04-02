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

void ImageInput::saveImage(const std::string & NoDate) {
    struct tm date;
    localtime_r(&_time, &date);
    char filename[PATH_MAX];
    if (NoDate == "NoDate") {strftime(filename, PATH_MAX, "/out.png", &date);}   /* &date is not used  zz04303 */
    else                    {strftime(filename, PATH_MAX, "/%Y%m%d-%H%M%S.png", &date);}
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

//Start section for start/end time filtering  zz04303
    if (!config.getOneFile()) {

      std::string StartDateTime = config.getStartDateTime();
      std::string EndDateTime   = config.getEndDateTime();

      if (StartDateTime != "") {
        if(StartDateTime.length() != 15 ) {
          std::cout << " Length error StartDateTime : " << config.getStartDateTime() << std::endl;
          std::cout << "                              yyyymmdd-hhmmss" << std::endl;
          return false;
          }
        StartDateTime = StartDateTime.erase(8,1);
        if(!isNumeric(StartDateTime)) {
          std::cout << " Format error StartDateTime : " << config.getStartDateTime() << std::endl;
          std::cout << "                              yyyymmdd-hhmmss" << std::endl;
          return false;
          }
        }

      if (EndDateTime   != "") {
        if(EndDateTime.length() != 15 ) {
          std::cout << " Length error EndDateTime : " << config.getEndDateTime() << std::endl;
          std::cout << "                            yyyymmdd-hhmmss" << std::endl;
          return false;
          }
        EndDateTime = EndDateTime.erase(8,1);
        if(!isNumeric(EndDateTime)) {
          std::cout << " Format error EndDateTime : " << config.getEndDateTime() << std::endl;
          std::cout << "                            yyyymmdd-hhmmss" << std::endl;
          return false;
          }
        }

      std::string FilenameDateTime = *_itFilename;
      FilenameDateTime = FilenameDateTime.erase(8,1);
      FilenameDateTime = FilenameDateTime.erase(14,4);

      if ( FilenameDateTime > EndDateTime && EndDateTime != "" ) {
        std::cout << " FilenameDateTime > EndDateTime : " << *_itFilename << " > " << config.getEndDateTime() << std::endl;
        return false;
      }

      int PrintFlag = 1;
      while ( FilenameDateTime < StartDateTime && StartDateTime != "" ) {
        if (PrintFlag) {
          std::cout << " FilenameDateTime < StartDateTime : " << *_itFilename << " < " << config.getStartDateTime() << std::endl;
          PrintFlag = 0;
          }
        _itFilename++;
        if (_itFilename == _filenameList.end()) {
          std::cout << " end of file list (outside DateTime range)" << std::endl;
          return false;
          }
        FilenameDateTime = *_itFilename;
        FilenameDateTime = FilenameDateTime.erase(8,1);
        FilenameDateTime = FilenameDateTime.erase(14,4);
      }
    }
//End section for start/end time filtering     zz04303

    std::string path = _directory.fullpath(*_itFilename);

    if (!config.getOneFile()) {
      std::cout << *_itFilename << " "  << std::flush;   // flush used due to learning mode
    }

    while(1) {                                        /* zz04303 */

      /* try/catch due to sometimes exception during imread */
      try { _img = cv::imread(path.c_str()); }
      catch (...) { std::cout << "An exception occurred during cv::imread() #1 in ImageInput.cpp (zz04303)" << std::endl; }

      /* sometimes misformed image from file/webcam */
      if ((_img.rows == config.getImgH()   && _img.cols == config.getImgW() ) ||       /* normal processing    zz04303 */
          (_img.rows == config.getCoordH() && _img.cols == config.getCoordW() ) ) {    /* apparently debugging zz04303 */
          break;  /* break when OK */                 /* zz04303 */
      }                                               /* zz04303 */
      std::cout << "Unexpected: _img.rows " << _img.rows << " _img.cols " << _img.cols << std::endl;    /* zz04303 */
      std::cout << "Write image unchanged and wait 10sec" << std::endl;    /* zz04303 */
      // save copy of image if requested
      if (!_outDir.empty()) {
        saveImage("");
      }
      usleep(10000000);   /* wait in miliseconden */  /* zz04303 */
    }

    int CoordX, CoordY;
    if (_img.rows == config.getCoordH() && _img.cols == config.getCoordW() ) {   /* apparently debugging, because same size as the originally intended cut-out of the camera image zz04303 */
      CoordX = 0;
      CoordY = 0;
      }
    else {                                                                       /* normal processing    zz04303 */
      CoordX = config.getCoordX();
      CoordY = config.getCoordY();
      }

    cv::Rect myROI( CoordX, CoordY, config.getCoordW(), config.getCoordH() );    /* zz04303 */

    cv::Mat _img_crop = _img(myROI);
    _img = _img_crop;

    time(&_time);

    if (!config.getOneFile()) {

      std::string path = _directory.fullpath(*_itFilename);

      /* try/catch due to sometimes exception during imread */
      try { _img = cv::imread(path.c_str()); }
      catch (...) { std::cout << "An exception occurred during cv::imread() #2 in ImageInput.cpp (zz04303)" << std::endl; }

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
        saveImage("");
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
        saveImage("");
    }

    return success;
}

bool ImageInput::isNumeric(const std::string& input) {           /* zz04303 */
    return std::all_of(input.begin(), input.end(), ::isdigit);   /* zz04303 */
    // Using all_of (requires C++11)  https://rosettacode.org/wiki/Determine_if_a_string_is_numeric#C.2B.2B
}
