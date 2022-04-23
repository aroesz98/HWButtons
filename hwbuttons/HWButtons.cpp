/**
 * @file HWButtons.cpp
 */

#include "HWButtons.h"

// ----- Initialization and Default Values -----

/**
 * @brief Construct a new HWButtons object but not (yet) initialize the IO pin.
 */
HWButtons::HWButtons()
{
  _pin = -1;
  // further initialization has moved to HWButtons.h
}

/**
 * Initialize the HWButtons library.
 * @param pin The pin to be used for input from a momentary button.
 * @param activeLow Set to true when the input level is LOW when the button is pressed, Default is true.
 * @param pullupActive Activate the internal pullup when available. Default is true.
 */
HWButtons::HWButtons(GPIO_TypeDef *port, const int pin, const bool activeLow)
{
  // HWButtons();
  _pin = pin;
  _port = port;

  if (activeLow) {
    // the button connects the input pin to GND when pressed.
    _buttonPressed = LOW;

  } else {
    // the button connects the input pin to VCC when pressed.
    _buttonPressed = HIGH;
  } // if

} // HWButtons


// explicitly set the number of millisec that have to pass by before a click is assumed stable.
void HWButtons::setDebounceTicks(const int ticks)
{
  _debounceTicks = ticks;
} // setDebounceTicks


// explicitly set the number of millisec that have to pass by before a click is detected.
void HWButtons::setClickTicks(const int ticks)
{
  _clickTicks = ticks;
} // setClickTicks


// explicitly set the number of millisec that have to pass by before a long button press is detected.
void HWButtons::setPressTicks(const int ticks)
{
  _pressTicks = ticks;
} // setPressTicks


// save function for click event
void HWButtons::attachClick(callbackFunction newFunction)
{
  _clickFunc = newFunction;
} // attachClick


// save function for parameterized click event
void HWButtons::attachClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramClickFunc = newFunction;
  _clickFuncParam = parameter;
} // attachClick


// save function for doubleClick event
void HWButtons::attachDoubleClick(callbackFunction newFunction)
{
  _doubleClickFunc = newFunction;
  _maxClicks = max(_maxClicks, 2);
} // attachDoubleClick


// save function for parameterized doubleClick event
void HWButtons::attachDoubleClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramDoubleClickFunc = newFunction;
  _doubleClickFuncParam = parameter;
  _maxClicks = max(_maxClicks, 2);
} // attachDoubleClick


// save function for multiClick event
void HWButtons::attachMultiClick(callbackFunction newFunction)
{
  _multiClickFunc = newFunction;
  _maxClicks = max(_maxClicks, 100);
} // attachMultiClick


// save function for parameterized MultiClick event
void HWButtons::attachMultiClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramMultiClickFunc = newFunction;
  _multiClickFuncParam = parameter;
  _maxClicks = max(_maxClicks, 100);
} // attachMultiClick


// save function for longPressStart event
void HWButtons::attachLongPressStart(callbackFunction newFunction)
{
  _longPressStartFunc = newFunction;
} // attachLongPressStart


// save function for parameterized longPressStart event
void HWButtons::attachLongPressStart(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramLongPressStartFunc = newFunction;
  _longPressStartFuncParam = parameter;
} // attachLongPressStart


// save function for longPressStop event
void HWButtons::attachLongPressStop(callbackFunction newFunction)
{
  _longPressStopFunc = newFunction;
} // attachLongPressStop


// save function for parameterized longPressStop event
void HWButtons::attachLongPressStop(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramLongPressStopFunc = newFunction;
  _longPressStopFuncParam = parameter;
} // attachLongPressStop


// save function for during longPress event
void HWButtons::attachDuringLongPress(callbackFunction newFunction)
{
  _duringLongPressFunc = newFunction;
} // attachDuringLongPress


// save function for parameterized during longPress event
void HWButtons::attachDuringLongPress(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramDuringLongPressFunc = newFunction;
  _duringLongPressFuncParam = parameter;
} // attachDuringLongPress


void HWButtons::reset(void)
{
  _state = HWButtons::FSM_INIT;
  _lastState = HWButtons::FSM_INIT;
  _nClicks = 0;
  _startTime = 0;
}


