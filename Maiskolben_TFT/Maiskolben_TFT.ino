#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <PID_v1.h>
#include <EEPROM.h>
#include "TimerOne.h"
#include "definitions.h"

/*
 * If red is blue and blue is red change this (2.0 and above have new display types)
 * If not sure, leave commented
 */
//#define HARDWARE_DEFINED_TFT 2
/*
 * Only used for testing, do not use.
 */
//#define INSTALL

volatile boolean off = true, stby = true, stby_layoff = true, sw_stby_old = false, sw_up_old = false, sw_down_old = false, clear_display = true, store_invalid = true, menu = false;
volatile uint8_t pwm, threshold_counter;
volatile int16_t cur_t, last_measured;
volatile error_type error = NO_ERROR;
error_type error_old;
int16_t stored[3] = {250, 300, 350}, set_t = 150, set_t_old, cur_t_old, target_t;
double pid_val, cur_td, set_td;
uint8_t store_to = 255;
p_source power_source, power_source_old = NO_INIT;
boolean blink;
uint16_t cnt_measure_voltage, cnt_compute, cnt_sw_poll, cnt_but_press, cnt_off_press, cnt_but_store;
float v_c1, v_c2, v_c3;
uint8_t array_index, array_count;
uint32_t sendNext;
uint32_t last_temperature_drop;
uint32_t last_on_state;
boolean wasOff = true, old_stby = false;
boolean autopower = true, bootheat = false;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, 0);
#define ST7735_GRAY 0x94B2

PID heaterPID(&cur_td, &pid_val, &set_td, kp, ki, kd, DIRECT);

