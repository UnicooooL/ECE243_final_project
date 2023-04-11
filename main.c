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
void config_KEYs();
void __attribute__((interrupt))__cs3_isr_irq(void);
void disable_A9_interrupts(void);
void enable_A9_interrupts(void);
void set_A9_IRQ_stack(void);
void config_GIC(void);
void pushbutton_ISR(void);
void switches_ISR (void);
void config_interrupt(int N, int CPU_target);
void wait();
void getColor(int colorNum);
int selectColor();
void changeColorRegion(int idx[25], int color[5], int user_select, int player);
int switchPlayer();
void draw_whole(int idx[25]);
void drawWhite(int color[5], int idx[25]);
bool checkWin(int color[5], int idx[25]);
void displayWinner();



/* global variable */
volatile int pixel_buffer_start; 
bool initial = false;
bool yellow_0 = false;
bool pink_1 = false;
bool cyan_2 = false;
bool blue_3 = false;
bool grey_4 = false;
bool playerA_0 = false;
bool playerB_1 = false;
bool win_A = false;
bool win_B = false;
bool game_over = false;

int playerA[25] = {0};
int playerB[25] = {0};



// Define the remaining exception handlers
void __attribute__((interrupt)) __cs3_reset(void) {
    while (1);
}
void __attribute__((interrupt)) __cs3_isr_undef(void) {
    while (1);
}
void __attribute__((interrupt)) __cs3_isr_swi(void) {
    while (1);
}
void __attribute__((interrupt)) __cs3_isr_pabort(void) {
    while (1);
}
void __attribute__((interrupt)) __cs3_isr_dabort(void) {
    while (1);
}
void __attribute__((interrupt)) __cs3_isr_fiq(void) {
    while (1);
}



/* main function */
int main(void){

    disable_A9_interrupts(); // disable interrupts in the A9 processor
    set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
    config_GIC(); // configure the general interrupt controller
    config_KEYs(); // configure pushbutton KEYs to generate interrupts
    enable_A9_interrupts(); // enable interrupts in the A9 processor

    srand(time(NULL)); // makes sure that a new color pattern is generated each time
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    
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
    scoreCount(idx, color);  //put after the color idx has been changed (after get user input)
	playerA_0 = true;
	
    while (1){
		/* decide which region to flashing */
		initial_setup(color, idx); 
		wait_for_vsync();
		drawWhite(color, idx);
		wait_for_vsync();
		
        /* LED 0-5: an indicator for users to tell them which color they cannot pick */
        colorRestrict(idx, color);  //each time change the idx to change the color stored

        /* change the color for boxes, use interrupt to get user input */
        //SW 0-4: used to switch colors; | 0, Yellow | 1, Pink | 2, Cyan | 3, Blue | 4, Grey |
        int user_select = selectColor();  //store the color code
        //KEY 0-2: switch player
        int player = switchPlayer();  //store the player; -1 for none, 0 for A(1), 1 for B(2)
        //get user chose color and change the user's 'region' into same color...
		changeColorRegion(idx, color, user_select, player);  //change the color idx
        /*starting from lower left corner, traverse to find all connected boxes that have same color as that corner,
          changed the idx of color according to user interrupt. Same for upper right corner. Only need to change 
          current region's color to the selected color; no need to manipulate other color boxes. */

		game_over = checkWin(color, idx);
		if(game_over){
			//call a function to display the winner
			scoreCount(idx, color);
			displayWinner();
			break;
		}
		
		/* animation for current selected region */
		//flashingAnimation(playerA_0, playerB_1);

        /* code for drawing the boxes with new color idx */
        initial_setup(color, idx);  //use precreated function to draw box with new colors

		/* HEX: a displayer to show the marks */
        //calculate the boxes two users owned
		scoreCount(idx, color);
		
		 //wait_for_vsync(); // swap front and back buffers on VGA vertical sync
		
		/* animation for current selected region */
		//flashingAnimation(playerA_0, playerB_1);
		
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        initial = false;
    }
    return 0;
}



/* code for subroutines (not shown) */

