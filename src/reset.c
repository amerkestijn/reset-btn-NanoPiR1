#include <unistd.h>
#include <linux/input.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>

#define EVENT_BUF_NUM 8

char *DEFAULT_BUTTON_PATH = "/dev/input/event0";
char *DEFAULT_LED_PATH    = "/sys/class/leds/LED3/brightness";

void toggleLED(u_int8_t on)
{
    int fd = open(DEFAULT_LED_PATH, O_WRONLY);
    if (fd >= 0) {
        write(fd, on ? "1" : "0", 1);
        close(fd);
    }
}

int main(int argc, char **argv)
{
    char *button_path = DEFAULT_BUTTON_PATH;
    if (argc > 1) {
        button_path = argv[1];
    }

    struct input_event events[EVENT_BUF_NUM];
    int                fd = open(button_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("\nUnable to read from the device\n");
        return (1);
    }

    clock_t pressed_at = 0;
    while (1) {
        ssize_t nread = read(fd, events, sizeof(struct input_event) * EVENT_BUF_NUM);
        if (nread < 0 && errno == EAGAIN) {
            if (pressed_at > 0) {
                clock_t msec = clock() - pressed_at;
                if (msec >= 3000) {
                    sync();
                    syslog(LOG_NOTICE, "Notice placed correctly");
                    system("shutdown -h now");
                    return 0;
                }
            }
            usleep(5000);
            continue;
        }

        if (nread < 0) {
            perror("bad read");
            close(fd);
            return errno;
            syslog(LOG_NOTICE, "Something wrong");
        }

        int num = nread / sizeof(struct input_event);
        for (int i = 0; i < num; i++) {
            if (events[i].type == EV_KEY) {
                switch (events[i].value) {
                case 0:
                    toggleLED(0);
                    pressed_at = 0;
                    break;
                case 1:
                    toggleLED(1);
                    pressed_at = clock();
                    break;
                }
            }
        }
    }

    close(fd);
    return (0);
}
