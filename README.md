# cpp_ldap_email_server

## Beschreibung des Kommunikationsprotokolls
Zuerst wird eine Verbindung zwischen Client und Server hergestellt. Bevor irgentwetwas gemacht werden kann
muss sich der User anmelden.

## LOGIN
Der Login Befehl macht nichts anderes als den Username und das Password an den Server zu schicken wo es dann auf übereinstimmung geprüft wird.

Der Protokollaufbau des LOGIN Befehls ist wie folgt definiert:
```` 
LOGIN\n
<LDAP Username max. 8 Zeichen>\n
<Passwort>\n
````

Der Server antwortet bei korrekten Parametern und erfolgreichem Login mit:
````
OK\n
````
Der Server antwortet im Fehlerfall (User nicht vorhanden, Fehler beim Authentifizieren, etc.) mit:
````
ERR\n
````
Im Fall das der User gesperrt ist antowrtet der Server mit:
````
BLOCK\n
````


## SEND
Der Send Befehl sendet email zum Server und kann nur benutzt werden wenn man angemeldet ist.

Der Protokollaufbau des SEND Befehls ist wie folgt definiert:
```` 
SEND\n
<Sender max. 8 Zeichen>\n
<Empfänger max. 8 Zeichen>\n
<Betreff max. 80 Zeichen>\n
<Nachricht, beliebige Anzahl an Zeilen\n>
.\n 
````

Der Server antwortet mit OK\no der ERR\n im Fehlerfall


## LIST
Der List Befehl listet alle E-mails des angemeldeten Benutzers auf.

Der Protokollaufbau des LIST Befehls ist wie folgt definiert:
```` 
LIST\n
<Username max. 8 Zeichen>\n
````

Der Server antwortet mit:
````
<Anzahl der Nachrichten für den User, 0 wenn keine Nachrichten vorhanden sind>\n
<Betreff1>\n
<Betreff2>\n
<Betreffn>\n
````

## READ
Der Read Befehl listet den Inhalt einer ausgewählten mail aus. (nur wenn angemeldet)

Der Protokollaufbau des READ Befehls ist wie folgt definiert:
```` 
READ\n
<Username max. 8 Zeichen>\n
<Nachrichten-Nummer>\n
````

Der Server antwortet bei korrekten Parametern mit:
````
OK\n
<kompletter Inhalt der Nachricht wie beim SEND Befehl>
````

Der Server antwortet im Fehlerfall (Nachricht nicht vorhanden) mit:
````
ERR\n
````


## DEL
Der Del Befehl löscht eine E-mail deren Id angegeben wurde. (nur wenn angemeldet)

Der Protokollaufbau des DEL Befehls ist wie folgt definiert:
```` 
DEL\n
<Username max. 8 Zeichen>\n
<Nachrichten-Nummer>\n
````

Der Server antwortet bei korrekten Parametern und erfolgreichem Löschen mit:
````
OK\n
````

Der Server antwortet im Fehlerfall (Nachricht nicht vorhanden, Fehler beim Löschen) mit:
````
ERR\n
````