/* display the winner */
void displayWinner(){
	//c: 3, 4, 6; 0b01011000; 0x58
	//o: 2, 3, 4, 6; 0b01011100; 0x5C
	//n: 2, 4, 6; 0b01010100; 0x54
	//g: 0, 1, 2, 3, 5, 6; 0b01101111; 0x6F 
	int HEX_0_3 = 0x585C546F;  //0 g, 1 n, 2 o, 3 c
	int HEX_4;
	if(win_A){
		HEX_4 = 0x00007700;  //4 blank, 5 A; 0, 1, 2, 4, 5, 6; 0b01110111; 0x77
	}
	if(win_B){
		HEX_4 = 0x00007C00;  //4 blank, 5 B; 2, 3, 4, 5, 6; 0b01111100; 0x7C
	}
    int *HEX_0_base = HEX3_HEX0_BASE;
    int *HEX_4_base = HEX5_HEX4_BASE;
    *HEX_0_base = HEX_0_3;
    *HEX_4_base = HEX_4;
}

/* check for the winner */
bool checkWin(int color[5], int idx[25]){
	int lower_left = color[idx[20]];
	int upper_right = color[idx[4]];
	
	/* to check if there are still colors other than these two */
	for(int i = 0; i < 25; i++){
		if(color[idx[i]] != lower_left && color[idx[i]] != upper_right){
			return false;  //represents game not over
		}
	}
	return true;  //represents game is over
}

void drawWhite(int color[5], int idx[25]){
	/* initialize needed 2d array */
	int color_idx[5][5];
	int status[5][5];
	int i = 0;
	//put color idx into 2d array
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			color_idx[row][col] = color[idx[i]];
			i++;
		}
	}
	//store the check status; 0 for unchecked (initially), -1 for need to be check, 1 for checked
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			status[row][col] = 0;
		}
	}
	
	/* start changing color idx */
	/* lower left corner */
	if(playerA_0 == true){
		int ll_color = color_idx[4][0];
		status[4][0] = -1;  //origin need to be checked
		int current = color_idx[4][0];
		for(int row = 4; row > 0 || row == 0; row--){
			for(int col = 0; col < 5; col++){
				if(color_idx[row][col] == current && status[row][col] == -1){
					color_idx[row][col] = WHITE;
					status[row][col] = 1;  //flip notation to checked
					if(row == 0){
						status[row][col + 1] = -1;  // right one
						continue;
					}
					if(col == 4){
						status[row - 1][col] = -1;  // up one
						continue;
					}
					status[row - 1][col] = -1;  // up one; up one and right one need to be checked
					status[row][col + 1] = -1;  // right one
				}  //move to current row next position at the right
			}
		}

		/* store back to idx */
		i = 0;
		int new_[25];
		for(int row = 0; row < 5; row++){
			for(int col = 0; col < 5; col++){
				new_[i] = color_idx[row][col];
				i++;
			}
		}
		draw_whole(new_);
	}
	
	/* upper right corner */
	if(playerB_1 == true){
		int ur_color = color_idx[0][4];
		status[0][4] = -1;  //origin need to be checked
		int current = color_idx[0][4];
		for(int row = 0; row < 5; row++){
			for(int col = 4; col >= 0; col--){
				if(color_idx[row][col] == current && status[row][col] == -1){
					color_idx[row][col] = WHITE;
					status[row][col] = 1;  //flip notation to checked
					if(row == 4){
						status[row][col - 1] = -1;  // left one
						continue;
					}
					if(col == 0){
						status[row + 1][col] = -1;  // down one
						continue;
					}
					status[row + 1][col] = -1;  // up one; down one and left one need to be checked
					status[row][col - 1] = -1;  // left one
				}  //move to current row next position at the right
			}
		}

		/* store back to idx */
		i = 0;
		int new[25];
		for(int row = 0; row < 5; row++){
			for(int col = 0; col < 5; col++){
				new[i] = color_idx[row][col];
				i++;
			}
		}
		draw_whole(new);
	}
}

