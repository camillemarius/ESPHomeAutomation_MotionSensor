#include <Arduino.h>
#include "Adafruit_VL53L0X.h"

/* ==========================================================================
== Constants
========================================================================== */
// Hardware In-Outputs constants
const int SHT_LOX1 = 14; // shutdown-pin D5
const int SHT_LOX2 = 12; // shutdown-pin D6

const int LOX1_ADDRESS = 0x30;
const int LOX2_ADDRESS = 0x31;

// Measure threshold constants
const int  ADDITIONAL_THRESHOLD_MAX = 10;
const int OBJECT_WIDTH = 10; // Körperbreite

/* ==========================================================================
== Variable
========================================================================== */
float distanceCm, distanceAverageCmRight, distanceAverageCmLeft;
int person_cnt = 0;

/* ==========================================================================
== Object
========================================================================== */
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measure1;
VL53L0X_RangingMeasurementData_t measure2;

/* ==========================================================================
== Functions
========================================================================== */

/* -----------------------------------------------------------------------------
-- Measure and Validate Distance Left
----------------------------------------------------------------------------- */
void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activating LOX1 and resetting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot left VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2
  if(!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("Failed to boot right VL53L0X"));
    while(1);
  }
}

float dst_measure_left(boolean check_measure)
{
  float distance_left = 0;
  VL53L0X_RangingMeasurementData_t measure;

  lox1.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  Serial.print(F("1: "));
  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    //Serial.print("Distance (mm): ");
    //Serial.println(measure.RangeMilliMeter);
    distance_left = measure.RangeMilliMeter;
  } else {
    Serial.println(" out of range ");
    return distanceAverageCmLeft;
  } 

  // Validate Measure and Calculate distance
  if((distance_left >= (ADDITIONAL_THRESHOLD_MAX + distanceAverageCmLeft)) && (check_measure == true)) {
    Serial.print("oVR: distance_left, :");
    Serial.print(distance_left);
    return distanceAverageCmLeft;
  }
  else {
    Serial.print("nVR: ");
    return distance_left;
  }
}

/* -----------------------------------------------------------------------------
-- Measure and Validate Distance Right
----------------------------------------------------------------------------- */
float dst_measure_right(boolean check_measure)
{
  float distance_right = 0;
  VL53L0X_RangingMeasurementData_t measure;

  lox2.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    //Serial.print("Distance (mm): ");
    //Serial.println(measure.RangeMilliMeter);
    distance_right = measure.RangeMilliMeter;
  } else {
    Serial.println(" out of range ");
    return distanceAverageCmRight;
  } 

  // Validate Measure and Calculate distance
  if((distance_right >= (ADDITIONAL_THRESHOLD_MAX + distanceAverageCmRight)) && (check_measure == true)) {
    Serial.print("oVR: distance_right, :");
    Serial.print(distance_right);
    return distanceAverageCmRight;
  }
  else {
    Serial.print("nVR: ");
    return distance_right;
  }
}

/* -----------------------------------------------------------------------------
-- Check if someone has entered the room from Left Side
----------------------------------------------------------------------------- */
boolean check_distance_threshold_left(float distanceCm_left)
{
  if (distanceCm_left <= (distanceAverageCmLeft-OBJECT_WIDTH))
  {
    return true;
  }
  return false;
}

/* -----------------------------------------------------------------------------
-- Check if someone has entered the room from Right Side
----------------------------------------------------------------------------- */
boolean check_distance_threshold_right(float distanceCm_right)
{
  if (distanceCm_right <= (distanceAverageCmRight-OBJECT_WIDTH))
  {
    return true;
  }
  return false;
}

