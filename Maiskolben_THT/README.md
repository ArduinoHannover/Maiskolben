# Maiskolben-Selfmade

## Selbstbau-Version des Maiskolben mit THT-Bauteilen

Die in diesem Ordner abgelegten Dateien sind für den Nachbau des Maiskolben mit THT-Bauteilen für die Make 5/16.

Bauteile dazu sind bei [Reichelt](http://sndstrm.de/maiskolbenEK) und [eBay](http://sndstrm.de/18tft) bestellbar, die [Platine oder das ganze Set kann ebenfalls online bestellt werden](https://hannio.org/produkt/maiskolben-tht/).


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

### Andere Probleme

[Fehlermeldungen](https://github.com/ArduinoHannover/Maiskolben#fehlermeldungen)