#include <cstdio>
#include <cstring>
#include <iostream>

#include "myMosq.h"
#include <mosquittopp.h>

using namespace std;


// The initialization are done in the constructor as well as the thread creation :

myMosq::myMosq(const char * _id,const char * _topic, const char * _host, int _port) : mosquittopp(_id)
 {
 mosqpp::lib_init();        // Mandatory initialization for mosquitto library
 this->keepalive = 60;    // Basic configuration setup for myMosq class
 this->id = _id;
 this->port = _port;
 this->host = _host;
 this->topic = _topic;
 connect_async(host,     // non blocking connection to broker request
 port,
 keepalive);
 loop_start();            // Start thread managing connection / publish / subscribe
 };

// The destructor, cleanly close what has been opened

myMosq::~myMosq() {
 loop_stop();            // Kill the thread
 mosqpp::lib_cleanup();    // Mosquitto library cleanup
 }

// To send a message, we can simplely call the associated function

bool myMosq::send_message(const  char * _message)
 {
 // Send message - depending on QoS, mosquitto lib managed re-submission this the thread
 //
 // * NULL : Message Id (int *) this allow to latter get status of each message
 // * topic : topic to be used
 // * lenght of the message
 // * message
 // * qos (0,1,2)
 // * retain (boolean) - indicates if message is retained on broker or not
 // Should return MOSQ_ERR_SUCCESS
 int ret = publish(NULL,this->topic,strlen(_message),_message,1,false);
 return ( ret == MOSQ_ERR_SUCCESS );
 }

// Then you can add some event handler

// on_connect / on_disconnect are called by thread each time we exeperience a server connection / disconnection

void myMosq::on_disconnect(int rc) {
 std::cout << ">> myMosq - disconnection(" << rc << ")" << std::endl;
 }

void myMosq::on_connect(int rc)
 {
 if ( rc == 0 ) {
 std::cout << ">> myMosq - connected with server" << std::endl;
 } else {
 std::cout << ">> myMosq - Impossible to connect with server(" << rc << ")" << std::endl;
 }
 }

// on_publish is called each time a message succeed to be sent to broker. The parameter is the message id you can set when publish.

void myMosq::on_publish(int mid)
 {
// std::cout << ">> myMosq - Message (" << mid << ") succeed to be published " << std::endl;
 }
