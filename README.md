#TODOOOOOOOOOOOOOOOOOOOOOOOOOO



Mise en place d�un g�otraceur

Environnement de d�veloppement	1
Pr�requis	1
Configuration du g�otraceur	1
Fonctions disponibles	2
Explication du code	2

Environnement de d�veloppement
Afin de pouvoir modifier ce projet, il vous sera n�cessaire d�avoir un environnement de d�veloppement conforme au HowToDevForArduino.
Une fois ceci fait, il vous faudra installer les biblioth�ques suivantes :
https://github.com/rweather/arduinolibs (ne pas h�siter � aller dans la documentation)
https://github.com/adafruit/Adafruit_FONA (version 1.3.2)
En ce qui concerne la biblioth�que FONA, certaines fonctions sont en b�ta � l�heure ou ces lignes sont �crites, il est donc possible que les fonctions HTTP_POST_* ou HTTP_GET_* ne soient plus exactement les m�mes.

Pr�requis
Une carte SIM plein format. Une antenne GPS passive, connectique uFL, une antenne GSM, connectique uFL aussi, et une batterie de 1200mAh maximum (la carte ne pouvant recharger plus), avec une connectique JST 2 pins.

Configuration du g�otraceur
Afin de limiter les actions possibles dans un cas r�el d�utilisation, seul un unique num�ro de t�l�phone peut envoyer des commandes au g�otraceur et recevoir une r�ponse. Avant tout usage il faut donc changer la valeur de la variable phoneNumber, par le num�ro d�sir�, sous le format +33xxxxxxxxx, en omettant le premier 0 de votre num�ro.
Renseignez aussi le code de d�verrouillage de la carte SIM que vous avez ins�r� dans la carte FONA, dans la variable simCode, sous le format xxxx.

Fonctions disponibles
Le g�otraceur fonctionne � partir de 3 modes.
0 - Envoi � la demande des donn�es de localisation sur le t�l�phone autoris�.
1 - Envoi automatique des donn�es de localisation sur le t�l�phone autoris�.
2 - Envoi automatique des donn�es de localisation chiffr�es sur un serveur Wares (cf le HowTo pour cette partie).
Par d�faut, le g�otraceur envoie automatiquement sa position � un serveur Wares, toutes les minutes.
Pour effectuer une action sur le g�otraceur, il suffit d�envoyer le SMS correspondant (en italique ci-dessous).
Mise sur �coute � la demande (appelle le t�l�phone autoris�).
	CallRequest
Demande de la localisation actuelle.
	LocationRequest
Mise � jour du mode, et de l�intervalle d�envoi via le t�l�phone autoris�.
	ParametersUpdate,x,y
	Avec x le mode d�sir�, et y l�intervalle d�envoi en minute, si mode vaut 1 ou 2.

Explication du code
L�utilisation de la fonction delay() a �t� totalement �vit�e. En effet, d�apr�s la documentation, cette derni�re bloque l�ex�cution de tout autre processus en arri�re-plan.
https://www.arduino.cc/en/reference/delay
De ce fait, nous avons opt� pour des boucles avec l�utilisation de millis() qui permet de continuer � recevoir des interruptions par exemple.
Un des probl�mes rencontr�s est le crash de la connexion GPRS si un SMS ou un appel arrive alors que la carte essaie de se connecter � un serveur (m�me si les interruptions sont d�sactiv�es).
Ce probl�me sera peut-�tre corrig� dans une future version de la biblioth�que FONA (certaines fonctions �tant encore en b�ta).
En ce qui concerne le chiffrement (AES128), la clef de chiffrement est commune � tous les arduinos, c�est-�-dire que le serveur ne prend actuellement en charge qu�une seule clef commune.
De plus, afin d��viter des probl�mes de d�passement de blocs pour chiffrer les donn�es, elles sont toutes trait�es s�par�ment. A chaque requ�te GET, l�ID de l�arduino est chiffr�, plac� dans la requ�te et envoy�. Pour les requ�tes POST, le nombre de caract�res pour la latitude et la longitude peuvent vari�s. Ces deux valeurs sont donc chiffr�es l�une apr�s l�autre pour �viter les d�bordements, plac�es dans les param�tres de la requ�te et envoy�es. 
Pour plus d�informations, le fichier source est comment�.
