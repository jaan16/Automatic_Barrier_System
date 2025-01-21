#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// Create objects for sensors
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change 0x27 to your I2C address if different

// Define stepper motor connections and motor interface type
#define dirPin 2
#define stepPin 3
#define enablePin 4 // Define the pin for enabling/disabling the driver
#define motorInterfaceType 1

// Create an instance of the stepper motor class
AccelStepper stepper(motorInterfaceType, stepPin, dirPin);

// HC-SR04 connections
const int trigPin = 7;
const int echoPin = 6;

// Define variables
long duration;
float distance;
float temperature;
bool faceMaskDetected = false;
char esp32Data;

// Gear ratio between pulleys
const float gearRatio = 3.0; // 60 teeth barrier pulley / 20 teeth motor pulley

// Steps per revolution of the stepper motor
const int stepsPerRevolution = 200 * 32; // Assuming 32 microsteps

// Function to move the barrier to a specific angle
void moveBarrierToAngle(int angle) {
  // Convert angle to steps
  float stepsPerDegree = (stepsPerRevolution * gearRatio) / 360.0;
  int targetSteps = (270 - angle) * stepsPerDegree;

  Serial.print("Moving barrier to angle: ");
  Serial.print(angle);
  Serial.print(" degrees (");
  Serial.print(targetSteps);
  Serial.println(" steps)");

  stepper.moveTo(targetSteps);
  stepper.runToPosition();

  Serial.println("Barrier move complete");
}

void setup() {
  // Start serial communication for debugging
  Serial.begin(9600);
  Serial.println("System Initializing...");

  // Initialize sensors
  mlx.begin();
  
  // Initialize LCD with 16 columns and 2 rows
  lcd.begin(16, 2);
  lcd.backlight();

  // Initialize HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set up the enable pin
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, LOW); // Enable the driver

  // Set up the stepper motor
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

  // Print initial debug information
  Serial.println("Initializing barrier system...");

  // Move the barrier to the initial position (270 degrees to 180 degrees)
  moveBarrierToAngle(180); // Move to 180 degrees initially
  Serial.println("Initialization complete. Barrier is at 180 degrees.");

  lcd.setCursor(0, 0);
  lcd.print("System Ready");
}

void loop() {
  // Measure distance using HC-SR04
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.034) / 2;

  // Measure temperature using MLX90614
  temperature = mlx.readObjectTempC();

  // Check for data from ESP32-CAM
  if (Serial.available() > 0) {
    esp32Data = Serial.read();
    if (esp32Data == '*') {
      faceMaskDetected = true;
    } else {
      faceMaskDetected = false;
    }
  }

  // Debugging prints
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("Face Mask Detected: ");
  Serial.println(faceMaskDetected ? "Yes" : "No");

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print("cm ");

  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("*C ");

  delay(1000);

  // Logic to open/close barrier
  if (distance < 50 && faceMaskDetected) {
    lcd.setCursor(0, 0);
    lcd.print("Barrier Opening  ");
    Serial.println("Barrier Opening");
    moveBarrierToAngle(130); // Open barrier to 130 degrees
    delay(3000); // Keep barrier open for 3 seconds
    lcd.setCursor(0, 0);
    lcd.print("Barrier Closing  ");
    Serial.println("Barrier Closing");
    moveBarrierToAngle(180); // Close barrier to 180 degrees
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Access Denied    ");
    Serial.println("Access Denied");
  }

  delay(500);
}
