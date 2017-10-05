#include <SPI.h>
#include <TFT_ILI9163C.h>
#include <PID_v1.h>
#include <EEPROM.h>
#include "TimerOne.h"
#include "definitions.h"

/*
 * If your display stays white, uncomment this.
 * Cut reset trace (on THT on upper layer/0R), connect STBY_NO (A1) with reset of TFT (at 4050).
 * See also readme in mechanical folder for reference.
 */
//#define USE_TFT_RESET

/*
 * If red is blue and blue is red change this
 * If not sure, leave commented, you will be shown a setup screen
 */
//#define HARDWARE_DEFINED_TFT 2
/*
 * Based on your Hardware-Revision there may be modifications to the PCB. 
 * In V3 and up is a second voltage measurement circuit.
 * HW REVS:
 * 1.5 - 2.8:
 * For THT this should be set to anything < 3
 * Normally leave this commented as it is stored in EEPROM
 */

// V 1.5 - 2.11, Maiskolben THT
//#define HARDWARE_REVISION     2
// V 3.0 and 3.1
//#define HARDWARE_REVISION     3

/*
 * Only used for testing, do not use.
 */
//#define INSTALL
//#define TEST_ADC

volatile boolean off = true, stby = true, stby_layoff = false, sw_stby_old = false, sw_up_old = false, sw_down_old = false, clear_display = true, store_invalid = true, menu = false;
volatile uint8_t pwm, threshold_counter;
volatile int16_t cur_t, last_measured;
volatile error_type error = NO_ERROR;
error_type error_old;
int16_t stored[3] = {300, 350, 450}, set_t = TEMP_MIN, set_t_old, cur_t_old, target_t;
double pid_val, cur_td, set_td;
uint8_t store_to = 255;
p_source power_source, power_source_old = NO_INIT;
boolean blink;
uint16_t cnt_measure_voltage, cnt_compute, cnt_sw_poll, cnt_but_press, cnt_off_press, cnt_but_store;
float v_c1, v_c2, v_c3, v_in, v;
uint8_t array_index, array_count;
uint32_t sendNext;
uint32_t last_temperature_drop;
uint32_t last_on_state;
boolean wasOff = true, old_stby = false;
boolean autopower = true, bootheat = false;
uint8_t revision = 1;
boolean menu_dismissed = false;
boolean autopower_repeat_under = false;
boolean force_redraw = false;
boolean power_down = false;
uint16_t charge = 0;
float adc_offset = ADC_TO_TEMP_OFFSET;
float adc_gain = ADC_TO_TEMP_GAIN;

#define RGB_DISP 0x0
#define BGR_DISP 0x2

#ifdef USE_TFT_RESET
TFT_ILI9163C tft = TFT_ILI9163C(TFT_CS,  TFT_DC, STBY_NO);
#else
TFT_ILI9163C tft = TFT_ILI9163C(TFT_CS,  TFT_DC);
#endif
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF
#define GRAY    0x94B2

PID heaterPID(&cur_td, &pid_val, &set_td, kp, ki, kd, DIRECT);

