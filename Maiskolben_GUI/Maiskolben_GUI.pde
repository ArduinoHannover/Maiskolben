import processing.serial.*;
import java.lang.reflect.Array;
import javax.swing.*;

static int gWidth = 400;
static int gHeight = 450;
static int frequency = 10; // receive data every 1/f seconds

Serial Maiskolben;
java.util.LinkedList<Integer> temperature = new java.util.LinkedList<Integer>();
float pidValue;
int setPoint;
int secs;
boolean off, standby, standby_layoff;
int[] stored = new int[3];
PGraphics history;

JToggleButton on = new JToggleButton();
JToggleButton stby = new JToggleButton();



void setup() {
  size(50,1);
  surface.setResizable(true);
  surface.setSize(gWidth+200,gHeight);
  Object[] list = Serial.list();
  JComboBox serialPort = new JComboBox(list);
  Object[] params = {"Select serial Port:",serialPort};
  Object[] opts = {"Select","Exit"};
  int opt = JOptionPane.showOptionDialog(null, params, "Select Maiskolben", JOptionPane.OK_CANCEL_OPTION, JOptionPane.QUESTION_MESSAGE, null, opts, opts[0]);
  if (opt == JOptionPane.CANCEL_OPTION) System.exit(0);
  else {
    Maiskolben = new Serial(this, (String)serialPort.getSelectedItem(), 115200);
    Maiskolben.bufferUntil(10);
  }
  history = createGraphics(gWidth,gHeight);
}

void draw() {
  if (Maiskolben != null && Maiskolben.available() > 0) {
    //println("Received something!");
    String line = Maiskolben.readStringUntil(10);
    if (line != null) {
      String values[] = line.split(";");
      if (values.length < 9) println("zu wenig!");
      else {
        try {
          secs = (secs+1)%frequency;
          for (int i = 0; i < 3; stored[i] = Integer.parseInt(values[i++]));
          off = values[3].equals("1");
          standby = values[4].equals("1");
          standby_layoff = values[5].equals("1");
          setPoint = Integer.parseInt(values[6]);
          temperature.add(Integer.parseInt(values[7]));
          if (temperature.size() > gWidth) temperature.remove(0);
          pidValue = Float.parseFloat(values[8]);
          history.beginDraw();
          history.background(50);
          history.stroke(150);
          for (int i = 50; i < gHeight; i+=50) {
            history.line(0,i,gWidth,i);
          }
          for (int i = (temperature.size() == gWidth)?secs:0; i < gWidth; i+=frequency) {
            history.line(gWidth-i,0,gWidth-i,gHeight);
          }
          history.stroke(255);
          int lastV = temperature.getFirst();
          int x = 0;
          for (int temp : temperature) {
            history.line(x,gHeight-lastV,++x,gHeight-(lastV = temp));
          }
          history.endDraw();
        } catch (Exception e) {
          
        }
      }
      println(line);
    }
  }
  background(0);
  image(history,0,0);
  text(off?"Off":"On",gWidth+10,10);
  text(standby?"Standby":"Active",gWidth+10,30);
  text(standby_layoff?"In Holder":"",gWidth+10,50);
  text("Set Temperature "+setPoint,gWidth+10,70);
  if (temperature.size() != 0)
    text("Current Temperature "+temperature.getLast(),gWidth+10,90);
}