void setup(void) {
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
	Serial.begin(115200);
	
	boolean force_menu = false;
	if (EEPROM.read(0) != EEPROM_CHECK) {
		EEPROM.update(0, EEPROM_CHECK);
		updateEEPROM();
		force_menu = true;
	}
#ifdef HARDWARE_DEFINED_TFT
#if HARDWARE_DEFINED_TFT == 1
	tft.initR(INITR_BLACKTAB);
	EEPROM.update(EEPROM_DISPLAY, INITR_BLACKTAB);
#else
	tft.initR(INITR_GREENTAB);
	EEPROM.update(EEPROM_DISPLAY, INITR_GREENTAB);
	delay(1000);
#endif
#else
	if (force_menu || EEPROM.read(EEPROM_VERSION) < 23 || EEPROM.read(EEPROM_VERSION) == 255 || (EEPROM.read(EEPROM_DISPLAY) != INITR_GREENTAB && EEPROM.read(EEPROM_DISPLAY) != INITR_BLACKTAB)) {
		tft.initR(INITR_GREENTAB);
		tft.setRotation(1);
		tft.fillScreen(ST7735_BLACK);
		tft.setTextSize(2);
		tft.setCursor(0,0);
		tft.setTextColor(ST7735_WHITE);
		tft.print(F("What color is displayed?"));
		tft.setCursor(10,112);
		tft.setTextColor(ST7735_RED);
		tft.print("RED     BLUE");
		while (true) {
			if (!digitalRead(SW_T1)) {
				EEPROM.update(EEPROM_DISPLAY, INITR_GREENTAB);
				break;
			}
			if (!digitalRead(SW_T3)) {
				EEPROM.update(EEPROM_DISPLAY, INITR_BLACKTAB);
				tft.initR(INITR_BLACKTAB);
				tft.setRotation(1);
				break;
			}
		}
		tft.fillScreen(ST7735_BLACK);
		tft.setTextColor(ST7735_YELLOW);
		tft.drawBitmap(0, 20, maiskolben, 160, 64, ST7735_YELLOW);
		tft.setCursor(20,86);
		tft.setTextColor(ST7735_YELLOW);
		tft.setTextSize(2);
		tft.print("Maiskolben");
		tft.setCursor(35,104);
		tft.print("Welcome!");
		delay(4000);
		while (!digitalRead(SW_T3) || !digitalRead(SW_T1)) delay(100);
	} else {
		tft.initR(EEPROM.read(EEPROM_DISPLAY));
	}
#endif
#ifdef INSTALL
	//EEPROM.update(EEPROM_INSTALL, 0);
	if (EEPROM.read(EEPROM_INSTALL) != EEPROM_CHECK) {
		tft.setRotation(1);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		tft.fillScreen(ST7735_BLACK);
		tft.setCursor(0,0);
		tft.setTextSize(2);
		tft.println("Installation");
		for (int16_t i = -255; i < 256; i++) {
			analogWrite(HEAT_LED, 255-abs(i));
			delay(1);
		}
		while (digitalRead(SW_STBY)) {
			int t = getTemperature();
			digitalWrite(HEATER_PWM, !digitalRead(SW_T1) | !digitalRead(SW_T2) | !digitalRead(SW_T3) | !digitalRead(SW_UP) | !digitalRead(SW_DOWN));
			tft.setCursor(0,18);
			tft.print(t);
			tft.println("  ");
			delay(50);
		}
		EEPROM.update(EEPROM_OPTIONS, (bootheat << 1) | autopower);
		EEPROM.update(EEPROM_VERSION, EE_VERSION);
		EEPROM.update(EEPROM_INSTALL, EEPROM_CHECK);
		tft.println("done.");
		delay(1000);
		asm volatile("jmp 0");
	}
#endif
	if (EEPROM.read(EEPROM_VERSION) != EE_VERSION) {
		force_menu = true;
	}
	tft.fillScreen(ST7735_BLACK);
	tft.setRotation(1);
	uint8_t options = EEPROM.read(EEPROM_OPTIONS);
	autopower = options & 1;
	bootheat = (options >> 1) & 1;
	if (force_menu) optionMenu();
	else {
		tft.drawBitmap(0, 20, maiskolben, 160, 64, ST7735_YELLOW);
		tft.setCursor(20,86);
		tft.setTextColor(ST7735_YELLOW);
		tft.setTextSize(2);
		tft.print("Maiskolben");
		tft.setCursor(50,110);
		tft.setTextSize(1);
		tft.print("Version ");
		tft.print(VERSION);
		//Allow Options to be set at startup
		attachInterrupt(digitalPinToInterrupt(SW_STBY), optionMenu, LOW);
		for (int i = 1; i < 11; i++) {
			digitalWrite(HEAT_LED, i % 2);
			delay(250);
		}
		detachInterrupt(digitalPinToInterrupt(SW_STBY));
	}
	/*
	 * lower frequency = noisier tip
	 * higher frequency = needs higher pwm
	 */
	//PWM Prescaler = 1024
	//TCCR2B = TCCR2B & (0b11111000 | 7);
	//PWM Prescaler = 128
	TCCR2B = TCCR2B & (0b11111000 | 5);
	stby = EEPROM.read(1);
	for (uint8_t i = 0; i < 3; i++) {
		stored[i] = EEPROM.read(2+i*2) << 8;
		stored[i] |= EEPROM.read(3+i*2);
	}
	set_t = EEPROM.read(EEPROM_SET_T) << 8;
	set_t |= EEPROM.read(EEPROM_SET_T+1);

	for (uint8_t i = 0; i < 50; i++)
		measureVoltage(); //measure average 50 times to get realistic results
	
	tft.fillScreen(ST7735_BLACK);
	last_measured = getTemperature();
	Timer1.initialize(1000);
	Timer1.attachInterrupt(timer_isr);
	heaterPID.SetMode(AUTOMATIC);
	sendNext = millis();
	if (bootheat) {
		threshold_counter = TEMP_UNDER_THRESHOLD;
		setOff(false);
	}
}

void optionMenu(void) {
	tft.fillScreen(ST7735_BLACK);
	digitalWrite(HEAT_LED, LOW);
	tft.setTextSize(2);
	tft.setCursor(0,0);
	tft.setTextColor(ST7735_WHITE);
	tft.println("Options\n");
	tft.setTextColor(ST7735_WHITE);
	tft.setCursor(10,112);
	tft.print("ON  OFF EXIT");
	uint8_t options = 2;
	uint8_t opt = 0;
	boolean redraw = true;
	while (true) {
		if (redraw) {
			tft.setCursor(0,36);
			#ifdef SHUTOFF_ACTIVE
				tft.setTextColor(autopower?ST7735_GREEN:ST7735_RED);
			#else
				tft.setTextColor(ST7735_GRAY);
			#endif
			tft.println(" Autoshutdown");
			#ifdef BOOTHEAT_ACTIVE
				tft.setTextColor(bootheat?ST7735_GREEN:ST7735_RED);
			#else
				tft.setTextColor(ST7735_GRAY);
			#endif
			tft.println(" Heat on boot");
			
			tft.setCursor(0, (opt+2)*18);
			tft.setTextColor(ST7735_WHITE);
			tft.print(">");
			redraw = false;
		}
		if (!digitalRead(SW_UP)) {
			tft.setCursor(0, (opt+2)*18);
			tft.setTextColor(ST7735_BLACK);
			tft.print(">");
			opt = (opt+options-1)%options;
			while (!digitalRead(SW_UP)) delay(100);
			redraw = true;
		}
		if (!digitalRead(SW_DOWN)) {
			tft.setCursor(0, (opt+2)*18);
			tft.setTextColor(ST7735_BLACK);
			tft.print(">");
			opt = (opt+1)%options;
			while (!digitalRead(SW_DOWN)) delay(100);
			redraw = true;
		}
		if (!digitalRead(SW_T1)) {
			switch (opt) {
				case 0: autopower = 1; break;
				case 1: bootheat = 1; break;
			}
			redraw = true;
		}
		if (!digitalRead(SW_T2)) {
			switch (opt) {
				case 0: autopower = 0; break;
				case 1: bootheat = 0; break;
			}
			redraw = true;
		}
		if (!digitalRead(SW_T3)) break;
	}
	EEPROM.update(EEPROM_OPTIONS, (bootheat << 1) | autopower);
	EEPROM.update(EEPROM_VERSION, EE_VERSION);
}

