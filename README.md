#TODOOOOOOOOOOOOOOOOOOOOOOOOOO



Mise en place d’un géotraceur

Environnement de développement	1
Prérequis	1
Configuration du géotraceur	1
Fonctions disponibles	2
Explication du code	2

Environnement de développement
Afin de pouvoir modifier ce projet, il vous sera nécessaire d’avoir un environnement de développement conforme au HowToDevForArduino.
Une fois ceci fait, il vous faudra installer les bibliothèques suivantes :
https://github.com/rweather/arduinolibs (ne pas hésiter à aller dans la documentation)
https://github.com/adafruit/Adafruit_FONA (version 1.3.2)
En ce qui concerne la bibliothèque FONA, certaines fonctions sont en béta à l’heure ou ces lignes sont écrites, il est donc possible que les fonctions HTTP_POST_* ou HTTP_GET_* ne soient plus exactement les mêmes.

Prérequis
Une carte SIM plein format. Une antenne GPS passive, connectique uFL, une antenne GSM, connectique uFL aussi, et une batterie de 1200mAh maximum (la carte ne pouvant recharger plus), avec une connectique JST 2 pins.

Configuration du géotraceur
Afin de limiter les actions possibles dans un cas réel d’utilisation, seul un unique numéro de téléphone peut envoyer des commandes au géotraceur et recevoir une réponse. Avant tout usage il faut donc changer la valeur de la variable phoneNumber, par le numéro désiré, sous le format +33xxxxxxxxx, en omettant le premier 0 de votre numéro.
Renseignez aussi le code de déverrouillage de la carte SIM que vous avez inséré dans la carte FONA, dans la variable simCode, sous le format xxxx.

Fonctions disponibles
Le géotraceur fonctionne à partir de 3 modes.
0 - Envoi à la demande des données de localisation sur le téléphone autorisé.
1 - Envoi automatique des données de localisation sur le téléphone autorisé.
2 - Envoi automatique des données de localisation chiffrées sur un serveur Wares (cf le HowTo pour cette partie).
Par défaut, le géotraceur envoie automatiquement sa position à un serveur Wares, toutes les minutes.
Pour effectuer une action sur le géotraceur, il suffit d’envoyer le SMS correspondant (en italique ci-dessous).
Mise sur écoute à la demande (appelle le téléphone autorisé).
	CallRequest
Demande de la localisation actuelle.
	LocationRequest
Mise à jour du mode, et de l’intervalle d’envoi via le téléphone autorisé.
	ParametersUpdate,x,y
	Avec x le mode désiré, et y l’intervalle d’envoi en minute, si mode vaut 1 ou 2.

Explication du code
L’utilisation de la fonction delay() a été totalement évitée. En effet, d’après la documentation, cette dernière bloque l’exécution de tout autre processus en arrière-plan.
https://www.arduino.cc/en/reference/delay
De ce fait, nous avons opté pour des boucles avec l’utilisation de millis() qui permet de continuer à recevoir des interruptions par exemple.
Un des problèmes rencontrés est le crash de la connexion GPRS si un SMS ou un appel arrive alors que la carte essaie de se connecter à un serveur (même si les interruptions sont désactivées).
Ce problème sera peut-être corrigé dans une future version de la bibliothèque FONA (certaines fonctions étant encore en béta).
En ce qui concerne le chiffrement (AES128), la clef de chiffrement est commune à tous les arduinos, c’est-à-dire que le serveur ne prend actuellement en charge qu’une seule clef commune.
De plus, afin d’éviter des problèmes de dépassement de blocs pour chiffrer les données, elles sont toutes traitées séparément. A chaque requête GET, l’ID de l’arduino est chiffré, placé dans la requête et envoyé. Pour les requêtes POST, le nombre de caractères pour la latitude et la longitude peuvent variés. Ces deux valeurs sont donc chiffrées l’une après l’autre pour éviter les débordements, placées dans les paramètres de la requête et envoyées. 
Pour plus d’informations, le fichier source est commenté.
