#pragma once
#include "Pokitto.h"

#define audioBufferSize 512
#define AUDIOINLEFT P0_12            // Pin for capturing audio
#define AUDIOINRIGHT P0_13            // Pin for capturing audio
#define my_TIMER_16_0_IRQn 16          // for Timer setup
#define MAXSTATIONS 10
#define AUDIOSIZE 512*4             // Size of buffer for holding captured audio
#define QUARTAUDIOSIZE 512          // How much buffer to write to SD card at once, 512 bytes is most efficient
#define BYTESIZE sizeof(uint8_t)    // Size of 1 byte
#define BUFFER_MAX_LENGTH 32    // max length of text filereading buffer

#include "SDFileSystem.h"
#include "timer_11u6x.h"
#include "clock_11u6x.h"
#include "RDA5807M.h"
#include "sound.h"

struct radioData {

	float curFreq;

	int LISTENINGSAMPLERATE = 65536;   // Double the radio samplerate for passthrough to Pokitto
	int RECORDINGSAMPLERATE = 22050;    // We can't record at a high rate because of SD card write speed

	unsigned char *audioBuffer = (unsigned char *) 0x20000000;
	unsigned char audioBuffer2[audioBufferSize*4];// = (unsigned char *) 0x20000800;
	int bufferOffset[4]={ audioBufferSize*3,  0, audioBufferSize, audioBufferSize*2 };

	int audioOffset = 0;
	uint8_t currentBuffer = 0;          // Section of buffer being written to
	uint8_t completeBuffer = 0;         // Which section is full
	long int actualSampleRate = 0;
	long int actualSampleCount = 0;
	long int lastMillis = 0;
	uint8_t recordStereo = 0;
	uint8_t globalVolume = 16;          // Our own volume control as we bypass the Pokitto volume functions
	char presetNames[MAXSTATIONS][10];           // Preset station names
	float presetFrequencies[MAXSTATIONS];          // Preset station frequencies 
	uint8_t currentPreset = 0;
	uint8_t selectedPreset = 0;

	char displayRDSText[66];
	char displayStation[10];

    bool isMute=true;

	uint8_t LRSwitch = 0;

};

extern radioData myRadio;

void audioTimer(void);
void initTimer(uint32_t sampleRate);
int extractLastIntegral(const char* s);
void saveSettings();
void loadSettings();
void startRadio();
void stationUp();
void stationDown();
void muteRadio();
void unMuteRadio();
