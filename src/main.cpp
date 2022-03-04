#include <Arduino.h>

// Hardware In/-Outputs
const int trigPin_1 = 14;
const int echoPin_1 = 12;
const int trigPin_2 = 5;
const int echoPin_2 = 4;

const int object_distance = 70; // Körperbreite
float distanceCm, distanceAverageCmRight, distanceAverageCmLeft;
int person_cnt = 0;

// define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
#define TOLERANZ 50 // cm

float dst_measure_left()
{
  noInterrupts();
  // Clears the trigPin_1
  digitalWrite(trigPin_1, LOW);
  delayMicroseconds(2);

  // Sets the trigPin_1 on HIGH state for 10 micro seconds
  digitalWrite(trigPin_1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_1, LOW);

  // Reads the echoPin_1, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin_1, HIGH);
  interrupts();

  // Calculate the distance
  return duration * SOUND_SPEED / 2;
}

float dst_measure_right()
{
  noInterrupts();
  // Clears the trigPin_1
  digitalWrite(trigPin_2, LOW);
  delayMicroseconds(2);

  // Sets the trigPin_1 on HIGH state for 10 micro seconds
  digitalWrite(trigPin_2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_2, LOW);

  // Reads the echoPin_1, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin_2, HIGH);
  interrupts();

  // Calculate the distance
  return duration * SOUND_SPEED / 2;
}

boolean check_distance_threshold_left(float distance_Cm)
{
  if (distance_Cm <= (distanceAverageCmLeft-object_distance))
  {
    return true;
  }
  return false;
}

boolean check_distance_threshold_right(float distance_Cm)
{
  if (distance_Cm <= (distanceAverageCmRight-object_distance))
  {
    return true;
  }
  return false;
}

float build_average_left(float newValue)
{
  static float values[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static float value_sum = 0;
  static int next_index = 0;

  value_sum = value_sum - values[next_index] + newValue;
  values[next_index] = newValue;
  next_index = (next_index + 1) % 10;

  //Serial.print("Average Left: ");
  //Serial.println((float)(value_sum / 10.0));

  return (float)(value_sum / 10.0); // return average
}

float build_average_right(float newValue)
{
  static float values[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static float value_sum = 0;
  static int next_index = 0;

  value_sum = value_sum - values[next_index] + newValue;
  values[next_index] = newValue;
  next_index = (next_index + 1) % 10;
  
  //Serial.print("Average Right: ");
  //Serial.println((float)(value_sum / 10.0));

  return (float)(value_sum / 10.0); // return average
}

void setup()
{
  Serial.begin(9600);

  pinMode(trigPin_1, OUTPUT); // Sets the trigPin_1 as an Output
  pinMode(echoPin_1, INPUT);  // Sets the echoPin_1 as an Input

  pinMode(trigPin_2, OUTPUT); // Sets the trigPin_2 as an Output
  pinMode(echoPin_2, INPUT);  // Sets the echoPin_2 as an Input
}

void loop()
{

  /* -----------------------------------------------------------------------------
  -- Critical, time-sensitive code measure distance
  ----------------------------------------------------------------------------- */
  // Measure Left
  float distanceCm = dst_measure_left();


  if (check_distance_threshold_left(distanceCm))
  {
    Serial.println("Detected Left");
    for (int msr_cnt = 0; msr_cnt < 10; msr_cnt++)
    {
      distanceCm = dst_measure_right();
      Serial.print("Distance Right: ");
      Serial.print(distanceCm);
      Serial.print(", ");
      if (check_distance_threshold_right(distanceCm))
      {
        // Person ausgetretten
        if (person_cnt > 0)
        {
          person_cnt--;
        }
        Serial.println("");
        Serial.print(person_cnt);
        Serial.println(" Person im Raum");
        delay(3000);
        break;
      }
      delay(100);
    }
  }
  else
  {
    // Average Links bilden
    distanceAverageCmLeft = build_average_left(distanceCm);
  }

  // Measure Right
  distanceCm = dst_measure_right();

  if (check_distance_threshold_right(distanceCm))
  {
    Serial.println("Detected Right");
    for (int msr_cnt = 0; msr_cnt < 10; msr_cnt++)
    {
      distanceCm = dst_measure_left();
      Serial.print("Distance Left: ");
      Serial.print(distanceCm);
      Serial.print(", ");
      if (check_distance_threshold_left(distanceCm))
      {
        // Person eingetretten
        person_cnt++;
        Serial.println("");
        Serial.print(person_cnt);
        Serial.println(" Person im Raum");
        delay(3000);
        break;
      }
      delay(100);
    }
  }
  else
  {
    // Average Rechts bilden
    distanceAverageCmRight = build_average_right(distanceCm);
  }
  delay(100);
}