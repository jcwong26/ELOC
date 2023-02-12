#define V_INV_ENABLE_PIN 15
#define FAN_CTRL_PIN 8
#define BRIDGE_CTRL_PIN 10
#define NOT_SHUTDOWN_PIN 11
#define WPT_ACTIVE_LED_PIN 12

// Inverter Definitions
#define MAX_INV_CURRENT_AMPS 5.0        // TODO: Change this to something higher once it all works
#define DEFAULT_SW_FREQ 100E3       // Default 100 kHz Switching Freq
//      Inverter Timer
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_1_BIT // Set duty resolution to 8 bits
// #define LEDC_DUTY               (1) // Set duty to 100%
#define LEDC_FREQUENCY          DEFAULT_SW_FREQ // Frequency in Hertz. Set frequency at 100 kHz

void init_inverter(void);
void turn_on_inv_rail(void);
void turn_off_inv_rail(void);
void set_sw_freq(uint32_t f_sw);
void enable_bridge(void);
void disable_bridge(void);
void turn_fan_on(void);
void turn_fan_off(void);

void flash_wpt_led(void*);