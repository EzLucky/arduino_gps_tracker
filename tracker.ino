/***************************************************
This is an example for our Adafruit FONA Cellular Module

Designed specifically to work with the Adafruit FONA
----> http://www.adafruit.com/products/1946
----> http://www.adafruit.com/products/1963
----> http://www.adafruit.com/products/2468
----> http://www.adafruit.com/products/2542

These cellular modules use TTL Serial to communicate, 2 pins are
required to interface
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, all text above must be included in any redistribution
****************************************************/

#include "Adafruit_FONA.h"
#include <AES.h>
#include <Crypto.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define DEBUG 0

//La structure ou l'on stocke les données à chiffrer
struct TestVector
{
    //Le nom de l'algo de chiffrement, pour faire joli
    const char * name;
    //La clef de chiffrement
    uint8_t key[16];
    //Le texte à chiffrer
    uint8_t plaintext[16];
    //Le texte chiffré
    uint8_t ciphertext[16];
};


static TestVector testVectorAES128 = {
    .name        = "AES-128-ECB",
    //La clef de chiffrement, la même que sur le serveur, ou que sur n'importe quel autre traceur
    .key         = {'c','h', 'o', 's',
                    'e', 'a', 'k', 'e',
                    'y', 't', 'o', 'o',
                    'n', 'i', 'c', 'e'},
    //De base on stoke l'ID de l'arduino
    .plaintext   = {'c', 'h', 'o', 's',
                    'e', 'a', 'n', 'i',
                    'd', '!'}
};
AES128 aes128;

char replybuffer[150];
char finalData[70];
char * buffer = (char *) calloc(sizeof(char), 23);
char gpsData[100];
char x; //pour le sallage

//L'ID unique de l'arduino, doit obligatoirement commencer par TRACK
char id[12] = "choseanid!";
//Le code PIN de la carte SIM
char simCode[5] = "0000";
//L'adresse ou vont les requêtes GET
char website[34] = "http://server.com/request/get";
//Celle ou vont les requêtes POST
char website2[37] = "http://your/server/post";
//boucle = l'intervalle en minutes au bout duquel on doit envoyer des données
//mode = le mode de fonctionnement de la carte
uint8_t boucle = 1, mode = 2;
//Le numéro de téléphone autorisé = celui duquel on attend des SMS et auquel on répond
char phoneNumber[13] = "+33000000000";
unsigned long previousMillis = 0;
char delimiter[2] = ",";

#include <SoftwareSerial.h>

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial * fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

/**
  Chiffrement des données
  @param cipher un pointeur sur un objet BlockCipher
  @param test un pointeur sur la structure contenant les données à chiffrer et le buffer ou stocker les données une fois chiffrées
*/
void testCipher(BlockCipher *cipher, const struct TestVector *test)
{
    cipher->setKey(test->key, cipher->keySize());
    cipher->encryptBlock(test->ciphertext, test->plaintext);
    #ifdef DEBUG
    for(int i = 0; i < 16; i++){
        Serial.print(test->ciphertext[i], HEX);
    }
    Serial.println();
    #endif
}

/**
  Remplie le buffer plaintext de la varaible testVectorAES128 pour procéder au chiffrement
  @param coords pointeur sur la chaine de caractères contenant les données
*/
void fillCipher(char * coords){
    uint8_t i;
    #ifdef DEBUG
    Serial.println(strlen(coords));
    #endif
    //On recopie les coordonnées dans le buffer en clair
    for(i = 0; i < strlen(coords) && i < 16; i++){
        testVectorAES128.plaintext[i] = coords[i];
    }
    //S'il reste de la place, on rajoute des \0 pour éviter les caractères indésirables
    for( ; i < 16; i++){
        testVectorAES128.plaintext[i] = '\0';
    }
    #ifdef DEBUG
    Serial.println((char *) testVectorAES128.plaintext);
    #endif
}

/**
  Active ou désactive le GPS
  @param activate booleen true = activer, false = désactiver
*/
void GPS(boolean activate){
    if (!fona.enableGPS(activate)){
        #ifdef DEBUG
        Serial.println("GPS failed" + activate);
        #endif
    }
}

/**
  Active ou désactive le GPRS
  @param activate booleen true = activer, false = désactiver
*/
void GPRS(boolean activate){
    if (!fona.enableGPRS(activate)){
        #ifdef DEBUG
        Serial.println("GPRS failed" + activate);
        #endif
    }
}

