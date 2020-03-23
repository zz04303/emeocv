/*
 * main.cpp
 *
 */

#include <ctime>                                     /* zz04303 */
#include <string>
#include <list>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <log4cpp/Category.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/Priority.hh>

#include "Config.h"
#include "Directory.h"
#include "ImageProcessor.h"
#include "KNearestOcr.h"
#include "Plausi.h"
#include "RRDatabase.h"

#include "myMosq.h"

static int delay = 1000;

#ifndef VERSION
#define VERSION "0.9.7"
#endif

static void testOcr(ImageInput* pImageInput) {
    log4cpp::Category::getRoot().info("testOcr");

    Config config;
    config.loadConfig();
    ImageProcessor proc(config);
    proc.debugWindow();
    proc.debugDigits();
    proc.debugEdges(); /* zz04303 */
    proc.debugSkew();  /* zz04303 */

    Plausi plausi;

    KNearestOcr ocr(config);
    if (! ocr.loadTrainingData()) {
        std::cout << "Failed to load OCR training data\n";
        return;
    }
    std::cout << "OCR training data loaded.\n";
    std::cout << "<q> to quit.\n";

    while (pImageInput->nextImage()) {

        config.loadConfig();

        proc.setInput(pImageInput->getImage());
        proc.process();

        std::string result = ocr.recognize(proc.getOutput());
        std::cout << result;
        if (plausi.check(result, pImageInput->getTime())) {
            std::cout << "  " << std::fixed << std::setprecision(1) << plausi.getCheckedValue() << std::endl;
        } else {
            std::cout << "  -------" << std::endl;
        }
        int key = cv::waitKey(delay) & 255;

        if (key == 'q') {
            std::cout << "Quit\n";
            break;
        }
    }
}

static void learnOcr(ImageInput* pImageInput) {
    log4cpp::Category::getRoot().info("learnOcr");

    Config config;
    config.loadConfig();
    ImageProcessor proc(config);
    proc.debugWindow();
    proc.debugEdges(); /* zz04303 */
    proc.debugSkew();  /* zz04303 */

    KNearestOcr ocr(config);
    Plausi plausi;     /* zz04303 */

    std::cout << "Entering OCR training mode!\n";
    std::cout << "<0>..<9> to answer digit, <space> to ignore digit,\n";
    std::cout << "<n> to skip to next vector of images (when available),\n";
    std::cout << "<s> to save and quit, <q> to quit without saving.\n";



//    int key = 0;
    int fileindex = 0;
    while (1) {

        if (! ocr.loadTrainingData()) {
        std::cout << "Failed to load OCR training data\n";
        break;
        }

        if (! pImageInput->nextImage()) {
        break;
        }

        proc.setInput(pImageInput->getImage());
        proc.process();
        fileindex++;

        std::string result = ocr.recognize(proc.getOutput());
        std::cout << fileindex << " digits=" << result.length() << "/" << config.getDigitNum() << " result=" << result << " ";

        if (plausi.check(result, pImageInput->getTime())) {            /* zz04303 */
            std::cout << "  " << std::fixed << std::setprecision(1) << plausi.getCheckedValue() << " " << std::endl; /* zz04303 */
            if (config.getOneFile()) {break;}                                                     /* zz04303 */
            } else {                                                   /* zz04303 */
            std::cout << "  -------" << " " << std::endl;              /* zz04303 */
            }                                                          /* zz04303 */

        if (result.find('?') != std::string::npos) {
          std::string result = ocr.recognize_learn(proc.getOutput());
          ocr.saveTrainingData();
          std::cout << "\nSaving training data\n";
        }
        if ( result != "" ) {  //zz04303 delete class/object/data o.i.d.
          ocr.~KNearestOcr();  //zz04303 delete class/object/data o.i.d.
        }
    }

}

static void adjustCamera(ImageInput* pImageInput) {
    log4cpp::Category::getRoot().info("adjustCamera");


    Config config;

    config.loadConfig();
    ImageProcessor proc(config);
    proc.debugWindow();
    proc.debugDigits();
    proc.debugEdges(); /* zz04303 */
    proc.debugSkew();  /* zz04303 */

    std::cout << "Adjust camera.\n";
    std::cout << "<r>, <p> to select raw or processed image, <s> to save config and quit, <q> to quit without saving.\n";

    bool processImage = true;
    int key = 0;
    while (pImageInput->nextImage()) {
        proc.setInput(pImageInput->getImage());
        if (processImage) {
            proc.process();
        } else {
            proc.showImage();
        }

        key = cv::waitKey(delay) & 255;

        if (key == 'q' || key == 's') {
            std::cout << "Quit\n";
            break;
        } else if (key == 'r') {
            processImage = false;
        } else if (key == 'p') {
            processImage = true;
        }
    }
    if (key != 'q') {
        std::cout << "Saving config\n";
        config.saveConfig();
    }
}

static void capture(ImageInput* pImageInput) {
    log4cpp::Category::getRoot().info("capture");

    std::cout << "Capturing images into directory.\n";
    std::cout << "<Ctrl-C> to quit.\n";

    while (pImageInput->nextImage()) {
        usleep(delay*1000L);
    }
}