void draw_whole(int idx[25]){  //here idx[i] stores a color
    int delta_j = 47;
    int delta_i = 47;
    int scale = 0;
    int id = 0;
    for(int i = 0; i < ROW; i++){
        for(int j = 0; j < COLUMN; j++){
            //drawBoxInitial(40 + scale * delta_i, j * delta_j, color[idx[id]]);  //draw the color box
            drawBoxInitial(40 + j * delta_j, scale * delta_i, idx[id]);  //draw the color box
            id++;
        }
        scale++;
    }
}

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
            //drawBoxInitial(40 + scale * delta_i, j * delta_j, color[idx[id]]);  //draw the color box
            drawBoxInitial(40 + j * delta_j, scale * delta_i, color[idx[id]]);  //draw the color box
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
    int tens_B_num = 0;
    int rest_B = cntB;  //hold the ones digit for B
    int tens_A = ZERO;  //hold the tens digit for A
    int tens_A_num = 0;
    int rest_A = cntA;  //hold the ones digit for A

    /* user A score */
    while(rest_B - 10 > 0 || rest_B - 10 == 0){  //seperate the tens digit
        rest_B = rest_B - 10;
        tens_B_num++;
    }
    int number[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    /* HEX1 display */
    int bit_code_HEX1[10] = {ZERO_T, ONE_T, TWO_T, THREE_T, FOUR_T, FIVE_T, SIX_T, SEVEN_T, EIGHT_T, NINE_T};
    for(int i = 0; i < 10; i++){
        if(number[i] == tens_B_num){
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

    /* upper right corner score */
    while(rest_A - 10 > 0 || rest_A - 10 == 0){  //seperate the tens digit
        rest_A = cntA - 10;
        tens_A_num++;
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
        if(number[i] == tens_A_num){
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



/* new counting logic */
void scoreCount(int idx[25], int color[5]){
	/* initialize needed 2d array */
	int color_idx[5][5];
	int status[5][5];
	//int idx_number[5][5];
	int i = 0;
	//put color idx into 2d array
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			color_idx[row][col] = color[idx[i]];
			i++;
		}
	}
	//store the check status; 0 for unchecked (initially), -1 for need to be check, 1 for checked
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			status[row][col] = 0;
		}
	}
	
	/* start counting */
	/* lower left corner */
	int ll_color = color_idx[4][0];
	int cnt_ll = 0;
	status[4][0] = -1;  //origin need to be checked
	for(int row = 4; row >= 0; row--){
		for(int col = 0; col < 5; col++){
			if(color_idx[row][col] == ll_color && status[row][col] == -1){
				cnt_ll++;  //current position need to be checked and equal to the color
				status[row][col] = 1;  //flip notation to checked
				if(row == 0){
					status[row][col + 1] = -1;  // right one
					continue;
				}
				if(col == 4){
					status[row - 1][col] = -1;  // up one
					continue;
				}
				status[row - 1][col] = -1;  // up one; up one and right one need to be checked
				status[row][col + 1] = -1;  // right one
			}  //move to current row next position at the right
		}
	}
	
	/* upper right corner */
	//store the check status; 0 for unchecked (initially), -1 for need to be check, 1 for checked
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			status[row][col] = 0;
		}
	}
	int ur_color = color_idx[0][4];
	int cnt_ur = 0;
	status[0][4] = -1;  //origin need to be checked
	for(int row = 0; row < 5; row++){
		for(int col = 4; col >= 0; col--){
			if(color_idx[row][col] == ur_color && status[row][col] == -1){
				cnt_ur++;  //current position need to be checked and equal to the color
				status[row][col] = 1;  //flip notation to checked
				if(row == 4){
					status[row][col - 1] = -1;  // left one
					continue;
				}
				if(col == 0){
					status[row + 1][col] = -1;  // down one
					continue;
				}
				status[row + 1][col] = -1;  // down one; up one and right one need to be checked
				status[row][col - 1] = -1;  // left one
			}  //move to current row next position at the right
		}
	}
	/* display on HEX */
	scoreDisplay(cnt_ll, cnt_ur);
}

