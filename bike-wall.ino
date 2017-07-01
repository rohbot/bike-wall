#include <SPI.h>
#include <bitBangedSPI.h>
#include <MAX7219_Dot_Matrix.h>
#include <FastLED.h>


#define NUM_DISPLAYS 6

MAX7219_Dot_Matrix display1 (4, 24, 22, 26);  // 8 chips, and then specify the LOAD pin only
MAX7219_Dot_Matrix display2 (4, 34, 32, 36);  // 8 chips, and then specify the LOAD pin only
MAX7219_Dot_Matrix display3 (4, 42, 40, 44);  // 8 chips, and then specify the LOAD pin only
MAX7219_Dot_Matrix display4 (4, 50, 48, 52);  // 8 chips, and then specify the LOAD pin only
MAX7219_Dot_Matrix display5 (4, 27, 25, 29);  // 8 chips, and then specify the LOAD pin only
MAX7219_Dot_Matrix display6 (4, 35, 31, 37);  // 8 chips, and then specify the LOAD pin only

MAX7219_Dot_Matrix displays[] = {display1, display2, display3, display4, display5, display6};


#define NUM_STRIPS 2
#define NUM_LEDS_PER_STRIP 9
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS


// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806, define both DATA_PIN and CLOCK_PIN

#define LED_PIN 13

#define RED_BIKE_PIN A0

#define BLUE_BIKE_PIN A1

#define MAX_ENERGY 20

float RED_BIKE_MAGIC  = 8;
float BLUE_BIKE_MAGIC  = 8;

float GAIN = 8.5;

CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];


enum {WAITING, GET_READY, START, RUNNING, FINISHED, TEST};

int state = WAITING;
//int state = TEST;

bool is_ready = false;
unsigned long last_touch = 0;

const int numReadings = 10;

float red_readings[numReadings];      // the readings from the analog input
float blue_readings[numReadings];      // the readings from the analog input

int index = 0;                  // the index of the current reading

float red_total = 0;                  // the running total
float red_average = 0;                // the average

float red_power = 0;
float red_energy = 0;

float blue_total = 0;                  // the running total
float blue_average = 0;                // the average

float blue_power = 0;
float blue_energy = 0;

float total_energy = 0;

int red_val = 0;
int blue_val = 0;

int prev_power = 0;

int freq = 100;

int counter = 0;

int prev_counter = 0;

int tokens = 0;

unsigned long last_counter = 0;
unsigned long some_time = 0;

bool changed = false;


void clear_leds() {
  LEDS.clear();
  LEDS.show();
  //Serial.println("Waiting");
}


