# Maiskolben-Selfmade

## Selbstbau-Version des Maiskolben mit THT-Bauteilen

Die in diesem Ordner abgelegten Dateien sind für den Nachbau des Maiskolben mit THT-Bauteilen für die Make 5/16.

### Bauteile

| Menge | Bauteil |
---: | ---
1 | 2N700
1 | IRF5305
1 | OPA703PA (OPA336PA)
1 | 74HC4050
1 | Arduino Nano (o.vgl.)
1 | LED 5mm
1 | 220Ω (oder anderer passender Vorwiderstand für die LED)
1 | 100Ω
1 | 1kΩ
2 | 5k6Ω
5 | 10kΩ
1 | 68kΩ
1 | 100kΩ
1 | 10MΩ
2 | 100nF
2 | 10nF
ggf. | Buchsenleisten nach Bedarf
6 | Taster 6x6mm; möglichst hoch (12mm)
1 | Stereo Klinkenbuchse (EBS 35)
1 | DC Buchse (Netzteilspezifisch etwa Lumberg NEB21R)
1 | Passendes Netzteil
1 | Weller Lötspitze

Ein Großteil der Bauteile sind beim Elektronikeinzelhandel verfügbar, das Display hingegen fast ausschließlich bei [eBay](http://sndstrm.de/18tft) bestellbar (sofern nicht mehr verfügbar nach `1.8 tft` suchen, blaue Platine, **nicht** Adafruit), die [Platine oder das ganze Set kann ebenfalls online bestellt werden](https://hannio.org/produkt/maiskolben-tht/).


## Aufbau

![Bestückte Platine](https://github.com/ArduinoHannover/Maiskolben/raw/master/images/Maiskolben_THT_placed.png "Ohne LED")

Der MOSFET (IRF5305) kann unter das Display gebogen werden, dazu vorher etwas mit Klebeband isolieren, um Kontakt zu vermeiden. Den 2N7000 dafür weit genug versenken.

## Software

Für die Software bitte [die Update-Anleitung](https://github.com/ArduinoHannover/Maiskolben#software-update) zur Hilfe nehmen.

## Problembehandlung

### Display bleibt weiß
In diesem Fall arbeitet der Reset vom Display nicht sauber. Softwareseitig ist dafür `#define USE_TFT_RESET` vorgesehen. Um einen Reset vom Arduino zu triggern, muss allerdings die Reset-Leitung zwischen Arduino und Display getrennt werden.

Bei neueren Platinen gibt es dafür einen Jumper `R A` - dieser ist standardmäßig an R (Reset) angeschlossen. Diese Verbindung muss etwa mit einem Cutter aufgetrennt werden und dann eine Brücke an `A` gesetzt werden.

Ansonsten muss die Leiterbahn aufgekratzt werden oder brutal der Reset-Pin am Arduino weggeknipst werden. Dann muss die Leiterbahn zum Display mit A0 vom Arduino verbunden werden.

### Display zeigt Artefakte
Teilweise arbeitet der Spannungsregler nicht zufriedenstellend, sodass der Treiber des TFT ab und zu Spikes bekommt. In diesem Fall sollten verschiedene Kondensatoren (100nF sollte klappen) am Display VCC/GND oder auch zwischen GND und 5V ein größerer Kondensator (100uF/6,3V) eingebaut werden.

### Andere Probleme

[Fehlermeldungen](https://github.com/ArduinoHannover/Maiskolben#fehlermeldungen)