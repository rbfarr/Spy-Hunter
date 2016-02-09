//Richard Farr (rfarr6)
//Awesome Spy Hunter game!

#include <debugging.h>
#include <stdio.h>
#include "mylib.h"
#include "mydma.h"

#include "background.h"
#include "player_car.h"
#include "civilian_car1.h"
#include "civilian_car2.h"
#include "spy_car1.h"
#include "spy_car2.h"
#include "car_gun.h"
#include "explosion.h"
#include "title_screen.h"
#include "game_over.h"

/***** Primary game configuration *****/
#define MEDIAN_WIDTH 6
#define MEDIAN_HEIGHT 12
#define MAX_VEHICLES 4

#define MAX_PLAYER_BULLETS 3
#define MAX_ENEMY_BULLETS 4

#define INIT_LIVES 10
#define INIT_SCORE 0

//Player stats
static u16 player_lives;
static u16 player_score;

//Background configuration
static u16 road_color = RGB(14,14,14);
static u16 median_color = RGB(31,31,0);
static u16 line_color = WHITE;
/**************************************/

#define NULL_GUN (const u16*)0

//Why the hell doesn't the current C standard include a boolean type?!
#ifndef __cplusplus
typedef enum {false, true} bool;
#endif

//Game enums
typedef enum {UP, DOWN, LEFT, RIGHT, DOWN_FAST} direction;
typedef enum {SPY_C, CIVILIAN_C, PLAYER_C, EXPLODED_C} car_type;
typedef enum {GUNLEFT, GUNRIGHT} gun_select;
typedef enum {LOSE, WIN} end_type;

//Car structure for player and other vehicles. row and col are signed to allow for pseudo "off-screen" cars.
typedef struct {
	short row;
	short col;
	direction cur_dir;
	u16 width;
	u16 height;
	const u16* bitmap;
	const u16* gun1;
	const u16* gun2;
	car_type type;
} car;

//Center median structure
typedef struct {
	u16 row;
	u16 col;
	u16 width;
	u16 height;
	u16 color;
} median_t;

//Bullet structure
typedef struct {
	u16 row;
	u16 col;
	u16 width;
	u16 height;
	bool fired;
	u16 color;
	gun_select f_gun;
} bullet;

//Array of gun orientations
static u16 guns[4][CAR_GUN_WIDTH*CAR_GUN_HEIGHT];

//Prototypes. See function definition for description.
void update_frame(car*, car*, median_t*, bullet*, bullet*);
void update_vehicles(car*, car*, bullet*);
void draw_vehicle(car*, short);
void fire_weapon(gun_select, bullet*, u16, car*);
void update_bullets(bullet*, bullet*, car*, car*);
void detect_collisions(car*, car*);

void end_game(end_type, car*, car*, bullet*, bullet*);
void init_game(car*, car*, bullet*, bullet*);