void setup() {
  Serial.begin(9600);
  // Serial.setTimeout(100);
  for (int i = 0; i < NUM_DISPLAYS; i++) {
    displays[i].begin ();
    displays[i].setIntensity (15);

  }

  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<WS2812, 5>(leds, 0, NUM_LEDS_PER_STRIP);

  // tell FastLED there's 60 NEOPIXEL leds on pin 11, starting at index 60 in the led array
  FastLED.addLeds<WS2812, 7>(leds, NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  LEDS.setBrightness(84);
  last_touch = millis();
  clear_leds();
  for (int i = 0; i < numReadings; i++) {
    red_readings[i] = 0;
    blue_readings[i] = 0;
  }
  testDisplays();
  delay(1000);

  //Serial.print("Total:\t");
  //Serial.println(total_energy);
  clearDisplays();

  // displayInit();
  //Serial.println("here");
  Serial.println("F Arduino Connected!");

}


void displayNumber(int display_num, int v) {
  char ones;
  char tens;
  char hundreds;
  char thou;
  char buff[4];

  int offset = 0; //display_num * 4;

  ones = (char) (v % 10) + 48;
  v = v / 10;
  tens = (char) (v % 10) + 48;
  v = v / 10;
  hundreds = (char) (v % 10) + 48;
  v = v / 10;
  thou = (char) (v) + 48;

  bool has_val = false;

  if (thou > 48 || has_val) {
    has_val = true;
    displays[display_num].sendChar( offset, thou);
  } else {
    displays[display_num].sendChar( offset, ' ');


  }

  if (hundreds > 48 || has_val) {
    has_val = true;
    displays[display_num].sendChar(1 + offset, hundreds);


  } else {
    displays[display_num].sendChar(1 + offset, ' ');


  }
  if (tens > 48 || has_val) {
    has_val = true;
    displays[display_num].sendChar(2 + offset, tens);

  } else {
    displays[display_num].sendChar(2 + offset, ' ');

  }

  displays[display_num].sendChar(3 + offset, ones);



}

void testDisplays() {
  for (int i = 0 ; i < NUM_DISPLAYS; i++) {
    char num = i + 48;
    //displayNumber(i, i + 1);
    char str[] = {num, num, num, num};
    displays[i].sendString(str);
  }

}

void clearDisplays() {
  for (int i = 0; i < NUM_DISPLAYS; i++) {
    displays[i].sendString ("    ");
  }
  delay(10);
  for (int i = 0; i < NUM_DISPLAYS; i++) {
    displays[i].sendString ("    ");
  }
  delay(10);
  some_time = millis();

}




void stripRed(int ledsOn) {
  ledsOn = constrain(ledsOn, 0, NUM_LEDS_PER_STRIP);
  for (int i = 0; i < ledsOn; i++) {
    leds[i] = CRGB::Red;

  }
}

void stripBlue(int ledsOn) {
  ledsOn = constrain(ledsOn, 0, NUM_LEDS_PER_STRIP);

  for (int i = 0; i < ledsOn; i++) {
    leds[NUM_LEDS - 1 - i] = CRGB::Blue;
  }

}




void check_waiting() {
  // wait for Serial message
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'S') {
      Serial.println("START");
      is_ready = true;
    } else {
      Serial.print("Bad cmd: ");
      Serial.println(cmd);
    }

  }
}

void readRedBike() {
  red_total = red_total - red_readings[index];
  red_readings[index] = analogRead(RED_BIKE_PIN); //Raw data reading
  //red_readings[index] = readings[index] - 512) * 5 / 1024 / 0.04 - 0.04;

  red_total = red_total + red_readings[index];
  index = index + 1;
  if (index >= numReadings)
    index = 0;
  red_average = red_total / numReadings; //Smoothing algorithm (http://www.arduino.cc/en/Tutorial/Smoothing)
  int val = map(int(red_average), 200, 500, 12, 24);
  red_power = constrain(val, 0, 50) * RED_BIKE_MAGIC;
  if (red_power > 6 && red_average > 30) {
    red_energy += (red_power * 0.01);
    red_val = map(int(red_energy), 0, MAX_ENERGY, 0, NUM_LEDS_PER_STRIP);
    if (red_energy > MAX_ENERGY) {
      incTokens();
      total_energy += red_energy;
      red_energy = 0;
    }


    //    Serial.print("RED\t");
    //    Serial.print(red_energy);
    //    Serial.print("\t");
    //    Serial.print(red_average);
    //    Serial.print("\t");
    //    Serial.print(power);
    //    Serial.print("\t");
    //    Serial.println(red_val);

    changed = true;

  } else {
    red_power = 0;
  }

}