/**
  Récupère les coordonnées GPRS chiffrées
  @return 0 si il y a eu un problème, 1 sinon
*/
uint8_t GPRSLocation(uint8_t cipher){
    uint8_t status = fona.GPRSstate();
    uint16_t returncode;
    //On récupère les coordonnées GPRS, et si on a pas de problème on continue
    if(status != -1 && fona.getGSMLoc(&returncode, replybuffer, 150)){
        //Si le GPRS n'a pas renvoyé de données
        if (returncode != 0){
            return 0;
        //Sinon
        } else {
            //On récupère ce qui nous intéresse
            char * longitude = strtok(replybuffer, delimiter);
            char * latitude = strtok(NULL, delimiter);
            //Ainsi que l'heure locale
            fona.getTime(buffer, 23);
            buffer++;
            if(cipher == 1){
                //On génère un sel aléatoire
                x = random(33,127);
                //On recopie la latitude dans le buffer gpsData (pour avoir le \0 à la fin)
                sprintf(gpsData, "%s%c", latitude, x);
                //On remplie le buffer de la structure
                fillCipher(gpsData);
                //On chiffre
                testCipher(&aes128, &testVectorAES128);

                //On recopie en hexa
                char cipher2[33];
                uint8_t i;
                for(i = 0; i < 16; i++){
                    sprintf(&cipher2[i*2], "%02x", testVectorAES128.ciphertext[i]);
                }

                //On génère un sel aléatoire
                x = random(33,127);
                //Pareil avec la longitude
                sprintf(gpsData, "%s%c", longitude, x);
                fillCipher(gpsData);
                testCipher(&aes128, &testVectorAES128);

                //On recopie les données chiffrées de la latitude dans le buffer gpsData
                //La longitude est déjà chiffrée dans le buffer de la structure
                sprintf(gpsData, "%s,", cipher2);

                //On recopie la longitude en hexa
                for(i = 0; i < 16; i++){
                    sprintf(&cipher2[i*2], "%02x", testVectorAES128.ciphertext[i]);
                }

                //On l'ajoute à la suite de la latitude
                sprintf(gpsData, "%s%s,", gpsData, cipher2);
                //On concatène le tout avec la date
                strncat(gpsData, buffer, 17);
            } else {
              sprintf(gpsData, "%s,%s,", latitude, longitude);
              strncat(gpsData, buffer, 17);
            }
            memset(buffer, '\0', 23);
            return 1;
        }
    } else {
        GPRS(true);
        return 0;
    }
}

/**
  Récupère les coordonnées GPS chiffrées
  @return 0 si il y a eu un problème, 1 sinon
*/
uint8_t GPSLocation(uint8_t cipher){
    int8_t status = fona.GPSstatus();
    //Les codes de retour ne sont pas tous très clair dans la librairie
    //Mais on sait que si c'est différent de 3, ça ne fonctionnera pas
    if(status != 3){
        if(status == 0){
            GPS(true);
        }
        return 0;
    //Si on a un fix (2D ou 3D, il n'y a pas de différence dans la librairie à l'heure actuelle)
    } else {
        //On récupère les données GPS (altitude, longitute, latitude, heure, vitesse ...)
        fona.getGPS(0, replybuffer, 150);
        strtok(replybuffer, delimiter);
        //Si on a pas eu de problème, donc on a un FIX et des données
        if(atoi(strtok(NULL, delimiter)) == 1){
            strtok(NULL, delimiter);
            //On récupère ce qui nous intéresse
            char * latitude = strtok(NULL, delimiter);
            char * longitude = strtok(NULL, delimiter);
            //Ainsi que l'heure locale
            fona.getTime(buffer, 23);
            //Dont on supprime le premier " pour le passer dans une chaine sans problème
            buffer++;
            if(cipher == 1){
                //On génère un sel aléatoire
                x = random(33,127);
                //On recopie la latitude dans le buffer gpsData (pour avoir le \0 à la fin)
                sprintf(gpsData, "%s%c", latitude, x);
                //On remplie le buffer de la structure
                fillCipher(gpsData);
                //On chiffre
                testCipher(&aes128, &testVectorAES128);

                //On recopie en hexa
                char cipher2[33];
                uint8_t i;
                for(i = 0; i < 16; i++){
                    sprintf(&cipher2[i*2], "%02x", testVectorAES128.ciphertext[i]);
                }

                //On génère un sel aléatoire
                x = random(33,127);
                //Pareil avec la longitude
                sprintf(gpsData, "%s%c", longitude, x);
                fillCipher(gpsData);
                testCipher(&aes128, &testVectorAES128);

                //On recopie les données chiffrées de la latitude dans le buffer gpsData
                //La longitude est déjà chiffrée dans le buffer de la structure
                sprintf(gpsData, "%s,", cipher2);

                //On recopie la longitude en hexa
                for(i = 0; i < 16; i++){
                    sprintf(&cipher2[i*2], "%02x", testVectorAES128.ciphertext[i]);
                }

                //On l'ajoute à la suite de la latitude
                sprintf(gpsData, "%s%s,", gpsData, cipher2);

                //On concatène le tout avec la date
                strncat(gpsData, buffer, 17);
            } else {
              sprintf(gpsData, "%s,%s,", longitude, latitude);
              strncat(gpsData, buffer, 17);
            }
            #ifdef DEBUG
            Serial.println("Buffer GPS");
            Serial.println(gpsData);
            #endif

            memset(buffer, '\0', 23);
            return 1;
        }
    }
}

