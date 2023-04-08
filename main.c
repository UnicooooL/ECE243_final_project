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
//tens digit for HEX3-0; on HEX1
#define ZERO_T  0x00003F00
#define ONE_T   0x00000600
#define TWO_T   0x00005B00
#define THREE_T 0x00004F00
#define FOUR_T  0x00006600
#define FIVE_T  0x00006D00
#define SIX_T   0x00007D00
#define SEVEN_T 0x00000700
#define EIGHT_T 0x00007F00
#define NINE_T  0x00006700

//ones digit for HEX3
#define ZERO_B  0x3F000000
#define ONE_B   0x06000000
#define TWO_B   0x5B000000
#define THREE_B 0x4F000000
#define FOUR_B  0x66000000
#define FIVE_B  0x6D000000
#define SIX_B   0x7D000000
#define SEVEN_B 0x07000000
#define EIGHT_B 0x7F000000
#define NINE_B  0x67000000

//tens digit for HEX4, ones digit for HEX0
#define ZERO  0x0000003F
#define ONE   0x00000006
#define TWO   0x0000005B
#define THREE 0x0000004F
#define FOUR  0x00000066
#define FIVE  0x0000006D
#define SIX   0x0000007D
#define SEVEN 0x00000007
#define EIGHT 0x0000007F
#define NINE  0x00000067
#define BLANK 0x00000000

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
#include <time.h>

/* function declare */
void initial_setup(int color[5], int idx[25]);
void wait_for_vsync();
void drawBoxInitial(int coor_x, int coor_y, int color);
void clear_screen_init();
void draw_pixel(int x, int y, int line_color);
void scoreDisplay(int cntB, int cntA);
void colorRestrict(int idx[25], int color[5]);
void scoreCount(int idx[25], int color[5]);


/* global variable */
volatile int pixel_buffer_start; 
bool initial = false;


/* main function */
int main(void){
    srand(time(NULL)); // makes sure that a new color pattern is generated each time
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

    //set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
    //config_GIC(); // configure the general interrupt controller

    // declare other variables
    int color[5] = {YELLOW, PINK, CYAN, BLUE, GREY};
    int idx[25];  //CONTAINER FOR 25 RANDOM

    /* code generates the numbers such that no neighbouring colors are the same as the current colour */
    int grid_width = 5; 
    int grid_height = 5;
    for (int h = 0; h < grid_height; h++) {
        for (int w = 0; w < grid_width; w++) {
            int color_idx;
            // Loop generates a new color, until the neighbouring colors are all unique
            do {
                // gets the random color number until it doesn't match its neighbouring colors
                color_idx = rand() % 5;
            // loop checks if the current color is the color above it or the color beside it, is the same color
            } while ((h > 0 && idx[(h - 1) * grid_width + w] == color_idx) || (w > 0 && idx[h * grid_width + (w - 1)] == color_idx));
            idx[h * grid_width + w] = color_idx;
        }
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
    colorRestrict(idx, color);

    while (1){
        /* HEX: a displayer to show the marks */
        //calculate the boxes two users owned
        scoreCount(idx, color);  //put after the color idx has been changed (after get user input)

        /* LED 0-5: an indicator for users to tell them which color they cannot pick */
        colorRestrict(idx, color);  //each time change the idx to change the color stored
        /* change the color for boxes, use interrupt to get user input */
        //SW 0-4: used to switch colors; | 0, Yellow | 1, Pink | 2, Cyan | 3, Blue | 4, Grey |
        //KEY 0-2: switch player

        /* code for drawing the boxes; read from user interrupt */
        //get user chose color and change the user's 'region' into same color...
        /*starting from lower left corner, traverse to find all connected boxes that have same color as that corner,
          changed the idx of color according to user interrupt. Same for upper right corner. Only need to change 
          current region's color to the selected color; no need to manipulate other color boxes. */

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
void drawBoxInitial(int coor_x, int coor_y, int color){  //48 pixels in total
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
void draw_pixel(int x, int y, int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

/* initialization of the original state; changed when restart */
void initial_setup(int color[5], int idx[25]){
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
void colorRestrict(int idx[25], int color[5]){
    /* boolean values for different color pairs */
    bool yellow = false;
    bool pink = false;
    bool cyan = false;
    bool blue = false;
    /* light the LED for lower left corner and upper right color */
    //yellow, 1
    if(color[idx[20]] == YELLOW){
        int* LED = LEDR_BASE;
        *LED = 0x1;
        yellow = true;
    }
    if(color[idx[4]] == YELLOW){
        int* LED = LEDR_BASE;
        *LED = 0x1;
        yellow = true;
    }
    //pink, 2
    if(color[idx[20]] == PINK){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x3;
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x2;
        }
        pink = true;
    }
    if(color[idx[4]] == PINK){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x3;
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x2;
        }
        pink = true;
    }
    //cyan, 3
    if(color[idx[20]] == CYAN){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x5;        
        }else if(pink == true){
            int* LED = LEDR_BASE;
            *LED = 0x6;        
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x4;            
        }
        cyan = true;
    }
    if(color[idx[4]] == CYAN){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x5;        
        }else if(pink == true){
            int* LED = LEDR_BASE;
            *LED = 0x6;        
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x4;            
        }
        cyan = true;
    }
    //blue, 4
    if(color[idx[20]] == BLUE){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x9;
        }else if(pink == true){
            int* LED = LEDR_BASE;
            *LED = 0xA;
        }else if(cyan == true){
            int* LED = LEDR_BASE;
            *LED = 0xC;
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x8;
        }
        blue = true;
    }
    if(color[idx[4]] == BLUE){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x9;
        }else if(pink == true){
            int* LED = LEDR_BASE;
            *LED = 0xA;
        }else if(cyan == true){
            int* LED = LEDR_BASE;
            *LED = 0xC;
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x8;
        }
        blue = true;
    }
    //grey, 5
    if(color[idx[20]] == GREY){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x11;
        }else if(pink == true){
            int* LED = LEDR_BASE;
            *LED = 0x12;
        }else if(cyan == true){
            int* LED = LEDR_BASE;
            *LED = 0x14;
        }else if(blue == true){
            int* LED = LEDR_BASE;
            *LED = 0x18;
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x10;
        }
    }
    if(color[idx[4]] == GREY){
        if(yellow == true){
            int* LED = LEDR_BASE;
            *LED = 0x11;
        }else if(pink == true){
            int* LED = LEDR_BASE;
            *LED = 0x12;
        }else if(cyan == true){
            int* LED = LEDR_BASE;
            *LED = 0x14;
        }else if(blue == true){
            int* LED = LEDR_BASE;
            *LED = 0x18;
        }else{
            int* LED = LEDR_BASE;
            *LED = 0x10;
        }
    }
}

