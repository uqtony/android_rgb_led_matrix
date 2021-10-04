#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "gpio.h"

/*
 * nanosleep() takes longer than requested because of OS jitter.
 * In about 99.9% of the cases, this is <= 25 microcseconds on
 * the Raspberry Pi (empirically determined with a Raspbian kernel), so
 * we substract this value whenever we do nanosleep(); the remaining time
 * we then busy wait to get a good accurate result.
 *
 * You can measure the overhead using DEBUG_SLEEP_JITTER below.
 *
 * Note: A higher value here will result in more CPU use because of more busy
 * waiting inching towards the real value (for all the cases that nanosleep()
 * actually was better than this overhead).
 *
 * This might be interesting to tweak in particular if you have a realtime
 * kernel with different characteristics.
 */
#define EMPIRICAL_NANOSLEEP_OVERHEAD_US 12

/*
 * In case of non-hardware pulse generation, use nanosleep if we want to wait
 * longer than these given microseconds beyond the general overhead.
 * Below that, just use busy wait.
 */
#define MINIMUM_NANOSLEEP_TIME_US 5

/* In order to determine useful values for above, set this to 1 and use the
 * hardware pin-pulser.
 * It will output a histogram atexit() of how much how often we were over
 * the requested time.
 * (The full histogram will be shifted by the EMPIRICAL_NANOSLEEP_OVERHEAD_US
 *  value above. To get a full histogram of OS overhead, set it to 0 first).
 */
#define DEBUG_SLEEP_JITTER 0

// Raspberry 1 and 2 have different base addresses for the periphery
#define BCM2708_PERI_BASE        0x20000000
#define BCM2709_PERI_BASE        0x3F000000
#define BCM2711_PERI_BASE        0xFE000000

#define GPIO_REGISTER_OFFSET         0x200000
#define COUNTER_1Mhz_REGISTER_OFFSET   0x3000

#define GPIO_PWM_BASE_OFFSET    (GPIO_REGISTER_OFFSET + 0xC000)
#define GPIO_CLK_BASE_OFFSET    0x101000

#define REGISTER_BLOCK_SIZE (4*1024)

#define PWM_CTL      (0x00 / 4)
#define PWM_STA      (0x04 / 4)
#define PWM_RNG1     (0x10 / 4)
#define PWM_FIFO     (0x18 / 4)

#define PWM_CTL_CLRF1 (1<<6)    // CH1 Clear Fifo (1 Clears FIFO 0 has no effect)
#define PWM_CTL_USEF1 (1<<5)    // CH1 Use Fifo (0=data reg transmit 1=Fifo used for transmission)
#define PWM_CTL_POLA1 (1<<4)    // CH1 Polarity (0=(0=low 1=high) 1=(1=low 0=high)
#define PWM_CTL_SBIT1 (1<<3)    // CH1 Silence Bit (state of output when 0 transmission takes place)
#define PWM_CTL_MODE1 (1<<1)    // CH1 Mode (0=pwm 1=serialiser mode)
#define PWM_CTL_PWEN1 (1<<0)    // CH1 Enable (0=disable 1=enable)

#define PWM_STA_EMPT1 (1<<1)
#define PWM_STA_FULL1 (1<<0)

#define CLK_PASSWD  (0x5A<<24)

#define CLK_CTL_MASH(x)((x)<<9)
#define CLK_CTL_BUSY    (1 <<7)
#define CLK_CTL_KILL    (1 <<5)
#define CLK_CTL_ENAB    (1 <<4)
#define CLK_CTL_SRC(x) ((x)<<0)

#define CLK_CTL_SRC_PLLD 6  /* 500.0 MHz */

#define CLK_DIV_DIVI(x) ((x)<<12)
#define CLK_DIV_DIVF(x) ((x)<< 0)

#define CLK_PWMCTL 40
#define CLK_PWMDIV 41

// We want to have the last word in the fifo free
#define MAX_PWM_BIT_USE 224
#define PWM_BASE_TIME_NS 2

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x).
#define INP_GPIO(g) *(s_GPIO_registers+((g)/10)) &= ~(7ull<<(((g)%10)*3))
#define OUT_GPIO(g) *(s_GPIO_registers+((g)/10)) |=  (1ull<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

static volatile uint32_t *s_Timer1Mhz = NULL;

#ifndef GPIO_BIT
#define GPIO_BIT(b) (1ull<<(b))
#endif


#define CLOCK_CON_BASE_ADDRESS 0xFF760000
#define GPIO5_CLOCK_CON_OFFSET 0x0198
#define REGISTER_BLOCK_SIZE (4*1024)

#ifdef RK3399
#define CLOCK_CON_BASE_ADDRESS 0xFF760000
#define GPIO5_CLOCK_CON_OFFSET 0x037C
#define REGISTER_BLOCK_SIZE (4*1024)
#endif

struct RockchipGPIO {
    char const *name;
    uint32_t base_address;
    volatile uint32_t* base_read_reg, *base_write_reg;
    volatile uint32_t* data_read_reg, *data_write_reg;
    volatile uint32_t* direction_read_reg, *direction_write_reg;
    volatile bool change;
    volatile bool clear;
    volatile uint32_t buffer;
};

