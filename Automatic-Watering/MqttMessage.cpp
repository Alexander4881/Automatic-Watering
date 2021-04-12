#include "Arduino.h"
#include "MqttMessage.h"

MqttMessage :: MqttMessage() {
}

MqttMessage :: MqttMessage(String topic, String message) {
  Topic = topic;
  Message = message;
}