int main() {
	// Initialize display control register.
	REG_DISPCNT = MODE3 | BG2;

	//Draw title screen with flashing PRESS START button.
	volatile int k;
	dma_drawimage3(0, 0, TITLE_SCREEN_WIDTH, TITLE_SCREEN_HEIGHT, title_screen);

	while (!KEY_PRESSED(BUTTON_START)) {
		drawString3(0, 0, "PRESS START", WHITE, road_color);
		for (k = 0; k < 100000; k++) {
			if (KEY_PRESSED(BUTTON_START)) break;
		}

		drawRect3(0, 0, 80, 8, road_color);
		for (k = 0; k < 100000; k++) {
			if (KEY_PRESSED(BUTTON_START)) break;
		}
	}

	//Initialize median.
	median_t median = {.row = 0, .col = SCREEN_WIDTH/2-MEDIAN_WIDTH/2, .width = MEDIAN_WIDTH, .height = MEDIAN_HEIGHT, .color = median_color};

	//Declare player and other vehicles.
	car player;
	car vehicles[MAX_VEHICLES];

	//Generate rotated and mirrored guns.
	rotateImage3(CAR_GUN_WIDTH, CAR_GUN_HEIGHT, car_gun, guns[0], 360);
	mirrorImage3(CAR_GUN_WIDTH, CAR_GUN_HEIGHT, car_gun, guns[1], 'v');
	rotateImage3(CAR_GUN_WIDTH, CAR_GUN_HEIGHT, car_gun, guns[2], 180);
	mirrorImage3(CAR_GUN_WIDTH, CAR_GUN_HEIGHT, car_gun, guns[3], 'h');

	//Declare bullet arrays.
	bullet player_bullets[MAX_PLAYER_BULLETS];
	bullet enemy_bullets[MAX_ENEMY_BULLETS];

	//Initalize game and draw initial frame.
	init_game(&player, vehicles, player_bullets, enemy_bullets);
	update_frame(&player, vehicles, &median, player_bullets, enemy_bullets);

	//Initialize gun states.
	bool fire_left = false, fire_right = false;

	//Need initial height.
	u16 i_height = player.height;

	//Kill some spies.
	while (true) {

		//Don't allow exploded players to move.
		if (player.type != EXPLODED_C) {

			//Move player with boundary checking. Don't allow player to move into upper-right portion of the road because that makes the game too easy.
			if (KEY_PRESSED(BUTTON_UP) && (player.row > SCREEN_HEIGHT/2-15 || (player.col <= SCREEN_WIDTH/2 && player.row > 0))) {
				player.row--;
				player.cur_dir = UP;
			} else if (KEY_PRESSED(BUTTON_LEFT) && player.col > SCREEN_WIDTH/4+CAR_GUN_WIDTH) {
				player.col--;
				player.cur_dir = LEFT;
			} else if (KEY_PRESSED(BUTTON_RIGHT) && player.col < 3*SCREEN_WIDTH/4-PLAYER_CAR_WIDTH-CAR_GUN_WIDTH) {
				player.col++;
				player.cur_dir = RIGHT;
			} else if (KEY_PRESSED(BUTTON_DOWN)) {
				player.row += 2;
				player.cur_dir = DOWN_FAST;
			} else {
				player.row++;
				player.cur_dir = DOWN;
			}

			//Increment overall score.
			player_score++;
		} else {

			//Move exploded cars to the bottom of the screen.
			player.row++;
			player.cur_dir = DOWN;
		}

		//Redraw player car if it goes off screen.
		if (player.row >= SCREEN_HEIGHT) {

			//Remove last little bit of car remaining at bottom of screen.
			dma_drawrect3(player.row-1, player.col-CAR_GUN_WIDTH, player.width+2*CAR_GUN_WIDTH, 1, road_color);

			player.row = SCREEN_HEIGHT/4;
			player.col = SCREEN_WIDTH/2+20;
			player.width = PLAYER_CAR_WIDTH;
			player.height = PLAYER_CAR_HEIGHT;
			player.bitmap = player_car;
			player.gun1 = guns[0];
			player.gun2 = guns[1];
			player.type = PLAYER_C;

			player_lives--;
		} else if (player.row > SCREEN_HEIGHT-player.height) {

			//Prevents drawing car outside of video buffer.
			player.height = SCREEN_HEIGHT-player.row;
		} else {
			player.height = i_height;
		}

		//If ammo is to be fired, move into a dummy state.
		if (KEY_PRESSED(BUTTON_L) && player.type == PLAYER_C) {
			fire_left = true;
			drawString3(0, 0, "PEW!PEW!", WHITE, BLUE);
		} else if (KEY_PRESSED(BUTTON_R) && player.type == PLAYER_C) {
			fire_right = true;
			drawString3(0, 0, "PEW!PEW!", WHITE, BLUE);
		} else {
			dma_drawrect3(0, 0, SCREEN_WIDTH/4-10, 8, BLUE);
		}

		//Fire weapons
		if (fire_left && !KEY_PRESSED(BUTTON_L)) {
			fire_weapon(GUNLEFT, player_bullets, MAX_PLAYER_BULLETS, &player);
			fire_left = false;
		} else if (fire_right && !KEY_PRESSED(BUTTON_R)) {
			fire_weapon(GUNRIGHT, player_bullets, MAX_PLAYER_BULLETS, &player);
			fire_right = false;
		}

		//Update frame to match game state.
		update_frame(&player, vehicles, &median, player_bullets, enemy_bullets);

		//Player loses when he runs out of lives.
		if (player_lives == 0) {
			end_game(LOSE, &player, vehicles, player_bullets, enemy_bullets);

		//Player wins when he reaches 9999 points.
		} else if (player_score >= 9999) {
			end_game(WIN, &player, vehicles, player_bullets, enemy_bullets);
		}
	}

	return 0;
}