/* -----------------------------------------------------------------------------
-- Build continually average of measurement left
----------------------------------------------------------------------------- */
float build_average_left(float newValue)
{
  static float values_left[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static float value_sum_left = 0;
  static int next_index_left = 0;

  value_sum_left = value_sum_left - values_left[next_index_left] + newValue;
  values_left[next_index_left] = newValue;
  next_index_left = (next_index_left + 1) % 10;

  return (float)(value_sum_left / 10.0); // return average
}

/* -----------------------------------------------------------------------------
-- Build continually average of measurement right
----------------------------------------------------------------------------- */
float build_average_right(float newValue)
{
  static float values_right[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static float value_sum_right = 0;
  static int next_index_right = 0;

  value_sum_right = value_sum_right - values_right[next_index_right] + newValue;
  values_right[next_index_right] = newValue;
  next_index_right = (next_index_right + 1) % 10;

  return (float)(value_sum_right / 10.0); // return average
}

/* ==========================================================================
== Setup
========================================================================== */
void setup()
{
  Serial.begin(115200);
  while (! Serial) {delay(1);} // wait until serial port opens for native USB devices
  
  // Sensor
  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  
  Serial.println(F("Shutdown pins inited..."));
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  Serial.println(F("Both in reset mode...(pins are low)"));
  Serial.println(F("Starting..."));
  setID();

  //lox1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
  //lox2.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);

  // Average Links und Rechts bilden
  for(int msg_cnt=0; msg_cnt<10 ; msg_cnt++) {
    distanceAverageCmLeft = build_average_left(dst_measure_left(false));
    delay(50);
    distanceAverageCmRight = build_average_right(dst_measure_right(false));
    delay(50);
    Serial.println("");
  }

  Serial.print("Init distanceAverageCmLeft: ");
  Serial.println(distanceAverageCmLeft);
  Serial.print("Init distanceAverageCmRight: ");
  Serial.println(distanceAverageCmRight);
  //publish_state(0);
}

/* ==========================================================================
== Loop
========================================================================== */
//void update()
void loop()
{
  static float distanceCmRight = 0;
  static float distanceCmLeft = 0;
  
  /* ------------------------------------------------------------------------------
  -- Measure Left
  ------------------------------------------------------------------------------ -*/
  distanceCmLeft = dst_measure_left(true);
  Serial.print(distanceCmLeft);
  if (check_distance_threshold_left(distanceCmLeft))
  {
    Serial.print("; Detected Left; ");
    for (int msr_cnt = 0; msr_cnt < 10; msr_cnt++)
    {
      distanceCmRight = dst_measure_right(true);
      if (check_distance_threshold_right(distanceCmRight))
      {
        // Person exits the room
        if (person_cnt > 0)
        {
          person_cnt--;
        }
        Serial.println("");
        Serial.print(person_cnt);
        Serial.println(" Person im Raum");
        //publish_state(person_cnt);
        delay(3000);
        break;
      }
      delay(100);
    }
  }
  else
  {
    // build average left
    distanceAverageCmLeft = build_average_left(distanceCmLeft);
  }
  Serial.print(" AL: ");
  Serial.println((float)(distanceAverageCmLeft));

  /* ------------------------------------------------------------------------------
  -- Measure Right
  -------------------------------------------------------------------------------- */
  distanceCmRight = dst_measure_right(true);
  Serial.print(distanceCmRight);
  if (check_distance_threshold_right(distanceCmRight))
  {
    Serial.println("Detected Right");
    for (int msr_cnt = 0; msr_cnt < 10; msr_cnt++)
    {
      distanceCmLeft = dst_measure_left(true);
      if (check_distance_threshold_left(distanceCmLeft))
      {
        // Person entered the room
        person_cnt++;
        Serial.println("");
        Serial.print(person_cnt);
        Serial.println(" Person im Raum");
        //publish_state(person_cnt);
        delay(3000);
        break;
      }
      delay(100);
    }
  }
  else
  {
    // build average right
    distanceAverageCmRight = build_average_right(distanceCmRight);
  }
  Serial.print(" AR: ");
  Serial.println((float)(distanceAverageCmRight));

  /* ------------------------------------------------------------------------------
  -- delay
  -------------------------------------------------------------------------------- */
  delay(200);
}