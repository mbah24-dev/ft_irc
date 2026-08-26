User , Response, Request are implemented

Explication initializeServerSocket etape

socket()		
	Crée le socket
memset()		
	Nettoie la structured'adresse
htons()	
	Convertit le port en format réseau
INADDR_ANY	
	Écoute sur toutes les interfaces
setsockopt()	
	Permet la réutilisation du port
fcntl()	
	Rend le socket non-bloquant
bind()	
	Associe le socket à un port
listen()	
	Met le socket en écoute
pollfd	
	Structure pour surveiller les FD
POLLIN	
	Surveille les données entrantes
memset(_eventPolling) 
	Nettoyer le tableau poll
_eventPolling[0].fd
	Ajoute le serveur à poll() et 
_activeDescriptors = 1
	Initialise le compteur