static void writeData(ImageInput* pImageInput) {
    log4cpp::Category::getRoot().info("writeData");

    Config config;
    config.loadConfig();
    ImageProcessor proc(config);

    Plausi plausi;

    double SampleCount     = 0;     /* zz04303 */
    double SampleSucces    = 0;     /* zz04303 */
    double SampleRatio     = 0;     /* zz04303 */
    double SampleRAvg      = 0;     /* zz04303 */
    double SampleRAvgIdeal = 0;     /* zz04303 */
    double SampleOK        = 0;     /* zz04303 */

    RRDatabase rrd("emeter.rrd");

    struct stat st;

    KNearestOcr ocr(config);
    if (! ocr.loadTrainingData()) {
        std::cout << "Failed to load OCR training data\n";
        return;
    }
    std::cout << "OCR training data loaded.\n";
    std::cout << "<Ctrl-C> to quit.\n";

    char emeocvhost[24];                                                               /* zz04303 */
    gethostname(emeocvhost,24);                                                        /* zz04303 */
    std::string _emeocv_pid = "emeocv|" + std::to_string(getpid()) + "-" + emeocvhost; /* zz04303 */
    std::cout << "program|pid-hostname:"<< _emeocv_pid << "\n";                        /* zz04303 */
    const char* emeocv_pid = _emeocv_pid.c_str() ;                                     /* zz04303 */

    // below due to string length in config.yml issue (max 15 char) splitted parent and sub topics
    std::string _MqttTopic = config.getMqttParentTopic() + "/" + config.getMqttSubTopics(); /* zz04303 */
    const char* MqttTopic = _MqttTopic.c_str() ;                                       /* zz04303 */

    std::string _MqttHost = config.getMqttHost();                                      /* zz04303 */
    const char* MqttHost = _MqttHost.c_str() ;                                         /* zz04303 */
    myMosq *mosq;                                                                      /* zz04303 */
    mosq = new myMosq(emeocv_pid,MqttTopic, MqttHost, config.getMqttPort() );          /* zz04303 */

//    int mqtt_constat;
//    mosq.mosquitto_connack_string(mqtt_constat);
//    std::cout << "mqtt connection status:"<< mqtt_constat << "\n";

    while (pImageInput->nextImage()) {
        proc.setInput(pImageInput->getImage());
        proc.process();

        struct tm * timeinfo;                                    /* zz04303 , need #include <ctime> */
        time_t _time;                                            /* zz04303 */
        char timestr[20];                                        /* zz04303 */
        time(&_time);                                            /* zz04303 */
        timeinfo = localtime(&_time);                            /* zz04303 */
        strftime(timestr, 20, "%Y%m%d-%H%M%S ", timeinfo);       /* zz04303 */
        std::string result = ocr.recognize(proc.getOutput());    /* zz04303 */
        std::cout << timestr << result;                          /* zz04303 */

        config.loadConfig();                                     /* zz04303 */

        SampleCount++;                                           /* zz04303 */
        SampleOK = 0;                                            /* zz04303 */


        if (proc.getOutput().size() == config.getDigitNum()) {   /* zz04303 (originally 7) */
            std::string result = ocr.recognize(proc.getOutput());

            struct tm * Ctimeinfo;                                /* zz04303 */
            time_t _Ctime;                                        /* zz04303 */
            char Ctimestr[20];                                    /* zz04303 */
            _Ctime = plausi.getCheckedTime();                     /* zz04303 */
            Ctimeinfo = localtime(&_Ctime);                       /* zz04303 */
            strftime(Ctimestr, 20, "%Y%m%d-%H%M%S", Ctimeinfo);   /* zz04303 */

            if (plausi.check(result, pImageInput->getTime())) {
              SampleSucces++;
              SampleOK = 1;     /* see below for RRD write */
              std::cout << "  " << std::fixed << std::setprecision(1) << plausi.getCheckedValue() << " " << Ctimestr; /* zz04303 */
            } else {                                                   /* zz04303 */
              std::cout << "  -------" << " " << Ctimestr;             /* zz04303 */
            }                                                          /* zz04303 */
        }

        if(SampleCount > 12) {
        SampleRatio = SampleSucces * 100 / (SampleCount - 12);

        SampleRAvg -= SampleRAvg / config.getRollAvgInt();
        SampleRAvg += SampleOK / config.getRollAvgInt();

        SampleRAvgIdeal -= SampleRAvgIdeal / config.getRollAvgInt();
        SampleRAvgIdeal += 1.0 / config.getRollAvgInt();
        }

        std::cout << " " << std::setw (6) << SampleRatio << "%/"<<std::setw (6)<<int(SampleCount)<< " " <<
          std::setw (6) << SampleRAvg*100 << "%/"<< config.getRollAvgInt() << "("<<SampleRAvgIdeal*100<<"%)"<<std::endl;                                  /* zz04303 */

        std::string _SvalueOK = "";

        if ( SampleOK == 1 ) {
          int ret = rrd.update(plausi.getCheckedTime(), plausi.getCheckedValue());
          if ( ret != 0 )                                        /* zz04303 */
            {std::cout << " rrd.update error # = " << ret;}      /* zz04303 */

          _SvalueOK = "counter=" + std::to_string(plausi.getCheckedValue()) + ",";          /* zz04303 */
        }

        std::string _Svalue = "emeocv " + _SvalueOK +                                        /* zz04303 */
          "SampleRatio="     + std::to_string(SampleRatio) +                                 /* zz04303 */
          ",SampleCount="     + std::to_string(SampleCount) +                                /* zz04303 */
          ",SampleRAvg="      + std::to_string(SampleRAvg)  +                                /* zz04303 */
          ",RollAvgInt="      + std::to_string(config.getRollAvgInt()) +                     /* zz04303 */
          ",SampleRAvgIdeal=" + std::to_string(SampleRAvgIdeal);                             /* zz04303 */
        const char* Svalue = _Svalue.c_str() ;                                               /* zz04303 */
          if (!mosq->send_message(Svalue))       /* issue mqtt publish */                    /* zz04303 */
            {std::cout << " mqtt: not delivered?\n";}          /* when not succesful */      /* zz04303 */


        if (0 == stat("imgdebug", &st) && S_ISDIR(st.st_mode)) {
            // write debug image
            pImageInput->setOutputDir("imgdebug");
            pImageInput->saveImage("");
            pImageInput->setOutputDir("");
        }
        if (0 == stat("imgsingle", &st) && S_ISDIR(st.st_mode)) {
            // write debug image
            pImageInput->setOutputDir("imgsingle");
            pImageInput->saveImage("NoDate");
            pImageInput->setOutputDir("");
        }
        usleep(delay*1000L);
    }

  mosq->disconnect(); /* zz04303 */
}

