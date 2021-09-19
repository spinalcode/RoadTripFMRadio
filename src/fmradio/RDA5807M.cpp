#include "RDA5807M.h"
#include "SoftI2C.h"

#define SOFTSDA P0_17               // Pin for software I2C data pin
#define SOFTCLK P0_14               // Pin for software I2C clock pin

SoftI2C i2c_soft = SoftI2C(SOFTSDA, SOFTCLK);

unsigned int RDA5807M_WriteRegDef[6] ={0xC004,0x0000,0x0100,0x84D4,0x4000,0x0000}; // initial data

RDA5807M::RDA5807M(PinName sda, PinName scl) : i2c(sda, scl)
{   
    i2c_soft.frequency(400000);
    Init();
}

RDA5807M::~RDA5807M()
{
}

void RDA5807M::WriteAll()
{ 
    char buf[30];  
    int i,x = 0;
    for(i=0; i<12; i=i+2){
        buf[i] = RDA5807M_WriteReg[x] >> 8;
        x++;}
    x = 0;
    for(i=1; i<13; i=i+2){
        buf[i] = RDA5807M_WriteReg[x] & 0xFF;
        x++;}
    i2c_soft.write(0x20, buf, 14);
}

void RDA5807M::Init(){
    int i;
    for(i=0; i<6; i++){
        RDA5807M_WriteReg[i] = RDA5807M_WriteRegDef[i];
        WriteAll();
    }
}

void RDA5807M::PowerOn(){
    RDA5807M_WriteReg[1] = RDA5807M_WriteReg[1] | RDA_TUNE_ON;
    RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | RDA_POWER;
    WriteAll(); power=1;
    RDA5807M_WriteReg[1] = RDA5807M_WriteReg[1] & 0xFFEF;   //Disable tune after PowerOn operation
}

void RDA5807M::PowerOff(){
    RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] ^ RDA_POWER;
    WriteAll();power=0;
}

void RDA5807M::Reset(){
    Init();
    PowerOn();
    RDSinit();
    RDS();
    Volume(InitialVolume);
    Frequency(DefaultFreq);  // set default start frequency.
}

void RDA5807M::Volume(int vol){
    if(vol > 15){
        vol = 15;
    }
    if(vol < 0){
        vol = 0;
    }
    RDA5807M_WriteReg[3] = (RDA5807M_WriteReg[3] & 0xFFF0)| vol;   // Set New Volume
    volume=vol;
    WriteAll();
}

void RDA5807M::BassBoost(){
    if ((RDA5807M_WriteReg[0] & 0x1000)==0){
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | RDA_BASS_ON;
        bass=1;
    }
    else{
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] & RDA_BASS_OFF;
        bass=0;
    }
    WriteAll();
}

void RDA5807M::Mono(){
    if ((RDA5807M_WriteReg[0] & 0x2000)==0){
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | RDA_MONO_ON;
        mono=1;
    }
    else{
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] & RDA_MONO_OFF;
        mono=0;
    }
    WriteAll();
}

void RDA5807M::Mute(){
    if ((RDA5807M_WriteReg[0] & 0x8000)==0){
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | 0x8000;
        mute=0;
    }
    else{
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] & 0x7FFF;
        mute=1;
    }
    WriteAll();
}

void RDA5807M::Softmute(){
    if ((RDA5807M_WriteReg[2] & 0x0200)==0){
        RDA5807M_WriteReg[2] = RDA5807M_WriteReg[2] | 0x0200;
        softmute=1;
    }
    else{
        RDA5807M_WriteReg[2] = RDA5807M_WriteReg[2] & 0xFDFF;
        softmute=0;
    }
    WriteAll();
}

void RDA5807M::SoftBlend(){
    if ((RDA5807M_WriteReg[6] & 0x0001)==0){
        RDA5807M_WriteReg[6] = RDA5807M_WriteReg[6] | 0x0001;
        softblend=1;
    }
    else{
        RDA5807M_WriteReg[6] = RDA5807M_WriteReg[6] & 0xFFFE;
        softblend=0;
    }
    WriteAll();
}

