#pragma once

typedef enum {
  // Digital
  STEERING_INPUT_HORN_EVENT = 0,
  STEERING_RADIO_PPT_EVENT,
  STEERING_HIGH_BEAM_FORWARD_EVENT,
  STEERING_HIGH_BEAM_REAR_EVENT,
  STEERING_REGEN_BRAKE_EVENT,
  STEERING_INPUT_CC_TOGGLE_PRESSED_EVENT,
  NUM_DIGITAL_STEERING_EVENTS,
} SteeringDigitalEvent;

typedef enum {
  // Analog
  STEERING_CONTROL_STALK_EVENT_LEFT = 7,
  STEERING_CONTROL_STALK_EVENT_RIGHT,
  STEERING_CC_EVENT_INCREASE_SPEED,
  STEERING_CC_EVENT_DECREASE_SPEED,
  STEERING_CC_BRAKE_PRESSED,
  NUM_STEERING_EVENTS
} SteeringAnalogEvent;
