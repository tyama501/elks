/*
 * Architecture Specific stubs
 */
#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "basic.h"

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}

int host_analogRead(int pin) {
	return 0;
}

void host_pinMode(int pin,int mode) {
}
