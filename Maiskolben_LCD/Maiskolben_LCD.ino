#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <PID_v1.h>
#include <EEPROM.h>
#include "TimerOne.h"

#define VERSION	100
#define EEPROM_CHECK 42
//#define HAS_BATTERY

#define STBY_TEMP	150
//SOFTWARE CAN'T MEASURE MORE THAN 422 DUE TO RESISTOR CONFIGURATION, IF SET TO >= 422 IT'S LIKELY TO KILL YOUR TIP!
//If read 1024 on Analog in, the tip is turned off
#define MAX_TEMP	400
#define MIN_TEMP	100

//Temperature in degree to rise at least in given time
#define TEMP_MIN_RISE 10
//Time in that the temperature must rise by the set temperature
#define TEMP_RISE_TIME 1000

#define HEATER_PWM	3
#define SW_T1		5
#define SW_T2		6
#define SW_T3		7
#define TEMP_SENSE	A0
#define STBY_NO		A1
#define SW_STBY		A2
#define HEAT_LED	A3
#define SW_DOWN		A4
#define SW_UP		A5
#define BATTERY_IN	A6

#define kp			0.1
#define ki			0.0001
#define kd			0.0

#define TIME_DISP_REFRESH_IN_MS 300
#define TIME_SW_POLL_IN_MS 10
#define DELAY_BEFORE_MEASURE 10
#define DELAY_MAIN_LOOP 10
#define PID_SAMPLE_TIME 10

#define ADC_TO_TEMP_GAIN 0.39
#define ADC_TO_TEMP_OFFSET 23.9
#define CTRL_GAIN 10

#define NUM_DIFFS 8 //Dividable by 8

volatile boolean off = true, stby = true, stby_layoff = true, sw_stby_old = false, sw_up_old = false, sw_down_old = false, clear_display = true, store_invalid = true, error = false, menu = false;
uint16_t stored[3] = {250, 300, 350}, set_t = 150, cur_t, set_t_old, cur_t_old;
double pid_val, cur_td, set_td;
uint8_t pwm, store_to = 255, contrast = 50;
uint16_t cnt_disp_refresh = TIME_DISP_REFRESH_IN_MS, cnt_sw_poll, cnt_but_press, cnt_off_press, cnt_but_store;
float battery_voltage;
uint16_t last_measured;
int16_t last_diffs[NUM_DIFFS];
uint8_t array_index, array_count;
uint32_t sendNext;

Adafruit_PCD8544 display = Adafruit_PCD8544(10, 9, -1);

PID heaterPID(&cur_td, &pid_val, &set_td, kp, ki, kd, DIRECT);

void setup() {
	digitalWrite(HEATER_PWM, LOW);
	pinMode(HEATER_PWM, OUTPUT);
	pinMode(HEAT_LED, OUTPUT);
	pinMode(TEMP_SENSE, INPUT);
	pinMode(SW_T1, INPUT_PULLUP);
	pinMode(SW_T2, INPUT_PULLUP);
	pinMode(SW_T3, INPUT_PULLUP);
	pinMode(SW_UP, INPUT_PULLUP);
	pinMode(SW_DOWN, INPUT_PULLUP);
	pinMode(STBY_NO, INPUT_PULLUP);
	pinMode(SW_STBY, INPUT_PULLUP);
	
	for (uint8_t i = 0; i < NUM_DIFFS; i++) {
		last_diffs[i] = 0;
	}
	
	//PWM Prescaler = 1024
	TCCR2B = TCCR2B & 0b11111000 | 7;
	delay(500);
	display.begin();
	delay(500);
	display.clearDisplay();
	if (EEPROM.read(0) != EEPROM_CHECK) {
		EEPROM.update(0, EEPROM_CHECK);
		updateEEPROM();
	}
	stby = EEPROM.read(1);
	for (uint8_t i = 0; i < 3; i++) {
		stored[i] = EEPROM.read(2+i*2) << 8;
		stored[i] |= EEPROM.read(3+i*2);
	}
	set_t = EEPROM.read(8) << 8;
	set_t |= EEPROM.read(9);
	setContrast(EEPROM.read(10));
	
	
	Serial.begin(115200);
	//Serial.println("DIFF SUM;SET TEMPERATURE;IS TEMPERATURE;DUTY CYCLE;VOLTAGE");
	last_measured = getTemperature();
	Timer1.initialize(1000);
	Timer1.attachInterrupt(timer_isr);
	heaterPID.SetMode(AUTOMATIC);
	sendNext = millis();
}

void updateEEPROM() {
	EEPROM.update(1, stby);
	for (uint8_t i = 0; i < 3; i++) {
		EEPROM.update(2+i*2, stored[i] >> 8);
		EEPROM.update(3+i*2, stored[i] & 0xFF);
	}
	EEPROM.update(8, set_t >> 8);
	EEPROM.update(9, set_t & 0xFF);
	EEPROM.update(10, contrast);
}

