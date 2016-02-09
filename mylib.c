//Richard Farr (rfarr6)
//Implementation of mylib.h

#include "mylib.h"
#include "text.h"

u16* const videoBuffer = (u16*)0x6000000;
static char get_bit(char, char);

void wait_for_vblank() {
	while (SCANCOUNT > SCREEN_HEIGHT);
	while (SCANCOUNT < SCREEN_HEIGHT);
}

//Sets a pixel's RGB value at the screen's r'th row and c'th column (row and column indexing starts at 0)
void setPixel3(int r, int c, u16 color) {
	videoBuffer[OFFSET(r, c, SCREEN_WIDTH)] = color;
}

//Draws a filled rectangle
void drawRect3(int r, int c, int width, int height, u16 color) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			setPixel3(i+r, j+c, color);
		}
	}
}

//Draws a hollow rectangle
void drawHollowRect3(int r, int c, int width, int height, u16 color) {

	//Draw the top and bottom of the rectangle
	for (int j = 0; j < width; j++) {
		setPixel3(r, j, color);
		setPixel3(r+height-1, j, color);
	}

	//Draw the sides
	for (int i = 1; i < height-1; i++) {
		setPixel3(i, c, color);
		setPixel3(i, c+width-1, color);
	}
}

//Draws a filled triangle
void drawTriangle3(int r, int c, int side, u16 color) {
	int width = 1, i = 0, j = 0;
	while (width <= side) {
		drawRect3(i+r, j+c, width, 1, color);
		width += 2;
		i++;
		j--;
	}
}

//Draws an image array
void drawImage3(int r, int c, int width, int height, const u16* img) {
	int img_ndx = 0;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			setPixel3(i+r, j+c, img[img_ndx++]);
		}
	}
}

//Draws an image array slowly
void drawImageDelayed3(int r, int c, int width, int height, const u16* img, int delay) {
	int img_ndx = 0;
	volatile int k;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			setPixel3(i+r, j+c, img[img_ndx++]);
			for (k = 0; k < 250*delay; k++);
		}
	}
}

//Draws a string
void drawString3(int r, int c, const char* str, u16 fg_color, u16 bg_color) {
	int char_count = 0;
	while (*str != 0) {
		drawChar3(r, c+TEXT_WIDTH*char_count, *str, fg_color, bg_color);
		char_count++;
		str++;
	}
}

//Draws a character
void drawChar3(int r, int c, char chr, u16 fg_color, u16 bg_color) {
	for (int i = 0; i < TEXT_WIDTH; i++) {
		for (int j = 0; j < TEXT_HEIGHT; j++) {
			if (!get_bit(fontdata[OFFSET(chr, i, TEXT_WIDTH)], TEXT_HEIGHT-j-1)) {
				setPixel3(j+r, i+c, bg_color);
			} else {
				setPixel3(j+r, i+c, fg_color);
			}
		}
	}
}

char get_bit(char c, char bit) {
	return ((c >> bit) & 0x1);
}

//Returns an image rotated a multiple of 90 degrees
void rotateImage3(int width, int height, const u16* img, u16* ret_image, u16 degree_90) {
	int img_ndx = 0;

	switch (degree_90) {
		case 90:
			for (int j = 0; j < width; j++) {
				for (int i = height-1; i >= 0; i--) {
					ret_image[img_ndx++] = img[OFFSET(i, j, width)];
				}
			}

			break;
		case 180:
			for (int i = height-1; i >= 0; i--) {
				for (int j = width-1; j >= 0; j--) {
					ret_image[img_ndx++] = img[OFFSET(i, j, width)];
				}
			}

			break;
		case 270:
			for (int j = width-1; j >= 0; j--) {
				for (int i = 0; i < height; i++) {
					ret_image[img_ndx++] = img[OFFSET(i, j, width)];
				}
			}

			break;
		default:
			for (int i = 0; i < width*height; i++) {
				ret_image[i] = img[i];
			}

			break;
	}
}

//Returns an image mirrored about either its horizontal or vertical axis
void mirrorImage3(int width, int height, const u16* img, u16* ret_image, char vh) {
	int img_ndx = 0;

	if (vh == 'h' || vh == 'H') {
		for (int i = height-1; i >= 0; i--) {
			for (int j = 0; j < width; j++) {
				ret_image[img_ndx++] = img[OFFSET(i, j, width)];
			}
		}
	} else if (vh == 'v' || vh == 'V') {
		for (int i = 0; i < height; i++) {
			for (int j = width-1; j >= 0; j--) {
				ret_image[img_ndx++] = img[OFFSET(i, j, width)];
			}
		}
	} else {
		for (int i = 0; i < width*height; i++) {
			ret_image[i] = img[i];
		}
	}
}