/**
  Récupère les coordonnées GPS
  Choisit en priorité le GPS, et sinon le GPRS
  @return les coordonnées GPS si on en a, sinon "NoLocation"
*/
char * getLocation(uint8_t cipher){
    if(GPSLocation(cipher) == 1){
        return gpsData;
    } else {
        if (GPRSLocation(cipher) == 1){
            return gpsData;
        }
    }
    return (char *) "NoLocation";
}

/**
  Envoyer une requête POST au serveur
  @param data les données à envoyer au site en POST (uniquement les données)
*/
void postToWebsite(char * data){
    uint16_t statuscode;
    int16_t length;

    uint8_t dataLength = strlen(data);

    #ifdef DEBUG
    Serial.println(data);
    Serial.println(F("****"));
    #endif

    if (!fona.HTTP_POST_start(website2, F("application/x-www-form-urlencoded"), (uint8_t *) data, dataLength, &statuscode, (uint16_t *) &length)) {
        #ifdef DEBUG
        Serial.println("Failed!");
        #endif
    } else {
        while (length > 0) {
            while (fona.available()) {
                char c = fona.read();
                #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
                loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
                UDR0 = c;
                #else
                Serial.write(c);
                #endif

                length--;
                if (! length) break;
            }
        }
        #ifdef DEBUG
        Serial.println(F("\n****"));
        #endif
        fona.HTTP_POST_end();
    }
}

/**
  Envoyer une requête GET au server
  @param data les données à envoyer en GET (url + paramètres)
*/
void getToWebsite(char * data){
    uint16_t statuscode;
    sprintf(replybuffer, "%s%s", website, data);
    #ifdef DEBUG
    Serial.println(replybuffer);
    #endif
    uint16_t dataLength = strlen(replybuffer);
    if(!fona.HTTP_GET_start(replybuffer, &statuscode, &dataLength)) {
        #ifdef DEBUG
        Serial.println("Failed!");
        #endif
    }
    #ifdef DEBUG
    Serial.println(F("\n****"));
    #endif
    fona.HTTP_GET_end();
}

/**
  Le code qui est exécuté une seul fois au démarrage de l'arduino
*/
void setup() {
    while (!Serial);
    Serial.begin(115200);

    #ifdef DEBUG
    Serial.println(F("Initializing...."));
    #endif

    //On regarde si on a accès au FONA
    fonaSerial->begin(4800);
    if (! fona.begin(*fonaSerial)) {
        #ifdef DEBUG
        Serial.println(F("FONA not found"));
        #endif
        while (1);
    }

    //On déverrouille la SIM
    if (! fona.unlockSIM(simCode)) {
        #ifdef DEBUG
        Serial.println(F("SIMFailed"));
        #endif
    }

    //Et on active la synchronisation réseau
    //Les GPS et GSM retourne le temps UTC, pas le temps local
    if (!fona.enableNetworkTimeSync(true)){
        #ifdef DEBUG
        Serial.println(F("NetworkFailed"));
        #endif
    }

    //On initialise le générateur de nombres aléatoires
    randomSeed(analogRead(3));

    //On attend 30sec pour s'assurer que l'initilisation de la SIM est faite
    //Et on crois les doigts pour que ce délais soit suffisant pour avoir un fix GPS
    unsigned long currentMillis = millis();
    while (millis() < currentMillis + 30000);

    //On désactive les interruption lorsque l'on reçoit un SMS ou un appel
    fona.setSMSInterrupt(0);
    fona.callerIdNotification(false);

    //On active le GPS et le GPRS (pour envoyer les données sur le serveur)
    GPS(true);
    GPRS(true);
}