/* change the color region new logic */
void changeColorRegion(int idx[25], int color[5], int user_select_, int player){
	/* initialize needed 2d array */
	int color_idx[5][5];
	int status[5][5];
	int i = 0;
	//put color idx into 2d array
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			color_idx[row][col] = color[idx[i]];
			i++;
		}
	}
	//store the check status; 0 for unchecked (initially), -1 for need to be check, 1 for checked
	for(int row = 0; row < 5; row++){
		for(int col = 0; col < 5; col++){
			status[row][col] = 0;
		}
	}
	
	/* start changing color idx */
	/* lower left corner */
	if(player == 1){
		int ll_color = color_idx[4][0];
		status[4][0] = -1;  //origin need to be checked
		int current = color_idx[4][0];
		for(int row = 4; row > 0 || row == 0; row--){
			for(int col = 0; col < 5; col++){
				if(color_idx[row][col] == current && status[row][col] == -1){
					color_idx[row][col] = user_select_;
					status[row][col] = 1;  //flip notation to checked
					if(row == 0){
						status[row][col + 1] = -1;  // right one
						continue;
					}
					if(col == 4){
						status[row - 1][col] = -1;  // up one
						continue;
					}
					status[row - 1][col] = -1;  // up one; up one and right one need to be checked
					status[row][col + 1] = -1;  // right one
				}  //move to current row next position at the right
			}
		}

		/* store back to idx */
		i = 0;
		for(int row = 0; row < 5; row++){
			for(int col = 0; col < 5; col++){
				for(int id = 0; id < 5; id++){
					if(color[id] == color_idx[row][col]){
						idx[i] = id;
					}
				}
				i++;
			}
		}
	}
	
	/* upper right corner */
	if(player == 0){
		int ur_color = color_idx[0][4];
		status[0][4] = -1;  //origin need to be checked
		int current = color_idx[0][4];
		for(int row = 0; row < 5; row++){
			for(int col = 4; col >= 0; col--){
				if(color_idx[row][col] == current && status[row][col] == -1){
					color_idx[row][col] = user_select_;
					status[row][col] = 1;  //flip notation to checked
					if(row == 4){
						status[row][col - 1] = -1;  // left one
						continue;
					}
					if(col == 0){
						status[row + 1][col] = -1;  // down one
						continue;
					}
					status[row + 1][col] = -1;  // up one; down one and left one need to be checked
					status[row][col - 1] = -1;  // left one
				}  //move to current row next position at the right
			}
		}

		/* store back to idx */
		i = 0;
		for(int row = 0; row < 5; row++){
			for(int col = 0; col < 5; col++){
				for(int id = 0; id < 5; id++){
					if(color[id] == color_idx[row][col]){
						idx[i] = id;
					}
				}
				i++;
			}
		}
	}
	
}


/* return the selected color */
int selectColor(){
    if(yellow_0 == true){
        return YELLOW;
    }else if(pink_1 == true){
        return PINK;
    }else if(cyan_2 == true){
        return CYAN;
    }else if(blue_3 == true){
        return BLUE;
    }else if(grey_4 == true){
        return GREY;
    }else{
        return -1;  //case when no color is selected
    }
}

/* return the turn */
int switchPlayer(){
    if(playerA_0 == true){
        return 0;
    }else if(playerB_1 == true){
        return 1;
    }else{
        return -1;
    }
}



/* setup the KEY interrupts in the FPGA */
void config_KEYs() { 
    volatile int * KEY_ptr = (int *) 0xFF200050; // pushbutton KEY base address
    *(KEY_ptr + 2) = 0xF; // enable interrupts for the two KEYs
}


// Define the IRQ exception handler 
void __attribute__((interrupt)) __cs3_isr_irq(void) {
    // Read the ICCIAR from the CPU Interface in the GIC
    int interrupt_ID = *((int *)0xFFFEC10C);
    if (interrupt_ID == 73){ // check if interrupt is from the KEYs
        pushbutton_ISR();
    }
    else {
        while (1);
    }
    // Write to the End of Interrupt Register (ICCEOIR)
    *((int *)0xFFFEC110) = interrupt_ID;
}


/*
* Turn off interrupts in the ARM processor
*/
void disable_A9_interrupts(void) {
    int status = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}


/*
* Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void) {
    int status = 0b01010011;
    asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}



/*
* Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack(void) {
    int stack, mode;
    stack = 0xFFFFFFFF - 7; // top of A9 onchip memory, aligned to 8 bytes
    /* change processor to IRQ mode with interrupts disabled */
    mode = 0b11010010;
    asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
    /* set banked stack pointer */
    asm("mov sp, %[ps]" : : [ps] "r"(stack));
    /* go back to SVC mode before executing subroutine return! */
    mode = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
}


/*
* Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void) {
    config_interrupt (73, 1); // configure the FPGA KEYs interrupt (73)
    // Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all priorities
    *((int *) 0xFFFEC104) = 0xFFFF;
    // Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
    *((int *) 0xFFFEC100) = 1;
    // Configure the Distributor Control Register (ICDDCR) to send pending interrupts to CPUs
    *((int *) 0xFFFED000) = 1;
}

/********************************************************************
* Pushbutton - Interrupt Service Routine
*******************************************************************/
void pushbutton_ISR(void) {
    /* KEY base address */
    volatile int * KEY_ptr = (int *) KEY_BASE;
    int press;
    press = *(KEY_ptr + 3); // read the key interrupt register
    *(KEY_ptr + 3) = press; // Clear the interrupt
    if (press & 0x1){ // KEY0
        printf("Key0 is pressed, player A's turn\n");  //start of player A
		switches_ISR();
        playerA_0 = false;  //after pressed, bool is true, A select color
        playerB_1 = true;
        // 
        // select colour for player one function
    } else if (press & 0x2){ // KEY1
		switches_ISR();
        printf("Key1 is pressed, player B's turn\n");  //end of player A, start of player B
        playerB_1 = false;  //after pressed, bool is true, B select color
        playerA_0 = true;
        // select colour for player two function
    } else{ 
       printf("Wrong key pressed, press KEY0 for player A's turn and KEY1 for player B's turn\n");
       playerA_0 = false;
       playerB_1 = false;
    }
    return;
}

