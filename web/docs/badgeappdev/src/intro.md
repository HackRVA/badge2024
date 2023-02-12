# Intro
This is a handbook that aims to help you build build badge apps.

For a quickstart, check out the [badge-app-template](https://github.com/HackRVA/badge2023/blob/main/source/apps/badge-app-template.c)

## Structure of a Badge App
### Flow
1. app's callback function is invoked
2. app's callback function will determine what state the app is in
3. do something based on the current app state
4. return the process back to the OS

### Callback
It's important that badge apps don't lock up the processor.  For this reason, most apps use a state machine instead of a loop.

When an app is selected from the menu, the app's callback function will be called.
The callback will reference the app's current state that is stored in memory.

### State
App states are typically defined by the app.

e.g. 
```C
static enum cube_state_t {
	CUBE_INIT,
	CUBE_RUN,
	CUBE_EXIT,
} cube_state = CUBE_INIT;
```
