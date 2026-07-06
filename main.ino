#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

#define ARM_LEFT D5 
#define ARM_RIGHT D7 

#define ARM_UPPER_LEFT D4
#define ARM_UPPER_RIGHT D6

#define ANGLE_MAX 180
#define ANGLE_STEP 5

Servo arm_right;
Servo arm_left;

Servo arm_upper_left;
Servo arm_upper_right;


struct ArmPos {
  float upper_left_pos;
  float upper_right_pos;
  float lower_left_pos;
  float lower_right_pos;
};

ArmPos positions;

void setArmsPos(ArmPos* pos_data) {
  arm_right.write(pos_data->lower_right_pos);
  arm_left.write(pos_data->lower_left_pos);
}

void wave_lower_arms(){
  if(positions.lower_right_pos + ANGLE_STEP < ANGLE_MAX){
    positions.lower_right_pos += ANGLE_STEP;
  }
  else{
    positions.lower_right_pos = 0;
  }

  if(positions.lower_left_pos + ANGLE_STEP < 180){
    positions.lower_left_pos += ANGLE_STEP;
  }
  else{
    positions.lower_left_pos = 0;
  }
}

class LGFX_Xiao_35; 

class Eye {
  private:
	float x, y, rx, ry, original_ry; 
	int blink_step = 0; 

  public:
  int animation_state = 0;

	Eye(float _x, float _y, float _rx, float _ry): x(_x), y(_y), rx(_rx), ry(_ry), original_ry(_ry){}

	void draw(LGFX_Sprite *sprite);
	void start_blink();
	bool update_blink(LGFX_Sprite *sprite);
  void set_state(int state); 
};

class LGFX_Xiao_35 : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488     _panel_instance;
  lgfx::Bus_SPI           _bus_instance;
  lgfx::Touch_XPT2046     _touch_instance;

public:
  LGFX_Xiao_35() {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;     
      cfg.spi_mode = 0;             
      cfg.freq_write = 40000000;    
      cfg.freq_read  = 16000000;    
      cfg.pin_sclk = D8;            
      cfg.pin_mosi = D10;           
      cfg.pin_miso = D9;            
      cfg.pin_dc   = D2;            
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = D1;    
      cfg.pin_rst          = D3;    
      cfg.panel_width      = 320;   
      cfg.panel_height     = 480;   
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 300;         
      cfg.x_max      = 3900;
      cfg.y_min      = 200;
      cfg.y_max      = 3700;
      cfg.pin_cs     = D4;          
      cfg.bus_shared = true;        
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};

LGFX_Xiao_35 lcd;
LGFX_Sprite canvas(&lcd);

uint16_t COLOR_EYES  = 0x8B2C;
uint16_t COLOR_MASK  = 0xD636;

void Eye::draw(LGFX_Sprite* sprite){ 
  if (this->ry > 0) {
    sprite->fillEllipse(this->x, this->y, this->rx, this->ry, COLOR_EYES);
  }
}

void Eye::set_state(int state){
  this->animation_state = state;
}

void Eye::start_blink() {
  if (blink_step == 0) {
    blink_step = 1; 
  }
}

bool Eye::update_blink(LGFX_Sprite* sprite) {
  if (blink_step == 1) { 
    this->ry -= 8;
    if (this->ry <= 0) {
      this->ry = 0;
      blink_step = 2; 
    }
  } 
  else if (blink_step == 2) { 
    this->ry += 8;
    if (this->ry >= this->original_ry) {
      this->ry = this->original_ry;
      blink_step = 0; 
    }
  }

  this->draw(sprite);
  return (blink_step != 0);
}

int start_x = 140;        
int start_y = 320 / 2;
int radius_x = 50;
int radius_y = 80;
int eye_spacing = 200;

unsigned long int lastWiFiCheck = 0;
const unsigned long int wifi_check_interval = 3000;

unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 4000;

unsigned long lastMotorTime = 0;
unsigned long motorInterval = 100;

Eye eye_a(start_x, start_y, radius_x, radius_y);
Eye eye_b(start_x + eye_spacing, start_y, radius_x, radius_y);

WiFiManager wifi_manager;

void setup(void) {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(1); 

  arm_right.setPeriodHertz(50);
  arm_left.setPeriodHertz(50);

  arm_upper_right.setPeriodHertz(50);
  arm_upper_left.setPeriodHertz(50);

  arm_right.attach(ARM_RIGHT);
  arm_left.attach(ARM_LEFT);

  arm_upper_right.attach(ARM_UPPER_RIGHT);
  arm_upper_left.attach(ARM_UPPER_LEFT);
  
  canvas.setColorDepth(8);
  canvas.createSprite(480, 320);
  canvas.fillScreen(COLOR_MASK);
  canvas.pushSprite(0, 0);

  positions.lower_left_pos = 0;
  positions.lower_right_pos = 0;
  positions.upper_left_pos = 0;
  positions.upper_right_pos = 0;

  wifi_manager.resetSettings();
  wifi_manager.setConfigPortalBlocking(false);
  wifi_manager.autoConnect("SOUP HEAD v2");
}

void loop(void) {
  if (millis() - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = millis();
    eye_a.start_blink();
    eye_b.start_blink();
    blinkInterval = random(2000, 6000); 
  }

  canvas.fillScreen(COLOR_MASK);
  
  eye_a.update_blink(&canvas);
  eye_b.update_blink(&canvas);
  
  canvas.pushSprite(0, 0);

  if (millis() - lastMotorTime >= motorInterval) {
    lastMotorTime = millis();
    wave_lower_arms();
    setArmsPos(&positions);
  }
  
  delay(16); 
}
