/*
 * Config.h
 *
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>

class Config {
public:
    Config();
    void saveConfig();
    void loadConfig();

    int getDigitMaxHeight() const {
        return _digitMaxHeight;
    }

    int getDigitMinHeight() const {
        return _digitMinHeight;
    }

    int getDigitYAlignment() const {
        return _digitYAlignment;
    }

    std::string getTrainingDataFilename() const {
        return _trainingDataFilename;
    }

    float getOcrMaxDist() const {
        return _ocrMaxDist;
    }

    int getRotationDegrees() const {
        return _rotationDegrees;
    }

    int getCannyThreshold1() const {
        return _cannyThreshold1;
    }

    int getCannyThreshold2() const {
        return _cannyThreshold2;
    }

    int getDigitNum() const {
        return _digitNum;
    }

    int getDivisionNum() const {
        return _divisionNum;
    }

    int getCoordX() const {
        return _coordX;
    }

    int getCoordY() const {
        return _coordY;
    }

    int getCoordW() const {
        return _coordW;
    }

    int getCoordH() const {
        return _coordH;
    }

    int getImgW() const {
        return _imgW;
    }

    int getImgH() const {
        return _imgH;
    }
    int getOneFile() const {
        return _oneFile;
    }
    float getNegativeNum() const {
        return _negativeNum;
    }
     int getRollAvgInt() const {
        return _rollAvgInt;
    }
      std::string getMqttHost() const {
        return _mqttHost;
    }
// below due to string length in config.yml issue (max 15 char) splitted parent and sub topics
      std::string getMqttParentTopic() const {
        return _mqttParentTopic;
    }
      std::string getMqttSubTopics() const {
        return _mqttSubTopics;
    }
      int getMqttPort() const {
        return _mqttPort;
    }
      std::string getStartDateTime() const {
        return _StartDateTime;
    }
      std::string getEndDateTime() const {
        return _EndDateTime;
    }
private:
    int _rotationDegrees;
    float _ocrMaxDist;
    int _digitMinHeight;
    int _digitMaxHeight;
    int _digitYAlignment;
    int _cannyThreshold1;
    int _cannyThreshold2;
    std::string _trainingDataFilename;
    int _digitNum;
    int _divisionNum;
    int _coordX;
    int _coordY;
    int _coordW;
    int _coordH;
    int _imgW;
    int _imgH;
    int _oneFile;
    float _negativeNum;
    int _rollAvgInt;
    std::string _mqttHost;
    std::string _mqttParentTopic;
    std::string _mqttSubTopics;
    int _mqttPort;
    std::string _StartDateTime;
    std::string _EndDateTime;
};

#endif /* CONFIG_H_ */
