#include <PubSubClient.h> 


WiFiClient   espClient;
PubSubClient mqtt(espClient);
const char* userID = "mqttdash-14ab1192";
const char* topic_led = "esp3oat/led";
const char* topic_capture = "esp3oat/capture";


void callback(char* topic, byte* s, unsigned int length) {
    if (strcmp(topic, "esp3oat/led") == 0) {
        digitalWrite(led, s[0] - '0');
        if (s[0] - '0' == true) donneesW.headlights = 1;
        else donneesW.headlights = 0;
    }
}

// reconnect MQTT  ------------------------------
uint16_t compteur_mqtt_connexion;
void reconnect(void) {
    compteur_mqtt_connexion = 0;
    // Boucle tant que pas connect�
    while (!mqtt.connected()) {
        compteur_mqtt_connexion++;
        digitalWrite(led_iot, HIGH);
        digitalWrite(led_iot, LOW);
        //Serial.print("En attente de connexion MQTT...");
        if (mqtt.connect(userID)) {
            //Serial.println("connect�");
            // On s'abonne aux topics
            mqtt.subscribe("esp3oat/led");
            //mqtt.subscribe("esp3oat/capture");
            mqtt.setCallback(callback);
        }    // procedure appel�e automatiquement sur abonnement
        else {
            //Serial.print("�chec, rc=");
            //Serial.print(mqtt.state());
            //delay(50);
        }
        if (compteur_mqtt_connexion == 1) break;
    }
}