/* Keeps game screen up to date. */
void update_frame(car* player, car* vehicles, median_t* med, bullet* player_bullets, bullet* enemy_bullets) {
	//Go ahead and declare a buffer for player status strings generated by sprintf.
	char string_buffer[20];

	//Wait for next vblank to begin updating the video buffer.
	wait_for_vblank();

	//Erase previous median and draw a rectangle from the top of the screen to the lower limit of the first median line.
	dma_drawrect3(0, med->col, med->width, SCREEN_HEIGHT, road_color);
	dma_drawrect3(0, med->col, med->width, med->row-6, med->color);

	//Generate a cool animated median effect by drawing a series of vertical rectangles.
	int m_row = med->row, m_col = med->col;
	while (m_row < SCREEN_HEIGHT-med->height) {
		dma_drawrect3(m_row, m_col, med->width, med->height, med->color);
		m_row += med->height+6;
	}

	//Draw the final median line until hitting the bottom of the screen.
	dma_drawrect3(m_row, m_col, med->width, SCREEN_HEIGHT-m_row, med->color);

	//Increment starting row of median and reset if it exceeds the median height.
	med->row++;
	if (med->row > med->height) med->row = 0;

	//Update all vehicles on the screen. Pass enemy_bullets as an argument so that the spy vehicles can fire their weapons too.
	update_vehicles(player, vehicles, enemy_bullets);

	//Update all bullets on the screen, checking for bullet-vehicle collisions.
	update_bullets(player_bullets, enemy_bullets, player, vehicles);

	//Detect all possible vehicle collisions.
	detect_collisions(player, vehicles);

	//Draw player stats in upper right-hand corner of screen.
	sprintf(string_buffer, "Lives:%2i", player_lives);
	drawString3(0, 3*SCREEN_WIDTH/4+10, string_buffer, WHITE, BLUE);
	sprintf(string_buffer, "PTS:%4i", player_score);
	drawString3(9, 3*SCREEN_WIDTH/4+10, string_buffer, WHITE, BLUE);
}

