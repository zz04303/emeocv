/*
 * Plausi.cpp
 *
 */

#include <string>
#include <deque>
#include <utility>
#include <ctime>
#include <cstdlib>

#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>

#include "Plausi.h"
#include "Config.h"   /* zz04303 */

Plausi::Plausi(double maxPower, int window) :
        _maxPower(maxPower), _window(window), _value(-1.), _time(0) {
}



bool Plausi::check(const std::string& value, time_t time) {
    log4cpp::Category& rlog = log4cpp::Category::getRoot();
    if (rlog.isInfoEnabled()) {
        rlog.info("Plausi check: %s of %s", value.c_str(), ctime(&time));
    }

    //  get from config: start zz04303 */
    Config config;
    config.loadConfig();
    //  get from config: end   zz04303 */

    if (value.length() != config.getDigitNum() ) {                       /*zz04303 */
        // exact num of digits                                           /*zz04303 */
        rlog.info("Plausi rejected: no match config.getDigitNum() ");    /*zz04303 */
        return false;
    }
    if (value.find_first_of('?') != std::string::npos) {
        // no '?' char
        rlog.info("Plausi rejected: no '?' char");
        return false;
    }

    double dval = atof(value.c_str() ) / config.getDivisionNum() ;    /* zz04303 divide by n */
    _queue.push_back(std::make_pair(time, dval));

    if (_queue.size() < _window) {
        rlog.info("Plausi rejected: not enough values: %d", _queue.size());
        return false;
    }
    if (_queue.size() > _window) {
        _queue.pop_front();
    }

    // iterate through queue and check that all values are ascending   /* zz04303 take into account solar panel decrease */
    // and consumption of energy is less than limit
    std::deque<std::pair<time_t, double> >::const_iterator it = _queue.begin();
    time = it->first;
    dval = it->second;
    double solarpanel = config.getNegativeNum();    /* zz04303 */                                                               /* zz04303 */
    ++it;
    while (it != _queue.end()) {
        if (it->second + solarpanel < dval) {                                                  /* zz04303 */
            // value must be >= previous value
            rlog.info("Plausi rejected #1.0: value must be >= previous value");                 /* zz04303 */
            rlog.info("Plausi rejected #1.1: if (it->second + solarpanel < dval)");             /* zz04303 */
            rlog.info("Plausi rejected #1.2: %.3f %.3f %.3f",it->second, solarpanel, dval);     /* zz04303 */
            rlog.info("Plausi rejected #1.3: Delta value %.3f",it->second - dval);              /* zz04303 */
             return false;
        }
        double power = (it->second - dval) / (it->first - time) * 3600.;
        if (power < 0.) { power = power * -1.0; }                                               /* zz04303 */
        if (power > _maxPower) {
            // consumption of energy must not be greater than limit

            // prepare for output format                             /* zz04303 */

            struct tm * TEMPtimeinfo;                                /* zz04303 */
            time_t _TEMPtime;                                        /* zz04303 */

            char itfirststr[20];                                     /* zz04303 */
            _TEMPtime = it->first;                                   /* zz04303 */
            TEMPtimeinfo = localtime(&_TEMPtime);                    /* zz04303 */
            strftime(itfirststr, 20, "%H:%M:%S", TEMPtimeinfo);      /* zz04303 */

            char timestr[20];                                        /* zz04303 */
            _TEMPtime = time;                                        /* zz04303 */
            TEMPtimeinfo = localtime(&_TEMPtime);                    /* zz04303 */
            strftime(timestr, 20, "%H:%M:%S", TEMPtimeinfo);         /* zz04303 */

            // finish prepare for output format                      /* zz04303 */

            rlog.info("Plausi rejected #2.0: consumption of energy %.3f must not be greater than limit %.3f", power, _maxPower);    /* zz04303 */
            rlog.info("Plausi rejected #2.1: IF ((it->second - dval) / (it->first - time) * 3600.) > _maxPower");                   /* zz04303 */
            rlog.info("Plausi rejected #2.2: %.3f %.3f %s %s %.3f", it->second, dval, itfirststr, timestr, _maxPower);              /* zz04303 */
            rlog.info("Plausi rejected #2.3: Delta value %.3f, Delta time %i seconds", it->second - dval, it->first - time);        /* zz04303 */
            return false;
        }
        time = it->first;
        dval = it->second;
        ++it;
    }

    // values in queue are ok: use the center value as candidate, but test again with latest checked value
    if (rlog.isDebugEnabled()) {
        rlog.debug(queueAsString());
    }
    time_t candTime = _queue.at(_window/2).first;
    double candValue = _queue.at(_window/2).second;
    if (candValue + solarpanel < _value) {                                                   /* zz04303 */
        rlog.info("Plausi rejected #3.0: value must be >= previous checked value");          /* zz04303 */
        rlog.info("Plausi rejected #3.1: if (candValue + solarpanel < _value)");             /* zz04303 */
        rlog.info("Plausi rejected #3.2: %.3f %.3f %.3f",candValue, solarpanel, _value);     /* zz04303 */
        rlog.info("Plausi rejected #3.3: Delta value %.3f",candValue - _value);              /* zz04303 */
        return false;
    }
    double power = (candValue - _value) / (candTime - _time) * 3600.;
    if (power < 0.) { power = power * -1.0; }                                                /* zz04303 */
    if (power > _maxPower) {

        // prepare for output format                             /* zz04303 */

        struct tm * TEMPtimeinfo;                                /* zz04303 */
        time_t _TEMPtime;                                        /* zz04303 */

        char candTimestr[20];                                    /* zz04303 */
        _TEMPtime = candTime;                                    /* zz04303 */
        TEMPtimeinfo = localtime(&_TEMPtime);                    /* zz04303 */
        strftime(candTimestr, 20, "%H:%M:%S", TEMPtimeinfo);     /* zz04303 */

        char _timestr[20];                                       /* zz04303 */
        _TEMPtime = _time;                                       /* zz04303 */
        TEMPtimeinfo = localtime(&_TEMPtime);                    /* zz04303 */
        strftime(_timestr, 20, "%H:%M:%S", TEMPtimeinfo);        /* zz04303 */

        // finish prepare for output format                      /* zz04303 */


        rlog.info("Plausi rejected #4.0: consumption of energy (checked value) %.3f must not be greater than limit %.3f", power, _maxPower); /*zz04303 */
        rlog.info("Plausi rejected #4.1: IF ((candValue - _value) / (candTime - _time) * 3600.) > _maxPower");                               /*zz04303 */
        rlog.info("Plausi rejected #4.2: %.3f %.3f %s %s %.3f",candValue, _value, candTimestr, _timestr, _maxPower);                         /*zz04303 */
        rlog.info("Plausi rejected #4.3: Delta value %.3f, Delta time %i seconds",candValue - _value, candTime - _time);                     /*zz04303 */
        return false;
    }

    // everything is OK -> use the candidate value
    _time = candTime;
    _value = candValue;
    if (rlog.isInfoEnabled()) {
        rlog.info("Plausi accepted: %.1f of %s", _value, ctime(&_time));
    }
    return true;
}

double Plausi::getCheckedValue() {
    return _value;
}

time_t Plausi::getCheckedTime() {
    return _time;
}

std::string Plausi::queueAsString() {
    std::string str;
    char buf[20];
    str += "[";
    std::deque<std::pair<time_t, double> >::const_iterator it = _queue.begin();
    for (; it != _queue.end(); ++it) {
        sprintf(buf, "%.1f", it->second);
        str += buf;
        str += ", ";
    }
    str += "]";
    return str;
}


