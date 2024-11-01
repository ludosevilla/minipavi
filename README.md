# MiniPavi - Mini Point d'Accès Videotex

MiniPavi est développé dans le cadre d'un projet personnel de préservation du patrimoine technologique que représente le Minitel. 

Si cette préservation passe naturellement par la conservation du matériel, elle passe également par celle de l'expérience utilisateur nécessitant ainsi l'existence de services télématiques. 

Ce projet est principalement réalisé en PHP et a été initialement développé afin de faciliter la création de services Minitel à l'époque actuelle.

MiniPavi est un point d’accès destiné à fournir des services Minitel (en mode videotex ou mode mixte, mode téléinformatique ASCII non-supporté) à des utilisateurs.

MiniPavi accepte les connexions des utilisateurs par websocket (sécurisée ou non), telnet et modem V23.

MiniPavi donne accès à des services Minitel développés spécifiquement pour fonctionner avec lui mais peut également se connecter, et ainsi donner accès, à des serveurs Minitel tiers accessibles par telnet, websockets (sécurisées ou non)  ou modem V23.

Un accès web intégré permet de connaître l’état des connexions, les visualiser en temps réel et en différé, et accéder à des statistiques.

La fonction « WebMedia » intégrée permet aux services d’envoyer du contenu multimédia (video, son, image) aux utilisateurs simultanément à leur consultation d’un service afin de fournir une « expérience Minitel » enrichie.

## Ce que contient le projet

Le projet contient essentiellement 5 parties :

- La passerelle MiniPavi (PHP) à installer sur un système type Rapsberry
- Le module soft modem (C) pour [Asterisk](https://www.asterisk.org/) et des exemples de fichiers de configuration, à installer sur un système type Rapsberry
- Un émulateur Minitel (PHP,HTML,JS,CSS) à installer sur un hébérgement web/PHP
- L'interface WebMedia (PHP,HTML,JS,CSS) à installer sur un hébérgement web/PHP
- Deux "services" (PHP) à installer sur un hébérgement web/PHP

L'ensemble de la documentation (explications, installation et configuration) est disponible dans le fichier **MiniPavi-doc.pdf** fourni.

Cette documentation est à consulter avant toute chose !

## Essayer MiniPavi

Une instance publique de MiniPavi est disponible :

Par émulateur sur:  [https://www.minipavi.fr/emulminitel/](https://www.minipavi.fr/emulminitel/)

Par websocket : ws://go.minipavi.fr:8182 et  wss://go.minipavi.fr:8181

Par « telnet » go.minipavi.fr:516

Par téléphone au 09 72 10 17 21 (ou 00 33 9 72 10 17 21)

Le site du projet est : [https://www.minipavi.fr/](https://www.minipavi.fr/)

## Développer ses propres services pour MiniPavi

Vous pouvez réaliser vous-même des services qui seront accessibles depuis votre passerelle MiniPavi, ou, plus simplement (c'est à dire si vous ne souhaitez pas installer votre propre passerelle MiniPavi), depuis l'instance publique de MiniPavi.

Pour plus d’information concernant le protocole d’échange de MiniPavi avec les services et le développement de ceux-ci, se reporter à la documentation spécifique sur ce sujet accessible depuis [www.minipavi.fr](http://www.minipavi.fr) ou depuis le dépôt consacré spécifiquement au développement de services [https://github.com/ludosevilla/minipaviCli](https://github.com/ludosevilla/minipaviCli)

Ce dépôt contient également quelques services d'exemple (Méteo, Sncf, France24,...)