void updateEEPROM(void) {
	EEPROM.update(1, stby);
	for (uint8_t i = 0; i < 3; i++) {
		EEPROM.update(2+i*2, stored[i] >> 8);
		EEPROM.update(3+i*2, stored[i] & 0xFF);
	}
	EEPROM.update(8, set_t >> 8);
	EEPROM.update(9, set_t & 0xFF);
}

int getTemperature(void) {
	analogRead(TEMP_SENSE);//Switch ADC MUX
	uint16_t adc = median(TEMP_SENSE);
	if (adc >= 1020) { //Illegal value, tip not plugged in
		analogWrite(HEATER_PWM, 0);
		if (!off)
			setError(NO_TIP);
		return 999;
	} else {
		analogWrite(HEATER_PWM, pwm); //switch heater back to last value
	}
	return round(adc < 210 ? (((float)adc) * 0.530805 + 38.9298) : (((float)adc) * 0.415375 + 64.6123));
	//return round(((float) adc)*ADC_TO_TEMP_GAIN+ADC_TO_TEMP_OFFSET); //old conversion
}

void measureVoltage(void) {
	analogRead(BAT_C1); //Switch analog MUX before measuring
	v_c1 = v_c1*.9+(analogRead(BAT_C1)*5/1024.0)*.1;
	analogRead(BAT_C2);
	v_c2 = v_c2*.9+(analogRead(BAT_C2)*5/512.0)*.1;
	analogRead(BAT_C3);
	v_c3 = v_c3*.9+(analogRead(BAT_C3)*(5.0*1510.0)/(510.0*1024.0))*.1; //maximum measurable is 14.79V
}

uint16_t median(uint8_t analogIn) {
	uint16_t adcValue[3];
	for (uint8_t i = 0; i < 3; i++) {
		adcValue[i] = analogRead(analogIn); // read the input 3 times
	}
	uint16_t tmp;
	if (adcValue[0] > adcValue[1]) {
		tmp = adcValue[0];
		adcValue[0] = adcValue[1];
		adcValue[1] = tmp;
	}
	if (adcValue[1] > adcValue[2]) {
		tmp = adcValue[1];
		adcValue[1] = adcValue[2];
		adcValue[2] = tmp;
	}
	if (adcValue[0] > adcValue[1]) {
		tmp = adcValue[0];
		adcValue[0] = adcValue[1];
		adcValue[1] = tmp;
	}
	return adcValue[1];
}

void timer_sw_poll(void) {
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
		store_to = 255;
		store_invalid = true;
	} else if (error != NO_ERROR) {
		if (!(t1 |  t2 | t3)) {
			store_invalid = false;
		} else if (!store_invalid && t3) {
			error = NO_ERROR; //dismiss
			set_t_old = 0; //refresh set_t display
			store_invalid = true; //wait for release
		}
	} else {
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
	if((sw_up && sw_down) || !(sw_up || sw_down)) {
		cnt_but_press = 0;
		return;
	}
	if(sw_up || sw_down) {
		cnt_but_press++;
		if((cnt_but_press >= 100) || sw_changed) {
			setStandby(false);
			if(sw_up && set_t < MAX_TEMP) set_t++;
			else if (sw_down && set_t > MIN_TEMP) set_t--;
			if(!sw_changed) cnt_but_press = 97;
			updateEEPROM();
		}
	}
}

