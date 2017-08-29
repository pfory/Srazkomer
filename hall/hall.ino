const int ledPin = 13;
const int A0Pin = A0;
#define D0Pin 8
// initialize the library with the numbers of the interface pins

void setup()
{
  pinMode(ledPin,OUTPUT);
  pinMode(D0Pin, INPUT);
  Serial.begin(9600);
}

void loop()
{
  int analogVal = analogRead(A0Pin);
  int digitalVal = digitalRead(D0Pin);
  Serial.print("Analog Value:");
  Serial.println(analogVal);
  Serial.print("Digital Value:");
  Serial.println(digitalVal);
  delay(500);
}