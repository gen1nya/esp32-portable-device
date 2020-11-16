#ifndef _UTILS_H
#define _UTILS_H

#include <Arduino.h>

inline int bytes2int(byte h, byte l){
	int high = static_cast<int>(h);
	int low = static_cast<int>(l);
	return (256 * high) + low;
}

#endif