/* HEX display for score */
void scoreDisplay(int cntB, int cntA){
    int tens_B = ZERO_T;  //hold the tens digit for B
    int rest_B = cntB;  //hold the ones digit for B
    int tens_A = ZERO;  //hold the tens digit for A
    int rest_A = cntA;  //hold the ones digit for A

    /* user B score */
    while(rest_B - 10 > 0 || rest_B - 10 == 0){  //seperate the tens digit
        rest_B = rest_B - 10;
        tens_B++;
    }
    int number[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    /* HEX1 display */
    int bit_code_HEX1[10] = {ZERO_T, ONE_T, TWO_T, THREE_T, FOUR_T, FIVE_T, SIX_T, SEVEN_T, EIGHT_T, NINE_T};
    for(int i = 0; i < 10; i++){
        if(number[i] == tens_B){
            tens_B = bit_code_HEX1[i];  //assign tens the bit code for HEX1 position
            break;
        }
    }
    /* HEX0 display */
    int bit_code_HEX0[10] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE};
    for(int i = 0; i < 10; i++){
        if(number[i] == rest_B){
            rest_B = bit_code_HEX0[i];  //assign tens the bit code for HEX0 position
            break;
        }
    }

    /* user A score */
    while(rest_A - 10 > 0 || rest_A - 10 == 0){  //seperate the tens digit
        rest_A = cntA - 10;
        tens_A++;
    }
    /* HEX3 display */
    int bit_code_HEX3[10] = {ZERO_B, ONE_B, TWO_B, THREE_B, FOUR_B, FIVE_B, SIX_B, SEVEN_B, EIGHT_B, NINE_B};
    for(int i = 0; i < 10; i++){
        if(number[i] == rest_A){
            rest_A = bit_code_HEX3[i];  //assign tens the bit code for HEX3 position
            break;
        }
    }
    /* HEX4 display */
    int bit_code_HEX4[10] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE};
    for(int i = 0; i < 10; i++){
        if(number[i] == tens_A){
            tens_A = bit_code_HEX4[i];  //assign tens the bit code for HEX4 position
            break;
        }
    }

    /* Prepare for display */
    int HEX_0_3 = tens_B + rest_B + rest_A + 0x00400000;
    int HEX_4 = tens_A;
    int *HEX_0_base = HEX3_HEX0_BASE;
    int *HEX_4_base = HEX5_HEX4_BASE;
    *HEX_0_base = HEX_0_3;
    *HEX_4_base = HEX_4;
}

