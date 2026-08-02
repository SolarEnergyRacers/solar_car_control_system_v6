/*
 * MCP23017 I/O Extension over I2C  !!! UNTESTED !!!
 */

#ifndef SER_IOEXT_HANDLER_H
#define SER_IOEXT_HANDLER_H

#include "../definitions.h"

void nextScreenButtonHandler();
void constModeOrMountRequestHandler();
void sdCardDetectHandler();

#endif // SER_IOEXT_HANDLER_H
