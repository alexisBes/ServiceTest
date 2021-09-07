# Service window basique
## projet de test pour un service window en c/c++
Le nom du service est "MonService", pour le moment il ne fait rien et je m'en sers juste pour tester.
## Source et tutoriel
Pour faire ce service je me suis basé sur 
https://docs.microsoft.com/en-us/windows/win32/services/services  

## Présentation des projets
Le service est divisé en 4 projet :
- une dll ressource
- une application avec le service
- une application pour configurer le service
- une application pour controller le service
## dll de ressource
Une dll de ressource permet de partager des fichiers de ressource window entre plusieurs application https://docs.microsoft.com/en-us/cpp/build/creating-a-resource-only-dll  
Pour la générer de 0, il faut utiliser le fichier de message compiler (.mc) dans lequel est configuré les element pour la dll de ressource, https://docs.microsoft.com/fr-fr/windows/win32/wes/message-compiler--mc-exe- .  
Puis utiliser les commande suivante :
1. mc -U resource.mc
2. rc -r resource.rc

## les autres projet
tous les autres projet sont des application console qui doivent etre controlé depuis un invité de commande CMD. Pour la plupart des commande, il est nécéssaire d'ouvrir l'invité de commande en mode administrateur.

## les commandes implémenté :
### svc.exe
`svc install`  
Installation du service. Il n'est pas lancé et ne possede pas de descrption.

### svcconfig.exe
`svcconfig describe MonService`  
change la descrption du service.

`svcconfig query MonService`  
afiche les information de configuration du service

`svcconfig disable MonService`  
Desactive le service.

`svcconfig enable MonService`  
(Ré)Active le service.

`svcconfig delete MonService`  
Desinstalle le service.
   

### svccontrol.exe
`svccontrol start MonService`  
lance l'execution du service

`svccontrol dacl MonService`  
change les droit des utilisateur du service

`svcsvccontrol stop MonService`  
arrete l'execution du service