void readBlueBike() {
  blue_total = blue_total - blue_readings[index];
  blue_readings[index] = analogRead(BLUE_BIKE_PIN); //Raw data reading
  //blue_readings[index] = readings[index] - 512) * 5 / 1024 / 0.04 - 0.04;

  blue_total = blue_total + blue_readings[index];
  blue_average = blue_total / numReadings; //Smoothing algorithm (http://www.arduino.cc/en/Tutorial/Smoothing)

  int val = map(int(blue_average), 200, 500, 12, 24);
  blue_power = constrain(val, 0, 50) * BLUE_BIKE_MAGIC;
  if (blue_power > 6 && blue_average > 30) {
    blue_energy += (blue_power * 0.01);
    blue_val = map(int(blue_energy), 0, MAX_ENERGY, 0, NUM_LEDS_PER_STRIP);

    //    Serial.print("BLUE\t");
    //    Serial.print(blue_energy);
    //    Serial.print("\t");
    //    Serial.print(blue_average);
    //    Serial.print("\t");
    //    Serial.print(power);
    //    Serial.print("\t");
    //    Serial.println(blue_val);

    changed = true;
    if (blue_energy > MAX_ENERGY) {
      incTokens();
      total_energy += blue_energy;
      blue_energy = 0;
    }

  } else {
    blue_power = 0;
  }

}

void incTokens() {
  tokens++;
  Serial.print("t:");
  Serial.println(tokens);
}

void showCounter() {
  displayNumber(2, counter);
}

void run() {
  changed = false;
  prev_power = red_power + blue_power;
  readRedBike();
  readBlueBike();
  if ((red_power + blue_power) !=  prev_power) {

  }


  if (millis() - last_counter >= 1000) {
    counter--;
    if (counter < 0) {
      counter = 0;
      state = FINISHED;
      Serial.println("Finished!");
      // Serial.print("t:");
      // Serial.println(tokens);
      displays[2].sendString(" ");
    }
    else {
      showCounter();
    }
    last_counter = millis();
    //displayNumber(2, 0);
    changed = true;

  }

  if (changed) {
    FastLED.clear();
    stripRed(red_val);
    stripBlue(blue_val);
    FastLED.show();

    int energy = int(total_energy + blue_energy + red_energy) * GAIN;
    displayNumber(3, energy);

    int power = int( (red_power + blue_power) ); //* 10);
    displayNumber(1, power);

    int calories = energy / 80;
    displayNumber(0, calories);

    int light = energy / 9;
    displayNumber(4, light);

    int aircon = energy / 3500;
    displayNumber(5, aircon);


    Serial.print("Energy:\t");
    Serial.print(energy);
    Serial.print("\tpower:\t");
    Serial.println(power);

  }
  delay(10);



}

void show_results() {
  clear_leds();
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'V') {
      Serial.println("VALID");
      is_ready = true;

      state = WAITING;
      clear_leds();
      clearDisplays();


    } else {
      Serial.print("Bad cmd:\t");
      Serial.println(cmd);
    }

  }


}

void count_down() {
  delay(1000);
  Serial.println("c:3");
  displayNumber(2, 3);
  delay(1000);
  Serial.println("c:2");
  displayNumber(2, 2);
  delay(1000);
  Serial.println("c:1");
  displayNumber(2, 1);
  delay(1000);
  Serial.println("c:0");
  displayNumber(2, 0);
}

void test_mode() {
  clearDisplays();
  counter++;
  if (counter >= NUM_LEDS_PER_STRIP)
    counter = 0;
  testDisplays();
  FastLED.clear();
  stripRed(counter);
  stripBlue(counter);
  FastLED.show();
  Serial.println(counter);

  delay(2000);


}

void loop() {


  switch (state) {

    case WAITING:
      tokens = 0;
      is_ready = false;
      FastLED.clear();
      FastLED.show();

      check_waiting();

      if (is_ready) {
        //is_touched = false;
        state = GET_READY;
      }

      break;

    case GET_READY:

      Serial.println("get_ready");
      count_down();
      state = RUNNING;
      red_energy = 0;
      red_total = 0;
      blue_energy = 0;
      blue_total = 0;
      tokens = 0;
      index = 0;
      for (int i = 0; i < numReadings; i++) {
        red_readings[i] = 0;
        blue_readings[i] = 0;
      }
      counter = 60;
      last_counter = 0;
      Serial.println("t:0");
      break;


    case RUNNING:
      run();
      break;
    case FINISHED:
      show_results();

      break;
    case TEST:
      test_mode();
      //counter = 0;
      break;

  }

}