static struct RockchipGPIO s_rk3288_gpios[] = {
    {
        .name = "gpio0",

        .base_address = 0xFF750000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,
   
        .change = false,
        .clear = false,
        .buffer = 0,
    },
    {
        .name = "gpio1",

        .base_address = 0xFF780000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,
    },
    {
        .name = "gpio2",

        .base_address = 0xFF790000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,
    },
    {
        .name = "gpio3",

        .base_address = 0xFF7A0000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,
    },
    {
        .name = "gpio4",

        .base_address = 0xFF7B0000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,

    },
    {
        .name = "gpio5",

        .base_address = 0xFF7C0000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,

    },
    {
        .name = "gpio6",

        .base_address = 0xFF7D0000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,

    },
    {
        .name = "gpio7",

        .base_address = 0xFF7E0000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,

    },
    {
        .name = "gpio8",

        .base_address = 0xFF7F0000,

        .base_read_reg = NULL,
        .base_write_reg = NULL,

        .data_read_reg = NULL,
        .data_write_reg = NULL,

        .direction_read_reg = NULL,
        .direction_write_reg = NULL,

        .change = false,
        .clear = false,
        .buffer = 0,

    }
};

struct RPIMappingRockchip_GPIO {
    uint32_t rpi_mask;
    uint32_t rockchip_mask;
    struct RockchipGPIO* rockchipGpio;
};

struct RPIMappingRockchip {
    const char *name;
    struct RPIMappingRockchip_GPIO output_enable;
    struct RPIMappingRockchip_GPIO clock;
    struct RPIMappingRockchip_GPIO strobe;

    struct RPIMappingRockchip_GPIO a, b, c, d, e;
    
    struct RPIMappingRockchip_GPIO p0_r1, p0_g1, p0_b1, p0_r2, p0_g2, p0_b2;
};

static struct RPIMappingRockchip s_rpiRegularMappingRK3288 = {
    .name = "regular",

