//--------------------------------------------------------------------------------------------------------------------------------------------
// betelgeuse.cpp
// Author:      Mike LiVolsi
//
// Notes about the sign in the movie
// There are 3 lighting compoments to the sign.
//      1. The backlight for the actual signage.
//      2. The arrow
//      3. The marquis lights
//
// Backlight:
//              The light comes on/off at different intervals.
//              It's supposed to mimic one of those blinker lights that
//              come on/off based on the temperature of the bulb. So, we'll use a random number between 2 and 8 (seconds)
//              Another note: The light does seem to gradient full on/off. I'm not sure if I'm going to be dealing with
//              that.
//              Timing:  The light does appear to stay on for about 2 seconds.
//
// Arrow:       There are 13 real rows of lights starting from the top, and ~7 phantom lights. ie. the time delay between the last
//              row of lights in the arrow , then "behind the sign", the emerges on the bottom. So, we'll need to deal with that delay
//              In the movie, the last bulb in the arrow isn't the last sequence. It actually "bounces" to the 2nd row from the bottom
//              before it's off.
//              because of the "Bounce", we'll steal a "phantom" light, so we'll process 14 rows of lights, and 6 fake rows
//              Timing: it takes about 1 second to go through the entire arrow. and 1.5 seconds between the end of the last sequence and
//                      the start of a new one.
//
// Maquis:      The lights (I'm not sure how many) rotate from left, then right, switching directions every second. so for an off "bulb", it takes
//              1 second to go through a 5 bulb sequence, before switching directions
//
// Timing:      we'll break eveything into 50ms frames. Each line in the arrow is handled in that frame. Marquis will be handled in 200ms
//
// Arduino pins: The backlight will map to pin  22
//               The marquis   will map to pins 23-27
//               The arrow part1     will map to pins 28-34
//               The arrow part1     will map to pins 35-42
// maxchannels:  24
//
//
//--------------------------------------------------------------------------------------------------------------------------------------------
const int startPin=22;
const int maxChannels=21;
const int waitTime=40;                                         // 40 ms

int  byteArray[maxChannels+1];
int  bannerSleep=random(2,8);
bool bannerOn=false;                                            // don't wake if already awake
int  bannerOffCount=0;                                          // Compare this to the banner sleep value
int  bannerOnCount=0;

int timeSlice=1000/waitTime;                                        
//------------------------------------------------
// Arrow variables
//------------------------------------------------
bool arrowOn=false;
int arrowOffCount=0;

//------------------------------------------------
// Marquis variables                                           // marquis is always on. there are 5 bulbs per segment. Only direction changes
//------------------------------------------------
bool clockwise=true;
int  currentBulb=4;                                          //  [ 0 1 2 3 4 5 ]  [ 0 1 2 3 4 5 ] [ 0 1 2 3 4 5 ]
int  mIncrement=1;

void setup() {
    unsigned int idx=0;
    Serial.begin(9600);
 
//    Serial.println("Setup");
    for (int i = startPin; i < (maxChannels+startPin); i++) {            // start with pin '2' (0 and 1 are rx/tx)  and end at pin 'maxChannels'
//        Serial.print("Setting array "); Serial.print(idx); Serial.print(" To pin: "); Serial.println(i);
        pinMode(i,  OUTPUT);
        digitalWrite(i, LOW);
        byteArray[idx]=i;
        idx++;
    }
    //-----------------------------------------------------------------------------
    // Override index-0 with pin 46, since we're now using a mosfet for the LEDs
    //-----------------------------------------------------------------------------
    byteArray[0]=46;
    pinMode(byteArray[0], OUTPUT);
    analogWrite(byteArray[0], 0);
    
  
    // Turn on all the marquis lights
    for(int i=0; i<5; i++) {
        digitalWrite(byteArray[i+1], HIGH);
    }
}