void RDA5807M::AFC(){
    if ((RDA5807M_WriteReg[2] & 0x0100)==0){
        RDA5807M_WriteReg[2] = RDA5807M_WriteReg[2] | 0x0100;
        afc=1;
    }
    else{
        RDA5807M_WriteReg[2] = RDA5807M_WriteReg[2] & 0xFEFE;
        afc=0;
    }
    WriteAll();
}

void RDA5807M::SeekUp(){
    RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | RDA_SEEK_UP;   // Set Seek Up
    WriteAll();
    RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] & RDA_SEEK_STOP;   // Disable Seek
}

void RDA5807M::SeekDown(){
    RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | RDA_SEEK_DOWN;   // Set Seek Down
    WriteAll();
    RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] & RDA_SEEK_STOP;   // Disable Seek
}

void RDA5807M::Frequency(float Freq){
    int Channel;
    Channel = ((Freq-StartingFreq)/0.1)+0.05;
    Channel = Channel & 0x03FF;
    RDA5807M_WriteReg[1] = Channel*64 + 0x10;  // Channel + TUNE-Bit + Band=00(87-108) + Space=00(100kHz)
    WriteAll();
    RDA5807M_WriteReg[1] = RDA5807M_WriteReg[1] & RDA_TUNE_OFF;
}

void RDA5807M::RDS(){
    if ((RDA5807M_WriteReg[0] & RDA_RDS_ON)==0){
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] | RDA_RDS_ON;
        rds=1;
    }
    else{
        RDA5807M_WriteReg[0] = RDA5807M_WriteReg[0] & RDA_RDS_OFF;
        rds=0;
    }
    WriteAll();
}

void RDA5807M::Readregisters(){
   int i;
   char rcv[12];    
   i2c_soft.read(0x20, rcv,12);   // read 12 bytes for reg 0x0A .. reg 0x0F 
        for(i=0; i<6; i++){
            RDA5807M_ReadReg[i] = ((rcv[i *2] << 8) | rcv [(i*2) +1] );
        }
    block1 = RDA5807M_ReadReg[2];
    block2 = RDA5807M_ReadReg[3];
    block3 = RDA5807M_ReadReg[4];
    block4 = RDA5807M_ReadReg[5];    
   
    rdsready = RDA5807M_ReadReg[0] & 0x8000;            //if rdsready != 0 rds data are ready
    tuneok = RDA5807M_ReadReg[0] & 0x4000;              //if tuneok != 0 seek/tune completed
    nochannel = RDA5807M_ReadReg[0] & 0x2000;           //if nochannel != 0 no channel found
    rdssynchro = RDA5807M_ReadReg[0] & 0x1000;          //if rdssynchro = 1000 rds decoder syncrhonized
    stereo = RDA5807M_ReadReg[0] & 0x0400;              //if stereo = 0 station is mono else stereo
    freq = (((RDA5807M_ReadReg[0] & 0x03FF) * 100) + 87000);    //return freq ex 102600KHz > 102.6MHz
    signal = RDA5807M_ReadReg[1] >> 10;                 //return signal strength rssi
    fmready = RDA5807M_ReadReg[1] & 0x0008;             //if fmready = 8 > fm is ready
    fmstation = RDA5807M_ReadReg[1] & 0x0100;           //if fmstation = 100 fm station is true
    rdsblockerror = RDA5807M_ReadReg[1] & 0x000C;       //check for rds blocks errors
                                                        //00= 0 errors,01= 1~2 errors requiring correction
                                                        //10= 3~5 errors requiring correction
                                                        //11= 6+ errors or error in checkword, correction not possible.
}

void RDA5807M::RDSinit() {
  strcpy(StationName, "        ");
  strcpy(PSName, "        ");
  strcpy(PSName1, "        ");
  strcpy(PSName2, "        ");
  memset(RDSText, '\0', sizeof(RDSText));
  memset(RDSTxt, '\0', sizeof(RDSTxt));
  lastTextIDX = 0;
  mins=0;
  sprintf(CTtime, "CT --:--");
} 