void setup(void) {
	digitalWrite(HEATER_PWM, LOW);
	pinMode(HEATER_PWM, OUTPUT);
	pinMode(POWER, INPUT_PULLUP);
	pinMode(HEAT_LED, OUTPUT);
	digitalWrite(HEAT_LED, HIGH);
	pinMode(TEMP_SENSE, INPUT);
	pinMode(SW_T1, INPUT_PULLUP);
	pinMode(SW_T2, INPUT_PULLUP);
	pinMode(SW_T3, INPUT_PULLUP);
	pinMode(SW_UP, INPUT_PULLUP);
	pinMode(SW_DOWN, INPUT_PULLUP);
	pinMode(STBY_NO, INPUT_PULLUP);
	pinMode(SW_STBY, INPUT_PULLUP);
	pinMode(TFT_CS, OUTPUT);
	digitalWrite(TFT_CS, HIGH);
	Serial.begin(115200);
	
	boolean force_menu = false;
	if (EEPROM.read(0) != EEPROM_CHECK) {
		EEPROM.update(0, EEPROM_CHECK);
		updateEEPROM();
		force_menu = true;
	}
	tft.begin();
#ifdef HARDWARE_DEFINED_TFT
	#if HARDWARE_DEFINED_TFT == 1
		EEPROM.update(EEPROM_DISPLAY, RGB_DISP);
		setDisplayMode(0);
	#else
		EEPROM.update(EEPROM_DISPLAY, BGR_DISP);
		setDisplayMode(1);
	#endif
#else
	if (force_menu || EEPROM.read(EEPROM_VERSION) < 23 || EEPROM.read(EEPROM_VERSION) == 255 || (EEPROM.read(EEPROM_DISPLAY) != BGR_DISP && EEPROM.read(EEPROM_DISPLAY) != RGB_DISP)) {
		tft.fillScreen(BLACK);
		setDisplayMode(1);
		tft.setTextSize(2);
		tft.setCursor(0,0);
		tft.setTextColor(WHITE);
		tft.print(F("What color is displayed?"));
		tft.setCursor(10,112);
		tft.setTextColor(RED);
		tft.print("RED     BLUE");
		while (true) {
			if (!digitalRead(SW_T1)) {
				EEPROM.update(EEPROM_DISPLAY, BGR_DISP);
				setDisplayMode(1);
				break;
			}
			if (!digitalRead(SW_T3)) {
				EEPROM.update(EEPROM_DISPLAY, RGB_DISP);
				setDisplayMode(0);
				break;
			}
		}
		tft.fillScreen(BLACK);
		tft.setTextColor(YELLOW);
		tft.drawBitmap(0, 20, maiskolben, 160, 64, YELLOW);
		tft.setCursor(20,86);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("Maiskolben");
		tft.setCursor(35,104);
		tft.print("Welcome!");
		delay(4000);
		while (!digitalRead(SW_T3) || !digitalRead(SW_T1)) delay(100);
	} else {
		setDisplayMode(EEPROM.read(EEPROM_DISPLAY) == BGR_DISP);
	}
#endif
#ifdef INSTALL
	if (EEPROM.read(EEPROM_INSTALL) != EEPROM_CHECK) {
		tft.fillScreen(BLACK);
		tft.setTextColor(RED, BLACK);
		tft.setCursor(0,0);
		tft.setTextSize(2);
		tft.println("Installation");
		for (int16_t i = -255; i < 256; i++) {
			analogWrite(HEAT_LED, 255-abs(i));
			delay(1);
		}
		uint16_t adc1 = 0, adc2 = 0;
		while (digitalRead(SW_STBY)) {
			int t = getTemperature();
			uint16_t adc = analogRead(TEMP_SENSE);
			Serial.println(t);
			digitalWrite(HEATER_PWM, !digitalRead(SW_T1) | !digitalRead(SW_T2) | !digitalRead(SW_T3)/* | !digitalRead(SW_UP) | !digitalRead(SW_DOWN)*/);
			if (!digitalRead(SW_DOWN)) {
				if (!adc) {
					digitalWrite(HEATER_PWM, HIGH);
				} else {
					adc1 = adc;
				}
			}
			if (!digitalRead(SW_UP)) {
				if (!adc) {
					digitalWrite(HEATER_PWM, HIGH);
				} else {
					adc2 = adc;
				}
			}
			tft.setCursor(0,18);
			tft.print(t);
			tft.println("  ");
			tft.print(adc);
			tft.println("   ");
			tft.println(adc * adc_gain + adc_offset);
			if (adc1 != 0 && adc2 != 0) {
				adc_gain = DELTA_REF_T / (float)(adc2 - adc1);
				adc_offset = REF_T1 - adc_gain * adc1;
				tft.println(adc_gain);
				tft.println(adc_offset);
			}
			delay(50);
		}
		EEPROM.update(EEPROM_OPTIONS, (bootheat << 1) | autopower);
		EEPROM.update(EEPROM_VERSION, EE_VERSION);
		EEPROM.update(EEPROM_INSTALL, EEPROM_CHECK);
		EEPROM.put(EEPROM_ADCTTG, adc_gain);
		EEPROM.put(EEPROM_ADCOFF, adc_offset);
		
		tft.println("done.");
		delay(1000);
		asm volatile("jmp 0");
	}
#endif
	if (EEPROM.read(EEPROM_VERSION) != EE_VERSION) {
		force_menu = true;
	}
	tft.fillScreen(BLACK);
	uint8_t options = EEPROM.read(EEPROM_OPTIONS);
	autopower = options & 1;
	bootheat = (options >> 1) & 1;
	if (force_menu) optionMenu();
	else {
		updateRevision();
		tft.drawBitmap(0, 20, maiskolben, 160, 64, YELLOW);
		tft.setCursor(20,86);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("Maiskolben");
		tft.setCursor(50,110);
		tft.setTextSize(1);
		tft.print("Version ");
		tft.print(VERSION);
		tft.setCursor(46,120);
		tft.print("HW Revision ");
		tft.print(revision);
		//Allow Options to be set at startup
		delay(100);
		attachInterrupt(digitalPinToInterrupt(SW_STBY), optionMenu, LOW);
		for (int i = 0; i < 10 && !menu_dismissed; i++) {
			digitalWrite(HEAT_LED, i % 2);
			delay(250);
		}
		detachInterrupt(digitalPinToInterrupt(SW_STBY));
	}
	/*
	 * lower frequency = noisier tip
	 * higher frequency = needs higher pwm
	 */
	//PWM Prescaler = 1024 31Hz
	//TCCR2B = (TCCR2B & 0b11111000) | 7;
	//PWM Prescaler = 256 122Hz
	//TCCR2B = (TCCR2B & 0b11111000) | 6;
	//PWM Prescaler = 128 245Hz
	TCCR2B = (TCCR2B & 0b11111000) | 5;
	//PWM Prescaler = 64  490Hz
	//TCCR2B = (TCCR2B & 0b11111000) | 4
	//PWM Prescaler = 32  980Hz
	//TCCR2B = (TCCR2B & 0b11111000) | 3;
	//PWM Prescaler = 8  3.9kHz
	//TCCR2B = (TCCR2B & 0b11111000) | 2
	//PWM Prescaler = 1    31kHz - no Noise
	//TCCR2B = (TCCR2B & 0b11111000) | 1;
	stby = EEPROM.read(1);
	for (uint8_t i = 0; i < 3; i++) {
		stored[i] = EEPROM.read(2+i*2) << 8;
		stored[i] |= EEPROM.read(3+i*2);
	}
	set_t = EEPROM.read(EEPROM_SET_T) << 8;
	set_t |= EEPROM.read(EEPROM_SET_T+1);

	for (uint8_t i = 0; i < 50; i++)
		measureVoltage(); //measure average 50 times to get realistic results
	
	tft.fillScreen(BLACK);
	last_measured = getTemperature();
	Timer1.initialize(1000);
	Timer1.attachInterrupt(timer_isr);
	heaterPID.SetMode(AUTOMATIC);
	sendNext = millis();
	if (bootheat) {
		threshold_counter = TEMP_UNDER_THRESHOLD;
		setOff(false);
	}
	EEPROM.get(EEPROM_ADCTTG, adc_gain);
	EEPROM.get(EEPROM_ADCOFF, adc_offset);
}