/* Keeps vehicles up to date */
void update_vehicles(car* player, car* vehicles, bullet* ammo) {

	//Draw player vehicle.
	draw_vehicle(player, player->height);

	//Update all other vehicles.
	for (int i = 0; i < MAX_VEHICLES; i++) {

		//Generate a random new vehicle (starting at a negative row) when an old one goes off screen.
		if (vehicles[i].row >= SCREEN_HEIGHT) {

			//Generate a spy with guns.
			if (GBA_RAND(SPY_C, CIVILIAN_C) == SPY_C) {
				if (GBA_RAND(1, 2) == 1) {
					vehicles[i].row = -SPY_CAR2_HEIGHT;
					vehicles[i].col = SCREEN_WIDTH/4+6;
					vehicles[i].cur_dir = DOWN;
					vehicles[i].width = SPY_CAR2_WIDTH;
					vehicles[i].height = SPY_CAR2_HEIGHT;
					vehicles[i].bitmap = spy_car2;
					vehicles[i].gun1 = guns[3];
					vehicles[i].gun2 = guns[2];
					vehicles[i].type = SPY_C;
				} else {
					vehicles[i].row = -SPY_CAR1_HEIGHT;
					vehicles[i].col = SCREEN_WIDTH/2-27;
					vehicles[i].cur_dir = DOWN;
					vehicles[i].width = SPY_CAR1_WIDTH;
					vehicles[i].height = SPY_CAR1_HEIGHT;
					vehicles[i].bitmap = spy_car1;
					vehicles[i].gun1 = guns[3];
					vehicles[i].gun2 = guns[2];
					vehicles[i].type = SPY_C;
				}

			//Generate a civilian vehicle.
			} else {
				if (GBA_RAND(1, 2) == 1) {
					vehicles[i].row = -CIVILIAN_CAR2_HEIGHT;
					vehicles[i].col = SCREEN_WIDTH/4+10;
					vehicles[i].cur_dir = DOWN;
					vehicles[i].width = CIVILIAN_CAR2_WIDTH;
					vehicles[i].height = CIVILIAN_CAR2_HEIGHT;
					vehicles[i].bitmap = civilian_car2;
					vehicles[i].gun1 = NULL_GUN;
					vehicles[i].gun2 = NULL_GUN;
					vehicles[i].type = CIVILIAN_C;
				} else {
					vehicles[i].row = -CIVILIAN_CAR1_HEIGHT;
					vehicles[i].col = SCREEN_WIDTH/2-27;
					vehicles[i].cur_dir = DOWN;
					vehicles[i].width = CIVILIAN_CAR1_WIDTH;
					vehicles[i].height = CIVILIAN_CAR1_HEIGHT;
					vehicles[i].bitmap = civilian_car1;
					vehicles[i].gun1 = NULL_GUN;
					vehicles[i].gun2 = NULL_GUN;
					vehicles[i].type = CIVILIAN_C;
				}
			}

			continue;
		}

		//Move vehicle down the screen.
		vehicles[i].row++;

		/* At the start of the game all vehicles are initially of zero height. No need to draw any of them until the first "flat" vehicle hits
		 * the bottom of the screen and gets converted into a real 2-dimensional vehicle. Gives player time to catch his breath before enemies
		 * start showing up.
		 */
		if (vehicles[i].height != 0) {

			//Draw portion of vehicle based on where its starting row, taking care not to draw outside video buffer.
			if (vehicles[i].row > -vehicles[i].height && vehicles[i].row < 0) {
				draw_vehicle(&vehicles[i], -(vehicles[i].height+vehicles[i].row));
			} else if (vehicles[i].row >= 0 && vehicles[i].row <= SCREEN_HEIGHT-vehicles[i].height) {
				draw_vehicle(&vehicles[i], vehicles[i].height);
			} else if (vehicles[i].row > SCREEN_HEIGHT-vehicles[i].height) {
				draw_vehicle(&vehicles[i], SCREEN_HEIGHT-vehicles[i].row);
			}

			//Make enemies fire weapons 1 out of 20 possible times.
			if (vehicles[i].type == SPY_C && GBA_RAND(1, 20) == 10) {
				fire_weapon(GUNRIGHT, ammo, MAX_ENEMY_BULLETS, &vehicles[i]);
			}
		}
	}
}

/* Draws vehicle structure */
void draw_vehicle(car* vehicle, short height) {

	//Based on vehicle direction calculate its previous row and column.
	int p_row = vehicle->row, p_col = vehicle->col;
	switch (vehicle->cur_dir) {
		case UP:
			p_row++;
			break; 
		case DOWN:
			p_row--;
			break;
		case LEFT:
			p_col++;
			break;
		case RIGHT:
			p_col--;
			break;
		case DOWN_FAST:
			p_row -= 2;
	}

	//By my convention, negative heights will indicate that the vehicle should be drawn from the bottom up.
	if (height < 0) {
		dma_drawimage3(0, vehicle->col, vehicle->width, -height, vehicle->bitmap+OFFSET(-(vehicle->row), 0, vehicle->width));
	} else {
		//Clear out old vehicle and gun locations using the road color and draw the vehicle in its new location.
		dma_drawrect3(p_row, p_col, vehicle->width, vehicle->height, road_color);
		dma_drawrect3(p_row+vehicle->height/4, p_col-CAR_GUN_WIDTH, CAR_GUN_WIDTH, CAR_GUN_HEIGHT, road_color);
		dma_drawrect3(p_row+vehicle->height/4, p_col+vehicle->width, CAR_GUN_WIDTH, CAR_GUN_HEIGHT, road_color);
		dma_drawimage3(vehicle->row, vehicle->col, vehicle->width, height, vehicle->bitmap);

		//Draw guns, if the vehicle has them.
		if (vehicle->gun1 != NULL_GUN && vehicle->gun2 != NULL_GUN) {
			dma_drawimage3(vehicle->row+vehicle->height/4, vehicle->col-CAR_GUN_WIDTH, CAR_GUN_WIDTH, CAR_GUN_HEIGHT, vehicle->gun2);
			dma_drawimage3(vehicle->row+vehicle->height/4, vehicle->col+vehicle->width, CAR_GUN_WIDTH, CAR_GUN_HEIGHT, vehicle->gun1);
		}
	}
}