void RDA5807M::ProcessData()
{
    Readregisters();        
    if (rdssynchro != 0x1000){  // reset all the RDS info.
        RDSinit();
        return;
    }   
  // analyzing Block 2
    rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
    rdsTP = (block2 & 0x0400);
    rdsPTY = (block2 & 0x0400);

    switch (rdsGroupType) {
        case 0x0A:
        case 0x0B:
        // The data received is part of the Service Station Name  
            idx = 2 * (block2 & 0x0003);
            // new data is 2 chars from block 4
            c1 = block4 >> 8;
            c2 = block4 & 0x00FF;
        // check that the data was received successfully twice
        // before sending the station name
        if ((PSName1[idx] == c1) && (PSName1[idx + 1] == c2)) {
            // retrieve the text a second time: store to _PSName2
            PSName2[idx] = c1;
            PSName2[idx + 1] = c2;
            PSName2[8] = '\0';
            if (strcmp(PSName1, PSName2) == 0) {
                // populate station name          
                n=0;
                for(i=0;i<(8);i++){ // remove non-printable error ASCCi characters               
                    if(PSName2[i] > 31 && PSName2[i] < 127){          
                        StationName[n] = PSName2[i];         
                        n++;
                    }                  
                } 
            }
        }
        if ((PSName1[idx] != c1) || (PSName1[idx + 1] != c2)) {
            PSName1[idx] = c1;
            PSName1[idx + 1] = c2;
            PSName1[8] = '\0';
        } 
    break;
    
    case 0x2A:
        // RDS text
    textAB = (block2 & 0x0010);
    idx = 4 * (block2 & 0x000F);  
    if (idx < lastTextIDX) {
      // The existing text might be complete because the index is starting at the beginning again.
      // Populate RDS text array.
        n=0;       
        for(i=0;i<strlen(RDSTxt);i++){
            if(RDSTxt[i] > 31 && RDSTxt[i] < 127){    // remove any non printable error charcters      
                RDSText[n] = RDSTxt[i];         
                n++;
            }
        }
      }
    lastTextIDX = idx;
    if (textAB != lasttextAB) {
      // when this bit is toggled the whole buffer should be cleared.
      lasttextAB = textAB;
      memset(RDSTxt, 0, sizeof(RDSTxt));
      memset(RDSText, '\0', sizeof(RDSText));
    }    
    if(rdsblockerror < 4){     
        // new data is 2 chars from block 3
        RDSTxt[idx] = (block3 >> 8);     idx++;
        RDSTxt[idx] = (block3 & 0x00FF); idx++;
        // new data is 2 chars from block 4
        RDSTxt[idx] = (block4 >> 8); idx++;
        RDSTxt[idx] = (block4 & 0x00FF); idx++;
    }             
    break;

  case 0x4A:
    // Clock time and date
    if(rdsblockerror <3){ // allow limited RDS data errors as we have no correctioin code 
        offset = (block4) & 0x3F; // 6 bits
        mins = (block4 >> 6) & 0x3F; // 6 bits
        mins += 60 * (((block3 & 0x0001) << 4) | ((block4 >> 12) & 0x0F));
    }
    // adjust offset
    if (offset & 0x20) {
      mins -= 30 * (offset & 0x1F);
    }
    else {
      mins += 30 * (offset & 0x1F);
    }    
    if(mins == lastmins+1){ // get CT time twice before populating time
        minutes=mins;}
        lastmins=mins;    
    if(rdssynchro == 0x1000){
        if(minutes>0 && minutes<1500){sprintf(CTtime, "CT %2d:%02d",(minutes/60),(minutes%60));}
        }
        else{minutes=0;sprintf(CTtime, "CT --:--");} // CT time formatted string         
    break;

  case 0x6A: 
    // IH
    break;

  case 0x8A:
    // TMC
    break;

  case 0xAA:
    // TMC
    break;

  case 0xCA:
    // TMC
    break;

  case 0xEA:
    // IH
    break;

  default:
    
    break;
  }
}