void scoreCount(int idx[25], int color[5]){
    //color for user A; lower left corner
    int A_color = color[idx[20]];
    int cntA = 1;
    int curr_loc = 20;
    int right = 5;
    int up = 5;
    int up_limit = 5;
    int right_limit = 5;
    bool enter_A = false;
    for(int j = 0; j < 4; j++){
        if(color[idx[curr_loc]] != A_color){
            break;
        }
        if(enter_A == true){
            cntA++;  //count previous current location if neighbouring color is same for region
        }
        for(int i = 1; i < right_limit; i++){  //right direction; not including origin
            if(color[idx[curr_loc + i]] == A_color){
                enter_A = true;
                cntA++;
            }else{
                break;
            }
            if(color[idx[curr_loc + i + 1]] != A_color){  //if connected box is not same color
                break;  //stop counting
            }
        }
        for(int i = 1; i < up_limit; i++){  //up direction
            if(color[idx[curr_loc - i * up]] == A_color){
                cntA++;
            }else{
                break;
            }
            if(i != up - 1 && color[idx[curr_loc - (i + 1) * up]] != A_color){
                break;
            }
        }
        curr_loc = 20 - (2 * j + 2) * 2;  //update current location; 20, 16, 12, 8, 4
        right_limit--;
        up_limit--;
    }
    //color for user B; upper right corner
    int B_color = color[idx[4]];
    int cntB = 1;
    int curr_loc_B = 4;
    int left = 5;
    int down = 5;
    int left_limit = 5;
    int down_limit = 5;
    bool enter_B = false;
    for(int j = 0; j < 4; j++){
        if(color[idx[curr_loc]] != B_color){
            break;
        }
        if(enter_B == true){
            cntB++;  //count previous current location if neighbouring color is same for region
        }
        for(int i = 1; i < left_limit; i++){  //left direction; not including origin
            if(color[idx[curr_loc_B - i]] == B_color){
                enter_B = true;
                cntB++;
            }else{
                break;
            }
            if(i != left - 1 && color[idx[curr_loc_B - i - 1]] != B_color){  //if connected box is not same color
                break;  //stop counting
            }
        }
        for(int i = 1; i < down_limit; i++){  //up direction
            if(color[idx[curr_loc_B + i * down]] == B_color){
                cntB++;
            }else{
                break;
            }
            if(i != down - 1 && color[idx[curr_loc_B + (i + 1) * down]] != B_color){
                break;
            }
        }
        curr_loc = 4 + (2 * j + 2) * 2;  //update current location; 20, 16, 12, 8, 4
        down_limit--;
        left_limit--;
    }
    scoreDisplay(cntB, cntA);
}

// /*
// Initialize the banked stack pointer register for IRQ mode
// Code from DE1-SoC_Computer_ARM.pdf, Listing 12 from 9.4 interrupts
// */
// void set_A9_IRQ_stack(void){
//     int stack, mode;
//     stack = A9_ONCHIP_END - 7; // top of A9 onchip memory, aligned to 8 bytes
//     /* change processor to IRQ mode with interrupts disabled */
//     mode = INT_DISABLE | IRQ_MODE;
//     asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
//     /* set banked stack pointer */
//     asm("mov sp, %[ps]" : : [ps] "r"(stack));
//     /* go back to SVC mode before executing subroutine return! */
//     mode = INT_DISABLE | SVC_MODE;
//     asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
// }


// /*
// Turn on interrupts in the ARM processor
// Code from DE1-SoC_Computer_ARM.pdf, Listing 12 from 9.4 interrupts
// */
// void enable_A9_interrupts(void){
//     int status = SVC_MODE | INT_ENABLE;
//     asm("msr cpsr, %[ps]" : : [ps] "r"(status));
// }

// /*
// Configure the Generic Interrupt Controller (GIC)
// Code from DE1-SoC_Computer_ARM.pdf, Listing 12 from 9.4 interrupts
// */
// void config_GIC(void){ // still editing //

//     int address; // used to calculate register addresses
    
//     // configure the key interrupts
//     // configure the SW interrupts

//     // Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all
//     // priorities
//     address = MPCORE_GIC_CPUIF + ICCPMR;
//     *((int *)address) = 0xFFFF;
//     // Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
//     address = MPCORE_GIC_CPUIF + ICCICR;
//     *((int *)address) = ENABLE;
//     // Configure the Distributor Control Register (ICDDCR) to send pending
//     // interrupts to CPUs
//     address = MPCORE_GIC_DIST + ICDDCR;
//     *((int *)address) = ENABLE;
// }