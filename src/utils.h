#ifndef _UTILS_H
#define _UTILS_H

#include <Arduino.h>
#include <EEPROM.h>

inline int bytes2int(byte h, byte l){
	int high = static_cast<int>(h);
	int low = static_cast<int>(l);
	return (256 * high) + low;
}

void writeStringToEEPROM(const String &data, uint16_t address) {
	EEPROM.write(address, (byte) data.length());
  	EEPROM.writeString(address + 1, data);
}

String readStringFromEEPROM(uint16_t address) {
	uint8_t strLength = EEPROM.read(address);
	char data[strLength];
	EEPROM.readString(address + 1, data, strLength);
	return String(data);
}

#endif