/**
  Le code qui sera exécuté en boucle
*/
void loop() {
    //On récupère le nombre de SMS sur la SIM
    uint8_t current = fona.getNumSMS();
    //S'il y en a, on lit le dernier, puis on supprime tout
    if(current > 0) {
        uint16_t smslen;
        //On lit le dernier et on récupère l'expéditeur
        fona.readSMS(current, replybuffer, 150, &smslen);
        fona.getSMSSender(current, finalData, 12);
        //On supprime ce qu'il y a après le dernier chiffre du numéro
        finalData[12] = '\0';
        //Si le numéro de l'expéditeur est celui autorisé
        if(strstr(finalData, phoneNumber) != NULL){
            //Si c'est une demande de localisation
            if(strstr(replybuffer, "LocationRequest")) {
                //On envoie la position au numéro autorisé
                sprintf(replybuffer, "%s", getLocation(0));
                fona.sendSMS(phoneNumber, replybuffer);
            //Si c'est une mise à jour des paramètres
            } else if(strstr(replybuffer, "ParametersUpdate")){
                //On récupère les nouvelles valeurs
                strtok(replybuffer, delimiter);
                //Le mode de fonctionnement de la carte
                mode = atoi(strtok(NULL, delimiter));
                //Le nombre de minutes à attendre entre chaque envoie
                boucle = atoi(strtok(NULL, delimiter));
            //Si c'est une demande de mise sur écoute
            } else if(strstr(replybuffer, "CallRequest")){
                //On appelle le numéro autorisé
                fona.callPhone(phoneNumber);
            //Si on veut économiser de la batterie
            //On coupe le GPS et le GPRS pour économiser la batterie
            //Et on passe le mode à 0 pour uniquement attendre la réception d'un SMS
            //A utiliser avec précautions car le WakeUp est instable
            //Le GPRS a beaucoup de mal à se réactiver, il vaut mieux passer la variable boucle à une valeur plus grande
            } else if(strstr(replybuffer, "Sleep")){
                GPS(false);
                GPRS(false);
                mode = 0;
            //Si on veut repasser en mode normal
            //On réactive tout
            //On repasse le mode à 2 (= envoi sur le serveur à intervalle régulier)
            //On envoie un relevé de position toutes les minutes
            } else if(strstr(replybuffer, "Wakeup")){
                GPS(true);
                GPRS(true);
                mode = 2;
                boucle = 1;
            }
        }
        //On supprime tout les SMS (on ne prend en compte que le dernier reçu à chaque fois)
        for(uint8_t i = 1; i <= 90; i++){
            delay(10);
            fona.deleteSMS(i);
        }
    }

    //Toutes les x minutes, on va envoyer des données (avec x = boucle)
    //(dans le cas ou on est en mode 1 ou 2)
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= (unsigned long) boucle * 1000 * 20 && mode != 0) {
        previousMillis = currentMillis;
        uint16_t vbat;
        //On récupère le pourcentage de batterie
        fona.getBattPercent(&vbat);
        //Si la batterie est faible, on notifie le numéro autorisé
        if(vbat < 10){
            fona.sendSMS(phoneNumber, (char *) "LowBattery");
        }
        //Si on est en envoi de SMS
        if (mode == 1) {
            //On récupère la position et on l'envoie au numéro autorisé
            sprintf(replybuffer, "%s", getLocation(0));
            fona.sendSMS(phoneNumber, replybuffer);
        //Si on est en envoi au serveur (et donc on log les positions)
        } else if (mode == 2) {
            //On génère un sel aléatoire
            x = random(33,127);
            id[10] = x;
            //On remplit le buffer qui va servir à chiffrer
            fillCipher(id);
            //On chiffre l'identifiant (unique) de l'arduino
            testCipher(&aes128, &testVectorAES128);
            //On recopie le buffer chiffré dans un autre buffer en hexa
            char cipher[33];
            for(uint8_t i = 0; i < 16; i++){
                sprintf(&cipher[i*2], "%02x", testVectorAES128.ciphertext[i]);
            }
            //On envoie au serveur un requête GET pour notifier que l'arduino est up
            sprintf(finalData, "?botid=%s&sysinfo=arduinoUnoFona808V2", cipher);
            getToWebsite(finalData);

            //Puis on envoie les données de géolocalisation
            sprintf(replybuffer, "botid=%s&output=%s", cipher, getLocation(1));
            postToWebsite(replybuffer);
        }
    }
    while (fona.available()) {
        Serial.write(fona.read());
    }
}