void updateRevision(void) {	
#if (HARDWARE_REVISION > 2)
	EEPROM.update(EEPROM_REVISION, HARDWARE_REVISION);
	revision = 3;
#else
	if (EEPROM.read(EEPROM_VERSION) < 26 || EEPROM.read(EEPROM_REVISION) > 100) {
		EEPROM.update(EEPROM_REVISION, 2);
		revision = 2;
	} else {
		revision = EEPROM.read(EEPROM_REVISION);
	}
#endif
}

void setDisplayMode(boolean bgr) {
	tft.colorSpace(bgr);
	tft.setRotation(3);
}

void optionMenu(void) {
	tft.fillScreen(BLACK);
	digitalWrite(HEAT_LED, LOW);
	tft.setTextSize(2);
	tft.setCursor(0,0);
	tft.setTextColor(WHITE);
	tft.println("Options\n");
	tft.setTextColor(WHITE);
	tft.setCursor(10,112);
	tft.print("ON  OFF EXIT");
	uint8_t options = 2;
	uint8_t opt = 0;
	boolean redraw = true;
	while (true) {
		if (redraw) {
			tft.setCursor(0,36);
			#ifdef SHUTOFF_ACTIVE
				tft.setTextColor(autopower?GREEN:RED);
			#else
				tft.setTextColor(GRAY);
			#endif
			tft.println(" Autoshutdown");
			#ifdef BOOTHEAT_ACTIVE
				tft.setTextColor(bootheat?GREEN:RED);
			#else
				tft.setTextColor(GRAY);
			#endif
			tft.println(" Heat on boot");
			
			tft.setCursor(0, (opt+2)*18);
			tft.setTextColor(WHITE);
			tft.print(">");
			redraw = false;
		}
		if (!digitalRead(SW_UP)) {
			tft.setCursor(0, (opt+2)*18);
			tft.setTextColor(BLACK);
			tft.print(">");
			opt = (opt+options-1)%options;
			while (!digitalRead(SW_UP)) delay(100);
			redraw = true;
		}
		if (!digitalRead(SW_DOWN)) {
			tft.setCursor(0, (opt+2)*18);
			tft.setTextColor(BLACK);
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
	updateRevision();
	EEPROM.update(EEPROM_VERSION, EE_VERSION);
	if (EEPROM.read(EEPROM_VERSION) < 30) {
		EEPROM.put(EEPROM_ADCTTG, ADC_TO_TEMP_GAIN);
		EEPROM.put(EEPROM_ADCOFF, ADC_TO_TEMP_OFFSET);
	}
	menu_dismissed = true;
}

void updateEEPROM(void) {
	EEPROM.update(1, stby);
	for (uint8_t i = 0; i < 3; i++) {
		EEPROM.update(2+i*2, stored[i] >> 8);
		EEPROM.update(3+i*2, stored[i] & 0xFF);
	}
	EEPROM.update(8, set_t >> 8);
	EEPROM.update(9, set_t & 0xFF);
	EEPROM.update(EEPROM_OPTIONS, (bootheat << 1) | autopower);
}

void powerDown(void) {
	if (power_source != POWER_LIPO) {
		power_down = false;
		return;
	}
	//Timer1.stop();
	setOff(true);
	delay(10);
	tft.fillScreen(BLACK);
	tft.setTextSize(4);
	tft.setTextColor(RED);
	tft.setCursor(50,40);
	tft.print("OFF");
	delay(3000);
	SPI.end();
	digitalWrite(POWER, LOW);
	pinMode(POWER, OUTPUT);
	delay(100);
	force_redraw = true;
	power_down = false;
	Timer1.start(); //unsuccessful
}

int getTemperature(void) {
	analogRead(TEMP_SENSE);//Switch ADC MUX
	uint16_t adc = median(TEMP_SENSE);
#ifdef TEST_ADC
	Serial.println(adc);
#endif
	if (adc >= 900) { //Illegal value, tip not plugged in - would be around 560deg
		analogWrite(HEATER_PWM, 0);
		if (!off)
			setError(NO_TIP);
		return 999;
	} else {
		analogWrite(HEATER_PWM, pwm); //switch heater back to last value
	}
	//return round(adc < 210 ? (((float)adc) * 0.530805 + 38.9298) : (((float)adc) * 0.415375 + 64.6123)); //old conversion
	return round(((float) adc) * adc_gain + adc_offset);
}

void measureVoltage(void) {
	analogRead(BAT_C1); //Switch analog MUX before measuring
	v_c1 = v_c1*.9+(analogRead(BAT_C1)*5/1024.0)*.1; //no divisor
	analogRead(BAT_C2);
	v_c2 = v_c2*.9+(analogRead(BAT_C2)*5/512.0)*.1; //divisor 1:1 -> /2
	analogRead(BAT_C3);
	v_c3 = v_c3*.9+(analogRead(BAT_C3)*(5.0*3.0)/1024.0)*.1; //maximum measurable is ~15V
	v = v_c3;
	if (revision < 3) return;
#ifdef VIN
	analogRead(VIN);
	v_in = v_in*.9+(analogRead(VIN)*25/1024.0)*.1; //maximum measurable is ~24.5V
	v = v_in; //backwards compatibility
#endif
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
	if (power_down) return;
	if (!digitalRead(SW_STBY)) {
		if (cnt_off_press == 100) {
			setOff(!off);
		}
		if (cnt_off_press == 200 && power_source == POWER_LIPO) {
			setOff(true);
			power_down = true;
			return;
		}
		cnt_off_press = min(201, cnt_off_press+1);
	} else {
		if (cnt_off_press > 0 && cnt_off_press <= 100) {
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
			if(sw_up && set_t < TEMP_MAX) set_t++;
			else if (sw_down && set_t > TEMP_MIN) set_t--;
			if(!sw_changed) cnt_but_press = 97;
			updateEEPROM();
		}
	}
}

void setStandby(boolean state) {
	if (stby_layoff) return;
	if (state == stby) return;
	stby = state;
	last_measured = cur_t;
	last_temperature_drop = millis();
	last_on_state = millis()/1000;
	EEPROM.update(1, stby);
}
void setStandbyLayoff(boolean state) {
	if (state == stby_layoff) return;
	stby_layoff = state;
	stby = false;
	last_measured = cur_t;
	last_on_state = millis()/1000;
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
	if (force_redraw) tft.fillScreen(BLACK);
	int16_t temperature = cur_t; //buffer volatile value
	boolean yell = stby || (stby_layoff && blink);
	tft.drawCircle(20,63,8, off?RED:yell?YELLOW:GREEN);
	tft.drawCircle(20,63,7,off?RED:yell?YELLOW:GREEN);
	tft.fillRect(19,55,3,3,BLACK);
	tft.drawFastVLine(20,53,10, off?RED:yell?YELLOW:GREEN);
	if (error != NO_ERROR) {
		if (error != error_old || force_redraw) {
			error_old = error;
			tft.setTextSize(1);
			tft.setTextColor(RED, BLACK);
			tft.setCursor(0,96);
			switch (error) {
				case EXCESSIVE_FALL:
					tft.print(F("Error: Temperature dropped\nTip slipped out?"));
					break;
				case NOT_HEATING:
					tft.print(F("Error: Not heating\nWeak power source or short"));
					break;
				case BATTERY_LOW:
					tft.print(F("Error: Battery low\nReplace or charge"));
					break;
				case USB_ONLY:
					tft.print(F("Error: Power too low\nConnect power >5V"));
					break;
				case NO_TIP:
					tft.print(F("Error: No tip connected\nTip slipped out?"));
					break;
			}
			tft.setTextSize(2);
			tft.setTextColor(YELLOW, BLACK);
			tft.setCursor(10,112);
			tft.print(F("         OK "));
			
			tft.setTextColor(RED, BLACK);
			tft.setCursor(36,26);
			tft.setTextSize(3);
			tft.print(F(" ERR  "));
		}
	} else {
		if (error != error_old || force_redraw) {
			tft.fillRect(0, 96, 160, 16, BLACK);
			error_old = NO_ERROR;
		}
		tft.setTextSize(2);
		tft.setCursor(15,112);
		tft.setTextColor(WHITE, BLACK);
		tft.print(stored[0]);
		tft.write(' ');
		tft.print(stored[1]);
		tft.write(' ');
		tft.print(stored[2]);
		
		if (set_t_old != set_t || old_stby != (stby || stby_layoff) || force_redraw) {
			tft.setCursor(36,26);
			tft.setTextSize(3);
			if (stby || stby_layoff) {
				old_stby = true;
				tft.setTextColor(YELLOW, BLACK);
				tft.print(F("STBY  "));
			} else {
				old_stby = false;
				set_t_old = set_t;
				tft.setTextColor(WHITE, BLACK);
				tft.write(' ');
				tft.print(set_t);
				tft.write(247);
				tft.write('C');
				tft.fillTriangle(149, 50, 159, 50, 154, 38, (set_t < TEMP_MAX) ? WHITE : GRAY);
				tft.fillTriangle(149, 77, 159, 77, 154, 90, (set_t > TEMP_MIN) ? WHITE : GRAY);
			}
		}
		if (!off) {
#ifdef SHUTOFF_ACTIVE
			if (autopower) {
				int16_t tout;
				if (stby || stby_layoff) {
					tout = min(max(0,(last_on_state + OFF_TIMEOUT - (millis())/1000)), OFF_TIMEOUT);
				} else {
					tout = min(max(0,(last_temperature_drop + STANDBY_TIMEOUT - (millis())/1000)), STANDBY_TIMEOUT);
				}
				tft.setTextColor(stby?RED:YELLOW, BLACK);
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
			tft.fillRect(46, 78, 60, 20, BLACK);
		}
	}
	if (cur_t_old != temperature || force_redraw) {
		tft.setCursor(36,52);
		tft.setTextSize(3);
		if (temperature == 999) {
			tft.setTextColor(RED, BLACK);
			tft.print(F(" ERR  "));
			tft.setCursor(44,76);
			tft.setTextSize(2);
			tft.print(F("NO TIP"));
		} else {
			if (cur_t_old == 999) {
				tft.fillRect(44,76,72,16,BLACK);
			}
			tft.setTextColor(off ? temperature < TEMP_COLD ? CYAN : RED : tft.Color565(min(10,abs(temperature-target_t))*25, 250 - min(10,max(0,(abs(temperature-target_t)-10)))*25, 0), BLACK);
			if (temperature < TEMP_COLD) {
				tft.print(F("COLD  "));
			} else {
				tft.write(' ');
				if (temperature < 100) tft.write(' ');
				tft.print(temperature);
				tft.write(247);
				tft.write('C');
			}
		}
		if (temperature < cur_t_old)
			tft.fillRect(max(0, (temperature - TEMP_COLD)/2.4), 0, 160-max(0, (temperature - TEMP_COLD)/2.4), BAR_HEIGHT, BLACK);
		else if (cur_t != 999) {
			for (int16_t i = max(0, (cur_t_old - TEMP_COLD)/2.4); i < max(0, (temperature - TEMP_COLD)/2.4); i++) {
				tft.drawFastVLine(i, 0, BAR_HEIGHT, tft.Color565(min(255, max(0, i*5)), min(255, max(0, 450-i*2.5)), 0));
			}
		}
		cur_t_old = temperature;
	}
	if (v_c3 > 1.0) {
		tft.setTextColor(YELLOW, BLACK);
		tft.setCursor(122,5);
		tft.setTextSize(2);
		int power = min(15,v)*min(15,v)/4.8*pwm/255;
		if (power < 10) tft.write(' ');
		tft.print(power);
		tft.write('W');
		
		if (v < 5.0) {
			power_source = POWER_USB;
		} else if (v_c2 < 1.0) {
			power_source = POWER_CORD;
		} else {
			power_source = POWER_LIPO; //Set charging later to not redraw if charging mode toggles
		}
		if (power_source != power_source_old || force_redraw) {
			tft.fillRect(0, 5, 128, 20, BLACK);
			tft.fillRect(11, 25, 21, 20, BLACK);
			switch (power_source) {
				case POWER_CHARGING:
				case POWER_LIPO:
					for (uint8_t i = 0; i < 3; i++) {
						tft.drawRect(11, 5+i*14, 20, 12, WHITE);
						//tft.fillRect(12, 6+i*14, 18, 10, BLACK);
						tft.drawFastVLine(31,8+i*14,6,WHITE);
					}
					break;
				case POWER_USB:
					tft.setTextSize(1);
					tft.setTextColor(RED, BLACK);
					tft.setCursor(0,5);
					tft.print("USB power only\nConnect power supply.");
					if (!off) setError(USB_ONLY);
					break;
			}
			power_source_old = power_source;
		}
		if (power_source == POWER_CORD) {
			/*if (v > v_c3) {
				tft.setTextSize(2);
				tft.setTextColor(GREEN, BLACK);
				tft.setCursor(0,5);
				tft.print(v);
				tft.print("V ");
			} else {*/
				tft.drawBitmap(0, 5, power_cord, 24, 9, tft.Color565(max(0, min(255, (14.5-v)*112)), max(0, min(255, (v-11)*112)), 0));
			//}
		} else if (power_source == POWER_LIPO || power_source == POWER_CHARGING) {
			float volt[] = {v_c1, v_c2-v_c1, v_c3-v_c2};
			uint8_t volt_disp[] = {max(1,min(16,(volt[0]-3.0)*14.2)), max(1,min(16,(volt[1]-3.0)*14.2)), max(1,min(16,(volt[2]-3.0)*14.2))};
			if (power_source == POWER_CHARGING) {
				uint8_t p = min(16, (millis() / 100) % 20);
				for (uint8_t i = 0; i < 3; i++) {
					volt_disp[i] = max(0, min(volt_disp[i], p));
				}
			}
			for (uint8_t i = 0; i < 3; i++) {
				if (volt[i] < 3.20) {
					setError(BATTERY_LOW);
					tft.fillRect(13, 7+14*i, volt_disp[i], 8, blink?RED:BLACK);
				} else {
					tft.fillRect(13, 7+14*i, volt_disp[i], 8, tft.Color565(250-min(250, max(0, (volt[i]-3.4)*1000.0)), max(0,min(250, (volt[i]-3.15)*1000.0)), 0));
				}
				tft.fillRect(13+volt_disp[i], 7+14*i, 17-volt_disp[i], 8, BLACK);
			}
		}
	}
#ifdef SHUTOFF_ACTIVE
	if (autopower) {
		if (!stby_layoff) {
			if (pwm > max(20, (cur_t-150)/50*round(25-min(15,v)))+5) {
			//if (target_t-cur_t > 0.715*exp(0.0077*target_t)) {
			//if (cur_t / (double)target_t < STANDBY_TEMPERATURE_DROP) {
				if (autopower_repeat_under || stby) {
					if (stby && !wasOff) {
						setStandby(false);
					} else {
						last_temperature_drop = millis()/1000;
					}
				}
				autopower_repeat_under = true;
			} else if (wasOff) {
				wasOff = false;
			} else {
				autopower_repeat_under = false; //over the max pwm for at least two times
			}
		}
		if (!off && !stby && millis()/1000 > (last_temperature_drop + STANDBY_TIMEOUT)) {
			setStandby(true);
		}
		if (!off && (stby || stby_layoff) && millis()/1000 > (last_on_state + OFF_TIMEOUT)) {
			setOff(true);
		}
	}
#endif
	blink = !blink;
	force_redraw = false;
}

void compute(void) {
#ifndef USE_TFT_RESET
	setStandbyLayoff(!digitalRead(STBY_NO)); //do not measure while heater is active, potential is not neccessary == GND
#endif
	cur_t = getTemperature();
	if (off) {
		target_t = 0;
		if (cur_t < adc_offset + TEMP_RISE) {
			threshold_counter = TEMP_UNDER_THRESHOLD; //reset counter
		}
	} else {
		if (stby_layoff || stby) {
			target_t = TEMP_STBY;
		} else {
			target_t = set_t;
		}
		if (cur_t-last_measured <= -30 && last_measured != 999) {
			setError(EXCESSIVE_FALL); //decrease of more than 30 degree is uncommon, short of ring and gnd is possible.
		}
		if (cur_t < adc_offset + TEMP_RISE) {
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
	if (cnt_compute >= TIME_COMPUTE_IN_MS) {
		analogWrite(HEATER_PWM, 0); //switch off heater to let the low pass settle
		
		if (cnt_compute >= TIME_COMPUTE_IN_MS+DELAY_BEFORE_MEASURE) {
			compute();
			cnt_compute=0;
		}
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
#ifndef TEST_ADC
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
		Serial.println(v);
#endif
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
					//Serial.println(t);
					if (t <= TEMP_MAX && t >= TEMP_MIN) {
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
						if (t <= TEMP_MAX && t >= TEMP_MIN) {
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
	if (power_down) {
		powerDown();
	}
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