    .output_enable = 
    {
        .rpi_mask = GPIO_BIT(18),
        .rockchip_mask = GPIO_BIT(4),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .clock = 
    {   
        .rpi_mask = GPIO_BIT(17),
        .rockchip_mask = GPIO_BIT(6),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .strobe =            
    {
        .rpi_mask = GPIO_BIT(4),
        .rockchip_mask = GPIO_BIT(5),
        .rockchipGpio = &s_rk3288_gpios[8],
    },

    .a =            
    {
        .rpi_mask = GPIO_BIT(22),
        .rockchip_mask = GPIO_BIT(8),
        .rockchipGpio = &s_rk3288_gpios[5],
    },
    .b =            
    {
        .rpi_mask = GPIO_BIT(23),
        .rockchip_mask = GPIO_BIT(9),
        .rockchipGpio = &s_rk3288_gpios[5],
    },
    .c =            // TBD
    {
        .rpi_mask = GPIO_BIT(24),
        .rockchip_mask = GPIO_BIT(7),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .d =            // Not determine
    {
        .rpi_mask = GPIO_BIT(25),
        .rockchip_mask = GPIO_BIT(8),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .e =            // Not determine
    {
        .rpi_mask = GPIO_BIT(15),
        .rockchip_mask = GPIO_BIT(9),
        .rockchipGpio = &s_rk3288_gpios[8],
    },

    .p0_r1 =           
    {
        .rpi_mask = GPIO_BIT(11),
        .rockchip_mask = GPIO_BIT(6),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_g1 =           
    {
        .rpi_mask = GPIO_BIT(27),
        .rockchip_mask = GPIO_BIT(5),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_b1 =            
    {
        .rpi_mask = GPIO_BIT(7),
        .rockchip_mask = GPIO_BIT(18),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_r2 =           
    {
        .rpi_mask = GPIO_BIT(8),
        .rockchip_mask = GPIO_BIT(17),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_g2 =            
    {
        .rpi_mask = GPIO_BIT(9),
        .rockchip_mask = GPIO_BIT(2),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_b2 =            
    {
        .rpi_mask = GPIO_BIT(10),
        .rockchip_mask = GPIO_BIT(0),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
};

static struct RPIMappingRockchip* s_rpiMappingRockchip = NULL;

static volatile uint32_t* s_clock_base_read_reg = NULL;
static volatile uint32_t* s_clock_base_write_reg = NULL;

static volatile uint32_t* s_gpio_clock_control_read_reg = NULL; 
static volatile uint32_t* s_gpio_clock_control_write_reg = NULL; 

static uint32_t *mmap_register(off_t base, bool memread = true) {

  int mem_fd;
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    perror("open /dev/mem");
    return NULL;
  }
  uint32_t real_addr = base &~4095;
  uint32_t offset = base - real_addr;
  uint32_t length = 4 + offset;
  length = (length+4095) & ~4095;
  

  uint32_t *result =
    (uint32_t*) mmap64(NULL,                  // Any adddress in our space will do
                     //REGISTER_BLOCK_SIZE,   // Map length
                     length,
                     memread?PROT_READ:PROT_WRITE,  // Enable r/w on GPIO registers.
                     MAP_SHARED,
                     mem_fd,                // File to map
                     //base + register_offset // Offset to bcm register
                     real_addr // Offset to bcm register
                     );
  close(mem_fd);

  //if (result == MAP_FAILED) {
  if (result == (void *)(-1)) {
    fprintf(stderr, "MMapping failed\n");
    perror("mmap error: ");
    fprintf(stderr, "MMapping from base 0x%lx, real_addr 0x%lx, length 0x%lx\n",
            base, (unsigned long)real_addr, (unsigned long)length);
    return NULL;
  }
  return result;
}

static bool init_rockchip_gpio(struct RockchipGPIO *rockchipGpio)
{
    if (rockchipGpio == NULL)
        return false;

    rockchipGpio->base_read_reg = mmap_register(rockchipGpio->base_address);
    rockchipGpio->base_write_reg = mmap_register(rockchipGpio->base_address, false);
    if (rockchipGpio->base_read_reg == NULL || rockchipGpio->base_write_reg == NULL)
        return false;
    
    rockchipGpio->data_read_reg = rockchipGpio->base_read_reg;
    rockchipGpio->data_write_reg = rockchipGpio->base_write_reg;
     
    rockchipGpio->direction_read_reg = rockchipGpio->base_read_reg + (4 / sizeof(uint32_t));        
    rockchipGpio->direction_write_reg = rockchipGpio->base_write_reg + (4 / sizeof(uint32_t));
    return true;
}

static bool mmap_all_register_once() {
    if (s_clock_base_read_reg != NULL) 
	return true;
    
    s_clock_base_read_reg = mmap_register(CLOCK_CON_BASE_ADDRESS); 
    if (s_clock_base_read_reg == NULL)
	return false;
    s_clock_base_write_reg = mmap_register(CLOCK_CON_BASE_ADDRESS, false); 
    if (s_clock_base_write_reg == NULL)
	return false;

    s_gpio_clock_control_read_reg = s_clock_base_read_reg + (GPIO5_CLOCK_CON_OFFSET/sizeof(uint32_t));
    s_gpio_clock_control_write_reg = s_clock_base_write_reg + (GPIO5_CLOCK_CON_OFFSET/sizeof(uint32_t));

    for (int idx = 0; idx <= 8; idx++) {
        if(init_rockchip_gpio(&s_rk3288_gpios[idx]) == false)
            return false; 
    }
    
    return true;
}

static bool init_rpi_mapping_rk3288_once()
{
    if (s_rpiMappingRockchip != NULL)
        return true;

    s_rpiMappingRockchip = &s_rpiRegularMappingRK3288;

    if (s_rpiMappingRockchip == NULL)
        return false;
    return mmap_all_register_once();
}

static bool enableGPIOClock() 
{
    if (s_clock_base_write_reg == NULL)
        return false;
    *(s_gpio_clock_control_write_reg) = 0xffff0000;
    return true;
}

static bool setGPIOMode(struct RPIMappingRockchip_GPIO* gpio, bool outputMode = true)
{
    if (gpio == NULL || gpio-> rockchipGpio == NULL)
        return false;
    uint32_t direct = gpio->rockchip_mask;
    outputMode?
    *(gpio-> rockchipGpio->direction_write_reg) |= direct:
    *(gpio-> rockchipGpio->direction_write_reg) &= ~direct;
    return true;
}

static bool setGPIOsMode(uint32_t inputs, bool outputMode = true)
{
    if (s_rpiMappingRockchip == NULL)
        return false;

    if(!enableGPIOClock())
        return false;
    //fprintf(stdout, " setGPIOsMode, inputs= 0x%lx\n", inputs);

    // Only care about GPIO mapping to rockchip
    if ( (s_rpiMappingRockchip->output_enable.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->output_enable), outputMode);

    if ( (s_rpiMappingRockchip->clock.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->clock), outputMode);   

    if ( (s_rpiMappingRockchip->strobe.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->strobe), outputMode); 


    if ( (s_rpiMappingRockchip->a.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->a), outputMode);

    if ( (s_rpiMappingRockchip->b.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->b), outputMode);

    if ( (s_rpiMappingRockchip->c.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->c), outputMode);

    if ( (s_rpiMappingRockchip->d.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->d), outputMode);

    if ( (s_rpiMappingRockchip->e.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->e), outputMode);

    if ( (s_rpiMappingRockchip->p0_r1.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->p0_r1), outputMode);

    if ( (s_rpiMappingRockchip->p0_g1.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->p0_g1), outputMode);

    if ( (s_rpiMappingRockchip->p0_b1.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->p0_b1), outputMode);

    if ( (s_rpiMappingRockchip->p0_r2.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->p0_r2), outputMode);

    if ( (s_rpiMappingRockchip->p0_g2.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->p0_g2), outputMode);

    if ( (s_rpiMappingRockchip->p0_b2.rpi_mask & inputs) > 0)
        setGPIOMode(&(s_rpiMappingRockchip->p0_b2), outputMode);
    return true;
}

static inline void flashGPIOs()
{
    for(int i = 5; i <=8; i++) {
        if (s_rk3288_gpios[i].change){
            !s_rk3288_gpios[i].clear?
             *(s_rk3288_gpios[i].data_write_reg) |= s_rk3288_gpios[i].buffer:
             *(s_rk3288_gpios[i].data_write_reg) &= (s_rk3288_gpios[i].buffer);
             s_rk3288_gpios[i].change = false;
             s_rk3288_gpios[i].clear = false;
           
        }
    }
}


static bool setGPIODirect(struct RPIMappingRockchip_GPIO* gpio, bool clear_bit = false)
{
    if (gpio == NULL || gpio-> rockchipGpio == NULL)
        return false;
    uint32_t data = gpio->rockchip_mask;

    !clear_bit?
    *(gpio-> rockchipGpio->data_write_reg) |= data:
    *(gpio-> rockchipGpio->data_write_reg) &= ~data;
    return true;
}

static inline bool setGPIO(struct RPIMappingRockchip_GPIO* gpio, bool clear_bit = false)
{
    if (gpio == NULL || gpio-> rockchipGpio == NULL)
        return false;
    uint32_t data = gpio->rockchip_mask;
/*    if (!gpio->rockchipGpio->change) 
        gpio->rockchipGpio->buffer = *(gpio->rockchipGpio->data_read_reg);
  */ 
    !clear_bit?
    gpio-> rockchipGpio->buffer |= data:
    gpio-> rockchipGpio->buffer &= ~data;
    
    gpio->rockchipGpio->change = true;
    gpio->rockchipGpio->clear = clear_bit;

/*    !clear_bit?
    *(gpio-> rockchipGpio->data_write_reg) |= data:
    *(gpio-> rockchipGpio->data_write_reg) &= ~data;
*/

    /*uint32_t result = 0;
    result = *(gpio-> rockchipGpio->data_read_reg);
    bool cmp_result = !clear_bit? result & data: ~result & data;
    if (!cmp_result) {
        fprintf(stderr, "setGPIO failed, data=0x%lx, result=0x%lx, clear_bit=%s\n", 
                data, result,clear_bit?"true":"false");
    }*/

    return true;
}

static inline uint32_t readGPIO(struct RPIMappingRockchip_GPIO* gpio)
{
    if (gpio == NULL || gpio-> rockchipGpio == NULL)
        return false;
    uint32_t data = *(gpio->rockchipGpio->data_read_reg);
    return data & gpio->rockchip_mask;
}

static uint32_t readGPIOs(uint32_t inputs);

static uint32_t last_gpios_status = 0xffffffff;
static bool last_clear_bit = false;

static bool setGPIOs(uint32_t inputs, bool clear_bit = false)
{
    if(last_gpios_status == inputs && last_clear_bit == clear_bit)
        return true;
    last_gpios_status = inputs;
    last_clear_bit = clear_bit;

    if (s_rpiMappingRockchip == NULL)
        return false;

    if(!enableGPIOClock())
        return false;

    //fprintf(stdout, " setGPIOs, inputs= 0x%lx, clear data=%d\n", inputs, clear_bit);
    /*switch(inputs){
        case GPIO_BIT(18):
            setGPIODirect(&(s_rpiMappingRockchip->output_enable), clear_bit);
            return true;
	    break;
        case GPIO_BIT(17):
            setGPIODirect(&(s_rpiMappingRockchip->clock), clear_bit);
            return true;
            break;
        case GPIO_BIT(4):
            setGPIODirect(&(s_rpiMappingRockchip->strobe), clear_bit);
            return true;
            break;
        case GPIO_BIT(22):
            setGPIODirect(&(s_rpiMappingRockchip->a), clear_bit);
            return true;
            break;
        case GPIO_BIT(23):
            setGPIODirect(&(s_rpiMappingRockchip->b), clear_bit);
            return true;
            break;
        case GPIO_BIT(24):
            setGPIODirect(&(s_rpiMappingRockchip->c), clear_bit);
            return true;
            break;
        case GPIO_BIT(25):
            setGPIODirect(&(s_rpiMappingRockchip->d), clear_bit);
            return true;
            break;
        case GPIO_BIT(15):
            setGPIODirect(&(s_rpiMappingRockchip->e), clear_bit);
            return true;
            break;
        case GPIO_BIT(11):
            setGPIODirect(&(s_rpiMappingRockchip->p0_r1), clear_bit);
            return true;
            break;
        case GPIO_BIT(27):
            setGPIODirect(&(s_rpiMappingRockchip->p0_g1), clear_bit);
            return true;
            break;
        case GPIO_BIT(7):
            setGPIODirect(&(s_rpiMappingRockchip->p0_b1), clear_bit);
            return true;
            break;
        case GPIO_BIT(8):
            setGPIODirect(&(s_rpiMappingRockchip->p0_r2), clear_bit);
            return true;
            break;
        case GPIO_BIT(9):
            setGPIODirect(&(s_rpiMappingRockchip->p0_g2), clear_bit);
            return true;
            break;
        case GPIO_BIT(10):
            setGPIODirect(&(s_rpiMappingRockchip->p0_b2), clear_bit);
            return true;
            break;
       case 0x8020f80:
            setGPIO(&(s_rpiMappingRockchip->clock), clear_bit);
            setGPIO(&(s_rpiMappingRockchip->p0_r1), clear_bit);
            setGPIO(&(s_rpiMappingRockchip->p0_g1), clear_bit);
            setGPIO(&(s_rpiMappingRockchip->p0_b1), clear_bit);
            setGPIO(&(s_rpiMappingRockchip->p0_r2), clear_bit);
            setGPIO(&(s_rpiMappingRockchip->p0_g2), clear_bit);
            setGPIO(&(s_rpiMappingRockchip->p0_b2), clear_bit);
            flashGPIOs();
	    return true;
	    break;

    }//end switch
*/
    //fprintf(stdout, " setGPIOs, inputs= 0x%lx, clear data=%d\n", inputs, clear_bit);
    // Only care about GPIO mapping to rockchip
    if ( (s_rpiMappingRockchip->output_enable.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->output_enable), clear_bit);

    if ( (s_rpiMappingRockchip->clock.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->clock), clear_bit);

    if ( (s_rpiMappingRockchip->strobe.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->strobe), clear_bit);

    
    if ( (s_rpiMappingRockchip->a.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->a), clear_bit);

    if ( (s_rpiMappingRockchip->b.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->b), clear_bit);

    if ( (s_rpiMappingRockchip->c.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->c), clear_bit);

    if ( (s_rpiMappingRockchip->d.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->d), clear_bit);

    if ( (s_rpiMappingRockchip->e.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->e), clear_bit);

    if ( (s_rpiMappingRockchip->p0_r1.rpi_mask & inputs) > 0){
        setGPIO(&(s_rpiMappingRockchip->p0_r1), clear_bit);
    }

    if ( (s_rpiMappingRockchip->p0_g1.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->p0_g1), clear_bit);

    if ( (s_rpiMappingRockchip->p0_b1.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->p0_b1), clear_bit);

    if ( (s_rpiMappingRockchip->p0_r2.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->p0_r2), clear_bit);

    if ( (s_rpiMappingRockchip->p0_g2.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->p0_g2), clear_bit);

    if ( (s_rpiMappingRockchip->p0_b2.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->p0_b2), clear_bit);
 
    flashGPIOs();   
    //fprintf(stdout, " data  after set= 0x%lx\n", readGPIOs(inputs));

    return true;
}

static uint32_t readGPIOs(uint32_t inputs)
{
    uint32_t result = 0;
    if (s_rpiMappingRockchip == NULL)
        return 0;

    if (!enableGPIOClock())
        return 0;

    //fprintf(stdout, " readGPIOs, inputs= 0x%lx\n", inputs);
    // Only care about GPIO mapping to rockchip
    if ( (s_rpiMappingRockchip->output_enable.rpi_mask & inputs) > 0)
        if (readGPIO(&(s_rpiMappingRockchip->output_enable)) > 0)
            result |= s_rpiMappingRockchip->output_enable.rpi_mask;

    if ( (s_rpiMappingRockchip->clock.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->clock)) > 0)
            result |= s_rpiMappingRockchip->clock.rpi_mask;

    if ( (s_rpiMappingRockchip->strobe.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->strobe)) > 0)
            result |= s_rpiMappingRockchip->strobe.rpi_mask;


    if ( (s_rpiMappingRockchip->a.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->a)) > 0)
            result |= s_rpiMappingRockchip->a.rpi_mask;

    if ( (s_rpiMappingRockchip->b.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->b)) >0)
            result |= s_rpiMappingRockchip->b.rpi_mask;

    if ( (s_rpiMappingRockchip->c.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->c)) > 0)
            result |= s_rpiMappingRockchip->c.rpi_mask;

    if ( (s_rpiMappingRockchip->d.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->d)) > 0)
            result |= s_rpiMappingRockchip->d.rpi_mask;


    if ( (s_rpiMappingRockchip->e.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->e)) > 0)
            result |= s_rpiMappingRockchip->e.rpi_mask;

    if ( (s_rpiMappingRockchip->p0_r1.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->p0_r1)) > 0)
            result |= s_rpiMappingRockchip->p0_r1.rpi_mask;

    if ( (s_rpiMappingRockchip->p0_g1.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->p0_g1)) > 0)
            result |= s_rpiMappingRockchip->p0_g1.rpi_mask;