/* Shoots a bullet out of one of the vehicles guns */
void fire_weapon(gun_select c_gun, bullet* ammo, u16 max_bullets, car* vehicle) {

	//Loop through bullets, and determine if one is available to be fired.
	for (int i = 0; i < max_bullets; i++) {
		if (!ammo[i].fired) {

			//Set the bullet's initial location at the tip of the vehicle's gun.
			ammo[i].row = vehicle->row+vehicle->height/4;
			ammo[i].f_gun = c_gun;

			if (c_gun == GUNLEFT) {
				ammo[i].col = vehicle->col-CAR_GUN_WIDTH;
			} else {
				ammo[i].col = vehicle->col+vehicle->width+CAR_GUN_WIDTH;
			}

			//Fire away!
			ammo[i].fired = true;
			break;
		}
	}
}

/* Updates all of the game's fired bullets */
void update_bullets(bullet* player_bullets, bullet* enemy_bullets, car* player, car* vehicles) {
	int p_row, p_col;

	//Loop through player bullets.
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		if (player_bullets[i].fired) {

			//Move bullet, keeping track of its previous location.
			p_row = player_bullets[i].row;
			player_bullets[i].row--;

			p_col = player_bullets[i].col;
			if (player_bullets[i].f_gun == GUNLEFT) {
				player_bullets[i].col--;
			} else {
				player_bullets[i].col++;
			}

			//Check boundary and draw bullet in its new location
			if (player_bullets[i].row >= 0 && player_bullets[i].row <= SCREEN_HEIGHT-player_bullets[i].height && player_bullets[i].col >= SCREEN_WIDTH/4 && player_bullets[i].col < 3*SCREEN_WIDTH/4-player_bullets[i].width) {
				dma_drawrect3(p_row, p_col, player_bullets[i].width, player_bullets[i].height, road_color);
				dma_drawrect3(player_bullets[i].row, player_bullets[i].col, player_bullets[i].width, player_bullets[i].height, player_bullets[i].color);
			} else {

				//Reset bullet if it goes out of bounds.
				dma_drawrect3(p_row, p_col, player_bullets[i].width, player_bullets[i].height, road_color);
				player_bullets[i].fired = false;
			}

			//Check for bullet-vehicle collisions.
			for (int j = 0; j < MAX_VEHICLES; j++) {

				//Update game if collision occurs.
				if (player_bullets[i].row >= vehicles[j].row && player_bullets[i].row <= vehicles[j].row+vehicles[j].height-1 &&
					player_bullets[i].col >= vehicles[j].col && player_bullets[i].col <= vehicles[j].col+vehicles[j].width-1) {

					//If a spy is hit, kill the spy and increment score.
					if (vehicles[j].type == SPY_C) {
						vehicles[j].width = EXPLOSION_WIDTH;
						vehicles[j].height = EXPLOSION_HEIGHT;
						vehicles[j].bitmap = explosion;
						vehicles[j].gun1 = NULL_GUN;
						vehicles[j].gun2 = NULL_GUN;
						vehicles[j].type = EXPLODED_C;
						player_score += 100;

					//If a civilian is hit, kill the civilian and decrement score.
					} else if (vehicles[j].type == CIVILIAN_C) {
						vehicles[j].width = EXPLOSION_WIDTH;
						vehicles[j].height = EXPLOSION_HEIGHT;
						vehicles[j].bitmap = explosion;
						vehicles[j].type = EXPLODED_C;
						player_lives--;

						if (player_score >= 200) {
							player_score -= 200;
						} else {
							player_score = 0;
						}
					}

					//Reset bullet.
					player_bullets[i].fired = false;
				}
			}
		}
	}

	//Do the same exact thing as above but with enemy bullets.
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
		if (enemy_bullets[i].fired) {
			p_row = enemy_bullets[i].row;
			enemy_bullets[i].row += 2;

			p_col = enemy_bullets[i].col;
			if (enemy_bullets[i].f_gun == GUNLEFT) {
				enemy_bullets[i].col--;
			} else {
				enemy_bullets[i].col++;
			}

			if (enemy_bullets[i].row >= 0 && enemy_bullets[i].row <= SCREEN_HEIGHT-enemy_bullets[i].height && enemy_bullets[i].col >= SCREEN_WIDTH/4 && enemy_bullets[i].col < 3*SCREEN_WIDTH/4-enemy_bullets[i].width) {
				dma_drawrect3(p_row, p_col, enemy_bullets[i].width, enemy_bullets[i].height, road_color);
				dma_drawrect3(enemy_bullets[i].row, enemy_bullets[i].col, enemy_bullets[i].width, enemy_bullets[i].height, enemy_bullets[i].color);
			} else {
				dma_drawrect3(p_row, p_col, enemy_bullets[i].width, enemy_bullets[i].height, road_color);
				enemy_bullets[i].fired = false;
			}

			//Kill player if hit. Score will be decremented when player goes off screen.
			if (enemy_bullets[i].row >= player->row && enemy_bullets[i].row <= player->row+player->height-1 &&
				enemy_bullets[i].col >= player->col && enemy_bullets[i].col <= player->col+player->width-1) {
					player->width = EXPLOSION_WIDTH;
					player->height = EXPLOSION_HEIGHT;
					player->bitmap = explosion;
					player->gun1 = NULL_GUN;
					player->gun2 = NULL_GUN;
					player->type = EXPLODED_C;
			}

			//Check for civilians hit by enemy bullets.
			for (int j = 0; j < MAX_VEHICLES; j++) {
				if (enemy_bullets[i].row >= vehicles[j].row && enemy_bullets[i].row <= vehicles[j].row+vehicles[j].height-1 &&
					enemy_bullets[i].col >= vehicles[j].col && enemy_bullets[i].col <= vehicles[j].col+vehicles[j].width-1) {
					if (vehicles[j].type == SPY_C) {
						vehicles[j].width = EXPLOSION_WIDTH;
						vehicles[j].height = EXPLOSION_HEIGHT;
						vehicles[j].bitmap = explosion;
						vehicles[j].gun1 = NULL_GUN;
						vehicles[j].gun2 = NULL_GUN;
						vehicles[j].type = EXPLODED_C;
					} else if (vehicles[j].type == CIVILIAN_C) {
						vehicles[j].width = EXPLOSION_WIDTH;
						vehicles[j].height = EXPLOSION_HEIGHT;
						vehicles[j].bitmap = explosion;
						vehicles[j].type = EXPLODED_C;
						player_lives--;
					}

					enemy_bullets[i].fired = false;
				}
			}
		}
	}
}

