#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>  
#include <signal.h>  
#include <time.h> 


#define R_ON    "R"
#define R_OFF   "r"
#define G_ON    "G"
#define G_OFF   "g"
#define B_ON    "B"
#define B_OFF   "b"
#define F_ON    "F"
#define F_OFF   "f"

int count = 0;

char r_buf[5];
int fd;

int led_flag = 0;

void initRGB(int fd);
void cleanRGB(int fd);
void lightRGB(int fd);

// 定时器到期时的信号处理函数  
static void timer_handler (int signum)  
{  
    printf("Timer expired!\n");  
}  
  

void *thread_button(void *arg) {  
    int ret;

    printf("my button fd = %d\n",fd);

    while (1)
    {
        ret = read(fd,r_buf,sizeof(r_buf));
        if(r_buf[0] == '1')
        {
            lightRGB(fd);
            led_flag = 1;
            printf("This is button experiment\n");
        }else{
            led_flag = 0;
            cleanRGB(fd);
        }
    }
    pthread_exit(NULL);  
} 

void *led_light(void *arg) {  
    int ret;

    printf("led light = %d\n",fd);

    while (1)
    {
        ret = read(fd,r_buf,sizeof(r_buf));
        while (led_flag == 0){}
        if(led_flag == 1)
        {
            count = write(fd,R_ON,sizeof(R_ON));
            usleep(200000);
            count = write(fd,R_OFF,sizeof(R_OFF));
            usleep(200000);
        }
        if(led_flag == 1){
            count = write(fd,G_ON,sizeof(G_ON));
            usleep(200000);  
            count = write(fd,G_OFF,sizeof(G_OFF));
            usleep(200000);            
        }
        if(led_flag == 1)
        {
            count = write(fd,B_ON,sizeof(B_ON));
            usleep(200000);
            count = write(fd,B_OFF,sizeof(B_OFF));
            usleep(200000);             
        }
    }
    pthread_exit(NULL);  
}  

void initRGB(int fd)
{
    count = write(fd,R_ON,sizeof(R_ON));
    //printf("count = %d\n",count);
    sleep(1);
    count = write(fd,R_OFF,sizeof(R_OFF));
    //printf("count = %d\n",count);
    sleep(1);

    count = write(fd,G_ON,sizeof(G_ON));
    //printf("count = %d\n",count);
    sleep(1);
    count = write(fd,G_OFF,sizeof(G_OFF));
    //printf("count = %d\n",count);
    sleep(1);

    count = write(fd,B_ON,sizeof(B_ON));
    //printf("count = %d\n",count);
    sleep(1);
    count = write(fd,B_OFF,sizeof(B_OFF));
    //printf("count = %d\n",count);
    sleep(1);

    count = write(fd,F_ON,sizeof(F_ON));
    //printf("count = %d\n",count);
    sleep(1);
    count = write(fd,F_OFF,sizeof(F_OFF));
    //printf("count = %d\n",count);
    sleep(1);
}

void lightRGB(int fd)
{
    count = write(fd,R_ON,sizeof(R_ON));
    //printf("count = %d\n",count);
    usleep(200000);
    count = write(fd,R_OFF,sizeof(R_OFF));
    //printf("count = %d\n",count);
    usleep(200000);

    count = write(fd,G_ON,sizeof(G_ON));
    //printf("count = %d\n",count);
    usleep(200000);
    count = write(fd,G_OFF,sizeof(G_OFF));
    //printf("count = %d\n",count);
    usleep(200000);

    count = write(fd,B_ON,sizeof(B_ON));
    //printf("count = %d\n",count);
    usleep(200000);
    count = write(fd,B_OFF,sizeof(B_OFF));
    //printf("count = %d\n",count);
    usleep(200000);
}


void cleanRGB(int fd)
{
    write(fd,R_OFF,sizeof(R_OFF));
    write(fd,G_OFF,sizeof(G_OFF));
    write(fd,B_OFF,sizeof(B_OFF));
}
int main()
{
	int ret=0;
	int num = 0;
    pthread_t thread_id;
    pthread_t thread_id2;
    printf("my test v1.0\n");
    //打开设备
    fd = open("/dev/myTest", O_RDWR);
    if(fd>0)
        printf("button open success\n");
    else
        printf("button open fail\n");

    initRGB(fd);

    ret = pthread_create(&thread_id, NULL, thread_button, NULL);
    ret = pthread_create(&thread_id2, NULL, led_light, NULL);
	while(1)
	{
	}
    close(fd);
    pthread_join(thread_id, NULL);
    pthread_join(thread_id2, NULL);
    return 0;
}

