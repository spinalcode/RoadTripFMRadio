#include "fmradio.h"

// create radio instance
RDA5807M fmradio(P0_5, P0_4);     // sda - scl LPC1768
AnalogIn inLeftPin = AnalogIn(AUDIOINLEFT);
AnalogIn inRightPin = AnalogIn(AUDIOINRIGHT);

void audioTimer(void){
	if (Chip_TIMER_MatchPending(LPC_TIMER16_0, 1)) {

    	// The audio input is only 0.3v so we need to multiply this to fit the range of the audio output.
    	// However, the input is 16bit and the output is 8bit, so we just >> 7 then remove the DC bias by -7.
        //int pinLData = inLeftPin.read_u16();

        if(myRadio.isMute==false){
            writeDAC( (((((inLeftPin.read_u16()>>7)-7)-128)*myRadio.globalVolume)>>4)+128);
        }

/*
        if(++myRadio.audioOffset == audioBufferSize*4){
            myRadio.audioOffset = 0;
        }
        myRadio.currentBuffer = myRadio.audioOffset/audioBufferSize;
*/
        // last thing we reset the time
    	Chip_TIMER_ClearMatch(LPC_TIMER16_0, 1);
    }

}

// timer init stolen directly from Pokittolib
void initTimer(uint32_t sampleRate){
    /* Initialize 32-bit timer 0 clock */
	Chip_TIMER_Init(LPC_TIMER16_0);
    /* Timer rate is system clock rate */
	uint32_t timerFreq = Chip_Clock_GetSystemClockRate();
	/* Timer setup for match and interrupt at TICKRATE_HZ */
	Chip_TIMER_Reset(LPC_TIMER16_0);
	/* Enable both timers to generate interrupts when time matches */
	Chip_TIMER_MatchEnableInt(LPC_TIMER16_0, 1);
    /* Setup 32-bit timer's duration (32-bit match time) */
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 1, (timerFreq / sampleRate));
	/* Setup both timers to restart when match occurs */
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER16_0, 1);
	/* Start both timers */
	Chip_TIMER_Enable(LPC_TIMER16_0);
	/* Clear both timers of any pending interrupts */
	NVIC_ClearPendingIRQ((IRQn_Type)my_TIMER_16_0_IRQn);
    /* Redirect IRQ vector - Jonne*/
    NVIC_SetVector((IRQn_Type)my_TIMER_16_0_IRQn, (uint32_t)&audioTimer);
	/* Enable both timer interrupts */
	NVIC_EnableIRQ((IRQn_Type)my_TIMER_16_0_IRQn);
}

// extracts the last positive integral value of string s
// returns...
// on success, the int value scanned
// -1, if the string is null, empty, or not terminated by a number
int extractLastIntegral(const char* s) {
    int value = -1;
    if (s && *s) {  // don't parse null and empty strings
        const char *scanstr = s + strlen(s) - 1;
        while (scanstr > s && (*(scanstr-1) >= '0' && *(scanstr-1) <= '9')) {
            scanstr--;
        }
        sscanf(scanstr,"%d", &value);
    }
    return value;
}

void saveSettings(){
    mkdir("/sd/_fmradio", 0777); // make sure recordings folder exist
    FILE *f = fopen("/sd/_fmradio/settings.ini","wb");
    if(f){
        fprintf(f, "playbackrate = %d\n",myRadio.LISTENINGSAMPLERATE);
        fprintf(f, "recordrate = %d\n",myRadio.RECORDINGSAMPLERATE);
        fprintf(f, "recordstereo = %d\n",myRadio.recordStereo);

        char tempText[20];
        for(int t=0; t<10; t++){
            fprintf(f, "station%d = %s\n",t, myRadio.presetNames[t]); // (int)radio.freq/10
            fprintf(f, "freq%d = %d\n",t, (int)(myRadio.presetFrequencies[t]*100)); // (int)radio.freq/10
        }

        fclose(f);
    }
}

void loadSettings(){

    char line[BUFFER_MAX_LENGTH];
    int tempChar;
    int index = 0;
    mkdir("sd/_fmradio", 0777); // make sure recordings folder exist
    
    FILE *f = fopen("/sd/_fmradio/settings.ini","r");
    if(f){

        int numStations = 0;
        while(tempChar = fgetc(f)){
    
            if (tempChar == EOF) {
                line[index] = '\0';
                break;
            }
    
            if (tempChar == '\n') {
                line[index] = '\0';
                index=0;
                if(strncmp(line, "playbackrate", strlen("playbackrate"))==0){
                    myRadio.LISTENINGSAMPLERATE = extractLastIntegral(line);
                }
                if(strncmp(line, "recordrate", strlen("recordrate"))==0){
                    myRadio.RECORDINGSAMPLERATE = extractLastIntegral(line);
                }
                if(strncmp(line, "recordstereo", strlen("recordstereo"))==0){
                    myRadio.recordStereo = extractLastIntegral(line);
                    if(myRadio.recordStereo >1)myRadio.recordStereo=1;
                }

                if(strncmp(line, "station", strlen("station"))==0){
                    char *p = strrchr(line, '=');
                    if (p && *(p + 1))
                        sprintf(myRadio.presetNames[numStations], "%9s\0", p + 1);
                    // terminate the string again, just incase
                    myRadio.presetNames[numStations][10]=0;
                    // remove any spaces from start of station name
                    while(myRadio.presetNames[numStations][0]==' '){
                        memmove(myRadio.presetNames[numStations], myRadio.presetNames[numStations]+1, strlen(myRadio.presetNames[numStations]));
                    }
                }
                
                if(strncmp(line,"freq", strlen("freq"))==0){
                    myRadio.presetFrequencies[numStations] = (float)(extractLastIntegral(line))/100;
                    if(numStations < MAXSTATIONS)numStations++;
                }
                

                continue;
            }
            else
                line[index++] = (char)tempChar;
        }
        
        fclose(f);
    }else{
        myRadio.LISTENINGSAMPLERATE=22050;//65536;
        myRadio.RECORDINGSAMPLERATE=11025;
        for(int t=0; t<MAXSTATIONS; t++){
            strcpy(myRadio.presetNames[t],"Empty");
            myRadio.presetFrequencies[t] = 0.0;
        }
        //saveSettings();
    }
}

void startRadio(){
    
    loadSettings();
    saveSettings();

    fmradio.Reset();      // reset and power up radio chip
    fmradio.BassBoost();
    fmradio.SoftBlend(); // no idea what this does at all
    fmradio.AFC();
    fmradio.Volume(15);    // Max Volume
    fmradio.Frequency(myRadio.presetFrequencies[0]);
    fmradio.ProcessData();
    //enableDAC();

    Pokitto::Core::update(); // do this to enable the irq

    initTimer(myRadio.LISTENINGSAMPLERATE);

}

void stationUp(){
//    if(myRadio.currentPreset >0){
//        myRadio.currentPreset--;
//        fmradio.Frequency(myRadio.presetFrequencies[myRadio.currentPreset]);
//    }
    fmradio.Volume(15);
    fmradio.SeekUp();
}

void stationDown(){
//    if(myRadio.currentPreset <9){
//        myRadio.currentPreset++;
//        fmradio.Frequency(myRadio.presetFrequencies[myRadio.currentPreset]);
//    }
    fmradio.Volume(15);
    fmradio.SeekDown();
}

void muteRadio(){
    myRadio.isMute=true;
}

void unMuteRadio(){
    myRadio.isMute=false;
}