void switches_ISR (void){
    volatile int* SW_ptr = (int *) SW_BASE;
    int switch_value = *SW_ptr;

    if (switch_value & 0x1){ // SW0
        printf("SW0 is ON\n");
        yellow_0 = true;
        pink_1 = false;
        cyan_2 = false;
        blue_3 = false;
        grey_4 = false;
    } else if (switch_value & 0x2){ // SW1
        printf("SW1 is ON\n");
        pink_1 = true;
        yellow_0 = false;
        cyan_2 = false;
        blue_3 = false;
        grey_4 = false;
    } else if(switch_value & 0x4){  //Sw2
        cyan_2 = true;
        pink_1 = false;
        yellow_0 = false;
        blue_3 = false;
        grey_4 = false;
    }else if(switch_value & 0x8){  //Sw3
        blue_3 = true;
        cyan_2 = false;
        pink_1 = false;
        yellow_0 = false;
        grey_4 = false;
    }else if(switch_value & 0x10){  //SW4
        grey_4 = true;
        blue_3 = false;
        cyan_2 = false;
        pink_1 = false;
        yellow_0 = false;
    }
    else{ 
       printf("No switch is ON\n");
       grey_4 = false;
        blue_3 = false;
        cyan_2 = false;
        pink_1 = false;
        yellow_0 = false;
    }

}

void config_interrupt(int N, int CPU_target) {
    int reg_offset, index, value, address;
    /* Configure the Interrupt Set-Enable Registers (ICDISERn).
    * reg_offset = (integer_div(N / 32) * 4
    * value = 1 << (N mod 32) */
    reg_offset = (N >> 3) & 0xFFFFFFFC;
    index = N & 0x1F;
    value = 0x1 << index;
    address = 0xFFFED100 + reg_offset;
    /* Now that we know the register address and value, set the appropriate bit */
    *(int *)address |= value;
    /* Configure the Interrupt Processor Targets Register (ICDIPTRn)
    * reg_offset = integer_div(N / 4) * 4
    * index = N mod 4 */
    reg_offset = (N & 0xFFFFFFFC);
    index = N & 0x3;
    address = 0xFFFED800 + reg_offset + index;
    /* Now that we know the register address and value, write to (only) the
    * appropriate byte */
    *(char *)address = (char)CPU_target;
}

void getColor(int colorNum){
    switch(colorNum){
        case 0:
            return YELLOW;
            break;
        case 1:
            return PINK;
            break;
        case 2:
            return CYAN;
            break;
        case 3:
            return BLUE;
            break;
        case 4:
            return GREY;
            break;
        default:
            return WHITE;
    }

}

void wait(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020; // pixel (DMA) controller (I/O)
    register int status;
    *pixel_ctrl_ptr = 1; // start synchronization; s bit is set to 1
    status = *(pixel_ctrl_ptr + 3); // read status register at address
    while ((status & 0x01) != 0){
        status = *(pixel_ctrl_ptr+3);
    }
}

/* animation */
/*void flashingAnimation(bool A, bool B){
    int delta_j = 47;
    int delta_i = 47;
    int scale = 0;

    // if player A turn
    if(A && !B){
        // iterate through the grid
        for (int a = 0; a < 25; a++){
            int id = 0;
            for(int i = 0; i < ROW; i++){
                for(int j = 0; j < COLUMN; j++){
                    // draw the box white, wait and re-draw the original color
                    if (playerA[id] != 0){
                        drawBoxInitial(40 + scale * delta_i, j * delta_j, WHITE);   // draw the color box as white
                        wait();
                        drawBoxInitial(40 + scale * delta_i, j * delta_j, idx[id]);
                    }
                    id++;
                }
                scale++;
            }
        }
    }
}*/
	
	
