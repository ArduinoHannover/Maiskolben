# Maiskolben

## SMD Lötstation, kompatibel zu Weller-Lötspitzen

Weller baut seit Jahren *die* Lötstationen für den professionellen Bereich.
Die WMRP/WXRP-Stationen heben sich durch ihre aktiven Lötspitzen von anderen Produkten ab.
Mit einer außergewöhnlich kurzen Aufheizzeit im Sekundenbereich, einer direkt in der Spitze integrierten Temperaturmessung und dementsprechend schnelle Nachregulierung in Sekundenbruchteilen sind die Spitzen in der Industrie äußerst beliebt.
Die Spitzen selbst sind für 40€ im Handel erhältlich, die original Weller-Station kostet an die 600€.
Für den einfachen Maker oder Hobbyist auf keinen Fall ein Produkt, das beim Onlineshopping ohne weitere Überlegungen in den Warenkorb wandert.
Der *Maiskolben* hingegen ist eine erschwingliche Lösung für lediglich 40€, mit der die Spitzen ebenso angesteuert werden können.

* Betrieb mit 12V Netzteil (oder bis zu 24V mit Fertigplatine) oder 3s LiPo (Modellbau-Akku)
* Spannungsüberwachung (bei Akku für alle 3 Zellen und automatische Abschaltung bei zu geringer Ladung)
* Einfaches Wechseln von Spitzen mittels Klinken-Buchse/-Kabel
* 3 individuelle Temperatur-Presets
* Temperaturbereich von 200-450°C
* Gut ablesbares Farbdisplay mit allen nötigen Informationen
* Auto-Standby (entweder sensor- oder zeitgesteuert auf 150°C)
* Auto-Wakeup (entweder sensorgesteuert oder durch Berühren von Lötschwamm, Messingwolle oder Lötstelle)
* USB-Schnittstelle zur Überwachung oder Steuerung mittels PC
* Potentialausgleich-/ESD-Buchse (über 1MΩ an GND)
* Optisches Feedback der Heizleistung durch LED
* Open-Source-Software auf Basis von Arduino
* Software-Update kann per USB eingespielt werden