void setStandby(boolean state) {
	if (state == stby) return;
	stby = state;
	last_measured = cur_t;
	last_temperature_drop = millis();
	last_on_state = millis()/1000;
	EEPROM.update(1, stby);
}

void setOff(boolean state) {
	if (state == off) return;
	if (!state)
		analogWrite(HEATER_PWM, 0);
	else
		setStandby(false);
	if (power_source == POWER_USB && !state) {
		state = true; //don't switch on, if powered via USB
		setError(USB_ONLY);
	}
	last_on_state = millis()/1000;
	off = state;
	wasOff = true;
	last_measured = cur_t;
}

void display(void) {
	int16_t temperature = cur_t; //buffer volatile value
	boolean yell = stby || (stby_layoff && blink);
	tft.drawCircle(20,63,8, off?ST7735_RED:yell?ST7735_YELLOW:ST7735_GREEN);
	tft.drawCircle(20,63,7,off?ST7735_RED:yell?ST7735_YELLOW:ST7735_GREEN);
	tft.fillRect(19,55,3,3,ST7735_BLACK);
	tft.drawFastVLine(20,53,10, off?ST7735_RED:yell?ST7735_YELLOW:ST7735_GREEN);
	if (error != NO_ERROR) {
		if (error != error_old) {
			error_old = error;
			tft.setTextSize(1);
			tft.setTextColor(ST7735_RED, ST7735_BLACK);
			tft.setCursor(0,96);
			switch (error) {
				case EXCESSIVE_FALL:
					tft.print("Error: Temperature dropped\nTip slipped out?");
					break;
				case NOT_HEATING:
					tft.print("Error: Not heating\nWeak power source or short");
					break;
				case BATTERY_LOW:
					tft.print("Error: Battery low\nReplace or charge");
					break;
				case USB_ONLY:
					tft.print("Error: Power too low\nConnect power >5V");
					break;
				case NO_TIP:
					tft.print("Error: No tip connected\nTip slipped out?");
					break;
			}
			tft.setTextSize(2);
			tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
			tft.setCursor(10,112);
			tft.print("         OK ");
			
			tft.setTextColor(ST7735_RED, ST7735_BLACK);
			tft.setCursor(36,26);
			tft.setTextSize(3);
			tft.print(" ERR");
		}
	} else {
		if (error != error_old) {
			tft.fillRect(0, 96, 160, 16, ST7735_BLACK);
			error_old = NO_ERROR;
		}
		tft.setTextSize(2);
		tft.setCursor(15,112);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		tft.print(stored[0]);
		tft.write(' ');
		tft.print(stored[1]);
		tft.write(' ');
		tft.print(stored[2]);
		
		if (set_t_old != set_t || old_stby != (stby || stby_layoff)) {
			tft.setCursor(36,26);
			tft.setTextSize(3);
			if (stby || stby_layoff) {
				old_stby = true;
				tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
				tft.print("STBY");
			} else {
				old_stby = false;
				set_t_old = set_t;
				tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
				tft.write(' ');
				tft.print(set_t);
				tft.fillTriangle(149, 50, 159, 50, 154, 38, (set_t < MAX_TEMP) ? ST7735_WHITE : ST7735_GRAY);
				tft.fillTriangle(149, 77, 159, 77, 154, 90, (set_t > MIN_TEMP) ? ST7735_WHITE : ST7735_GRAY);
			}
		}
		if (!off) {
#ifdef SHUTOFF_ACTIVE
			if (autopower) {
				int16_t tout;
				if (stby) {
					tout = min(max(0,(last_on_state + OFF_TIMEOUT - (millis())/1000)), OFF_TIMEOUT);
				} else {
					tout = min(max(0,(last_temperature_drop + STANDBY_TIMEOUT - (millis())/1000)), STANDBY_TIMEOUT);
				}
				tft.setTextColor(stby?ST7735_RED:ST7735_YELLOW, ST7735_BLACK);
				tft.setTextSize(2);
				tft.setCursor(46,78);
				if (tout < 600) tft.write('0');
				tft.print(tout/60);
				tft.write(':');
				if (tout%60 < 10) tft.write('0');
				tft.print(tout%60);
			}
#endif
		} else if (temperature != 999) {
			tft.fillRect(46, 78, 60, 20, ST7735_BLACK);
		}
	}
	if (cur_t_old != temperature) {
		tft.setCursor(36,52);
		tft.setTextSize(3);
		if (temperature == 999) {
			tft.setTextColor(ST7735_RED, ST7735_BLACK);
			tft.print(" ERR");
			tft.setCursor(44,76);
			tft.setTextSize(2);
			tft.print("NO TIP");
		} else {
			if (cur_t_old == 999) {
				tft.fillRect(44,76,72,16,ST7735_BLACK);
			}
			tft.setTextColor(off ? temperature < 50 ? ST7735_CYAN : ST7735_RED : tft.Color565(min(10,abs(temperature-target_t))*25, 250 - min(10,max(0,(abs(temperature-target_t)-10)))*25, 0), ST7735_BLACK);
			if (temperature < 50) {
				tft.print("COLD");
			} else {
				tft.write(' ');
				if (temperature < 100) tft.write(' ');
				tft.print(temperature);
			}
		}
		if (temperature < cur_t_old)
			tft.drawFastHLine((int)(temperature/2.6), 0, 160-(int)(temperature/2.6), ST7735_BLACK);
		else if (cur_t != 999) {
			for (int16_t i = 0; i < temperature/2.6; i++) {
				tft.drawPixel(i, 0, tft.Color565(min(255, max(0, i*5)), min(255, max(0, 400-i*2.5)), 0));
			}
		}
		cur_t_old = temperature;
	}
	if (v_c3 > 1.0) {
		if (v_c3 < 5.0) {
			power_source = POWER_USB;
		} else if (v_c2 < 1.0) {
			power_source = POWER_CORD;
		} else {
			power_source = POWER_LIPO;
		}
		if (power_source != power_source_old) {
			tft.fillRect(0, 5, 128, 20, ST7735_BLACK);
			tft.fillRect(11, 25, 21, 20, ST7735_BLACK);
			switch (power_source) {
				case POWER_LIPO:
					for (uint8_t i = 0; i < 3; i++) {
						tft.fillRect(11, 5+i*14, 20, 12, ST7735_WHITE);
						tft.fillRect(12, 6+i*14, 18, 10, ST7735_BLACK);
						tft.drawFastVLine(31,8+i*14,6,ST7735_WHITE);
					}
					break;
				case POWER_USB:
					tft.setTextSize(1);
					tft.setTextColor(ST7735_RED, ST7735_BLACK);
					tft.setCursor(0,5);
					tft.print("USB power only\nConnect power supply.");
					if (!off) setError(USB_ONLY);
					break;
			}
			power_source_old = power_source;
		}
		if (power_source == POWER_CORD) {
			tft.setTextSize(2);
			tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
			tft.setCursor(0,5);
			tft.print(v_c3);
			tft.print("V ");
		} else if (power_source == POWER_LIPO) {
			float volt[] = {v_c1, v_c2-v_c1, v_c3-v_c2};
			for (uint8_t i = 0; i < 3; i++) {
				if (volt[i] < 3.20) {
					setError(BATTERY_LOW);
					tft.fillRect(13, 7+14*i, max(1,min(16,(volt[i]-3.0)*14.2)), 8, blink?ST7735_RED:ST7735_BLACK);
				} else {
					tft.fillRect(13, 7+14*i, max(1,min(16,(volt[i]-3.0)*14.2)), 8, tft.Color565(250-min(250, max(0, (volt[i]-3.4)*1000.0)), max(0,min(250, (volt[i]-3.15)*1000.0)), 0));
				}
				tft.fillRect(13+max(1,min(16,(volt[i]-3.0)*14.2)), 7+14*i, 17-max(1,min(16,(volt[i]-3.0)*14.2)), 8, ST7735_BLACK);
			}
		}
	}
#ifdef SHUTOFF_ACTIVE
	if (autopower) {
		Serial.print(pwm);
		Serial.print(" > ");
		Serial.println(max(20, (cur_t-150)/50*round(25-v_c3))+5);
		Serial.println(round(25-v_c3));
		if (pwm > max(20, (cur_t-150)/50*round(25-v_c3))+5) {
		//if (target_t-cur_t > 0.715*exp(0.0077*target_t)) {
		//if (cur_t / (double)target_t < STANDBY_TEMPERATURE_DROP) {
			if (stby && !wasOff) {
				setStandby(false);
			} else {
				last_temperature_drop = millis()/1000;
			}
		} else if (wasOff) {
			wasOff = false;
		}
		if (!off && !stby && millis()/1000 > (last_temperature_drop + STANDBY_TIMEOUT)) {
			setStandby(true);
		}
		if (!off && stby && millis()/1000 > (last_on_state + OFF_TIMEOUT)) {
			setOff(true);
		}
	}
#endif
	blink = !blink;
}

