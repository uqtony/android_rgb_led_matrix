#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MMAP_PATH "/dev/mem"
//#define RK3399 "rk3399"
#define RK3288 "rk3288"

#ifdef RK3288
#define GPIO5_MMAP_BASE_ADDRESS 0xFF7C0000
#define CLOCK_CON_BASE_ADDRESS 0xFF760000
#define GPIO5_CLOCK_CON_OFFSET 0x0198
#define REGISTER_BLOCK_SIZE (4*1024)
//#define TEST_DATA 0x00000100
#define TEST_DATA 0x00000100
#endif

#ifdef RK3399
#define GPIO5_MMAP_BASE_ADDRESS 0xFF780000
#define CLOCK_CON_BASE_ADDRESS 0xFF760000
#define GPIO5_CLOCK_CON_OFFSET 0x037C
#define REGISTER_BLOCK_SIZE (4*1024)
#define TEST_DATA 0x08001004
#endif

static uint32_t* s_clock_base_reg = NULL;
static uint32_t* s_clock_base_reg_w = NULL;
static uint32_t* s_gpio5_base_reg = NULL;
static uint32_t* s_gpio5_base_reg_w = NULL;

static volatile uint32_t* s_gpio5_clock_con_reg = NULL;
static volatile uint32_t* s_gpio5_mmap_reg = NULL;
static volatile uint32_t* s_gpio5_mmap_direct_reg = NULL;


static uint32_t *mmap_register(off_t base, bool memread = true) {

  int mem_fd;
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    // Try to fall back to /dev/gpiomem. Unfortunately, that device
    // is implemented in a way that it _only_ supports GPIO, not the
    // other registers we need, such as PWM or COUNTER_1Mhz, which means
    // we only can operate with degraded performance.
    //
    // But, instead of failing, mmap() then silently succeeds with the
    // unsupported offset. So bail out here.
    //if (register_offset != GPIO_REGISTER_OFFSET)
    //  return NULL;

    //mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC);
    //if (mem_fd < 0) return NULL;
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

static bool mmap_all_register_once() {
    if (s_clock_base_reg != NULL) 
	return true;
    
    s_clock_base_reg = mmap_register(CLOCK_CON_BASE_ADDRESS); 
    if (s_clock_base_reg == NULL)
	return false;
    s_clock_base_reg_w = mmap_register(CLOCK_CON_BASE_ADDRESS, false); 
    if (s_clock_base_reg_w == NULL)
	return false;

    s_gpio5_base_reg = mmap_register(GPIO5_MMAP_BASE_ADDRESS);
    if (s_gpio5_base_reg == NULL)
	return false;
    s_gpio5_base_reg_w = mmap_register(GPIO5_MMAP_BASE_ADDRESS, false);
    if (s_gpio5_base_reg_w == NULL)
	return false;
    return true;
}

int main() {
    bool result = mmap_all_register_once();
    if(!result)
	return -1;

    s_gpio5_clock_con_reg = s_clock_base_reg + (GPIO5_CLOCK_CON_OFFSET/sizeof(uint32_t));
    s_gpio5_mmap_reg = s_gpio5_base_reg + (0/sizeof(uint32_t));
    s_gpio5_mmap_direct_reg = s_gpio5_base_reg_w + (4/sizeof(uint32_t));

    uint32_t clock_data = *(s_gpio5_clock_con_reg);    

    fprintf(stdout, "gpio5 clock_con data before= 0x%lx\n", clock_data);
    s_gpio5_clock_con_reg = s_clock_base_reg_w + (GPIO5_CLOCK_CON_OFFSET/sizeof(uint32_t));
    *(s_gpio5_clock_con_reg) = 0xffff0000;
    s_gpio5_clock_con_reg = s_clock_base_reg + (GPIO5_CLOCK_CON_OFFSET/sizeof(uint32_t));
    clock_data = *(s_gpio5_clock_con_reg);
    fprintf(stdout, "gpio5 clock_con data after = 0x%lx\n", clock_data);

    *(s_gpio5_mmap_direct_reg) |= TEST_DATA;

    uint32_t data = *(s_gpio5_mmap_reg);
    fprintf(stdout, "gpio5 data  before = 0x%lx\n", data);
    s_gpio5_mmap_reg = s_gpio5_base_reg_w + (0/sizeof(uint32_t));
    *(s_gpio5_mmap_reg) = TEST_DATA;
    s_gpio5_mmap_reg = s_gpio5_base_reg + (0/sizeof(uint32_t));
    data = *(s_gpio5_mmap_reg);
    fprintf(stdout, "gpio5 data  after = 0x%lx\n", data);
//    *(s_gpio5_mmap_reg) = 0x08001000;
//    data = *(s_gpio5_mmap_reg);
//    fprintf(stdout, "gpio5 data  after = 0x%lx\n", data);

    return 0;
}
