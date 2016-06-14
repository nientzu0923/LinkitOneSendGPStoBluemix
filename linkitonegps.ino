/*
   MQTT IOT Example : LinkIt ONE board to IBM Bluemix registered user account
 - continuously obtains values from the Virtuabotix DHT11 temperature/humidity sensor
 - formats the results as a JSON string for the IBM IOT example
 - connects to an MQTT server (either local or at the IBM IOT Cloud)
 - and publishes the JSON String to the topic named quickstart:MY_MAC_ADDRESS
 */

#include <SPI.h>
#include <LGPS.h>
#include <LWiFi.h>
#include <LWiFiClient.h>

/* PubSubClient has been taken from https://github.com/knolleary/pubsubclient/tree/master/PubSubClient */
#include <PubSubClient.h>




/* MOdification Needed : 
   Enter your WIFI Access Credentials. 
*/
#define WIFI_AP "wifi帳號" //wifi帳號
#define WIFI_PASSWORD "wifi密碼" //wifi密碼

#define WIFI_AUTH LWIFI_WPA  // 選擇加密方式 LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.


/* Please modify this part
   Enter your IBM Bluemix details here.
*/

char servername[] = "{組織ID}.messaging.internetofthings.ibmcloud.com";  // Enter Org Id here. 666666 is the dummy irg Id here

char clientName[] = "d:{組織ID}:{裝置類型}:{裝置ID}" ;  // Enter Org Id (666666), Device Type(LinkItONE) & Device Mac address (aabbccddeeff)  Format is : d:orgId:DeviceType:DeviceId


char password[]  ="12345678"; // 鑑別記號 :your password for the device you created in Bluemix
//pzd3RVMdOAjR)uVFvY
char topicName[] = "iot-2/evt/status/fmt/json"; // No Change needed here.
char username[]  ="use-token-auth"; // No Change needed here.

/* If you are using mosquitto set up to test MQTT locally, you enter your local IP address here.*/
char localserver[] = "192.168.1.196";


unsigned long time = 0;
  

LWiFiClient wifiClient;

// Uncomment this next line and comment out the line after it to test against a local MQTT  
//PubSubClient client(localserver, 1883, 0, wifiClient);

PubSubClient client(servername, 1883, 0, wifiClient);
//for GPS
gpsSentenceInfoStruct info;
char buff[256];
float latitude;
float longitude;
static unsigned char getComma(unsigned char num,const char *str)
{
  unsigned char i,j = 0;
  int len=strlen(str);
  for(i = 0;i < len;i ++)
  {
     if(str[i] == ',')
      j++;
     if(j == num)
      return i + 1; 
  }
  return 0; 
}

static double getDoubleNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atof(buf);
  return rev; 
}

static double getIntNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atoi(buf);
  return rev; 
}