/* Detects any combination of vehicle collisions */
void detect_collisions(car* player, car* vehicles) {
	bool collision_A, collision_B, collision_C, collision_D;

	//Check vehicle-vehicle collisions.
	for (int i = 0; i < MAX_VEHICLES; i++) {
		for (int j = 0; j < MAX_VEHICLES; j++) {
			if (i != j) {

				//Use a drawing to prove that these collision cases are correct.
				collision_A = vehicles[i].row+vehicles[i].height-1 >= vehicles[j].row && vehicles[i].row+vehicles[i].height-1 < vehicles[j].row+vehicles[j].height && vehicles[i].col+vehicles[i].width-1 >= vehicles[j].col && vehicles[i].col+vehicles[i].width < vehicles[j].col+vehicles[j].width;

				collision_B = vehicles[i].row+vehicles[i].height-1 >= vehicles[j].row && vehicles[i].row+vehicles[i].height-1 < vehicles[j].row+vehicles[j].height && vehicles[i].col >= vehicles[j].col && vehicles[i].col < vehicles[j].col+vehicles[j].width;

				collision_C = vehicles[j].row+vehicles[j].height-1 >= vehicles[i].row && vehicles[j].row+vehicles[j].height-1 < vehicles[i].row+vehicles[i].height && vehicles[j].col+vehicles[j].width-1 >= vehicles[i].col && vehicles[j].col+vehicles[j].width < vehicles[i].col+vehicles[i].width;

				collision_D = vehicles[j].row+vehicles[j].height-1 >= vehicles[i].row && vehicles[j].row+vehicles[j].height-1 < vehicles[i].row+vehicles[i].height && vehicles[j].col >= vehicles[i].col && vehicles[j].col < vehicles[i].col+vehicles[i].width;

				//Blow collided vehicles up!
				if ((collision_A || collision_B || collision_C || collision_D) && vehicles[i].type != EXPLODED_C && vehicles[j].type != EXPLODED_C) {
					vehicles[i].width = EXPLOSION_WIDTH;
					vehicles[i].height = EXPLOSION_HEIGHT;
					vehicles[i].bitmap = explosion;
					vehicles[i].gun1 = NULL_GUN;
					vehicles[i].gun2 = NULL_GUN;
					vehicles[i].type = EXPLODED_C;

					vehicles[j].width = EXPLOSION_WIDTH;
					vehicles[j].height = EXPLOSION_HEIGHT;
					vehicles[j].bitmap = explosion;
					vehicles[j].gun1 = NULL_GUN;
					vehicles[j].gun2 = NULL_GUN;
					vehicles[j].type = EXPLODED_C;
				}
			}
		}

		//Now, check for collisions involving the player.

		collision_A = vehicles[i].row+vehicles[i].height-1 >= player->row && vehicles[i].row+vehicles[i].height-1 < player->row+player->height && vehicles[i].col+vehicles[i].width-1 >= player->col && vehicles[i].col+vehicles[i].width < player->col+player->width;

		collision_B = vehicles[i].row+vehicles[i].height-1 >= player->row && vehicles[i].row+vehicles[i].height-1 < player->row+player->height && vehicles[i].col >= player->col && vehicles[i].col < player->col+player->width;

		collision_C = player->row+player->height-1 >= vehicles[i].row && player->row+player->height-1 < vehicles[i].row+vehicles[i].height && player->col+player->width-1 >= vehicles[i].col && player->col+player->width < vehicles[i].col+vehicles[i].width;

		collision_D = player->row+player->height-1 >= vehicles[i].row && player->row+player->height-1 < vehicles[i].row+vehicles[i].height && player->col >= vehicles[i].col && player->col < vehicles[i].col+vehicles[i].width;

		//Blow up the player and the vehicle it hit. After the player has exploded, do not keep decrementing his lives.

		if ((collision_A || collision_B || collision_C || collision_D) && player->type == PLAYER_C) {
			vehicles[i].width = EXPLOSION_WIDTH;
			vehicles[i].height = EXPLOSION_HEIGHT;
			vehicles[i].bitmap = explosion;
			vehicles[i].gun1 = NULL_GUN;
			vehicles[i].gun2 = NULL_GUN;
			vehicles[i].type = EXPLODED_C;

			player->width = EXPLOSION_WIDTH;
			player->height = EXPLOSION_HEIGHT;
			player->bitmap = explosion;
			player->gun1 = NULL_GUN;
			player->gun2 = NULL_GUN;
			player->type = EXPLODED_C;
		}
	}
}

