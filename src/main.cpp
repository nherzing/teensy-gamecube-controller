#include <Arduino.h>

#define NOP1 "nop\n\t"

#define DATA_PIN 2 // D0

#define READ_DATA() ((GPIOD_PDIR) & 0x1)
#define SET_BIT() (GPIOD_PDOR = 0x1)
#define CLEAR_BIT() (GPIOD_PDOR = 0x0)

#define BOOL(x) ((x) ? "1" : "0")

typedef struct {
  bool a, b, x, y;
  bool d_up, d_right, d_down, d_left;
  bool l, r, z, start;
  uint8_t joy_vert, joy_horz;
  uint8_t c_vert, c_horz;
  uint8_t l_trigger, r_trigger;
} gamepad_state;

inline void send_bit(boolean bit) {
  if (bit) {
    CLEAR_BIT();
    delayMicroseconds(1);
    SET_BIT();
    delayMicroseconds(3);    
  } else {
    CLEAR_BIT();
    delayMicroseconds(3);
    SET_BIT();
    delayMicroseconds(1);
  }
}

inline void send_byte(uint8_t data) {
  for (int i = 7; i >= 0; i--) {
    send_bit((data >> i) & 0x1);
  }
}

void req_data() {
  GPIOD_PDDR = 0x1;
  send_byte(0b01000000);
  send_byte(0b00000011);
  send_byte(0b00000000);
  send_bit(1);
  GPIOD_PDDR = 0x0;
}

inline int recv_data(uint8_t data[]) {
  const uint32_t us_per_bit = 4;
  const uint8_t num_bits = 64;
  const uint8_t max_samples_per_bit = 32;
  const uint32_t max_sample_count = num_bits * max_samples_per_bit;
  const uint32_t total_us = us_per_bit * num_bits;
  const uint32_t max_us = total_us + total_us/8;

  uint8_t samples [max_sample_count];
  uint32_t sample_count = 0;
  elapsedMicros elapsed;
  while (elapsed < max_us) {
    samples[sample_count++] = READ_DATA();    
  }

  uint32_t sample_idx = 0;

  while (samples[sample_idx] == 1) {
    sample_idx++;
  }
  for (size_t data_idx = 0; data_idx < 64; data_idx++) {
    uint32_t lows = 0; 
    uint32_t highs = 0;
    while (samples[sample_idx++] == 0) {
      lows++;
    }
    while (samples[sample_idx++] == 1) {
      highs++;
    }
    if (lows + highs > 16) {
      return -1;
    }
    data[data_idx] = (lows > highs) ? 0 : 1;
  }

  return 0;
}

void print_raw_data(uint8_t data[]) {
  Serial.println("Received:");\
  for (size_t i = 0; i < 8; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(data[i*8]);
    Serial.print(data[i*8+1]);
    Serial.print(data[i*8+2]);
    Serial.print(data[i*8+3]);
    Serial.print(" ");
    Serial.print(data[i*8+4]);
    Serial.print(data[i*8+5]);
    Serial.print(data[i*8+6]);
    Serial.print(data[i*8+7]);    
    Serial.println("");
  }
  Serial.println("");  
}

uint8_t get_byte(uint8_t data[], size_t offset) {
  uint8_t b = data[offset] & 0x1;
  for (size_t i = 1; i < 8; i++) {
    b <<= 1;
    b |= data[offset+i] & 0x1;
  }
  return b;
}

gamepad_state build_gamepad_state(uint8_t data[]) {
  gamepad_state result;

  result.start = data[3];
  result.y = data[4];
  result.x = data[5];
  result.b = data[6];
  result.a = data[7];
  result.l = data[9];
  result.r = data[10];
  result.z = data[11];
  result.d_up = data[12];
  result.d_down = data[13];
  result.d_right = data[14];
  result.d_left = data[15];
  result.joy_horz = get_byte(data, 16);
  result.joy_vert = get_byte(data, 24);
  result.c_horz = get_byte(data, 32);
  result.c_vert = get_byte(data, 40);
  result.l_trigger = get_byte(data, 48);
  result.r_trigger = get_byte(data, 56);

  return result;
}

gamepad_state read_gamepad() {
    uint8_t raw_data [64];
    req_data();
    while (recv_data(raw_data) != 0) {
      delay(10);
      req_data();
    };  

  return build_gamepad_state(raw_data);
}

void print_gamepad_state(gamepad_state *state) {
  Serial.printf("Start: %s\n", BOOL(state->start));
  Serial.printf("A: %s B: %s\n", BOOL(state->a), BOOL(state->b));
  Serial.printf("X: %s Y: %s\n", BOOL(state->x), BOOL(state->y));
  Serial.printf("Z: %s L: %s R: %s\n", BOOL(state->z), BOOL(state->l), BOOL(state->r));
  Serial.printf("D-pad: up: %s right: %s down: %s left: %s\n", BOOL(state->d_up), BOOL(state->d_right), BOOL(state->d_down), BOOL(state->d_left));
  Serial.printf("Joy: vert: %3d horz: %3d\n", state->joy_vert, state->joy_horz);
  Serial.printf("C: vert: %3d horz: %3d\n", state->c_vert, state->c_horz);
  Serial.printf("L-trigger: %3d ", state->l_trigger);
  Serial.printf("R-trigger: %3d\n", state->r_trigger);
}

void set_gamepad_state(gamepad_state *state) {
  Joystick.button(1, state->a);
  Joystick.button(2, state->b);
  Joystick.button(3, state->x);
  Joystick.button(4, state->y);
  Joystick.button(5, state->z);
  Joystick.button(6, state->d_up);
  Joystick.button(7, state->d_right);
  Joystick.button(8, state->d_down);
  Joystick.button(9, state->d_left);
  Joystick.button(10, state->start);
  Joystick.button(11, state->l);
  Joystick.button(12, state->r);

  Joystick.X(state->joy_horz * 4);
  Joystick.Y(state->joy_vert * 4);
  Joystick.sliderLeft(state->l_trigger * 4);
  Joystick.sliderRight(state->r_trigger * 4);
  Joystick.Z(state->c_horz * 4);
  Joystick.Zrotate(state->c_vert * 4);
  Joystick.send_now();
}

extern "C" int main(void) {
  pinMode(DATA_PIN, OUTPUT);

  Joystick.useManualSend(true);

  while (true) {
    gamepad_state state = read_gamepad();
    set_gamepad_state(&state);
//    print_gamepad_state(&state);
  }
}

// extern "C" int main(void) {
//   Serial.begin(38400);
//   Serial.println("Booting!");
//   delay(1000);
//   pinMode(DATA_PIN, OUTPUT);

//   do {
//     delay(10);
//   } while (Serial.available() && Serial.read());  

//   Serial.println("Read BLANK");    
//   while(!Serial.available()) { }
//   Serial.read();

//   gamepad_state state = read_gamepad();
//   Serial.println("OHHAI");
//   print_gamepad_state(&state);

//   while (true) {
//     Serial.println("Read button");    
//     while(!Serial.available()) { }
//     Serial.read();    
//     gamepad_state state2 = read_gamepad();
//     print_gamepad_state(&state2);

//     delay(100);
//   }
// }