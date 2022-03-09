#include <Arduino.h>

/* ==========================================================================
== Constants
========================================================================== */
// Hardware In-Outputs constants
const int TRIG_PIN_1 = 14;
const int ECHO_PIN_1 = 12;
const int TRIG_PIN_2 = 5;
const int ECHO_PIN_2 = 4;

// math constants
const float  SOUND_SPEED = 0.034;
const float  CM_TO_INCH = 0.393701;
const int  ADDITIONAL_THRESHOLD_MAX = 20;

// Measure threshold constants
const int OBJECT_WIDTH = 20; // Körperbreite

/* ==========================================================================
== Variable
========================================================================== */
float distanceCm, distanceAverageCmRight, distanceAverageCmLeft;
int person_cnt = 0;

/* ==========================================================================
== Functions
========================================================================== */

/* -----------------------------------------------------------------------------
-- Measure and Validate Distance Left
----------------------------------------------------------------------------- */
float dst_measure_left(boolean check_measure)
{
  long duration_left=0;
  float distance_left = 0;

  noInterrupts();
  // Clears the TRIG_PIN_1
  digitalWrite(TRIG_PIN_1, LOW);
  delayMicroseconds(2);

  // Sets the TRIG_PIN_1 on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN_1, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_1, LOW);

  // Reads the ECHO_PIN_1, returns the sound wave travel time in microseconds
  duration_left = pulseIn(ECHO_PIN_1, HIGH);
  distance_left = ((duration_left * SOUND_SPEED / 2) * CM_TO_INCH);
  interrupts();

  // Validate Measure and Calculate distance
  if((distance_left >= (ADDITIONAL_THRESHOLD_MAX + distanceAverageCmLeft)) && (check_measure == true)) {
    Serial.print("oVL: ");
    return distanceAverageCmLeft;
  }
  else {
    Serial.print("nVL: ");
    return distance_left;
  }
}

/* -----------------------------------------------------------------------------
-- Measure and Validate Distance Right
----------------------------------------------------------------------------- */
float dst_measure_right(boolean check_measure)
{
  long duration_right = 0;
  float distance_right = 0;

  noInterrupts();
  // Clears the TRIG_PIN_2
  digitalWrite(TRIG_PIN_2, LOW);
  delayMicroseconds(2);

  // Sets the TRIG_PIN_2 on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN_2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_2, LOW);

  // Reads the ECHO_PIN_2, returns the sound wave travel time in microseconds
  duration_right = pulseIn(ECHO_PIN_2, HIGH);
  distance_right = ((duration_right * SOUND_SPEED / 2) * CM_TO_INCH);
  interrupts(); 

  // Validate Measure and Calculate distance
  if((distance_right >= (ADDITIONAL_THRESHOLD_MAX + distanceAverageCmRight)) && (check_measure == true)) {
    Serial.print("oVR: ");
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
  Serial.begin(9600);

  pinMode(TRIG_PIN_1, OUTPUT); // Sets the TRIG_PIN_1 as an Output
  pinMode(ECHO_PIN_1, INPUT);  // Sets the ECHO_PIN_1 as an Input

  pinMode(TRIG_PIN_2, OUTPUT); // Sets the TRIG_PIN_2 as an Output
  pinMode(ECHO_PIN_2, INPUT);  // Sets the ECHO_PIN_2 as an Input


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
  delay(600);
}