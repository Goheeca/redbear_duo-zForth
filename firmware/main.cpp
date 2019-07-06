#include "main.hpp"
volatile Mode mode_flag = FORTH;

#include "fatfs/ff.h"
//#include "fatfs/diskio.h"

#include "zforth_host.hpp"

void set_mode(system_event_t event, int param)
{
    int clicks = system_button_clicks(param);
    Serial.printf("set_mode(..., param ~= %d)\r\n", clicks);
    switch (clicks) {
        default:
        case 1:
            mode_flag = IDLE;
            break;
        case 2:
            mode_flag = FORTH;
            break;
        case 3:
            mode_flag = IO;
            break;
        case 4:
            mode_flag = CREATE_FS;
            break;
    }
}

static FATFS *fs;           /* Filesystem object */

bool mount_fs() {
    FRESULT res;        /* API result code */

    /* Register work area */
    fs = (FATFS *) malloc(sizeof(FATFS));
    res = f_mount(fs, "", 1);
    if (res) return false;
    return true;
}

void umount_fs() {
    /* Unregister work area */
    f_mount(0, "", 0);
    free(fs);
    fs = NULL;
}

/*PARTITION VolToPart[] = {
    {0, 0}
};*/

static bool created = false;

void create_fs() {
    Serial.printf("create_fs()\r\n");
    created = false;

    FRESULT res;        /* API result code */

    BYTE work[FF_MAX_SS]; /* Work area (larger is better for processing time) */

    //DWORD plist[] = {100, 0, 0, 0};  /* Divide drive into one partition */
    //f_fdisk(0, plist, work);                    /* Divide physical drive 0 */

    /* Create FAT volume */
    res = f_mkfs("", FM_FAT, 0, work, sizeof work);
    if (res) {
        mode_flag = IDLE;
        return;
    } else {
        mode_flag = IO;
        return;
    }
}

static bool mounted = false;


#define BUF_SIZE 50

void io() {
    Serial.printf("io()\r\n");

    if (!mounted) {
        if(!(mounted = mount_fs())) {
            RGB.color(255,255,255);
            delay(1000);
            RGB.color(0,0,0);
            delay(1000);
            RGB.color(255,255,255);
            delay(1000);
            return;
        }
    }

    char *buf = (char *) calloc(BUF_SIZE, sizeof(char));

    FRESULT res;        /* API result code */
    FIL fil;            /* File object */

    FILINFO fno;
    UINT bw;

    res = f_stat("counter.txt", &fno);
    /*switch(res) {
        case FR_NO_FILE:
            Serial.printf("FR_NO_FILE\r\n");
            if(!created) {
                res = f_open(&fil, "counter.txt", FA_CREATE_ALWAYS | FA_WRITE);
                f_sync(&fil);
                f_close(&fil);
                created = true;
            }
            break;
        case FR_OK:
            Serial.printf("FR_OK\r\n");
            res = f_open(&fil, "counter.txt", FA_WRITE);
            f_sync(&fil);
            f_write(&fil, "Test", 5, &bw);
            f_sync(&fil);
            f_close(&fil);
            break;
        default:
            Serial.printf("FR_ERROR\r\n");
    }
    free(buf);
    delay(1000);
    return;*/

    switch(res) {
        case FR_NO_FILE:
            res = f_open(&fil, "counter.txt", FA_CREATE_ALWAYS | FA_WRITE);
            f_sync(&fil);
            if(!res) {
                UINT bw;            /* Bytes written */
                snprintf(buf, 25, "%d", 123);
                f_write(&fil, buf, 25, &bw);
                f_sync(&fil);
                f_close(&fil);
                Serial.printf("Created.\r\n");
                Serial.printf("Buffer: ");
                for(int i = 0; i < BUF_SIZE; i++) Serial.printf("%02X", buf[i]);
                Serial.printf("\r\n");
            } else {
                Serial.printf("Creating failed: %u\r\n", res);
            }
            break;
        case FR_OK:
            res = f_open(&fil, "counter.txt", FA_READ);
            if(!res) {
                FSIZE_t size = f_size(&fil);
                Serial.printf("Size: %u\r\n", size);
                f_read(&fil, buf, size, &bw);
                f_close(&fil);
                Serial.printf("%u up to %u\r\n", bw, BUF_SIZE);
                if (bw) {
                    Serial.printf("Buffer: ");
                    for(int i = 0; i < BUF_SIZE; i++) Serial.printf("%02X", buf[i]);

                    int counter = atoi(buf);
                    Serial.printf("\r\nValue: %d\r\n", counter);

                    counter++;
                    snprintf(buf, size, "%d", counter);

                    res = f_open(&fil, "counter.txt", FA_WRITE);
                    if(!res) {
                        f_write(&fil, buf, size, &bw);
                        f_sync(&fil);
                        f_close(&fil);
                        Serial.printf("%u up to %u\r\n", bw, BUF_SIZE);
                    } else {
                        Serial.printf("Opening failed: %u\r\n", res);
                    }
                } else {
                    Serial.printf("Fail.\r\n");
                }
            } else {
                Serial.printf("Opening failed: %u\r\n", res);
            }
            break;
    }
    //umount_fs();
    free(buf);
    delay(1000);
}

void idle() {
    Serial.printf("idle()\r\n");

    RGB.color(255,0,0);
    delay(1000);
    RGB.color(0,255,0);
    delay(1000);
    RGB.color(0,0,255);
    delay(1000);
}



void cleanup(system_event_t event) {
    umount_fs();
}

// Entry points

void setup() // Put setup code here to run once
{
    System.on(button_final_click, set_mode);
    System.on(reset, cleanup);
    Serial.begin(9600);
    RGB.control(true);
}

void loop() // Put code here to loop forever
{
    switch (mode_flag) {
        case CREATE_FS:
            create_fs();
            break;
        case IO:
            io();
            break;
        case IDLE:
            idle();
            break;
        case FORTH:
            forth();
            break;
    }
}