    if ( (s_rpiMappingRockchip->p0_b1.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->p0_b1)) > 0)
            result |= s_rpiMappingRockchip->p0_b1.rpi_mask;

    if ( (s_rpiMappingRockchip->p0_r2.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->p0_r2)) > 0)
            result |= s_rpiMappingRockchip->p0_r2.rpi_mask;

    if ( (s_rpiMappingRockchip->p0_g2.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->p0_g2)) > 0)
            result |= s_rpiMappingRockchip->p0_g2.rpi_mask;

    if ( (s_rpiMappingRockchip->p0_b2.rpi_mask & inputs) > 0)
        if(readGPIO(&(s_rpiMappingRockchip->p0_b2)) > 0)
            result |= s_rpiMappingRockchip->p0_b2.rpi_mask;
    return result;;
}

namespace rgb_matrix {

GPIO::GPIO() : output_bits_(0), input_bits_(0), reserved_bits_(0),
               slowdown_(1)
#ifdef ENABLE_WIDE_GPIO_COMPUTE_MODULE
             , uses_64_bit_(false)
#endif
{
}


gpio_bits_t GPIO::InitOutputs(gpio_bits_t outputs,
                              bool adafruit_pwm_transition_hack_needed) {
  if (s_rpiMappingRockchip == NULL) {
    fprintf(stderr, "Attempt to init outputs but not yet Init()-ialized.\n");
    return 0;
  }

  outputs &= ~(output_bits_ | input_bits_ | reserved_bits_);
#ifdef ENABLE_WIDE_GPIO_COMPUTE_MODULE
  const int kMaxAvailableBit = 45;
  uses_64_bit_ |= (outputs >> 32) != 0;
#else
  const int kMaxAvailableBit = 31;
#endif
  setGPIOsMode(outputs); // set outputs Mode: output
  output_bits_ |= outputs;
  return outputs;
}// end GPIO::InitOutputs

gpio_bits_t GPIO::RequestInputs(gpio_bits_t inputs) {
  if (s_rpiMappingRockchip == NULL) {
    fprintf(stderr, "Attempt to init inputs but not yet Init()-ialized.\n");
    return 0;
  }

  inputs &= ~(output_bits_ | input_bits_ | reserved_bits_);
#ifdef ENABLE_WIDE_GPIO_COMPUTE_MODULE
  const int kMaxAvailableBit = 45;
  uses_64_bit_ |= (inputs >> 32) != 0;
#else
  const int kMaxAvailableBit = 31;
#endif
  setGPIOsMode(inputs, false); // set inputs Mode: input;
  input_bits_ |= inputs;
  return inputs;
}// end GPIO::RequestInputs

// We are not interested in the _exact_ model, just good enough to determine
// What to do.
enum RaspberryPiModel {
  PI_MODEL_1,
  PI_MODEL_2,
  PI_MODEL_3,
  PI_MODEL_4
};

static int ReadFileToBuffer(char *buffer, size_t size, const char *filename) {
  const int fd = open(filename, O_RDONLY);
  if (fd < 0) return -1;
  ssize_t r = read(fd, buffer, size - 1); // assume one read enough
  buffer[r >= 0 ? r : 0] = '\0';
  close(fd);
  return r;
}

static RaspberryPiModel DetermineRaspberryModel() {
  char buffer[4096];
  if (ReadFileToBuffer(buffer, sizeof(buffer), "/proc/cpuinfo") < 0) {
    fprintf(stderr, "Reading cpuinfo: Could not determine Pi model, Return PI_MODEL_3\n");
    return PI_MODEL_3;  // safe guess fallback.
  }
  static const char RevisionTag[] = "Revision";
  const char *revision_key;
  if ((revision_key = strstr(buffer, RevisionTag)) == NULL) {
    fprintf(stderr, "non-existent Revision: Could not determine Pi model, Return PI_MODEL_3\n");
    return PI_MODEL_3;
  }
  unsigned int pi_revision = 0;
  /*if (sscanf(index(revision_key, ':') + 1, "%x", &pi_revision) != 1) {
    fprintf(stderr, "Unknown Revision: Could not determine Pi model\n");
    return PI_MODEL_3;
  }*/

  // https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/README.md
  const unsigned pi_type = (pi_revision >> 4) & 0xff;
  switch (pi_type) {
  case 0x00: /* A */
  case 0x01: /* B, Compute Module 1 */
  case 0x02: /* A+ */
  case 0x03: /* B+ */
  case 0x05: /* Alpha ?*/
  case 0x06: /* Compute Module1 */
  case 0x09: /* Zero */
  case 0x0c: /* Zero W */
    return PI_MODEL_1;

  case 0x04:  /* Pi 2 */
    return PI_MODEL_2;

  case 0x11: /* Pi 4 */
  case 0x14: /* CM4 */
    return PI_MODEL_4;

  default:  /* a bunch of versions representing Pi 3 */
    return PI_MODEL_3;
  }
}

static RaspberryPiModel GetPiModel() {
  static RaspberryPiModel pi_model = DetermineRaspberryModel();
  pi_model = PI_MODEL_1;
  fprintf(stdout, "GetPiModel= %d\n", pi_model);
  return pi_model;
}

static int GetNumCores() {
  return GetPiModel() == PI_MODEL_1 ? 1 : 4;
}

bool GPIO::Init(int slowdown) {
  slowdown_ = slowdown;

  // Pre-mmap all bcm registers we need now and possibly in the future, as to
  // allow  dropping privileges after GPIO::Init() even as some of these
  // registers might be needed later.
  if (!init_rpi_mapping_rk3288_once())
    return false;

  return true;
}//end GPIO::Init

#define CLEAR_DATA true

gpio_bits_t GPIO::ReadRegisters() const {
    uint32_t data = readGPIOs(0xffffffff);
    return (static_cast<gpio_bits_t>(data)
#ifdef ENABLE_WIDE_GPIO_COMPUTE_MODULE
            | (static_cast<gpio_bits_t>(data) << 32)
#endif
            );
  }// end GPIO::ReadRegisters

void GPIO::WriteSetBits(gpio_bits_t value) {
    setGPIOs(static_cast<uint32_t>(value & 0xFFFFFFFF), !CLEAR_DATA);
#ifdef ENABLE_WIDE_GPIO_COMPUTE_MODULE
    if (uses_64_bit_)
      setGPIOS(static_cast<uint32_t>(value >> 32), !CLEAR_DATA);
#endif
  }// end GPIO::WriteSetBits

void GPIO::WriteClrBits(gpio_bits_t value) {
    setGPIOs(static_cast<uint32_t>(value & 0xFFFFFFFF), CLEAR_DATA);
#ifdef ENABLE_WIDE_GPIO_COMPUTE_MODULE
    if (uses_64_bit_)
      setGPIOs(static_cast<uint32_t>(value >> 32), CLEAR_DATA);
#endif
  }// end GPIO::WriteClrBits

/*
 * We support also other pinouts that don't have the OE- on the hardware
 * PWM output pin, so we need to provide (impefect) 'manual' timing as well.
 * Hence all various busy_wait_nano() implementations depending on the hardware.
 */

// --- PinPulser. Private implementation parts.
namespace {
// Manual timers.
class Timers {
public:
  static bool Init();
  static void sleep_nanos(long t);
};

// Simplest of PinPulsers. Uses somewhat jittery and manual timers
// to get the timing, but not optimal.
class TimerBasedPinPulser : public PinPulser {
public:
  TimerBasedPinPulser(GPIO *io, gpio_bits_t bits,
                      const std::vector<int> &nano_specs)
    : io_(io), bits_(bits), nano_specs_(nano_specs) {
    if (!s_Timer1Mhz) {
      fprintf(stderr, "FYI: not running as root which means we can't properly "
              "control timing unless this is a real-time kernel. Expect color "
              "degradation. Consider running as root with sudo.\n");
    }
  }