*Zu erwerben bei [HannIO](https://hannio.org).*

## Stromzufuhr

Die Stromzufuhr wird wahlweise über die Hohlstecker-Buchse (10-24V) *ODER* die JST-XH4 (11,1V) Buchse angeschlossen.
Bei Versionen < 3.0 niemals beide gleichzeitig anschließen!
Der Lötkolben (Weller RT-XX, z.B. RT-1 für feine Lötarbeiten) wird über die Klinkenbuchse mit einer Verlängerung angesteckt.

## Display

Das Display zeigt mittig in weißer Schrift die Soll-Temperatur, darunter, sofern eine Spitze eingesteckt ist, die aktuelle Ist-Temperatur.
Bei eingeschaltetem Lötkolben erscheint zusätzlich noch die Zeit, nach der sich der Lötkolben automatisch in den Standby versetzt.
Der Balken am oberen Bildschirmrand spiegelt die Ist-Temperatur wieder.
Oben Links wird die Spannung bzw. die Ladezustände der einzelnen Zellen angezeigt.
Links wird durch das Power-Symbol der aktuelle Zustand (aus = rot, Standby = gelb, an = grün) signalisiert.
Am unteren Rand sind die drei Presets zu sehen.

## Bedienung

Nach dem Anstecken von Netzteil und Lötspitze kann die Station durch halten der Power-Taste (unten rechts) eingeschaltet werden.
Basierend auf der letzten Einstellung wird die angezeigte Soll- (Power-Symbol grün) oder die Standby-Temperatur (Symbol gelb) angefahren.

Mittels der Tasten rechts vom Display (Pfeile) lässt sich die Soll-Temperatur nach oben und unten korrigieren.
Sollte einer der Pfeile auf dem Display ausgegraut sein, so lässt sich über diese Temperatur hinaus keine weitere Änderung mehr vornehmen (200 ≥ T ≥ 450).

### Presets

Mittels der Preset-Tasten unter dem Bildschirm lassen sich voreingestellte Temperaturen als Soll-Temperatur setzen und die Station gleichzeitig aus dem Standby aufwecken.
Durch Gedrückthalten einer der drei Tasten wird der dort gespeicherte Wert durch die aktuelle Soll-Temperatur überschrieben.
Wird die Power-Taste nur kurz gedrückt, so wird der Standby-Zustand umgeschaltet.

### Startmenü

Es gibt ein verstecktes Menü, in dem sich der automatische Standby abschalten und ein automatisches Hochfahren auf die zuletzt gewählte Temperatur bei Spannungsversorgung aktiviert werden können. Um in dieses Menü zu gelangen, muss beim Anlegen der Spannungsversorgung während des Blinkens der LED die Power-Taste betätigt werden.

## Fehlermeldungen

#### USB power only/Connect power supply; Power too low/Connect power >5V
Die Lötstation wird entweder nur über USB oder eine Spannungsquelle mit knapp 5V versorgt.
Zum Betrieb wird ein adäquates Netzteil oder Akku benötigt (≥40W)

#### NO TIP; No tip connected/Tip slipped out?
Es wurde keine Spitze eingesteckt oder nicht erkannt.
Es kann sein, dass die Spitze defekt ist oder nicht richtig in der Buchse steckt.

#### Temperature dropped/Tip slipped out?
Es wurde ein zu großer Temperaturabfall bemerkt und die Wahrscheinlichkeit besteht, dass die Spitze leicht aus der Buchse herausgerutscht ist.
Durch das Herausrutschen kann eine Verbindung zwischen Masse und dem Sensor hergestellt werden, sodass die erkannte Temperatur beim Offset von 24°C liegt.

#### Not heating/Weak power source or short
Entweder kann die Stromversorgung nicht genug Leistung bringen, um die Spitze in einem angemessenen Zeitraum aufzuheizen oder es liegt wie bei obigem Fehler ein Kurzschluss von Masse und Sensor vor.

#### Battery low/Replace or charge
Der Akku hat auf mindestens einer Zelle eine zu geringe Spannung und sollte ersetzt oder geladen werden.

Fehlermeldungen müssen durch OK bestätigt werden.
Die Fehlermeldungen können nicht unterdrückt werden, sollte allerdings mehrfach hintereinander eine Meldung bestätigt und die Station wieder eingeschaltet werden, kann dies zu übermäßigem Erwärmen der Lötspitze führen.

## Software-Update

Um die aktuellste Software auf den Maiskolbe zu übertragen, muss zunächst die [Arduino-IDE](https://www.arduino.cc/en/Main/Software) heruntergeladen werden.
In diesem Repository befinden sich zwei Programme, zum einen Maiskolben_LCD für die Hardware-Revision 1.0 und Maiskolben_TFT für Hardware-Revisionen ab 1.5.
Die .ino Datei kann mit Arduino geöffnet werden.
Der Serielle-Port muss nach Herstellen einer Verbindung zwischen Maiskolben und PC unter _Werkzeuge_ > _Port_ ausgewählt werden.
Unter Umständen muss zuvor noch der [CH340 Treiber](http://www.wch.cn/download/CH341SER_ZIP.html) für den USB-Seriell-Wandler installiert werden.
Als _Board_ sollte _Arduino Nano_ mit _Prozessor: ATmega328 (Old Bootloader)_ ausgewählt werden.
Über _Sketch_ > _Hochladen_ bzw. den Pfeil nach Rechts in der oberen Leiste kann das Programm übertragen werden.

### Bibliotheken

Unter Umständen müssen vorher folgende Libraries noch über _Sketch_ > _Bibliothek einbinden_ > _.ZIP Bibliothek hinzufügen..._ eingebunden werden:

* [TimerOne](https://github.com/PaulStoffregen/TimerOne)
* [PID_v1](https://github.com/br3ttb/Arduino-PID-Library/)
* ~~[Adafruit_ST7735](https://github.com/adafruit/Adafruit-ST7735-Library)~~ (Software Version < 3.0)
* [TFT_ILI9163C](https://github.com/ArduinoHannover/TFT_ILI9163C)
* [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)

Dafür muss von den entsprechenden Seiten zuvor die ZIP heruntergeladen werden.
Diese kann dann in dem Dateibrowser, der sich nach obiger Anleitung öffnet, selektiert werden.

### Changelog

* 3.1
  * Option im Menü eingebaut, um zwischen Fahrenheit und Celsius zu wechseln
  * Bug behoben, der bei falsch initialisiertem EEPROM die Temperaturberechnung nicht richtig ausgeführt hat
* 3.0
  * Geänderte Display-Bibliothek (vgl. [Abschnitt Bibliotheken](#Bibliotheken))
  * Mehr Strings im Flash statt im RAM
  * Anpassungen um einen Maiskolben V3.2 im LiPo-Betrieb vollständig abzuschalten
  * Limitierung der Minimaltemperatur auf 200°C
  * `°C` wird mit bei Soll- und Ist-Temperatur angezeigt
  * Interne Temperatur-Kalibrierung für jeden Maiskolben individuell (bisher nur für Kalibrierung ab Werk)
* 2.8
  * Zeilenhöhe des Balkens ist nun variabel
  * Temperaturen als `#define` gesetzt
  * Temperaturberechnung wieder angepasst
  * EEPROM-Fehler behoben
* 2.7
  * TFT-Reset eingebaut
* 2.6
  * Schreibfehler korrigiert
  * Anpassungen an unterschiedliche Hardwareausführungen verbessert
  * Beim Aufruf des Menüs kommt man nach schließen direkt zum Programm
  * Autopower verfeinert
* 2.5
  * Interne Testroutinen hinzugefügt
  * Anpassung an Hardwarerevision ≥ 3.0
  * Wattanzeige hinzugefügt
* 2.4
  * Spannungsanzeige gegen Steckersymbol getauscht
* 2.3
  * Anpassung an unterschiedliche Display-Treiber (BGR/RGB)
  * Geänderte Temperaturmessung
* 2.1
  * Bidirektionale Steuerung über serielle Schnittstelle
* 2.0
  * Initiale Version für GitHub (TFT)
  * Auto-Standby hinzugefügt
  * Parameter angepasst

### Achtung!

Zur eigenen Sicherheit vor einem Software-Update die Lötspitze entfernen.
Zwar wird die Heizspannung beim Übertragen deaktiviert, dennoch kann es vereinzelt zu Fehlfunktionen und dadurch bedingtem unbeschränkten Aufheizen der Spitze kommen.

Modifikationen an der Software sind auf eigene Gefahr des Nutzers.

## Umbauten

Nachfolgend werden Modifikationen beschrieben, die die Funktionen des Maiskolben erweitern.
Für eigens durchgeführte Änderungen kann keine Haftung übernommen werden.

### Ablage-Sensor nachrüsten

Statt des automatischen Standbys nach einer bestimmten Zeit kann auch der Standby durch einen Ablage-Sensor erzwungen werden.
Auf der Platine befinden sich Kontakte mit STBY, 5V und GND.
Um den Standby zu aktivieren muss STBY mit GND verbunden werden, um wieder aufzuwachen muss die Verbindung getrennt werden.
Das Schalten von STBY nach GND kann entweder durch einen Schalter erfolgen, der beim auflegen der Spitze auslöst, alternativ mit einem Hall- oder Reed-Sensor.
Für letzteres muss an dem Ende des Klinken-Kabels, an dem die Spitze eingesteckt wird, ein Magnet angebracht werden, der den Sensor dann auslöst.

### Stromversorgungs-Schalter

Ab Hardware-Revision 1.5 gibt es auf der Platinenunterseite eine Lötbrücke mit der Beschriftung ON/OFF.
Bis einschließlich Revision 1.9 wird beim Durchtrennen dieser Brücke die gesamte Stromversorgung unterbrochen.
Ab Revision 1.11 wird hingegen nur noch die Zuleitung des LiPos getrennt.
An den beiden Lötpunkten können nach dem Durchtrennen der Leiterbahn Kabel zu einem Schalter gelötet werden, mit dem sich die Stromversorgung unterbrechen lässt.
