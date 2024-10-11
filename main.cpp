#include <string>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

class Pin
{
private:
  int pin;
  bool isOut;

public:
  bool state;

  Pin(int p, bool isO)
  {
    this->pin = p;
    this->isOut = isO;

    gpio_init(this->pin);
    gpio_set_dir(this->pin, this->isOut);
  }

  virtual void put(int high)
  {
    if(!this->isOut)
      {
	return;
      }

    gpio_put(this->pin, high);
    this->state = high;
  }

  virtual bool get()
  {
    if(this->isOut)
      {
	return false;
      }
  }

  virtual void toggle(){
    this->state = !this->state;

    gpio_put(this->pin, this->state == true ? 1 : 0);
  }
};

class Ultrasonic{
private:
  int last = 0;
  int trig = 0;
  int echo = 0;

  uint64_t pulseIn()
  {
    while((gpio_get(this->echo)) <= 0);
    uint64_t start = time_us_64();

    while((gpio_get(this->echo)) > 0);

    uint64_t end = time_us_64();

    this->last = (end - start) / 58;

    return (end - start) / 58;
  }

public:
  Ultrasonic(int t, int e)
  {
    this->trig = t;
    this->echo = e;

    gpio_init(this->trig);
    gpio_set_dir(this->trig, GPIO_OUT);

    gpio_init(this->echo);
    gpio_set_dir(this->echo, GPIO_IN);
  }

  void reset(){
    this->last = 0;
  }

  uint64_t get()
  {
    gpio_put(this->trig, 1);
    sleep_us(10);
    gpio_put(this->trig, 0);

    return this->pulseIn();
  }

  bool diff(int infelicity)
  {
    int l = this->last;
    int n = this->get();

    if(l == 0){return false;}

    if(n > l + infelicity || n < l - infelicity)
      {
	return true;
      }
    else
      {
	return false;
      }
    return false;
  }
};

class LED{
    private:
        int pin;
        bool state;
    public:
        LED(int p){
            this->pin = p;

            gpio_init(this->pin);
            gpio_set_dir(this->pin, GPIO_OUT);

            this->state = false;
        }

        void on(){gpio_put(this->pin, 1);}
        void off(){gpio_put(this->pin, 0);}

        void toggle(){
            this->state = !this->state;
            gpio_put(this->pin, this->state);
        }

        void toggle(int delay){
            this->state = !this->state;
            gpio_put(this->pin, this->state);
            sleep_ms(delay);
        }
};

class Button{
    private:
        int pin;
    public:
        Button(int p){
            this->pin = p;

            gpio_init(this->pin);
            gpio_set_dir(this->pin, GPIO_IN);
        }

        bool get(){
            return gpio_get(this->pin) > 0 ? true : false;
        }
};

class ESD{
private:
  int pins[12];
public:
  uint8_t letters[26] = {
    0b01110110, // A
    0b01111111, // B
    0b00111001, // C
    0b00111111, // D
    0b01111001, // E
    0b01110001, // F
    0b01111011, // G
    0b01110110, // H
    0b00000110, // I
    0b00001110, // J
    0b01110101, // K
    0b00111000, // L
    0b00110111, // M
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b01110111, // R
    0b01101101, // S
    0b01001111, // T
    0b00111110, // U
    0b00111110, // V
    0b01111110, // W
    0b01110110, // X
    0b01101111, // Y
    0b01011011, // Z
  };

  uint8_t numbers[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
  };

  ESD(int p[12]){
    for(int i = 0; i < 12; i++){this->pins[i] = p[i]; gpio_init(this->pins[i]); gpio_set_dir(this->pins[i], GPIO_OUT);}
  }

  void reset(){
    for(int i = 0; i < 12; i++){gpio_put(this->pins[i], 0);}
  }

  void symbol(int segment, uint8_t c, int duration=1, bool reset=true, LED* sig=NULL){
    int seg = this->pins[8 + segment];

    for(int i = 8; i < 12; i++){
      if(this->pins[i] == seg){continue;}
      gpio_put(this->pins[i], 1);
    }
 
    for(int i = 0; i < 8; i++){
      if((c & (((uint8_t)0x00000001) << i)) != 0){gpio_put(this->pins[i], 1);}
    }

    sleep_ms(duration);

    if(reset){
      for(int i = 0; i < 12; i++){
	gpio_put(this->pins[i], 0);
      }
    }
  }

  void all(uint8_t c, int duration=1){
    for(int i = 0; i < 4; i++){
      this->symbol(i, c, duration);
    }
  }

  void sequence(uint8_t letters[4], int d=10){
    for(int i = 0; i < 4; i++){
      this->symbol(i, letters[i], true);
      sleep_ms(d);
    }
  }

  void text(char* text, int times=1){
    for(int j = 0; j < times; j++){
      for(int i = 0; i < strlen(text) && i < 4; i++){
	int code = (int)(text[i]) - 97;

	this->symbol(i, this->letters[code]); // a is ASCII 65, char operations, YAY!!!
	sleep_ms(1);
      }
    }
  }

  void number(int num, int times=1){
    if(num > 9999 || num < -999){return;}

    char s[4];

    sprintf(s, "%d", num);

    for(int j = 0; j < times; j++){
      for(int i = 0; i < 4 && i < strlen(s); i++){
	if(s[i] == '-'){this->symbol(i, (uint8_t)0b01000000); continue;}

	int n = s[i] - 48;

	this->symbol(i, this->numbers[n]);
      }
    }
  }
};

class PWMPin{
private:
  int num;
  int slicenum;
public:
  PWMPin(int n){
    this->num = n;
    this->slicenum = pwm_gpio_to_slice_num(this->num);

    gpio_set_function(this->num, GPIO_FUNC_PWM);

    pwm_set_clkdiv(this->slicenum, 125.0f);
    pwm_set_wrap(this->slicenum, 1024);
    pwm_set_enabled(this->slicenum, false);
  }

  void on(int level){
    pwm_set_gpio_level(this->num, level);
    this->on();
  }

  void on(){
    pwm_set_enabled(this->slicenum, true);
  }

  void off(){
    pwm_set_enabled(this->slicenum, false);
  }
};

int main(){
  int pins[12] = {2, 6, 11, 9, 10, 3, 8, 12, 1, 4, 5, 7};
  ESD d(pins);

  PWMPin buzzer(15);

  Button control(23);
  Button mode(19);

  int cooldown = 0;
  bool random_mode = false;

  while(1){
    cooldown = 0;

    while(!control.get()){
      d.text((char*)"sel");
    }

    random_mode = mode.get();

    while(control.get()){
      cooldown += 10;
      d.number(cooldown, 100);
    }

    d.text(random_mode ? (char*)"rand" : (char*)"buzz", 50);

    for(int i = 0; i < 5; i++){d.text((char*)"fire", 50); sleep_ms(700);}

    while(cooldown > 0){
      cooldown--;
      d.number(cooldown, 50);
    }

    if(!random_mode){
      buzzer.on(512);
      d.text((char*)"buzz", 1000);
      buzzer.off();
    }
    else{
      for(int i = 0; i < 50; i++){
	buzzer.on(rand() % 1024);
	d.text((char*)"rand", rand() % 1000);
	buzzer.off();
	sleep_ms(rand() % 600000);
      }
    }
  }
}