void compute(void) {
	cur_t = getTemperature();
	if (off) {
		target_t = 0;
		if (cur_t < TEMP_THRESHOLD) {
			threshold_counter = TEMP_UNDER_THRESHOLD; //reset counter
		}
	} else {
		if (stby_layoff || stby) {
			target_t = STBY_TEMP;
		} else {
			target_t = set_t;
		}
		if (cur_t-last_measured <= -30 && last_measured != 999) {
			setError(EXCESSIVE_FALL); //decrease of more than 30 degree is not realistic, short of ring and gnd is common.
		}
		if (cur_t < TEMP_THRESHOLD) {
			if (threshold_counter == 0) {
				setError(NOT_HEATING); //temperature is not reached in desired time, short of sensor and gnd too?
			} else {
				threshold_counter--;
			}
		} else {
			threshold_counter = THRES_MAX_DECEED; //reset counter to a smaller value to allow small oscillation of temperature
		}
	}
	
	set_td = target_t;
	cur_td = cur_t;
	last_measured = cur_t;

	heaterPID.Compute();
	if (error != NO_ERROR || off)
		pwm = 0;
	else
		pwm = min(255,pid_val*255);
	analogWrite(HEATER_PWM, pwm);
}

void timer_isr(void) {
	if (cnt_compute >= TIME_COMPUTE_IN_MS+DELAY_BEFORE_MEASURE) {
		compute();
		cnt_compute=0;
	} else if(cnt_compute >= TIME_COMPUTE_IN_MS) {
		analogWrite(HEATER_PWM, 0); //switch off heater to let the low pass settle
	}
	cnt_compute++;
	
	if(cnt_sw_poll >= TIME_SW_POLL_IN_MS){
		timer_sw_poll();
		cnt_sw_poll=0;
	}
	cnt_sw_poll++;
	
	if(cnt_measure_voltage >= TIME_MEASURE_VOLTAGE_IN_MS) {
		measureVoltage();
		cnt_measure_voltage=0;
	}
	cnt_measure_voltage++;
}