  virtual void SendPulse(int time_spec_number) {
    io_->ClearBits(bits_);
    Timers::sleep_nanos(nano_specs_[time_spec_number]);
    io_->SetBits(bits_);
  }

private:
  GPIO *const io_;
  const gpio_bits_t bits_;
  const std::vector<int> nano_specs_;
};

static bool LinuxHasModuleLoaded(const char *name) {
  FILE *f = fopen("/proc/modules", "r");
  if (f == NULL) return false; // don't care.
  char buf[256];
  const size_t namelen = strlen(name);
  bool found = false;
  while (fgets(buf, sizeof(buf), f) != NULL) {
    if (strncmp(buf, name, namelen) == 0) {
      found = true;
      break;
    }
  }
  fclose(f);
  return found;
}

static void busy_wait_nanos_rpi_1(long nanos);
static void busy_wait_nanos_rpi_2(long nanos);
static void busy_wait_nanos_rpi_3(long nanos);
static void busy_wait_nanos_rpi_4(long nanos);
static void (*busy_wait_impl)(long) = busy_wait_nanos_rpi_3;

// Best effort write to file. Used to set kernel parameters.
static void WriteTo(const char *filename, const char *str) {
  const int fd = open(filename, O_WRONLY);
  if (fd < 0) return;
  (void) write(fd, str, strlen(str));  // Best effort. Ignore return value.
  close(fd);
}

// By default, the kernel applies some throtteling for realtime
// threads to prevent starvation of non-RT threads. But we
// really want all we can get iff the machine has more cores and
// our RT-thread is locked onto one of these.
// So let's tell it not to do that.
static void DisableRealtimeThrottling() {
  if (GetNumCores() == 1) return;   // Not safe if we don't have > 1 core.
  // We need to leave the kernel a little bit of time, as it does not like
  // us to hog the kernel solidly. The default of 950000 leaves 50ms that
  // can generate visible flicker, so we reduce that to 1ms.
  WriteTo("/proc/sys/kernel/sched_rt_runtime_us", "999000");
}

bool Timers::Init() {
  if (!init_rpi_mapping_rk3288_once())
    return false;

  fprintf(stdout, "Timers::Init");
  // Choose the busy-wait loop that fits our Pi.
  switch (GetPiModel()) {
  case PI_MODEL_1: busy_wait_impl = busy_wait_nanos_rpi_1; break;
  case PI_MODEL_2: busy_wait_impl = busy_wait_nanos_rpi_2; break;
  case PI_MODEL_3: busy_wait_impl = busy_wait_nanos_rpi_3; break;
  case PI_MODEL_4: busy_wait_impl = busy_wait_nanos_rpi_4; break;
  }

  DisableRealtimeThrottling();
  // If we have it, we run the update thread on core3. No perf-compromises:
  WriteTo("/sys/devices/system/cpu/cpu3/cpufreq/scaling_governor",
          "performance");
  return true;
}

static uint32_t JitterAllowanceMicroseconds() {
  // If this is a Raspberry Pi with more than one core, we add a bit of
  // additional overhead measured up to the 99.999%-ile: we can allow to burn
  // a bit more busy-wait CPU cycles to get the timing accurate as we have
  // more CPU to spare.
  switch (GetPiModel()) {
  case PI_MODEL_1:
    return EMPIRICAL_NANOSLEEP_OVERHEAD_US;  // 99.9%-ile
  case PI_MODEL_2: case PI_MODEL_3:
    return EMPIRICAL_NANOSLEEP_OVERHEAD_US + 35;  // 99.999%-ile
  case PI_MODEL_4:
    return EMPIRICAL_NANOSLEEP_OVERHEAD_US + 10;  // this one is fast.
  }
  return EMPIRICAL_NANOSLEEP_OVERHEAD_US;
}

void Timers::sleep_nanos(long nanos) {
  // For smaller durations, we go straight to busy wait.

  // For larger duration, we use nanosleep() to give the operating system
  // a chance to do something else.

  // However, these timings have a lot of jitter, so if we have the 1Mhz timer
  // available, we use that to accurately mesure time spent and do the
  // remaining time with busy wait. If we don't have the timer available
  // (not running as root), we just use nanosleep() for larger values.

  if (s_Timer1Mhz) {
    static long kJitterAllowanceNanos = JitterAllowanceMicroseconds() * 1000;
    if (nanos > kJitterAllowanceNanos + MINIMUM_NANOSLEEP_TIME_US*1000) {
      const uint32_t before = *s_Timer1Mhz;
      struct timespec sleep_time = { 0, nanos - kJitterAllowanceNanos };
      nanosleep(&sleep_time, NULL);
      const uint32_t after = *s_Timer1Mhz;
      const long nanoseconds_passed = 1000 * (uint32_t)(after - before);
      if (nanoseconds_passed > nanos) {
        return;  // darn, missed it.
      } else {
        nanos -= nanoseconds_passed; // remaining time with busy-loop
      }
    }
  } else {
    // Not running as root, not having access to 1Mhz timer. Approximate large
    // durations with nanosleep(); small durations are done with busy wait.
    if (nanos > (EMPIRICAL_NANOSLEEP_OVERHEAD_US + MINIMUM_NANOSLEEP_TIME_US)*1000) {
      struct timespec sleep_time
        = { 0, nanos - EMPIRICAL_NANOSLEEP_OVERHEAD_US*1000 };
      nanosleep(&sleep_time, NULL);
      return;
    }
  }

  busy_wait_impl(nanos);  // Use model-specific busy-loop for remaining time.
}

static void busy_wait_nanos_rpi_1(long nanos) {
  if (nanos < 70) return;
  // The following loop is determined empirically on a 700Mhz RPi
  for (uint32_t i = (nanos - 70) >> 2; i != 0; --i) {
    asm("nop");
  }
}

static void busy_wait_nanos_rpi_2(long nanos) {
  if (nanos < 20) return;
  // The following loop is determined empirically on a 900Mhz RPi 2
  for (uint32_t i = (nanos - 20) * 100 / 110; i != 0; --i) {
    asm("");
  }
}

static void busy_wait_nanos_rpi_3(long nanos) {
  if (nanos < 20) return;
  for (uint32_t i = (nanos - 15) * 100 / 73; i != 0; --i) {
    asm("");
  }
}

static void busy_wait_nanos_rpi_4(long nanos) {
  if (nanos < 20) return;
  // Interesting, the Pi4 is _slower_ than the Pi3 ? At least for this busy loop
  for (uint32_t i = (nanos - 5) * 100 / 132; i != 0; --i) {
    asm("");
  }

}

#if DEBUG_SLEEP_JITTER
static int overshoot_histogram_us[256] = {0};
static void print_overshoot_histogram() {
  fprintf(stderr, "Overshoot histogram >= empirical overhead of %dus\n"
          "%6s | %7s | %7s\n",
          JitterAllowanceMicroseconds(), "usec", "count", "accum");
  int total_count = 0;
  for (int i = 0; i < 256; ++i) total_count += overshoot_histogram_us[i];
  int running_count = 0;
  for (int us = 0; us < 256; ++us) {
    const int count = overshoot_histogram_us[us];
    if (count > 0) {
      running_count += count;
      fprintf(stderr, "%s%3dus: %8d %7.3f%%\n", (us == 0) ? "<=" : " +",
              us, count, 100.0 * running_count / total_count);
    }
  }
}
#endif
} // end anonymous namespace

// Public PinPulser factory
PinPulser *PinPulser::Create(GPIO *io, gpio_bits_t gpio_mask,
                             bool allow_hardware_pulsing,
                             const std::vector<int> &nano_wait_spec) {
  if (!Timers::Init()) return NULL;
  return new TimerBasedPinPulser(io, gpio_mask, nano_wait_spec);
  
}

// For external use, e.g. in the matrix for extra time.
uint32_t GetMicrosecondCounter() {
  if (s_Timer1Mhz) return *s_Timer1Mhz;

  // When run as non-root, we can't read the timer. Fall back to slow
  // operating-system ways.
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  const uint64_t micros = ts.tv_nsec / 1000;
  const uint64_t epoch_usec = (uint64_t)ts.tv_sec * 1000000 + micros;
  return epoch_usec & 0xFFFFFFFF;
}
}// end namespace rgb_matrix
