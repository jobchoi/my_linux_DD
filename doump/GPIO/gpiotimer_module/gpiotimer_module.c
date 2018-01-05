#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/timer.h>


#define BCM_IO_BASE         0x3F000000                   // RaspberryPi 2,3 I/O Peripherals Base 
#define GPIO_BASE           (BCM_IO_BASE + 0x200000)     // GPIO Register Base 
#define GPIO_SIZE           0xB4                         // 0x7E200000 – 0x7E20000B3  

#define GPIO_MAJOR 		    200
#define GPIO_MINOR 		    0
#define GPIO_DEVICE         "gpioled"              
#define GPIO_LED            10                         
#define GPIO_SW             24		       

static char msg[BLOCK_SIZE] = {0};                      // write( ) 함수에서 읽은 데이터 저장

/* 입출력 함수를 위한 선언 */
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

/* 유닉스 입출력 함수들의 처리를 위한 구조체 */
static struct file_operations gpio_fops = {
   .owner = THIS_MODULE,
   .read = gpio_read,
   .write = gpio_write,
   .open = gpio_open,
   .release = gpio_close,
};

struct cdev gpio_cdev;   
static int switch_irq;
static struct timer_list timer;              /* 타이머 처리를 위한 구조체 */

/* 타이머 처리를 위한 함수 */
static void timer_func(unsigned long data)
{
        gpio_set_value(GPIO_LED, data);      /* LED의 상태 설정 */

        /* 다음 실행을 위한 타이머 설정 */
        timer.data = !data;                                /* LED의 상태를 토글 */  
        timer.expires = jiffies + ((1*HZ)/8);             /* LED의 켜고 끄는 주기는 1초 */         
        add_timer(&timer);                                /* 타이머 추가 */         
}

/* 인터럽트 처리를 위한 인터럽트 서비스 루틴(Interrupt Service Routine) */
static irqreturn_t isr_func(int irq, void *data)
{
    if(irq == switch_irq && !gpio_get_value(GPIO_LED)) {
        gpio_set_value(GPIO_LED, 1);
    } else if(irq == switch_irq && gpio_get_value(GPIO_LED)) {
        gpio_set_value(GPIO_LED, 0);
    }
    printk("GPIO Interrupt!!\n");
    return IRQ_HANDLED;
}

static int gpio_open(struct inode *inod, struct file *fil)
{
    printk("GPIO Device opened(%d/%d)\n", imajor(inod), iminor(inod));

    //===========================================================
    // 모듈 사용 횟수 카운트 
    // 모듈을 여러곳에서 동시에 사용하고 있는 경우 사용 횟수 카운트를 증가 시킨다.
    //===========================================================
    try_module_get(THIS_MODULE);

    return 0;
}

static int gpio_close(struct inode *inod, struct file *fil)
{
    printk("GPIO Device closed(%d:%d)\n", imajor(inod), iminor(inod));


    //===========================================================
    // 모듈 사용 횟수 카운트 
    // 모듈을 여러곳에서 동시에 사용하고 있는 경우 사용 횟수 카운트를 증가 시킨다.
    //===========================================================
    module_put(THIS_MODULE);
    return 0;
}


static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off)
{
    int count;
    strcat(msg, " from Kernel");

    //===========================================================
    // 유저 영역으로 데이터를 보낸다 
    //===========================================================
    count = copy_to_user(buff, msg, strlen(msg)+1);     
 
    printk("GPIO Device read : %s(%d)\n", msg, count);
    return count;
}

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    short count;
    memset(msg, 0, BLOCK_SIZE);

    //===========================================================
    // 유저 영역으로 부터 데이터를 가져온다.
    //===========================================================
    count = copy_from_user(msg, buff, len);

    if(!strcmp(msg, "0")) {
        del_timer_sync(&timer);  
        gpio_set_value(GPIO_LED, 0);
    } else {
        /* 타이머 초기화와 타이머 처리를 위한 함수 등록 */  
        init_timer(&timer);
        timer.function = timer_func; 
        /* timer_list 구조체 초기화 : 기본값 LDE 켜기, 주기 1초 */  
        timer.data = 1L;                                 
        timer.expires = jiffies + ((1*HZ)/8);
        add_timer(&timer); 
    }

    printk("GPIO Device write : %s(%d)\n", msg, len);
    return count;
}

