# Dateien für 3D-Druck- oder Lasercut-Gehäuse

Sofern die Dateien nicht den Zusatz "THT" enthalten, sind diese für die fertig gelöteten Station im Shop geeignet. Dateien mit "THT" hingegen nur für den Bausatz (siehe [Maiskolben_THT](https://github.com/ArduinoHannover/Maiskolben/tree/master/Maiskolben_THT)).

## Lasercut-Gehäuse

Die `_lasercut`-Dateien eignen sich für 3mm Material (klares Plexiglas wird für die Front empfohlen, alternativ Ausschnitt für das Display vorsehen). Beim THT Gehäuse sind zudem noch 6 der Button Caps (3D-Druck) notwendig.

## Eigenes/3D-Druck Gehäuse

Die `_outlines`-Dateien, respektive die .obj-Dateien sind 2D-/3D-Modelle, die zum Erstellen von eigenen Gehäusen genutzt werden können.


# Displayfix

Sollte das Display weiß bleiben (inkonsistent, selbst innerhalb einer Charge nur sporadisch) gibt es verschiedene Möglichkeiten, dies zu beheben. In der Software kann per `#define USE_TFT_RESET` der `STBY_NO` (Ablagesensor) als Resetleitung des Displays genutzt werden. Hierfür ist die Verbindung von `Reset` zwischen Arduino und 74HC4050 zu trennen und am 74HC4050 eine Verbindung zu `A1` herzustellen. Eine andere Variante ist ein RC-Glied zur Zeitverzögerung einzubauen. Hierfür wird ebenfalls die Zuleitung getrennt und ein Widerstand eingelötet. Vom Reset am 4050 (also vom Arduino aus gesehen hinter dem eingelöteten Widerstand) ist gegen Masse ein Widerstand einzulöten. Näheres dazu im Displayfix-PDF.