void setContrast(uint8_t value) {
	if (value == 255) value = 0;
	contrast = min(100, value);
	EEPROM.update(10, contrast);
	display.setContrast(contrast);
}

int getTemperature() {
	analogWrite(HEATER_PWM, 0); //switch off heater
	delay(DELAY_BEFORE_MEASURE); //wait for some time (to get low pass filter in steady state)
	uint16_t adcValue = analogRead(TEMP_SENSE); // read the input
	if (adcValue >= 1015) { //Illegal value
		analogWrite(HEATER_PWM, 0);
		setOff(true);
		return 999;
	} else {
		analogWrite(HEATER_PWM, pwm); //switch heater back to last value
	}
	//return 124;
	return round(((float) adcValue)*ADC_TO_TEMP_GAIN+ADC_TO_TEMP_OFFSET); //apply linear conversion to actual temperature
}

void timer_sw_poll() {
	stby_layoff = !digitalRead(STBY_NO);
	if (!digitalRead(SW_STBY)) {
		if (cnt_off_press == 100) {
			setOff(!off);
		}
		cnt_off_press = min(101, cnt_off_press+1);
	} else {
		if (cnt_off_press > 0 && cnt_off_press != 101) {
			setStandby(!stby);
		}
		cnt_off_press = 0;
	}
	boolean t1 = !digitalRead(SW_T1);
	boolean t2 = !digitalRead(SW_T2);
	boolean t3 = !digitalRead(SW_T3);

	//simultanious push of multiple buttons 
	if (t1 + t2 + t3 > 1) {
		if (store_to != 255) cnt_but_store = 0;
		cnt_but_store = min(cnt_but_store+1, 255);
		store_to = 255;
		store_invalid = true;
		if (cnt_but_store > 100) {
			menu = true;
			clear_display = true;
		}
	} else if (menu) {
		if (t1 + t2 + t3 > 1) {
			store_invalid = true;
		} else if (!(t1 |  t2 | t3)) {
			store_invalid = false;
		} else if (!store_invalid) {
			if (t3) {
				store_invalid = true;
				menu = false;
				clear_display = true;
			}
		}
	} else if (error) {
		if (!(t1 |  t2 | t3)) {
			store_invalid = false;
		} else if (!store_invalid && t3) {
			error = false; //dismiss
			store_invalid = true; //wait for release
			clear_display = true;
		}
	} else if (!off) {
		//all buttons released
		if (!(t1 |  t2 | t3)) {
			if (store_to != 255) {
				if (cnt_but_store <= 100) {
					set_t = stored[store_to];
					setStandby(false);
					updateEEPROM();
				}
			}
			store_to = 255;
			store_invalid = false;
			cnt_but_store = 0;
		} else
		//one button pressed
		if (!store_invalid) {
			store_to = t2 + 2*t3;
			if (cnt_but_store > 100) {
				if (set_t != stored[store_to] && !stby) {
					stored[store_to] = set_t;
					cnt_but_store = 100;
					clear_display = true;
					updateEEPROM();
				}
			}
			cnt_but_store++;
		}
	}
	boolean sw_up = !digitalRead(SW_UP);
	boolean sw_down = !digitalRead(SW_DOWN);
	boolean sw_changed = (sw_up != sw_up_old) || (sw_down !=sw_down_old);
	sw_up_old = sw_up;
	sw_down_old = sw_down;
	if(sw_up && sw_down || !(sw_up || sw_down)) {
		cnt_but_press = 0;
		return;
	}
	if(sw_up || sw_down) {
		cnt_but_press++;
		if((cnt_but_press >= 100) || sw_changed) {
			if (menu) {
				if(sw_up) setContrast(contrast+1);
				else setContrast(contrast-1);
				if(!sw_changed) cnt_but_press = 70;
			} else if (!off) {
				setStandby(false);
				if(sw_up && set_t < MAX_TEMP) set_t++;
				else if (sw_down && set_t > MIN_TEMP) set_t--;
				if(!sw_changed) cnt_but_press = 97;
				updateEEPROM();
			}
		}
	}
}

void setStandby(boolean state) {
	if (state == stby) return;
	stby = state;
	//clear_display = true;
	last_measured = cur_t;
	EEPROM.update(1, stby);
}

void setOff(boolean state) {
	if (state == off) return;
	if (!state)
		analogWrite(HEATER_PWM, 0);
	off = state;
	clear_display = true;
	last_measured = cur_t;
}

