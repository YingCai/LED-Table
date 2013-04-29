#include <fix_fft.h>
#include <LPD8806.h> //Library for interfacing with LED strips
#include <SPI.h>
#include <avr/pgmspace.h>

//Ying Cai
//December 2011
//Last updated January 2013
//Controls the LED Table

LPD8806 strip = LPD8806(312);
//Initialize a strip of 312 lights

const int input_pin=0; //Input on Pin A0

unsigned long lastSample; //time in microseconds at which the last sample was collected

unsigned int theta; //theta refers to an angle on the color wheel.
const float pi = 3.14159;

uint32_t bg;
uint32_t fg;

const int SAMPLE_SIZE=128;
const int GAIN=2; //ADC value gets divided by this.
const int LPF=3; //Number of samples to average per buffer entry. Narrows dynamic range while minimizing aliasing.
int avg=0;

char reals[SAMPLE_SIZE];
char imags[SAMPLE_SIZE];
int buffer_index=0;
bool FFTmode = 0; //perform FFT on buffers if true. populate buffer otherwise
unsigned long time;


int heights[12]; //each element of this array corresponds to one column on the table

unsigned long int loudness = 0;

void setup() {
//Not much to do here, just tell the strip to wake up.
  strip.begin();
  strip.show();
}

void loop(){
  //Perform FFT mode
  if(FFTmode){
    //perform FFT, output data to table
    strip.show();
    
    setHeights();
    setColors();
    setEverything(bg, fg);
    FFTmode=0;
    theta=(theta+1)%3000;
  }
  //Populate buffer mode
  else{
    if(buffer_index==SAMPLE_SIZE){
      buffer_index=0;
      FFTmode=1;
    }
    else{
      reals[buffer_index] = (analogRead(input_pin)-512)/1.5; //may need to muck around with low-level registers for faster ADC conversion
      imags[buffer_index]= 0;
      buffer_index++;
    }
  }  
}

void setHeights(){
  //Calculates the FFT and populates the heights[] array.

  fix_fft(reals, imags, 7, 0);
  
  for(int i=0;i<SAMPLE_SIZE/2;i++){
  //takes the magnitude of the FFT's output, for the sake of visualization
    reals[i]=sqrt(reals[i]*reals[i] + imags[i]*imags[i]);
  }
  
  int freqsPerColumn = SAMPLE_SIZE/64; //divide first half of reals[] into 32 groups, use the first 12
  
  loudness = 0;
  
  for(int i=0; i<12;i++){
    heights[i]/=2;
    for(int j=0;j<freqsPerColumn;j++){
      heights[i] += reals[i*freqsPerColumn + j + 3]/2;
    }
    loudness += heights[i];
  }
  
}

uint32_t getColorFromTheta(int theta){
  //Takes a theta between 0 and 300 and returns a strip.Color
  if (theta<1000){
    return strip.Color((1000-theta)/50+1, theta/50+1, 1);
  }
  else if (theta<2000){
    return strip.Color(1, (2000-theta)/50+1, (theta-1000)/50+1);
  }
  else if (theta<3000){
    return strip.Color((theta-2000)/50+1, 1, (3000-theta)/50+1); 
  }
}

void setColors(){
   fg = getColorFromTheta((theta+(loudness*5)+1500)%3000);
   bg = getColorFromTheta((theta+(loudness*5))%3000); 
}

void setEverything(uint32_t bg, uint32_t fg){
//setEverything binds colors to LEDs, sweeping through the LED matrix horizontally. This can be thought of as the outer loop of a nested
//loop that iterates through a 2-dimensional array.
  int c;
  for(c=0; c<12;c++){
    setColumn(c, heights[c], bg, fg);
  }
}

void setColumn(int column, int height, uint32_t bg, uint32_t fg) {
//setColumn binds colors to LEDs, sweeping through a single given column. This can be thought of as the inner loop of a nested loop that
//iterates through a 2-dimensional array.
//every other column needs to be inverted because the LEDs consist of a single strip that snakes up and down across the table.
  int i=0;
    while(i<height && i<26){
      if(!(column%2)){
        strip.setPixelColor((26*column+i), fg);
      }
      else{
        strip.setPixelColor((26*column+25-i), fg);
      }
      i++;
    }
    while(i<26){
      if(!(column%2)){
        strip.setPixelColor((26*column+i), bg);
      }
      else{
        strip.setPixelColor((26*column+25-i), bg);
      }
      i++;
    }
}

