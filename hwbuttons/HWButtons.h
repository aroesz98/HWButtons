#ifndef HWButtons_h
#define HWButtons_h

#include "stm32f4xx_hal.h"
#include "algorithm"

using namespace std;

#define LOW 0
#define HIGH 1

extern "C++" {
	typedef void (*callbackFunction)(void);
	typedef void (*parameterizedCallbackFunction)(void *);
}


class HWButtons
{
public:

  HWButtons();
  HWButtons(GPIO_TypeDef *port, const int pin, const bool activeLow = true);

  void setDebounceTicks(const int ticks);
  void setClickTicks(const int ticks);
  void setPressTicks(const int ticks);

  void attachClick(callbackFunction newFunction);
  void attachClick(parameterizedCallbackFunction newFunction, void *parameter);

  void attachDoubleClick(callbackFunction newFunction);
  void attachDoubleClick(parameterizedCallbackFunction newFunction, void *parameter);

  void attachMultiClick(callbackFunction newFunction);
  void attachMultiClick(parameterizedCallbackFunction newFunction, void *parameter);

  void attachLongPressStart(callbackFunction newFunction);
  void attachLongPressStart(parameterizedCallbackFunction newFunction, void *parameter);

  void attachLongPressStop(callbackFunction newFunction);
  void attachLongPressStop(parameterizedCallbackFunction newFunction, void *parameter);

  void attachDuringLongPress(callbackFunction newFunction);
  void attachDuringLongPress(parameterizedCallbackFunction newFunction, void *parameter);

  void tick(void);
  void tick(bool level);

  void reset(void);

  int getNumberClicks(void);

  bool isIdle() const { return _state == FSM_INIT; }
  bool isLongPressed() const { return _state == FSM_PRESS; };


private:
  int _pin;                         // hardware pin number.
  GPIO_TypeDef *_port;
  unsigned int _debounceTicks = 20; // number of ticks for debounce times.
  unsigned int _clickTicks = 400;   // number of msecs before a click is detected.
  unsigned int _pressTicks = 800;   // number of msecs before a long button press is detected

  int _buttonPressed;

  // These variables will hold functions acting as event source.
  callbackFunction _clickFunc = NULL;
  parameterizedCallbackFunction _paramClickFunc = NULL;
  void *_clickFuncParam = NULL;

  callbackFunction _doubleClickFunc = NULL;
  parameterizedCallbackFunction _paramDoubleClickFunc = NULL;
  void *_doubleClickFuncParam = NULL;

  callbackFunction _multiClickFunc = NULL;
  parameterizedCallbackFunction _paramMultiClickFunc = NULL;
  void *_multiClickFuncParam = NULL;

  callbackFunction _longPressStartFunc = NULL;
  parameterizedCallbackFunction _paramLongPressStartFunc = NULL;
  void *_longPressStartFuncParam = NULL;

  callbackFunction _longPressStopFunc = NULL;
  parameterizedCallbackFunction _paramLongPressStopFunc = NULL;
  void *_longPressStopFuncParam;

  callbackFunction _duringLongPressFunc = NULL;
  parameterizedCallbackFunction _paramDuringLongPressFunc = NULL;
  void *_duringLongPressFuncParam = NULL;

  enum stateMachine_t : int {
    FSM_INIT = 0,
    FSM_DOWN = 1,
    FSM_UP = 2,
    FSM_COUNT = 3,
    FSM_PRESS = 6,
    FSM_PRESSEND = 7,
    UNKNOWN = 99
  };

  void _newState(stateMachine_t nextState);

  stateMachine_t _state = FSM_INIT;
  stateMachine_t _lastState = FSM_INIT; // used for debouncing

  unsigned long _startTime; // start of current input change to checking debouncing
  int _nClicks;             // count the number of clicks with this variable
  int _maxClicks = 1;       // max number (1, 2, multi=3) of clicks of interest by registration of event functions.
};

#endif