// ShaggyDog ---- return number of clicks in any case: single or multiple clicks
int HWButtons::getNumberClicks(void)
{
  return _nClicks;
}


/**
 * @brief Check input of the configured pin and then advance the finite state
 * machine (FSM).
 */
void HWButtons::tick(void)
{
  if (_pin >= 0) {
    tick(HAL_GPIO_ReadPin(_port, _pin) == _buttonPressed);
  }
}


/**
 *  @brief Advance to a new state and save the last one to come back in cas of bouncing detection.
 */
void HWButtons::_newState(stateMachine_t nextState)
{
  _lastState = _state;
  _state = nextState;
} // _newState()


/**
 * @brief Run the finite state machine (FSM) using the given level.
 */
void HWButtons::tick(bool activeLevel)
{
  unsigned long now = HAL_GetTick(); // current (relative) time in msecs.
  unsigned long waitTime = (now - _startTime);

  // Implementation of the state machine
  switch (_state) {
  case HWButtons::FSM_INIT:
    // waiting for level to become active.
    if (activeLevel) {
      _newState(HWButtons::FSM_DOWN);
      _startTime = now; // remember starting time
      _nClicks = 0;
    } // if
    break;

  case HWButtons::FSM_DOWN:
    // waiting for level to become inactive.

    if ((!activeLevel) && (waitTime < _debounceTicks)) {
      // button was released to quickly so I assume some bouncing.
      _newState(_lastState);

    } else if (!activeLevel) {
      _newState(HWButtons::FSM_UP);
      _startTime = now; // remember starting time

    } else if ((activeLevel) && (waitTime > _pressTicks)) {
      if (_longPressStartFunc) _longPressStartFunc();
      if (_paramLongPressStartFunc) _paramLongPressStartFunc(_longPressStartFuncParam);
      _newState(HWButtons::FSM_PRESS);
    } // if
    break;

  case HWButtons::FSM_UP:
    // level is inactive

    if ((activeLevel) && (waitTime < _debounceTicks)) {
      // button was pressed to quickly so I assume some bouncing.
      _newState(_lastState); // go back

    } else if (waitTime >= _debounceTicks) {
      // count as a short button down
      _nClicks++;
      _newState(HWButtons::FSM_COUNT);
    } // if
    break;

  case HWButtons::FSM_COUNT:
    // dobounce time is over, count clicks

    if (activeLevel) {
      // button is down again
      _newState(HWButtons::FSM_DOWN);
      _startTime = now; // remember starting time

    } else if ((waitTime > _clickTicks) || (_nClicks == _maxClicks)) {
      // now we know how many clicks have been made.

      if (_nClicks == 1) {
        // this was 1 click only.
        if (_clickFunc) _clickFunc();
        if (_paramClickFunc) _paramClickFunc(_clickFuncParam);

      } else if (_nClicks == 2) {
        // this was a 2 click sequence.
        if (_doubleClickFunc) _doubleClickFunc();
        if (_paramDoubleClickFunc) _paramDoubleClickFunc(_doubleClickFuncParam);

      } else {
        // this was a multi click sequence.
        if (_multiClickFunc) _multiClickFunc();
        if (_paramMultiClickFunc) _paramMultiClickFunc(_multiClickFuncParam);
      } // if

      reset();
    } // if
    break;

  case HWButtons::FSM_PRESS:
    // waiting for menu pin being release after long press.

    if (!activeLevel) {
      _newState(HWButtons::FSM_PRESSEND);
      _startTime = now;

    } else {
      // still the button is pressed
      if (_duringLongPressFunc) _duringLongPressFunc();
      if (_paramDuringLongPressFunc) _paramDuringLongPressFunc(_duringLongPressFuncParam);
    } // if
    break;

  case HWButtons::FSM_PRESSEND:
    // button was released.

    if ((activeLevel) && (waitTime < _debounceTicks)) {
      // button was released to quickly so I assume some bouncing.
      _newState(_lastState); // go back

    } else if (waitTime >= _debounceTicks) {
      if (_longPressStopFunc) _longPressStopFunc();
      if (_paramLongPressStopFunc) _paramLongPressStopFunc(_longPressStopFuncParam);
      reset();
    }
    break;

  default:
    // unknown state detected -> reset state machine
    _newState(HWButtons::FSM_INIT);
    break;
  } // if

} // HWButtons.tick()


// end.
