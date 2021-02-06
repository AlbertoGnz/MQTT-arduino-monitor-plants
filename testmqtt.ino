#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"
#include "DHT.h"

#define DHTPIN 8     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

//Conexión a la red wifi: nombre de la red y contraseña
#define WIFI_AP "SSIDname"
#define WIFI_PASSWORD "PASSWORDHERE"

//Nombre o IP del servidor mosquitto
char server[50] = "192.168.1.100";

char mqttbuffer[60];
const int idxdht = 4; /* must be the same as virual sensor idx in Domotocz */
float temperature = 0.0;
float humidity = 0.0;



// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

//Inicializamos el objeto de cliente esp
WiFiEspClient espClient;

//Iniciamos el objeto subscriptor del cliente 
//con el objeto del cliente
PubSubClient client(espClient);

//Conexión serial para el esp con una comunicación
//serial, pines 2: rx y 3: tx
SoftwareSerial soft(2, 3);

//Contador para el envio de datos
unsigned long lastSend;

int status = WL_IDLE_STATUS;

void setup() {
    //Inicializamos la comunicación serial para el log
    Serial.begin(9600);
    Serial.println(F("DHT11 sensor started!"));
    dht.begin();
    //Iniciamos la conexión a la red WiFi
    InitWiFi();
    //Colocamos la referencia del servidor y el puerto
    client.setServer( server, 1883 );
    lastSend = 0;
}

void loop() {
    //Validamos si el modulo WiFi aun esta conectado a la red
    status = WiFi.status();
    if(status != WL_CONNECTED) {
        //Si falla la conexión, reconectamos el modulo
        reconnectWifi();
    }

    //Validamos si esta la conexión del servidor
    if(!client.connected() ) {
        //Si falla reintentamos la conexión
        reconnectClient();
    }

    //Creamos un contador para enviar la data cada 2 segundos
    if(millis() - lastSend > 2000 ) {
        sendDataTopic();
        lastSend = millis();
    }

    client.loop();
}

void sendDataTopic()
{
    Serial.println("sendDataTopic");

    // Wait a few seconds between measurements.
    delay(2000);
  
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    int humidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    int temperature = dht.readTemperature();

    sprintf(mqttbuffer, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d;%d;0\" }", idxdht, temperature, humidity);    
    client.publish("domoticz/in", mqttbuffer);
    Serial.println(mqttbuffer);


}

//Inicializamos la conexión a la red wifi
void InitWiFi()
{
    //Inicializamos el puerto serial
    soft.begin(9600);
    //Iniciamos la conexión wifi
    WiFi.init(&soft);
    //Verificamos si se pudo realizar la conexión al wifi
    //si obtenemos un error, lo mostramos por log y denememos el programa
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("El modulo WiFi no esta presente");
        while (true);
    }
    reconnectWifi();
}

void reconnectWifi() {
    Serial.println("Iniciar conección a la red WIFI");
    while(status != WL_CONNECTED) {
        Serial.print("Intentando conectarse a WPA SSID: ");
        Serial.println(WIFI_AP);
        //Conectar a red WPA/WPA2
        status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
        delay(500);
    }
    Serial.println("Conectado a la red WIFI");
}

void reconnectClient() {
    //Creamos un loop en donde intentamos hacer la conexión
    while(!client.connected()) {
        Serial.print("Conectando a: ");
        Serial.println(server);
        //Creamos una nueva cadena de conexión para el servidor
        //e intentamos realizar la conexión nueva
        //si requiere usuario y contraseña la enviamos connect(clientId, username, password)
        if(client.connect("Arduino", "username", "PaS$Word")) {
            Serial.println("[DONE]");
        } else {
            Serial.print( "[FAILED] [ rc = " );
            Serial.print( client.state() );
            Serial.println( " : retrying in 5 seconds]" );
            delay( 5000 );
        }
    }
}
