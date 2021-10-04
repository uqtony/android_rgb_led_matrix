#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "exec_program.h"

#ifndef GPIO_BIT(b)
#define GPIO_BIT(b) ((uint32_t)1<<(b))
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
    char *name;
    uint32_t base_address;
    uint32_t* base_read_reg, *base_write_reg;
    volatile uint32_t* data_read_reg, *data_write_reg;
    volatile uint32_t* direction_read_reg, *direction_write_reg;;
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
    }
};

struct RPIMappingRockchip_GPIO {
    const char *name;
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
        .name = "output_enable",
        .rpi_mask = GPIO_BIT(18),
        .rockchip_mask = GPIO_BIT(4),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .clock = 
    {   
        .name = "clock",
        .rpi_mask = GPIO_BIT(17),
        .rockchip_mask = GPIO_BIT(6),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .strobe =            
    {
        .name = "strobe",
        .rpi_mask = GPIO_BIT(4),
        .rockchip_mask = GPIO_BIT(5),
        .rockchipGpio = &s_rk3288_gpios[8],
    },

    .a =            
    {
        .name = "a",
        .rpi_mask = GPIO_BIT(22),
        .rockchip_mask = GPIO_BIT(8),
        .rockchipGpio = &s_rk3288_gpios[5],
    },
    .b =            
    {
        .name = "b",
        .rpi_mask = GPIO_BIT(23),
        .rockchip_mask = GPIO_BIT(9),
        .rockchipGpio = &s_rk3288_gpios[5],
    },
    .c =            // TBD
    {
        .name = "c",
        .rpi_mask = GPIO_BIT(24),
        .rockchip_mask = GPIO_BIT(7),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .d =            // Not determine
    {
        .name = "d",
        .rpi_mask = GPIO_BIT(25),
        .rockchip_mask = GPIO_BIT(8),
        .rockchipGpio = &s_rk3288_gpios[8],
    },
    .e =            // Not determine
    {
        .name = "e",
        .rpi_mask = GPIO_BIT(15),
        .rockchip_mask = GPIO_BIT(9),
        .rockchipGpio = &s_rk3288_gpios[8],
    },

    .p0_r1 =           
    {
        .name = "p0_r1",
        .rpi_mask = GPIO_BIT(11),
        .rockchip_mask = GPIO_BIT(6),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_g1 =           
    {
        .name = "p0_g1",
        .rpi_mask = GPIO_BIT(27),
        .rockchip_mask = GPIO_BIT(5),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_b1 =            
    {
        .name = "p0_b1",
        .rpi_mask = GPIO_BIT(7),
        .rockchip_mask = GPIO_BIT(18),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_r2 =           
    {
        .name = "p0_r2",
        .rpi_mask = GPIO_BIT(8),
        .rockchip_mask = GPIO_BIT(17),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_g2 =            
    {
        .name = "p0_g2",
        .rpi_mask = GPIO_BIT(9),
        .rockchip_mask = GPIO_BIT(2),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
    .p0_b2 =            
    {
        .name = "p0_b2",
        .rpi_mask = GPIO_BIT(10),
        .rockchip_mask = GPIO_BIT(0),
        .rockchipGpio = &s_rk3288_gpios[7],
    },
};

static struct RPIMappingRockchip* s_rpiMappingRockchip = NULL;

static uint32_t* s_clock_base_read_reg = NULL;
static uint32_t* s_clock_base_write_reg = NULL;

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
            base, real_addr, length);
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

static bool setGPIO(struct RPIMappingRockchip_GPIO* gpio, bool clear_bit = false)
{
    if (gpio == NULL || gpio-> rockchipGpio == NULL)
        return false;
    uint32_t data = gpio->rockchip_mask;
    !clear_bit?
    *(gpio-> rockchipGpio->data_write_reg) |= data:
    *(gpio-> rockchipGpio->data_write_reg) &= ~data;
    uint32_t result = 0;
    result = *(gpio-> rockchipGpio->data_read_reg);
    bool cmp_result = !clear_bit? result & data: ~result & data;
    if (!cmp_result) {
        fprintf(stderr, "setGPIO failed, data=0x%lx, result=0x%lx, clear_bit=%s\n", data, result,clear_bit?"true":"false");
    }
    return true;
}

static uint32_t readGPIO(struct RPIMappingRockchip_GPIO* gpio)
{
    if (gpio == NULL || gpio-> rockchipGpio == NULL)
        return false;
    uint32_t data = *(gpio->rockchipGpio->data_read_reg);
    return data & gpio->rockchip_mask;
}

static bool setGPIOs(uint32_t inputs, bool clear_bit = false)
{
    if (s_rpiMappingRockchip == NULL)
        return false;

    if(!enableGPIOClock())
        return false;

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

    if ( (s_rpiMappingRockchip->p0_r1.rpi_mask & inputs) > 0)
        setGPIO(&(s_rpiMappingRockchip->p0_r1), clear_bit);

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
    return true;
}

static uint32_t readGPIOs(uint32_t inputs)
{
    uint32_t result = 0;
    if (s_rpiMappingRockchip == NULL)
        return 0;

    if (!enableGPIOClock())
        return 0;

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

#define CLEAR_DATA true

void test() {
    enableGPIOClock();    
    uint32_t data = *(s_rk3288_gpios[7].data_read_reg);
    fprintf(stdout, " gpio[7] data= 0x%lx\n", data);
    //data = *(s_rpiMappingRockchip->output_enable.rockchipGpio->data_read_reg);
    //ifprintf(stdout, " output_enable data= 0x%lx\n", data);
    
    //*(s_rk3288_gpios[7].data_write_reg) |= GPIO_BIT(17);
    //data = *(s_rk3288_gpios[8].data_read_reg);
    //fprintf(stdout, " gpio[8] data= 0x%lx\n", data);
    /*if(!setGPIO(&(s_rpiMappingRockchip->p0_b2), false)) {
        fprintf(stderr, "set gpio failed!!\n");
    } */
}

void test2() {
    uint32_t clock_data = *(s_gpio_clock_control_read_reg);
    fprintf(stdout, "clock data= 0x%lx\n", clock_data);

    uint32_t inputs = 0;
    inputs |= s_rpiMappingRockchip->p0_r1.rpi_mask| // 222 
              s_rpiMappingRockchip->output_enable.rpi_mask|//252
              s_rpiMappingRockchip->clock.rpi_mask|//254
              s_rpiMappingRockchip->strobe.rpi_mask|//253
              s_rpiMappingRockchip->p0_g2.rpi_mask|//218  // direction not work
              s_rpiMappingRockchip->p0_b2.rpi_mask|//216   // not work, pull down by someone?
              s_rpiMappingRockchip->p0_r2.rpi_mask|//233 // sometimes not work!?   
              s_rpiMappingRockchip->p0_g1.rpi_mask|//221   
              s_rpiMappingRockchip->p0_b1.rpi_mask|//234   // sometimes not work!?
              s_rpiMappingRockchip->a.rpi_mask |//160  
              s_rpiMappingRockchip->b.rpi_mask//161   
              ;

    uint32_t data = readGPIOs(inputs);
    fprintf(stdout, " data  before set= 0x%lx\n", data);
    //fprintf(stdout, "clear data\n");
    //setGPIOs(0xffffffff, CLEAR_DATA);   

    setGPIOsMode(inputs);
    setGPIOs(inputs, false);
    //test();

    data = readGPIOs(inputs);
    fprintf(stdout, " data  after set= 0x%lx, input=0x%lx\n", data, inputs);
    //setGPIOs(inputs, true);   
    //data = readGPIOs(inputs); 
    //fprintf(stdout, " data  after clear= 0x%lx, input=0x%lx\n", data, inputs);
#ifdef RK3288_LED
    fprintf(stdout, "RK3288_LED is defined\n");
#endif

}

typedef int (*program_main)(int argc, char* argv[]);

struct ProgramMain {
    const char* name;
    program_main main_function;
};

struct ProgramMain s_programMain[] = {
    {
        .name = "c_example",
        .main_function = &c_example_main
    },
    {
        .name = "scrolling_text_example",
        .main_function = &scrolling_text_example_main
    },
    {
        .name = "input_example",
        .main_function = &input_example_main
    },
    {
        .name = "clock",
        .main_function = &clock_main
    },
    {
        .name = "pixel_mover",
        .main_function = &pixel_mover_main
    },
    {
        .name = "text_example",
        .main_function = &text_example_main
    },
    {
        .name = "minimal_example",
        .main_function = &minimal_example_main
    },

    {
        .name = "demo",
        .main_function = &demo_main
    },
    {
        .name = "image_viewer",
        .main_function = &led_image_viewer_main
    }


};

static struct RPIMappingRockchip_GPIO* select_gpio_by_name(char *arg)
{
    if (strcmp(arg, s_rpiMappingRockchip->output_enable.name) == 0){
        return &(s_rpiMappingRockchip->output_enable);
    }
    if (strcmp(arg, s_rpiMappingRockchip->clock.name) == 0){
        return &(s_rpiMappingRockchip->clock);
    }

    if (strcmp(arg, s_rpiMappingRockchip->strobe.name) == 0){
        return &(s_rpiMappingRockchip->strobe);
    }

    if (strcmp(arg, s_rpiMappingRockchip->a.name) == 0){
        return &(s_rpiMappingRockchip->a);
    }

    if (strcmp(arg, s_rpiMappingRockchip->b.name) == 0){
        return &(s_rpiMappingRockchip->b);
    }

    if (strcmp(arg, s_rpiMappingRockchip->c.name) == 0){
        return &(s_rpiMappingRockchip->c);
    }

    if (strcmp(arg, s_rpiMappingRockchip->d.name) == 0){
        return &(s_rpiMappingRockchip->d);
    }

    if (strcmp(arg, s_rpiMappingRockchip->e.name) == 0){
        return &(s_rpiMappingRockchip->e);
    }

    if (strcmp(arg, s_rpiMappingRockchip->p0_r1.name) == 0){
        return &(s_rpiMappingRockchip->p0_r1);
    }

    if (strcmp(arg, s_rpiMappingRockchip->p0_g1.name) == 0){
        return &(s_rpiMappingRockchip->p0_g1);
    }

    if (strcmp(arg, s_rpiMappingRockchip->p0_b1.name) == 0){
        return &(s_rpiMappingRockchip->p0_b1);
    }

    if (strcmp(arg, s_rpiMappingRockchip->p0_r2.name) == 0){
        return &(s_rpiMappingRockchip->p0_r2);
    }

    if (strcmp(arg, s_rpiMappingRockchip->p0_g2.name) == 0){
        return &(s_rpiMappingRockchip->p0_g2);
    }

    if (strcmp(arg, s_rpiMappingRockchip->p0_b2.name) == 0){
        return &(s_rpiMappingRockchip->p0_b2);
    }

    return NULL;
} 

static int read_gpio(char *arg)
{
    struct RPIMappingRockchip_GPIO* gpio = select_gpio_by_name(arg);
    if (gpio == NULL) {
        fprintf(stderr, "Cannot find gpio by name[%s]\n", arg);
        return -1;
    }
    uint32_t data =readGPIOs(gpio->rpi_mask);
     
    fprintf(stdout, "Read gpio: name[%s], data=0x%lx, status:%s\n", arg, data, (data!=0)?"High":"Low");
    return 0;
}

static int write_gpio(char *arg)
{
    struct RPIMappingRockchip_GPIO* gpio = select_gpio_by_name(arg);
    if (gpio == NULL) {
        fprintf(stderr, "Cannot find gpio by name[%s]\n", arg);
        return -1;
    }
    fprintf(stdout, "write gpio, name[%s]\n", arg);
    setGPIOsMode(gpio->rpi_mask);
    setGPIOs(gpio->rpi_mask);
    uint32_t data =readGPIOs(gpio->rpi_mask);

    fprintf(stdout, "Read gpio: name[%s], data=0x%lx, status:%s\n", arg, data, (data!=0)?"High":"Low");

    return 0;
}

static int clear_gpio(char *arg)
{
    struct RPIMappingRockchip_GPIO* gpio = select_gpio_by_name(arg);
    if (gpio == NULL) { 
        fprintf(stderr, "Cannot find gpio by name[%s]\n", arg);
        return -1;
    }
    fprintf(stdout, "clear gpio, name[%s]\n", arg);
    setGPIOsMode(gpio->rpi_mask);
    setGPIOs(gpio->rpi_mask, true);
    uint32_t data =readGPIOs(gpio->rpi_mask);

    fprintf(stdout, "Read gpio: name[%s], data=0x%lx, status:%s\n", arg, data, (data!=0)?"High":"Low");

    return 0;
}


static int exec_program(int argc, char *argv[])
{ 
    char* arg = argv[optind];
    int programSize = sizeof(s_programMain)/ sizeof(s_programMain[0]);
    fprintf(stdout, "size of executable program=%d\n", programSize);
    for (int i = 0; i < programSize; i++)
    {
        if (strcmp(argv[optind], s_programMain[i].name) == 0) {
            return s_programMain[i].main_function(argc-optind+1, argv+optind-1);
        }
    }

    return 0;
}

static void fillScreenColor(uint32_t color, uint32_t width, uint32_t height);


static int usage(int argc, char* argv[])
{
    fprintf(stdout, "Usage: %s -Option[parameter] -p [program_name]\n", argv[0]);
    fprintf(stdout, "\t-R: read gpio by name, ex: -Rclock: Read clock data\n");
    fprintf(stdout, "\t-W: write gpio by name, ex: -Wclock: Write clock data as 1\n");
    fprintf(stdout, "\t-C: write gpio by name, ex: -Cclock: Clear clock data to be 0\n");
    
    fprintf(stdout, "\t-p: execute program by name\n");
    int programSize = sizeof(s_programMain)/ sizeof(s_programMain[0]);
    for (int i = 0; i < programSize; i++)
    {
        fprintf(stdout, "\t\t[%d]%s\n", i, s_programMain[i].name);
    }
    return 0;
}


static int parseOpt(int argc, char *argv[])
{
    int opt;

    while((opt = getopt(argc, argv, "T:R:W:C:p")) != -1) {
        switch(opt) {
            case 'T':
                fillScreenColor(atoi(optarg), 32, 16);
                break;
            case 'R':
                read_gpio(optarg);
		break;
	    case 'W':
                write_gpio(optarg);
                break;
            case 'C':
                clear_gpio(optarg);
                break;

            case 'p':
                return exec_program(argc, argv);
                break;
            default:
                return usage(argc, argv);
        }
    }
    return 0;
}

static void prepareGPIOs() {
    uint32_t inputs = 0;
    inputs |= 
              s_rpiMappingRockchip->output_enable.rpi_mask|//252
              s_rpiMappingRockchip->clock.rpi_mask|//254
              s_rpiMappingRockchip->strobe.rpi_mask|//253
              s_rpiMappingRockchip->p0_g2.rpi_mask|//218  // direction not work
              s_rpiMappingRockchip->p0_b2.rpi_mask|//216   // not work, pull down by someone?
              s_rpiMappingRockchip->p0_r2.rpi_mask|//233 // sometimes not work!?   
              s_rpiMappingRockchip->p0_r1.rpi_mask| // 222 
              s_rpiMappingRockchip->p0_g1.rpi_mask|//221   
              s_rpiMappingRockchip->p0_b1.rpi_mask|//234   // sometimes not work!?
              s_rpiMappingRockchip->a.rpi_mask |//160  
              s_rpiMappingRockchip->b.rpi_mask//161   
              ;
    setGPIOsMode(inputs); // set GPIOs Mode: output
}
/*
static void tick() {//clock function for data entry

    //sets clock high low very fast
    setGPIO(&(s_rpiMappingRockchip->clock));
    setGPIO(&(s_rpiMappingRockchip->clock), CLEAR_DATA);
}
*/
static void tick() {//clock function for data entry

    //sets clock high low very fast
    setGPIOs(s_rpiMappingRockchip->clock.rpi_mask);
    setGPIOs(s_rpiMappingRockchip->clock.rpi_mask, CLEAR_DATA);
}


/*
static void selectLine(uint32_t row)
{
   setGPIOs(s_rpiMappingRockchip->a.rpi_mask |  //160  
            s_rpiMappingRockchip->b.rpi_mask |   //161
            s_rpiMappingRockchip->c.rpi_mask   //161
            , CLEAR_DATA);
    if (row & 0x1) {
        setGPIO(&(s_rpiMappingRockchip->a));
    }
    if (row & 0x2) {
        setGPIO(&(s_rpiMappingRockchip->b));
    }
    if (row & 0x4) {
        setGPIO(&(s_rpiMappingRockchip->c));
    }
           
}*/

static void selectLine(uint32_t row)
{
   setGPIOs(s_rpiMappingRockchip->a.rpi_mask |  //160  
            s_rpiMappingRockchip->b.rpi_mask |   //161
            s_rpiMappingRockchip->c.rpi_mask   //161
            , CLEAR_DATA);
    if (row & 0x1) {
        setGPIOs(s_rpiMappingRockchip->a.rpi_mask);
    }
    if (row & 0x2) {
        setGPIOs(s_rpiMappingRockchip->b.rpi_mask);
    }
    if (row & 0x4) {
        setGPIOs(s_rpiMappingRockchip->c.rpi_mask);
    }
 
}


/*
static void latch() {//this function cleans up and latches the data, so diplays it
    //latch output
    setGPIO(&(s_rpiMappingRockchip->strobe));
    setGPIO(&(s_rpiMappingRockchip->strobe), CLEAR_DATA);

    setGPIO(&(s_rpiMappingRockchip->output_enable));

    setGPIO(&(s_rpiMappingRockchip->a), CLEAR_DATA);
    setGPIO(&(s_rpiMappingRockchip->b), CLEAR_DATA);
    setGPIO(&(s_rpiMappingRockchip->c), CLEAR_DATA);
    setGPIO(&(s_rpiMappingRockchip->d), CLEAR_DATA);

    setGPIO(&(s_rpiMappingRockchip->output_enable), CLEAR_DATA);
}
*/
static void latch() {//this function cleans up and latches the data, so diplays it
    //latch output
    setGPIOs(s_rpiMappingRockchip->strobe.rpi_mask);
    setGPIOs(s_rpiMappingRockchip->strobe.rpi_mask, CLEAR_DATA);

    setGPIOs(s_rpiMappingRockchip->output_enable.rpi_mask);

    setGPIOs(s_rpiMappingRockchip->a.rpi_mask, CLEAR_DATA);
    setGPIOs(s_rpiMappingRockchip->b.rpi_mask, CLEAR_DATA);
    setGPIOs(s_rpiMappingRockchip->c.rpi_mask, CLEAR_DATA);
    setGPIOs(s_rpiMappingRockchip->d.rpi_mask, CLEAR_DATA);

    setGPIOs(s_rpiMappingRockchip->output_enable.rpi_mask, CLEAR_DATA);
}

/*
static void setColor(uint32_t color)
{
    // For test, set color to be RED only
    setGPIO(&(s_rpiMappingRockchip->p0_r1));
    setGPIO(&(s_rpiMappingRockchip->p0_g1), CLEAR_DATA);
    setGPIO(&(s_rpiMappingRockchip->p0_b1), CLEAR_DATA);
    setGPIO(&(s_rpiMappingRockchip->p0_r2));
    setGPIO(&(s_rpiMappingRockchip->p0_g2), CLEAR_DATA);
    setGPIO(&(s_rpiMappingRockchip->p0_b2), CLEAR_DATA);

}*/

static void setColor(uint32_t color)
{
    // For test, set color to be RED only
    setGPIOs(s_rpiMappingRockchip->p0_r1.rpi_mask);
    setGPIOs(s_rpiMappingRockchip->p0_g1.rpi_mask, CLEAR_DATA);
    setGPIOs(s_rpiMappingRockchip->p0_b1.rpi_mask, CLEAR_DATA);
    setGPIOs(s_rpiMappingRockchip->p0_r2.rpi_mask);
    setGPIOs(s_rpiMappingRockchip->p0_g2.rpi_mask, CLEAR_DATA);
    setGPIOs(s_rpiMappingRockchip->p0_b2.rpi_mask, CLEAR_DATA);

}

static void fillScreenColor(uint32_t color, uint32_t width, uint32_t height){
    prepareGPIOs();
    while(1) {
        for(uint32_t col = 0; col < width; col++){
            for(uint32_t row = 0; row < (height/2); row++) {
                selectLine(row);
                setColor(color);   
                tick();
                if ( ((row+1)%8) == 0)
                    latch();
            }
        }
    }
}

int main(int argc, char *argv[]) {
   
    bool result = init_rpi_mapping_rk3288_once();
    if(!result){
        fprintf(stderr, "init rpi mapping rk3288 once failed!!\n");
	return -1;
    }
    if (argc <=1) {
        return usage(argc, argv);
    }
    parseOpt(argc, argv);

    return 0;
}
