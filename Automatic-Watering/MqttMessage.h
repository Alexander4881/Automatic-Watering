#ifndef MqttMessage_h
#define MqttMessage_h

#include "Arduino.h"

class MqttMessage {
  public :
    MqttMessage();
    MqttMessage(String topic, String message);
    String Topic;
    String Message;
  private:
};
#endif