void parseGPGGA(const char* GPGGAstr)
{
  /* Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
   * Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
   * Where:
   *  GGA          Global Positioning System Fix Data
   *  123519       Fix taken at 12:35:19 UTC
   *  4807.038,N   Latitude 48 deg 07.038' N
   *  01131.000,E  Longitude 11 deg 31.000' E
   *  1            Fix quality: 0 = invalid
   *                            1 = GPS fix (SPS)
   *                            2 = DGPS fix
   *                            3 = PPS fix
   *                            4 = Real Time Kinematic
   *                            5 = Float RTK
   *                            6 = estimated (dead reckoning) (2.3 feature)
   *                            7 = Manual input mode
   *                            8 = Simulation mode
   *  08           Number of satellites being tracked
   *  0.9          Horizontal dilution of position
   *  545.4,M      Altitude, Meters, above mean sea level
   *  46.9,M       Height of geoid (mean sea level) above WGS84
   *                   ellipsoid
   *  (empty field) time in seconds since last DGPS update
   *  (empty field) DGPS station ID number
   *  *47          the checksum data, always begins with *
   */
 
    
  int tmp, hour, minute, second, num ;
  if(GPGGAstr[0] == '$')
  {
    tmp = getComma(1, GPGGAstr);
    hour     = (GPGGAstr[tmp + 0] - '0') * 10 + (GPGGAstr[tmp + 1] - '0');
    minute   = (GPGGAstr[tmp + 2] - '0') * 10 + (GPGGAstr[tmp + 3] - '0');
    second    = (GPGGAstr[tmp + 4] - '0') * 10 + (GPGGAstr[tmp + 5] - '0');
    
    sprintf(buff, "UTC timer %2d-%2d-%2d", hour, minute, second);
    Serial.println(buff);
    
    
    tmp = getComma(2, GPGGAstr);
    latitude = getDoubleNumber(&GPGGAstr[tmp]);
    latitude/=100;
    tmp = getComma(4, GPGGAstr);
    longitude = getDoubleNumber(&GPGGAstr[tmp]);
    longitude/=100;
    sprintf(buff, "latitude = %f, longitude = %f", latitude, longitude);
    Serial.println(buff); 
    // only print
    //Serial.println("latitude = %10.4f",latitude);
    //Serial.println("longitude = %10.4f",longitude);
    
    
    tmp = getComma(7, GPGGAstr);
    num = getIntNumber(&GPGGAstr[tmp]);    
    sprintf(buff, "satellites number = %d", num);
    Serial.println(buff); 
  }
  else
  {
    Serial.println("Not get data"); 
  }
}


void setup()
{
  Serial.begin(9600);
  /* Initialise and connect to WIFI access point */   
  InitLWiFi();
  LGPS.powerOn();
  Serial.println("LGPS Power on, and waiting ..."); 
  delay(3000);

}

void loop()
{
  
  LGPS.getData(&info);
  parseGPGGA((const char*)info.GPGGA);
  //getData();
  
  if (!client.connected()) {
    Serial.print("Trying to connect to: ");
    Serial.println(clientName);
    client.connect(clientName,username,password);
  }
  delay(2000);
  if (client.connected() ) {
    
    Serial.println("Connected !!: ");
    String json = buildJson();
    char jsonStr[200];
    json.toCharArray(jsonStr,200);
    Serial.println("publishing... !!: ");
    boolean pubresult = client.publish(topicName,jsonStr);
    Serial.print("attempt to send ");
    Serial.println(jsonStr);
    Serial.print("to ");
    Serial.println(topicName);
    if (pubresult)
      Serial.println("successfully sent");
    else
      Serial.println("unsuccessfully sent");
  }
  else 
    Serial.println("NOT Connected !!: connect FAILED ");
  
  /* Wait for some more time */
  delay(5000);
}

/* Build the jason content with temperature and humidity values */
/*String buildJson() {
  String data = "{";
  data+="\n";
  data+= "\"d\": {";
  data+="\n";
  data+="\"myName\": \"LinkIt ONE\",";
  data+="\n";
  data+="\"temperature (F)\": ";
  data+=(int)tempF;
  data+= ",";
  data+="\n";
  data+="\"temperature (C)\": ";
  data+=(int)tempC;
  data+= ",";
  data+="\n";
  data+="\"humidity\": ";
  data+=(int)humidity;
  data+="\n";
  data+="}";
  data+="\n";
  data+="}";
  return data;
}*/
String buildJson() {  
  String data = "{";
  data+="\n";
  data+= "\"d\": {";
  data+="\n";
  data+="\"myName\": \"LinkIt ONE GPS\",";
  data+="\n";
  data+="\"latitude\": ";
  data+=latitude;
  data+= ",";
  data+="\n";
  data+="\"longitude\": ";
  data+=longitude;
  data+="\n";
  data+="}";
  data+="\n";
  data+="}";
  return data;
}


/* Initialise LinkIt ONE Wifi Module and connect to Wifi AP */
void InitLWiFi()
{
  LWiFi.begin();
  

  // keep retrying until connected to AP
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    delay(1000);
  }
  Serial.println("Connected to AP");
}