void setError(error_type e) {
	error = e;
	setOff(true);
}

void loop(void) {
	analogWrite(HEAT_LED, pwm);
	//Switch to following if the oscillation of the led bothers you
	//digitalWrite(HEAT_LED, cur_t+5 < target || (abs((int16_t)cur_t-(int16_t)target) <= 5 && (millis()/(stby?1000:500))%2));
	
	if (sendNext <= millis()) {
		sendNext += 100;
		Serial.print(stored[0]);
		Serial.print(";");
		Serial.print(stored[1]);
		Serial.print(";");
		Serial.print(stored[2]);
		Serial.print(";");
		Serial.print(off?0:1);
		Serial.print(";");
		Serial.print(error);
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
		Serial.print(v_c2>1.0?v_c1:0.0);
		Serial.print(";");
		Serial.print(v_c2);
		Serial.print(";");
		Serial.println(v_c3);
		Serial.flush();
		display();
	}
	if (Serial.available()) {
		uint16_t t = 0;
		switch (Serial.read()) {
			//Set new Temperature (eg. S350 to set to 350C)
			case 'T':
				if (Serial.available() >= 3) {
					t = serialReadTemp();
					Serial.println(t);
					if (t <= MAX_TEMP && t >= MIN_TEMP) {
						set_t = t;
						updateEEPROM();
					}
				}
				break;
			//Store new Preset (eg. P1200 to store 200C to Preset 1, NOT 0 indexed)
			case 'P':
				if (Serial.available() >= 4) {
					uint8_t slot = Serial.read()-'1';
					if (slot < 3) {
						t = serialReadTemp();
						if (t <= MAX_TEMP && t >= MIN_TEMP) {
							stored[slot] = t;
							updateEEPROM();
						}
					}
				}
				break;
			//Clear errors
			case 'C':
				error = NO_ERROR;
				break;
			//Set standby
			case 'S':
				setStandby(Serial.read() == '1');
				break;
			//Set on/off
			case 'O':
				setOff(Serial.read() == '0');
				break;
		}
	}
	delay(DELAY_MAIN_LOOP);
}

uint16_t serialReadTemp() {
	uint16_t t;
	uint8_t n;
	n = Serial.read()-'0';
	t = min(9, max(0, n))*100;
	n = Serial.read()-'0';
	t += min(9, max(0, n))*10;
	n = Serial.read()-'0';
	t += min(9, max(0, n))*1;
	return t;
}