static void usage(const char* progname) {
    std::cout << "Program to read and recognize the counter of an electricity meter with OpenCV.\n";
    std::cout << "Version: " << VERSION << std::endl;
    std::cout << "Usage: " << progname << " [-i <dir>|-c <cam>] [-l|-t|-a|-w|-o <dir>] [-s <delay>] [-v <level>\n";
    std::cout << "\nImage input:\n";
    std::cout << "  -i <image directory> : read image files (png) from directory.\n";
    std::cout << "  -c <camera number> : read images from camera.\n";
    std::cout << "\nOperation:\n";
    std::cout << "  -a : adjust camera.\n";
    std::cout << "  -o <directory> : capture images into directory.\n";
    std::cout << "  -l : learn OCR.\n";
    std::cout << "  -t : test OCR.\n";
    std::cout << "  -w : write OCR data to RR database. This is the normal working mode.\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -s <n> : Sleep n milliseconds after processing of each image (default=1000).\n";
    std::cout << "  -v <l> : Log level. One of DEBUG, INFO, ERROR (default).\n";
}

static void configureLogging(const std::string & priority = "INFO", bool toConsole = false) {
    log4cpp::Appender *fileAppender = new log4cpp::FileAppender("default", "emeocv.log");
    log4cpp::PatternLayout* layout = new log4cpp::PatternLayout();
    layout->setConversionPattern("%d{%d.%m.%Y %H:%M:%S} %p: %m%n");
    fileAppender->setLayout(layout);
    log4cpp::Category& root = log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::getPriorityValue(priority));
    root.addAppender(fileAppender);
    if (toConsole) {
        log4cpp::Appender *consoleAppender = new log4cpp::OstreamAppender("console", &std::cout);
        consoleAppender->setLayout(new log4cpp::SimpleLayout());
        root.addAppender(consoleAppender);
    }
}

int main(int argc, char **argv) {
    int opt;
    ImageInput* pImageInput = 0;
    int inputCount = 0;
    std::string outputDir;
    std::string logLevel = "ERROR";
    char cmd = 0;
    int cmdCount = 0;

    while ((opt = getopt(argc, argv, "i:c:ltaws:o:v:h")) != -1) {
        switch (opt) {
            case 'i':
                pImageInput = new DirectoryInput(Directory(optarg, ".png"));
                inputCount++;
                break;
            case 'c':
                pImageInput = new CameraInput(atoi(optarg));
                inputCount++;
                break;
            case 'l':
            case 't':
            case 'a':
            case 'w':
                cmd = opt;
                cmdCount++;
                break;
            case 'o':
                cmd = opt;
                cmdCount++;
                outputDir = optarg;
                break;
            case 's':
                delay = atoi(optarg);
                break;
            case 'v':
                logLevel = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }
    if (inputCount != 1) {
        std::cerr << "*** You should specify exactly one camera or input directory!\n\n";
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (cmdCount != 1) {
        std::cerr << "*** You should specify exactly one operation!\n\n";
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    configureLogging(logLevel, cmd == 'a');

    switch (cmd) {
        case 'o':
            pImageInput->setOutputDir(outputDir);
            capture(pImageInput);
            break;
        case 'l':
            learnOcr(pImageInput);
            break;
        case 't':
            testOcr(pImageInput);
            break;
        case 'a':
            adjustCamera(pImageInput);
            break;
        case 'w':
            writeData(pImageInput);
            break;
    }

    delete pImageInput;
    exit(EXIT_SUCCESS);
}



