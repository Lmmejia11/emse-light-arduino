#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// Variables for pins
int lightSensor = A2;
int noiseSensor = A3;
int ringer = 8;
int lightButton = 6;
int ringerButton = 7;

// Variables to see state of room
String roomId = "1";
String s;
char c[50];
bool lightButtonState = 0;
bool lastLightButtonState = 0;
bool ringerButtonState = 0;
bool lastRingerButtonState = 0;
bool lightState = 0;
bool ringerState = 0;
int interval = 5000;
unsigned long previousMillis = 0; 

// Variables for mqtt
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEA };
char userId[] = "ndtzubom"; 
char passwd[] = "yGCB-nJ4kcOC"; 
char server[] = "m23.cloudmqtt.com";
unsigned int port = 14984;
char lightLevelTopic[50];
char noiseLevelTopic[50];
EthernetClient ethClient;
PubSubClient client(server, port, 0, ethClient) ;


void setup() {
  Serial.begin(9600);
  
  // Sensor pins
  pinMode(lightSensor,INPUT);
  pinMode(noiseSensor,INPUT);

  // Actuator pins
  pinMode(ringer, OUTPUT);
  pinMode(lightButton, INPUT);
  pinMode(ringerButton, INPUT);

  Ethernet.begin(mac);
  client.setCallback(callback);
  s = "bd/level/light/" + roomId; s.toCharArray(lightLevelTopic,s.length()+1);
  s = "bd/level/noise/" + roomId; s.toCharArray(noiseLevelTopic,s.length()+1);
  connectMqtt();
  
  delay(1500);
}

void connectMqtt(){
  if (client.connect(NULL,userId,passwd)) {
      Serial.println("connected");
      // resubscribe
      s = "rl/state/+/" + roomId;
      s.toCharArray(c,s.length()+1);
      client.subscribe(c);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again");
    }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    connectMqtt();
  }
}

void publishSensor(){
  int intValue = analogRead(lightSensor);
  s = String(intValue);
  s.toCharArray(c,s.length()+1);
  Serial.println(String(lightLevelTopic) + ": " + s);
  client.publish(lightLevelTopic,c);
  
  intValue = analogRead(noiseSensor);
  s = String(intValue);
  s.toCharArray(c,s.length()+1);
  Serial.println(String(noiseLevelTopic) + ": " + s);
  client.publish(noiseLevelTopic, c);
}

void publishSwitchLight(char* value){
  s = "bd/state/light/"+roomId;
  s.toCharArray(c,s.length()+1);
  Serial.println(s + ": " + value);
  client.publish(c, value);
  
  s = "rl/state/light/"+roomId;
  s.toCharArray(c,s.length()+1);
  Serial.println(s + ": " + value);
  client.publish(c, value);
}

void publishSwitchNoise(char* value){
  s = "bd/state/noise/"+roomId;
  s.toCharArray(c,s.length()+1);
  Serial.println(s + ": " + value);
  client.publish(c, value);
}

void checkLightButton(){
  lightButtonState = digitalRead(lightButton);
  if (lightButtonState != lastLightButtonState && lightButtonState){
    String message;
    lightState = !lightState;
    if (lightState){ publishSwitchLight("ON");}
    else {publishSwitchLight("OFF");}
  }
  lastLightButtonState = lightButtonState;
}

void checkRingerButton(){
  ringerButtonState = digitalRead(ringerButton);
  if (ringerButtonState != lastRingerButtonState && ringerButtonState){
    ringerState = !ringerState;
    if (ringerState){digitalWrite(ringer, HIGH); publishSwitchNoise("ON");}
    else {digitalWrite(ringer, LOW); publishSwitchNoise("OFF"); }
  }
  lastRingerButtonState = ringerButtonState;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  String sTopic = String(topic);
  if(sTopic.indexOf("noise") >= 0){
     if (message == "ON"){digitalWrite(ringer, HIGH); ringerState = true;}
     else {digitalWrite(ringer, LOW); ringerState = false;}
  } else {
     if (message == "ON"){lightState = true;}
     else {lightState = false;}
  }
}

void loop() {
  unsigned long currentMillis = millis();
  checkRingerButton();
  checkLightButton();
  if (currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;

    if (!client.connected()) {
        reconnect();
    }
    publishSensor();
  } 
  client.loop();
}