int GPIO_init(void)
{
    //===========================================================
    // dev_t 
    // 디바이스 파일의 major와 minor를 지정하기 위한 변수 타입으로 
    // 32 비트의 크기를 갖는다. (major 12bit, minor 20bit)
    //===========================================================
    dev_t devno;
    unsigned int count;
   // static void *map;                                   /* I/O 접근을 위한 변수 */
    int err;

    //===========================================================
    // insmod를 통해 initModule이 호출되었음을 확인 
    //===========================================================
    printk(KERN_INFO "Hello module!\n");

    //===========================================================
    // major와 minor를 전달하고 디바이스 파일의 번호를 받는다.
    // Major 200, Minor 199의 경우 devNo=0x0c8000c7이다. 
    //===========================================================
    devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);

    //===========================================================
    // char device 등록
    //===========================================================
    register_chrdev_region(devno, 1, GPIO_DEVICE);

    //===========================================================
    // char device 구조체 초기화 
    // register_chrdev_region 와 alloc_chrdev_region 를 이용하면
    // major 와 minor 예약은 할 수 있지만 cdev(character device)를 
    // 생성 하지는 않는다. 별도로 cdev_init, cdev_add 함수를 호출하여야 한다.
    //===========================================================
    cdev_init(&gpio_cdev, &gpio_fops);

    gpio_cdev.owner = THIS_MODULE;
    count = 1;
    err = cdev_add(&gpio_cdev, devno, count);
    if (err < 0) {
        printk("Error : Device Add\n");
        return -1;
    }

    //printk("'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
    //printk("'chmod 666 /dev/%s'\n", GPIO_DEVICE);

    //===========================================================
    // GPIO 사용을 요청한다. [리눅스 GPIO 커널 함수 사용] 
    // <linux/gpio.h> 
    //===========================================================
    gpio_request(GPIO_LED, "LED");      // 출력 
    gpio_request(GPIO_SW, "SWITCH");    // 입력 

    //===========================================================
    // GPIO 핀 방향 설정 
    //===========================================================    
    gpio_direction_output(GPIO_LED, 0);

    // GPIO 핀 IRQ 등록 
    switch_irq = gpio_to_irq(GPIO_SW);

    // GPIO IRQ Handler 등록 
    //request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING | IRQF_DISABLED, "switch", NULL);
    request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch", NULL);
    return 0;
}

void GPIO_exit(void)
{
    dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);

    //===========================================================
    // 등록 했던 타이머를 삭제해 준다. 
    //===========================================================
    del_timer_sync(&timer);

    //===========================================================
    // 문자 디바이스의 등록을 해제한다.
    //===========================================================  
    unregister_chrdev_region(devno, 1);

    //===========================================================
    // 문자 디바이스의 구조체를 해제한다.
    //===========================================================
    cdev_del(&gpio_cdev);

    //===========================================================
    // 사용이 끝난 인터럽트 해제
    //=========================================================== 
    free_irq(switch_irq, NULL);

    //===========================================================   
    // 더 이상 사용이 필요없는 경우 관련 자원을 해제한다.
    //=========================================================== 
    gpio_free(GPIO_LED);
    gpio_free(GPIO_SW);

    module_put(THIS_MODULE);

    printk(KERN_INFO "GPIO_exit\n");
}

module_init(GPIO_init);
module_exit(GPIO_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heejin Park");
MODULE_DESCRIPTION("Raspberry Pi 4 GPIO Device Module");
