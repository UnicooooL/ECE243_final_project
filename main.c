/* address values that exist in the system */
#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000
#define HW_REGS_BASE          0xff200000
#define HW_REGS_SPAN          0x00200000
#define HW_REGS_MASK          (HW_REGS_SPAN - 1)

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* bit code for HEX display */
#define ZERO 0b00111111
#define ONE 0b00000110
#define TWO 0b01011011
#define THREE 0b01001111
#define FOUR 0b01100110
#define FIVE 0b01101101
#define SIX 0b01111101
#define SEVEN 0b00000111
#define EIGHT 0b01111111
#define NINE 0b01100111
#define BLANK 0b00000000

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000

/* Screen size */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 48
#define NUM_BOXES 25
#define ROW 5
#define COLUMN 5

#define FALSE 0
#define TRUE 1

/* self defined consts */
#define increase 1
#define decrease -1

/* include library */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/* function declare */
void initial_setup(short int color[5], int idx[25]);
void wait_for_vsync();
void drawBoxInitial(int coor_x, int coor_y, short int color);
void clear_screen_init();
void draw_pixel(int x, int y, short int line_color);
void scoreDisplay();

/* global variable */
volatile int pixel_buffer_start; 
bool initial = false;

/* main function */
int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables
    short int color[5] = {YELLOW, PINK, CYAN, BLUE, GREY};
    int idx[25];  //CONTAINER FOR 25 RANDOM
    for(int i = 0; i < 25; i++){
        idx[i] = rand() % 5;
    }
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; // first store the address in the back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen_init(); // pixel_buffer_start points to the pixel buffer
    initial_setup(color, idx);
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = SDRAM_BASE;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen_init(); // pixel_buffer_start points to the pixel buffer
    initial_setup(color, idx);
    initial = true;

    while (1){
        scoreDisplay();
        colorRestrict(idx);
        /* change the color for boxes, use interrupt to get user input */
        //SW 0-4: used to switch colors; | 0, Yellow | 1, Pink | 2, Cyan | 3, Blue | 4, Grey |
        //LED 0-5: an indicator for users to tell them which color they cannot pick
        //KEY 0-2: switch player
        //HEX: a displayer to show the marks

        /* code for drawing the boxes */
        //get user chose color and change the user's 'region' into same color...

        /* code for updating the locations of the new box based on user input (read from interrupt) */
        //find the new direction/position...
        //update location...

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        initial = false;
    }
    return 0;
}

/* code for subroutines (not shown) */
/* given helper function from lecture */
void wait_for_vsync(){
    volatile int* pixel_ctrl_ptr = (int*) PIXEL_BUF_CTRL_BASE;  //pixel controller; address is DMA
    register int status;
    *pixel_ctrl_ptr = 1;  //start the synchronization process
    status = *(pixel_ctrl_ptr + 3);  //read status register at adress 0xFF20302C
    while ((status & 0x01) != 0){  //wait for s bit; poll IO
        status = *(pixel_ctrl_ptr + 3);
    }
}

/* draw the colored pixel for initial set up */
void drawBoxInitial(int coor_x, int coor_y, short int color){  //48 pixels in total
    for(int i = 0; i < 48; i++){
        for(int j = 0; j < 48; j++){
            draw_pixel(coor_x + i, coor_y + j, color);
        }
    }
}

/* clean the whole screen */
void clear_screen_init(){
    for(int temp_x = 0; temp_x < RESOLUTION_X; temp_x++){
        for(int temp_y = 0; temp_y < RESOLUTION_Y; temp_y++){
            draw_pixel(temp_x, temp_y, WHITE);  //draw white background
        }
    }
}

/* plot a pixel on the VGA display */
void draw_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

/* initialization of the original state; changed when restart */
void initial_setup(short int color[5], int idx[25]){
    int delta_j = 47;
    int delta_i = 47;
    int scale = 0;
    int id = 0;
    for(int i = 0; i < ROW; i++){
        for(int j = 0; j < COLUMN; j++){
            drawBoxInitial(40 + scale * delta_i, j * delta_j, color[idx[id]]);  //draw the color box
            id++;
        }
        scale++;
    }
}  //240 for xy; 5*5; 48 pixels per box;

/* LED indicators */
void colorRestrict(int idx[25]){
    if(idx[20] ==YELLOW || idx[4] == YELLOW){
        int* LED = LEDR_BASE;
        *LED = 0x1;
    }else if(idx[20] == PINK || idx[4] == PINK){
        int* LED = LEDR_BASE;
        *LED = 0x1;
    }else if(idx[20] == CYAN || idx[4] == CYAN){
        int* LED = LEDR_BASE;
        *LED = 0x1;
    }else if(idx[20] == BLUE || idx[4] == BLUE){
        int* LED = LEDR_BASE;
        *LED = 0x1;
    }else if(idx[20] == GREY || idx[4] == GREY){
        int* LED = LEDR_BASE;
        *LED = 0x1;
    }
}

/* HEX display for score */
void scoreDisplay(){
    //initial state
    if(initial == true){
        int *HEX0 = HEX3_HEX0_BASE;
        *HEX0 = 0x3f403f3f;  //3f = 0; 40 = -;
        int *HEX5 = HEX5_HEX4_BASE;
        *HEX5 = ZERO;
    }
}