/* FINALLY! End the game upon losing or winning. */
void end_game(end_type e, car* player, car* enemies, bullet* player_bullets, bullet* enemy_bullets) {
	if (e == LOSE) {
		dma_drawimage3(0, 0, GAME_OVER_WIDTH, GAME_OVER_HEIGHT, game_over);
	} else {
		drawString3(SCREEN_HEIGHT/2, SCREEN_WIDTH/2, "YOU WIN!!!", WHITE, road_color);
	}

	//Present player with play again prompt.

	volatile int k;
	while (!KEY_PRESSED(BUTTON_START)) {
		drawString3(3*SCREEN_HEIGHT/4, SCREEN_WIDTH/4-20, "PRESS START TO PLAY AGAIN", WHITE, BLACK);
		for (k = 0; k < 100000; k++) {
			if (KEY_PRESSED(BUTTON_START)) break;
		}

		drawRect3(3*SCREEN_HEIGHT/4, SCREEN_WIDTH/4-20, 160, 8, BLACK);
		for (k = 0; k < 100000; k++) {
			if (KEY_PRESSED(BUTTON_START)) break;
		}
	}

	//Re-initialize game.
	init_game(player, enemies, player_bullets, enemy_bullets);
}

/* Sets up all initial game parameters */
void init_game(car* player, car* vehicles, bullet* player_bullets, bullet* enemy_bullets) {

	//Initial player stats
	player_lives = INIT_LIVES;
	player_score = INIT_SCORE;

	//Draw game background
	dma_drawimage3(0, 0, BACKGROUND_WIDTH, BACKGROUND_HEIGHT, background);
	dma_drawrect3(0, SCREEN_WIDTH/4, SCREEN_WIDTH/2, SCREEN_HEIGHT, road_color);
	dma_drawrect3(0, SCREEN_WIDTH/4-10, 10, SCREEN_HEIGHT, line_color);
	dma_drawrect3(0, 3*SCREEN_WIDTH/4, 10, SCREEN_HEIGHT, line_color);

	//Draw the little lines on the road barrier
	int lrow = 0, lcol = SCREEN_WIDTH/4-1;
	while (lcol > SCREEN_WIDTH/4-11 && lrow < SCREEN_HEIGHT) {
		setPixel3(lrow, lcol, BLACK);
		lrow++;
		if (!(--lcol > SCREEN_WIDTH/4-11)) lcol = SCREEN_WIDTH/4-1;
	}

	lrow = 0; lcol = 3*SCREEN_WIDTH/4+9;
	while (lcol > 3*SCREEN_WIDTH/4-1 && lrow < SCREEN_HEIGHT) {
		setPixel3(lrow, lcol, BLACK);
		lrow++;
		if (!(--lcol > 3*SCREEN_WIDTH/4-1)) lcol = 3*SCREEN_WIDTH/4+9;
	}

	//Put player in initial position.
	player->row = SCREEN_HEIGHT/4;
	player->col = SCREEN_WIDTH/2+20;
	player->cur_dir = UP;
	player->width = PLAYER_CAR_WIDTH;
	player->height = PLAYER_CAR_HEIGHT;
	player->bitmap = player_car;
	player->gun1 = guns[0];
	player->gun2 = guns[1];
	player->type = PLAYER_C;

	//Put pseudo-vehicles "off-screen". After pseudo-vehicle reaches bottom of screen it will get drawn as a real vehicle.
	for (int i = 0; i < MAX_VEHICLES; i++) {
		vehicles[i].height = 0;
		vehicles[i].row = -PLAYER_CAR_HEIGHT*(i+1);
		if (i > 0) {
			vehicles[i].row -= 10*i;
		}
	}

	//Initialize weaponry.
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		player_bullets[i].width = 2;
		player_bullets[i].height = 2;
		player_bullets[i].fired = false;
		player_bullets[i].color = BLACK;
	}
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
		enemy_bullets[i].width = 3;
		enemy_bullets[i].height = 3;
		enemy_bullets[i].fired = false;
		enemy_bullets[i].color = RED;
	}
}