void loop() {
    //------------------------------------------------
    // Banner variables
    //------------------------------------------------
    
    // Serial.print("Banner should sleep for "); Serial.println(bannerSleep);

    //----------------------------------------------------------------
    // Code for the banner - don't do any sleep stuff here
    // We should loop every second.. so the processing after this banner
    // section is equivalent to "wait(1000)"
    //----------------------------------------------------------------
    if(!bannerOn) {
        if (bannerOffCount == bannerSleep) {                         // The banner is off, and it's been off for 'x' seconds
            analogWrite(byteArray[0], 255);
            // Serial.print("Setting banner at pin "); Serial.print(byteArray[0]); Serial.println(" to high");
            bannerSleep=rand()%6; if(bannerSleep < 1) bannerSleep=1;  // for the next time
            bannerOn=true;
            bannerOffCount=0;
            bannerOnCount=0;
        }
        else bannerOffCount++;
    }
    else {
        if(bannerOnCount == 1) {
            // Serial.print("Setting banner at pin "); Serial.print(byteArray[0]); Serial.println(" to low");
            analogWrite(byteArray[0], 0);
            bannerOn=false;
            bannerOnCount=0;
            bannerOffCount=0;
            bannerSleep=random(2,8);
        }
        else bannerOnCount++;
    }

    //-------------------------------------------------------------------------------------------------------------------------------------------------
    // Arrow - Let's first set the values before we do any time stuff  - we don't need an arrowOnCount because it will all be done within this second
    //-------------------------------------------------------------------------------------------------------------------------------------------------
    if (arrowOffCount == 1 ) {
        arrowOn=true;
        arrowOffCount=0;
    }
    else arrowOffCount++;

    //-------------------------------------------------------------------------------------------------------------------------------------------------
    // Maquis - Which direction should we be heading
    //          if previously we were clockwise, then set to counterclockwise with the incrementer being -1
    //          else set to clockwise with the incrementer being +1
    //-------------------------------------------------------------------------------------------------------------------------------------------------
    if (clockwise) {
        clockwise=false;
        mIncrement=-1;
        currentBulb=4;
    }
    else {
        clockwise=true;
        mIncrement=1;
        currentBulb=0;
    }

    
    //----------------------------------------------------
    // Time slicing
    //----------------------------------------------------
    for (int i=0; i < timeSlice; i++) {
        //===================================
        // Start Arrow
        //===================================
        if(arrowOn) {
            if( ( i <= 6 ) || ( i >= 15  &&  i < timeSlice ) ){
                if( i <= 6 ) {
//                    Serial.print("Setting F pin "); Serial.print(byteArray[i+6]); Serial.println(" to HIGH");
                    digitalWrite(byteArray[i+6], HIGH);
                    if( i!= 0 ) {
//                        Serial.print("Setting F pin "); Serial.print(byteArray[i+5]); Serial.println(" to LOW");
                        digitalWrite(byteArray[i+5], LOW);
                    }
                    
                }
                else {
                    if ( i >= 15 ) {
                        if ( i < (timeSlice-2) ) {
//                            Serial.print("Setting P pin "); Serial.print(byteArray[i-2]); Serial.println(" to HIGH");  // + 6 offset plus -8 since we're ignore 8 loops
                            digitalWrite(byteArray[i-2], HIGH);
                            if (i != 15) {
//                                Serial.print("Setting P pin "); Serial.print(byteArray[i-3]); Serial.println(" to LOW");
                                digitalWrite(byteArray[i-3], LOW);
                            }
                        }
                        else {
                            if( i==timeSlice-2 ) {
//                                Serial.print("Setting B pin "); Serial.print(byteArray[i-4]); Serial.println(" to HIGH");
//                                Serial.print("Setting B pin "); Serial.print(byteArray[i-3]); Serial.println(" to LOW");
                                digitalWrite(byteArray[i-4], HIGH);
                                digitalWrite(byteArray[i-3], LOW);
                            }
                            else {
//                                Serial.print("Setting B pin "); Serial.print(byteArray[i-5]); Serial.println(" to LOW");
                                digitalWrite(byteArray[i-5], LOW);
                            }
                        }
                    }    
                }                
            }
            else {
                if(i==8) {                    
//                    Serial.print("Setting F pin "); Serial.print(byteArray[12]); Serial.println(" to LOW");    
                    digitalWrite(byteArray[12], LOW);
                }
            }
        }      // end arrow section

        //===================================
        // End Arrow - Start Marquis
        //===================================

        if( ( i%5 == 0) && (i > 0) ) {
            currentBulb= currentBulb + mIncrement;
//            Serial.print("Turning bulb ") + Serial.print(currentBulb+1); Serial.println(" OFF");
            digitalWrite(byteArray[currentBulb+1], LOW);
            if (clockwise ) {
                if (currentBulb > 0) {
//                    Serial.print("Turning bulb ") + Serial.print(currentBulb); Serial.println(" ON");
                    digitalWrite(byteArray[currentBulb], HIGH);                                      // currentBulb-1+1 (for the array)
                }
            }
            else {
                if (currentBulb < 5) {
//                    Serial.print("Turning bulb ") + Serial.print(currentBulb+2); Serial.println(" ON");
                    digitalWrite(byteArray[currentBulb+2], HIGH);
                }
            }
        } 
        
        delay(waitTime); 
    }
//    Serial.println("--------------------------------------------");
    arrowOn=false;

}