void timer_disp_refresh() {
	boolean disp = false, clr = false;
	if (menu) {
		if (clear_display) {
			clear_display = false;
			display.clearDisplay();
			display.setTextSize(1);
			display.setCursor(16,0);
			display.print("Contrast");
			display.setTextSize(1);
			display.setCursor(58,40);
			display.print("Exit");
		}
		display.setTextSize(3);
		display.fillRect(16, 10, 64, 24, WHITE);
		display.setCursor(16+8*(contrast<100),10);
		display.setTextSize(3);
		display.print(contrast);
		disp = true;
	} else if (error) {
		if (clear_display) {
			display.clearDisplay();
			display.setCursor(16,0);
			display.setTextSize(3);
			display.print("ERR");
			display.setTextSize(1);
			display.setCursor(41,40);
			display.print("Dismiss");
			disp = true;
		}
	} else {
		if (clear_display) {
			clear_display = false;
			clr = true;
			display.clearDisplay();
			display.setTextSize(1);
			display.setCursor(0,40);
			display.print(stored[0]);
			display.setCursor(32,40);
			display.print(stored[1]);
			display.setCursor(64,40);
			display.print(stored[2]);
			disp = true;
		}
		if (off) {
			if (clr || ((cur_t_old != cur_t) && ((cur_t_old == 999) || (cur_t == 999)))) {
				if (cur_t == 999) {
					display.fillRect(0,0,84,24, WHITE);
					display.setCursor(6,4);
					display.setTextSize(2);
					display.print("NO TIP");
				} else {
					display.fillRect(0, 0, 84, 24, WHITE);
					display.setCursor(16,0);
					display.setTextSize(3);
					display.print("OFF");
				}
				disp = true;
			}
		} else if (set_t_old != set_t || clr) {
			display.fillRect(16, 0, 64, 24, WHITE);
			display.setCursor(16,0);
			display.setTextSize(3);
			display.print(set_t);
			set_t_old = set_t;
			disp = true;
		}
		if (cur_t_old != cur_t || clr) {
			display.fillRect(24, 24, 60, 16, WHITE);
			display.setCursor(24+8*(cur_t<100),24);
			display.setTextSize(2);
			if (cur_t == 999)
				display.print("ERR");
			else
				display.print(cur_t);
			if (stby_layoff || stby) {
				display.setCursor(58,28);
				display.setTextSize(1);
				display.print("STBY");
			}
			cur_t_old = cur_t;
			disp = true;
		}
	}
	#ifdef HAS_BATTERY
	if (battery_voltage > 1) {
		display.fillRect(display.width()-11,0,10,6,BLACK);
		display.drawRect(display.width()-10,1,8,4,WHITE);
		uint8_t bars = min(6,max(0,48-4*battery_voltage));
		display.fillRect(display.width()-2-bars,2,bars,2,WHITE);
		display.drawFastVLine(display.width()-1,2,2,BLACK);
		if (battery_voltage < 10.5) {
			setOff(true);
		}
	}
	#endif
	if (disp)
		display.display();
}

void timer_isr() {
	if(cnt_disp_refresh >= TIME_DISP_REFRESH_IN_MS) {
		timer_disp_refresh();
		cnt_disp_refresh=0;
	}
	cnt_disp_refresh++;
	if(cnt_sw_poll >= TIME_SW_POLL_IN_MS){
		timer_sw_poll();
		cnt_sw_poll=0;
	}
	cnt_sw_poll++;
}

void setError() {
	error = true;
	clear_display = true;
	setOff(true);
}

uint8_t cnt_temp_rise;
int16_t diff_old;
void loop() {
	cur_t = getTemperature();
	uint16_t target;
	if (off) {
		target = 0;
	} else if (stby_layoff || stby) {
		target = STBY_TEMP;
	} else {
		target = set_t;
	}

	int16_t delta = cur_t-last_measured;
	if (!off && delta <= -20 && cur_td != 999) {
		setError();
	}
	
	set_td = target;
	cur_td = cur_t;
	int16_t diff = target-cur_t;
	cnt_temp_rise++;
	last_measured = cur_t;

	heaterPID.Compute();
	if (error || off)
		pwm = 0;
	else
		pwm = min(255,pid_val*255);
		//pwm = max(0, min(255, diff*CTRL_GAIN));
	//reset counter if not heating that much
	if (pwm < min(TEMP_MIN_RISE*CTRL_GAIN, 200)) cnt_temp_rise = 0;
	
	analogWrite(HEATER_PWM, pwm);
	digitalWrite(HEAT_LED, cur_t+5 < target || (abs((int16_t)cur_t-(int16_t)target) <= 5 && (millis()/(stby?1000:500))%2));
	battery_voltage = (analogRead(BATTERY_IN)*3*5/1024.0);
	if (sendNext <= millis()) {
		sendNext += 100;
		Serial.print(stored[0]);
		Serial.print(";");
		Serial.print(stored[1]);
		Serial.print(";");
		Serial.print(stored[2]);
		Serial.print(";");
		Serial.print(off?1:0);
		Serial.print(";");
		Serial.print(stby?1:0);
		Serial.print(";");
		Serial.print(stby_layoff?1:0);
		Serial.print(";");
		Serial.print(set_t);
		Serial.print(";");
		Serial.print(cur_t);
		Serial.print(";");
		Serial.print(pid_val);
		Serial.print(";");
		Serial.print(battery_voltage);
		Serial.print(";");
		Serial.print(battery_voltage);
		Serial.print(";");
		Serial.println(battery_voltage);
		Serial.flush();
	}
	delay(DELAY_MAIN_LOOP);
}
