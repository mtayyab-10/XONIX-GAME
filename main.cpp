#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Audio.hpp> 
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <sstream> 

using namespace sf;
using namespace std;

// idhar textures aur sprites hain
Texture t1, t3;
Sprite sTile, sEnemy;
Font font;

Texture backgroundTexture;//idhar hum load krein gay an image file for the background.
Sprite backgroundSprite;//that will display background  texture.

//here we have sound buffers and sounds
SoundBuffer captureBuffer;// holds the actual audio data (.wav files).
Sound captureSound;//plays the audio loaded into a Sound buffer.
SoundBuffer bonusBuffer;
Sound bonusSound;
SoundBuffer powerupGainBuffer; //for earning a power up
Sound powerupGainSound;
SoundBuffer powerupActivateBuffer; //for activating a power up, BONUS FEATURSS
Sound powerupActivateSound; 

Music backgroundMusic;

const int M = 35; // grid ki height
const int N = 60; // width
int ts = 18; //tile ka size

const int max_enemies = 16; 
const int max_protected = max_enemies + 2; //max enemies + 2 potential players

//the following is a static array, represents the M x N grid flattened into a 1D array.
int grid_1d[M * N]; 
const int claim_p1 = 4;
const int claim_p2 = 5;

const int tile_empty = 0;
const int tile_border = 1;
const int tile_trail_p1 = 2;
const int tile_trail_p2 = 3;
const int tile_flood_visited = -1;
const int tile_temp_protected = -2;
const int tile_out_of_bounds = -3;

const int linear_pattern = 0;
const int zigzag_pattern = 1; 
const int circular_pattern = 2;

bool patterns_activated = false;// used for the other Geometric patterns as aquired

  // Ye aik helper function grid ko access krnay k lea
int get_gridvalue(int r, int c) {
	if (r >= 0 && r < M && c >= 0 && c < N)
		return grid_1d[r * N + c];
	return tile_out_of_bounds; //return constant
}

void setgridvalue(int r, int c, int v) {
	if (r >= 0 && r < M && c >= 0 && c < N)
		grid_1d[r * N + c] = v;
}

const int state_main_menu = 0;
const int state_select_mode = 1;
const int state_select_level = 2;
const int state_playing_single = 3;
const int state_playing_two = 4;
const int state_show_scoreboard = 5;
const int state_gameover_single = 6;
const int state_gameover_two = 7;

// for next show
const int difficulty_easy = 0;
const int difficulty_medium = 1;
const int difficulty_hard = 2;
const int difficulty_continuous = 3;

int current_state = state_main_menu;       
int currentdifficulty = difficulty_medium;   
int numplayers = 1;                           // default to single player
bool gameactive = false;                    // controls if the main game logic should update
Clock gameclock;                         // to measures game time
float totalTime = 0.f;
float time_sincelastenemyadd = 0.f;          // this will use by us for continuous mode
float time_sinceSpeedup = 0.f;     // for enemy speed scaling

// Player 1 k var
int x = 1, y = 1, dx = 0, dy = 0; //to start p1 inside top left border
int score1 = 0;
int powerups1 = 0;
int moves1 = 0;
bool isplayer1alive = true;
bool isplayer1building = false;
int bonus_counter1 = 0;
int bonus_threshold1 = 10;
int bonus_multiplier1 = 2;
int nextpowerup_score1 = 50;  //for the powerup, when player reaches the score of 50.( here he gain powerup and and stops enemies for 3 sec)

int lastdx1 = 0, lastdy1 = 1; //here we have initial last move to allow moving down/right

// Player 2 k variables
int x2 = N - 2, y2 = 1, dx2 = 0, dy2 = 0; // to start P2 inside top-right border
int score2 = 0;
int powerups2 = 0;
int moves2 = 0;
bool isplayer2alive = true; //initialized assuming potential two-player start
bool isplayer2building = false;
int bonus_counter2 = 0;
int bonus_threshold2 = 10;
int bonus_multiplier2 = 2;
int nextpowerup_score2 = 50;
int lastdx2 = 0, lastdy2 = 1; //this is initial last move for P2

struct Enemy
{
	float x, y; //using float (instead of int) means the enemy can move in increments smaller than one pixel, which looks smoother.
	float dx, dy; // these are velocity components(the enemy’s speed and direction)

	float base_speed = 1.5f; // base speed magnitude (pixels per FRAME ========60fps equivalent)
	float current_speedfactor = 1.0f; //this is speed multiplier, can increase over time (like difficulty rising up).

	int movement_pattern = linear_pattern; //so enemies initially move in a straight, bouncing style.

	
	// here we have shared timer for pattern segment duration	
	float patterntimer = 0.0f; // it tracks time for changing direction in patterns
	
	
//  it is increased every frame using patterntimer += deltatime
//onCe it crosses pattern_segmentduration, the pattern (like zigzag) changes direction (horizontal to vertical or vice versa).
	
	
	static const float pattern_segmentduration; //how long each segment lasts (seconds)
	
	
// this sets how long one segment of a movement pattern should last.if pattern_segmentduration = 2.0f;, then zigzag direction changes every 2 seconds.
	

	//zig-Zag state
	bool zigzagishorizontal = true; //If true move horizontally (dx ≠ 0, dy = 0), If false, then move vertically(187)

	float current_angle_rad = 0.0f; //Current movement angle in radians. and the angle increases constantly
	float turn_speed_rad_per_sec = 0.5f; //  how fast the enemy turns (radians per second), turning speed for circular movement
	
	//here we have constructor initialization list
	Enemy() : x(0), y(0), dx(0), dy(0),
	current_speedfactor(1.0f),

//runs automatically when a new enemy is created.It sets the starting position, direction, and movement type of the enemy.for example,when the game calls Enemy e1, this constructor code runs behind the scenes.

	
	//MOVEMENT MODES (these are 3)
	
	movement_pattern(linear_pattern) //explicitly start linear
	{
		// placing enemy randomly in the central empty area
		int gridX, gridY;
		do {
			// Now, to ensure that, the valid random range within the inner grid
			x = (rand() % (N - 4) + 2) * ts;//multiplies by ts (tile size) to get pixel position on screen.
			y = (rand() % (M - 4) + 2) * ts;
			gridX = static_cast<int>(x / ts); 
			gridY = static_cast<int>(y / ts); 
		} while (gridX <= 0 || gridX >= N-1 || gridY <= 0 || gridY >= M-1 || get_gridvalue(gridY, gridX) != 0);
		//here i make ensure, starting on valid empty tile(random) excluding borders
		//Only accepts the position if the tile is empty (value 0 from get_gridvalue())


//This function sets dx and dy to a random direction (using angle).
		resetdirection(); //initialize dx/dy for linear start
		
	        current_angle_rad = atan2(dy, dx); //prepares angle for circular pattern, atan2(dy, dx) gives angle in radians.
	}

	void initializeSimpleZigZag() {
		movement_pattern = zigzag_pattern;
		patterntimer = 0.0f; //reset timer
		zigzagishorizontal = (rand() % 2 == 0); // start randomly horizontal or vertical, as this line will give 0 OR 1
		float targetspeed = base_speed * current_speedfactor;
		//it is calculates how fast the enemy should move
		
		if (zigzagishorizontal) {
			dx = targetspeed;//moving left right
			dy = 0;
		} 
		else {
			dx = 0;
			dy = targetspeed;//moving updown
		}
	}
	void initialize_circularpattern() {
		movement_pattern = circular_pattern; //set pattern type
		//randomize turn direction
		if (rand() % 2 == 0) {//left rotation
			turn_speed_rad_per_sec = abs(turn_speed_rad_per_sec);
		} else {//right wali
			turn_speed_rad_per_sec = -abs(turn_speed_rad_per_sec);
		}

		cout << "Enemy at (" << x/ts << "," << y/ts << ") switching to circular pattern." << endl;// by simply putting x/ts, we can get exact point
	}

	void update_speedcomponents() {
		float currentmagnitude = sqrt(dx * dx + dy * dy);
		float targetspeed = base_speed * current_speedfactor;

		if (currentmagnitude > 0.01f) { // avoid division by zero/tiny numbers
		     dx = (dx / currentmagnitude) * targetspeed;
		     dy = (dy / currentmagnitude) * targetspeed;//here recomputing dx,dy so their combined length always equals base_speed * current_speedfactor.
		} 
		else {
		     // idhar, agar magnitude is near to zero, give it a new random direction to it
		    //(0nly for linear, patterns handle their own zero-speed cases)
		     if (movement_pattern == linear_pattern){
		         resetdirection();
		     }
		}
	}

	void resetdirection() {
		// ab random angle generate krana
		float random_angle = (rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
//rand() / RAND_MAX hamein random number dei ga  0 and 1
//multipliyiing by 2π (6.28) will give us a full-circle angle in radians.
		
		float targetspeed = base_speed * current_speedfactor;
		
		//dx and dy are calculated from angle:
		dx = cos(random_angle) * targetspeed;// by use of cmath library
		dy = sin(random_angle) * targetspeed;

		//just a safety check, by removing it some enemies might freeze randomly on screen
		if (abs(dx) < 0.01f && abs(dy) < 0.01f) {
			//now to make sure that the object doesn't freeze if movement speed accidentally becomes zero
			dx = targetspeed; //if such happens, enemy moves rightward by default
			dy = 0;
		}
	}

	//This function handles the core movement and bouncing behavior of enemies when they are in the linear pattern mode
	void movelinear(float deltatime){
	
		// Convert to “60fps units”
		float scaled_deltatime = deltatime * 60.f;
		float nextX = x + dx * scaled_deltatime;//that literally moves the enemy by its velocity each frame.
		float nextY = y + dy * scaled_deltatime;

		int current_tileX = x / ts; // here i am implicitly yaani indirectly converting the ans from float to int
		int current_tileY = y / ts; 
		int next_tileX    = nextX / ts; 
		int next_tileY    = nextY / ts; 

		bool bounced_x = false; 
		bool bounced_y = false;

		// X-axis collision== walls (1), claimed blocks, ya phir out-of-bounds (-3)
		{
			int v = get_gridvalue(current_tileY, next_tileX);
			//here just using defined constants including original claim_p1 and claim_p2
			if (v == tile_border || v == claim_p1 || v == claim_p2 || v == tile_out_of_bounds) {
				dx = -dx; //reverse horizontal direction
				nextX = x;   //  keep x the same on X bounce(cancel movement)
				bounced_x = true;
			}
			else{
				x = nextX; //(proceed)only update x if no X bounce
			}
		}

		// Y-axis collision======= walls, claimed blocks, or out-of-bounds
		{
			//recalculating current X tile after potential X move/bounce
			current_tileX = static_cast<int>(x / ts);
			int v = get_gridvalue(next_tileY, current_tileX);
			if (v == tile_border || v == claim_p1 || v == claim_p2 || v == tile_out_of_bounds) {
				dy = -dy; // reversing the vertical direction
				nextY = y; //(NO M0Vement)keeping y the same on Y bounce
				bounced_y = true;
			}
			else {
				y = nextY; //Proceed
			}
		}

		// boundary clamp, If the enemy goes outside the visible screen, force it back inside and reverse its direction.
		if (x < ts) {
			x = (float)ts;
			if (!bounced_x) dx = abs(dx); // this will help use to avoid double-reversing if already bounced by wall logic
		}
		if (x >= (N - 1) * ts) {
			x = (float)((N - 1) * ts - 1);
			if (!bounced_x) dx = -abs(dx);
		}
		if (y < ts) {
			y = (float)ts;
			if (!bounced_y) dy = abs(dy);
		}
		if (y >= (M - 1) * ts) {
			y = (float)((M - 1) * ts - 1);
			if (!bounced_y) dy = -abs(dy);
		}

		//if, for some reason, the enemy is already inside a solid block, it gets pushed back out.
		{
			int finalTileX = static_cast<int>(x / ts); 
			int finalTileY = static_cast<int>(y / ts); 
			
			int v2 = get_gridvalue(finalTileY, finalTileX);

			if (v2 == tile_border || v2 == claim_p1 || v2 == claim_p2) {
				//move backwards along velocity vector
				x -= dx * scaled_deltatime * 1.5f;
				y -= dy * scaled_deltatime * 1.5f;

				// re clamp in case we went off‐screen
				if (x < ts)                     
					x = (float)ts;
				if (x >= (N - 1) * ts)          
					x = (float)((N - 1) * ts - 1);
				if (y < ts)                     
					y = (float)ts;
				if (y >= (M - 1) * ts)          
					y= (float)((M - 1) * ts - 1);
			}
		}
	}


    //geometric pattern movement k functions , pehle zigzag hai

       //switches between horizontal and vertical movement based on a timer.
    // it relies on movelinear to handle bounces within each segment.
	void movesimple_zigzag(float deltatime) {

//every frame, we add time to the timer .when the timer exceeds pattern_segmentduration, it means time to flip direction

		patterntimer += deltatime;
		float targetspeed = base_speed * current_speedfactor;

		if (patterntimer >= pattern_segmentduration) { 
			patterntimer = 0.0f;
			zigzagishorizontal = !zigzagishorizontal; //this line toggles the boolean variable, 
			// Every few seconds (e.g. 2 seconds), the enemy switches its direction
			
			//if previously moving left right, now switch to up down
			//  if switching to horizontal
			if (zigzagishorizontal) {
				dx = (abs(dx) > 0.1f && dy == 0) ? dx : targetspeed;
				//abs() is used to check the current speed in a direction, regardless of whether it’s positive or negative.
				
				dy = 0;// stop vertical movement
			
			} else {               //If switching to vertical
				dx = 0;           // stop horizontal movement
				dy = (abs(dy) > 0.1f && dx == 0) ? dy : targetspeed;
			}
			if (abs(dx) < 0.01f && abs(dy) < 0.01f) { //failsafe
				if(zigzagishorizontal) 
					dx = targetspeed; 
				else 
					dy = targetspeed;
			} //ensures the enemy is never stuck with 0 movement.
		}
		update_speedcomponents(); // idhar ensure kr raha correct speed magnitude
		movelinear(deltatime);    //using standard collision/movement
	}
//it's called by the main move function	

	//now comes 2nd GEOmetric pattern (whic we have circular pattern)
	//moves right, phir down, left, and then up, each for a set duration.
	// relies on movelinear to handle bounces
	
	void move_circularpattern(float deltatime) {
		// 1.update the angle
		current_angle_rad += turn_speed_rad_per_sec * deltatime;

		// 2# keep angle within 0 to 2*PI
		float two_pi = 2.0f * 3.14159265f;
		current_angle_rad = fmod(current_angle_rad, two_pi);//over time, angle becomes very large can cause glitches
		
		if (current_angle_rad < 0) {
			current_angle_rad += two_pi;
		}

		// 3# calculate dx and dy based on the new angle and desired speed
		float target_speed = base_speed * current_speedfactor;
		dx = cos(current_angle_rad) * target_speed;// for the CIRCULAR movemen, without these no circle
		dy = sin(current_angle_rad) * target_speed;

		// 4) using the existing movelinear for collision and actual movement
		
		//even though the enemy moves in a circular pattern, it still needs to bounce off walls and stay in bounds(so we use below line)
		movelinear(deltatime);
	}
	
	//basic movement logic, applies speed and checks for wall bounce
	void move(float deltatime)
	{
		switch (movement_pattern) {
			case zigzag_pattern:
				movesimple_zigzag(deltatime);
				break;
			case circular_pattern:
			        move_circularpattern(deltatime); 		
				break;
			case linear_pattern:
				movelinear(deltatime); 
			default: // fallback to linear
			update_speedcomponents(); // ensure dx/dy match current speed factor for linear
			movelinear(deltatime);    // always use linear movement (for this case)
			break;
		}
	}
};

const float Enemy::pattern_segmentduration = 2.0f; //allocates storage for the one shared variable pattern_segmentduration, and initializes it to 2.0f.

//this line gives your zigzag timer its duration
Enemy enemies_ptr[max_enemies];//here i am creating a fixed size array of Enemy objects. we can have max_enemies (16) at once

int currentenemy_count = 0; //simply tracks the number of active enemies in the array, and incrementing each time we add an enemy.
int initialenemy_count = 4; // it is being determined by difficulty, variies with the level 

// just for powerup state 
bool ispowerup_active = false;
float poweruptimer = 0.f;
const float powerup_duration = 3.0f;
int playerusingpower = 0; // 1 or 2

//iss trah jab bhi player power up key press kray ga, then ispowerup_active = true and playerusingpower = thatPlayer.

//menu k variables,  each one has its(font, size, color, position) in initializetext().
int menu_choice = 0;
Text menutext, scoretext1, scoretext2, timetext, movestext1, movestext2, poweruptext1, poweruptext2, gameovertext, finalscoretext, prompt_text, winnertext;

//ye hain variables of scoreboard 
const int sizeof_scoreboard = 5;//only we have to show(1 to 5), so size is accordingly
int highScores_scores[sizeof_scoreboard]; // matlab k highScores_scores[5]
float highScores_times[sizeof_scoreboard];
string scorefile_name = "score.txt";

void resetgame(bool setuptwoplayer);
void loadscoreboard();
void save_scoreboard();
void updatescoreboard(int score, float time);
void drop(int y, int x); // flood fill
void handleinput(RenderWindow &window);
void updategame(float deltatime);
void rendergame(RenderWindow &window);
 void draw_mainmenu(RenderWindow &window);
 
void select_Modemenu(RenderWindow &window);
void select_levelmenu(RenderWindow &window);
void drawscoreboard_display(RenderWindow &window);
void make_gameover_menu(RenderWindow &window);
string format_time(float timeSeconds);
void initializetext();
void initialize_gamegrid();
void completefill(int playerid, int startTileX, int startTileY);
void drawCenteredtext(RenderWindow &window, Text &text, float posY);

void initialize_gamegrid() {
	// grid_1d is a static global array
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < N; j++) {
			if (i == 0 || j == 0 || i == M - 1 || j == N - 1) {
				setgridvalue(i, j, 1); // borders are claimed
			} 
			else {
				setgridvalue(i, j, 0); //inner area is empty
			}
		}
	}
}

//used in completefill() function when a player closes a loop to claim an area. it helps track how much area a player has captured

void drop(int y, int x)
{
	if (get_gridvalue(y, x) != 0) {// we only have to fill empty tiles
		return;
	}
	setgridvalue(y, x, -1);// this is to mark the tile as visited
	drop(y - 1, x);//up
	drop(y + 1, x);//down
	drop(y, x - 1);//left
	drop(y, x + 1);//right
}

void loadscoreboard() {
	for (int i = 0; i < sizeof_scoreboard; ++i) {
		highScores_scores[i] = 0;
		highScores_times[i] = 0.0f;
	}
	ifstream file(scorefile_name);//this line opens the file score.txt for reading.


	if (file.is_open()) { 
	// it reads and loads top 5 scores and times from the file
		for (int i = 0; i < sizeof_scoreboard; ++i) {
			if (!(file >> highScores_scores[i] >> highScores_times[i])) {
				break;
			}
		}
		file.close();
	} 
	else {
		cout << "we don't find the file score.txt" << endl;
	}
}

void save_scoreboard() {// it saves the current high scores and times into the file score.txt
	
	ofstream file(scorefile_name);//this opens score.txt for writing.

	if (file.is_open()) {//It saves updated scores into the file
		for (int i = 0; i < sizeof_scoreboard; ++i) {
			file << highScores_scores[i] << " " << fixed << setprecision(2) << highScores_times[i] << endl;
		}
		file.close();
	} else {
		cout << "$0rry, i can't save scoreboard to " << scorefile_name << endl;
	}
}

void updatescoreboard(int score, float time) {
	int insertPos = -1;
	for (int i = 0; i < sizeof_scoreboard; ++i) {
		if (score > highScores_scores[i] || (score == highScores_scores[i] && time < highScores_times[i])) {
		//If the new score is higher (or same score but less time)
			insertPos = i;
			break;
		}
	}
	if (insertPos != -1) {
		for (int i = sizeof_scoreboard - 1; i > insertPos; --i) {
			highScores_scores[i] = highScores_scores[i - 1];
			highScores_times[i] = highScores_times[i - 1];
		}
		highScores_scores[insertPos] = score;
		highScores_times[insertPos] = time;
		save_scoreboard();
	}
}
string format_time(float timeSeconds) {
	int minutes = static_cast<int>(timeSeconds) / 60; // explicitly converting to intEGer
	int seconds =   static_cast<int>(timeSeconds) % 60; 
	stringstream ss;
	ss << setfill('0') << setw(2) << minutes << ":" << setfill('0') << setw(2) << seconds;// for this we have use cmath library
	return ss.str();
}
void initializetext() {// Ye sara ftn adjust krta hai fonts and display ko (for different levels and displays)
	if (!font.getInfo().family.empty()) {
	//If font.getInfo().family.empty() returns true, then all further text settings are skipped.
	
	//for the mainmenu
	menutext.setFont(font);
	menutext.setCharacterSize(24);
	menutext.setFillColor(Color::White);

	//these control the vertical placement of score, moves, powerup text.
	//M * ts means place it just below the grid (as M is the number of rows and ts is tile size).
	const int UI_y_start = M * ts + 10;
	const int UI_y_spacing = 25;

	// 1st player k score
	scoretext1.setFont(font); 
	scoretext1.setCharacterSize(18); 
	scoretext1.setFillColor(Color::Cyan); 
	scoretext1.setPosition(10, UI_y_start);
	
	//1st player kai moves   
	movestext1.setFont(font); 
	movestext1.setCharacterSize(16); 
	movestext1.setFillColor(Color::Cyan); 
	 movestext1.setPosition(10, UI_y_start + UI_y_spacing);
	
	// pehle player ki power up as demanded in question      
	poweruptext1.setFont(font); 
	poweruptext1.setCharacterSize(16); 

	poweruptext1.setFillColor(Color::Green); 
	poweruptext1.setPosition(10, UI_y_start + 2 * UI_y_spacing);

	timetext.setFont(font); 
	timetext.setCharacterSize(20); 
	
	timetext.setFillColor(Color::Yellow);
	
	// 2nd player t     
	scoretext2.setFont(font);     	// 2nd player kay score ka font     
	scoretext2.setCharacterSize(18); 
	scoretext2.setFillColor(Color::Magenta); 
	scoretext2.setPosition(N * ts - 200, UI_y_start);
	       
	movestext2.setFont(font);      // second player k moves  
	movestext2.setCharacterSize(16); 
        movestext2.setFillColor(Color::Magenta); 
	movestext2.setPosition(N * ts - 200, UI_y_start + UI_y_spacing);

	poweruptext2.setFont(font);     // uss ki power ups bhi aa gaien idhar      
	poweruptext2.setCharacterSize(16); 
	poweruptext2.setFillColor(Color::Green); 

	poweruptext2.setPosition(N * ts - 200, UI_y_start + 2 * UI_y_spacing);

        gameovertext.setFont(font); 
	gameovertext.setCharacterSize(50); // gameover ka style
	gameovertext.setFillColor(Color::Red); 
        gameovertext.setStyle(Text::Bold); 

	finalscoretext.setFont(font); 
	finalscoretext.setCharacterSize(30); // at the end, final score dekhana k lea

	finalscoretext.setFillColor(Color::White);

	prompt_text.setFont(font); 
	prompt_text.setCharacterSize(18); 

	prompt_text.setFillColor(Color::Yellow);

	winnertext.setFont(font); // winner wala text show krna at the end
        winnertext.setCharacterSize(35); 
	winnertext.setFillColor(Color::Yellow); 

	} 
}

void resetgame(bool setuptwoplayer) {
	numplayers = setuptwoplayer ? 2 : 1;
	totalTime = 0.f;
	time_sincelastenemyadd = 0.f;
	time_sinceSpeedup = 0.f;
	ispowerup_active = false;
	poweruptimer = 0.f;//agar hum is ko remove krdein, phir jab a powerup ends, it never reuse properly
	playerusingpower = 0;
	
	patterns_activated = false;

	initialize_gamegrid();

	switch (currentdifficulty) {// for each level initializing no. of enemies as per given
		case difficulty_easy:
			initialenemy_count = 2;
			break;
		case difficulty_medium:
			initialenemy_count = 4;
			break;
		case difficulty_hard:
			initialenemy_count = 6;
			break;
		case difficulty_continuous:
			initialenemy_count = 2;
			break;
		default:
			initialenemy_count = 4;
			break;
	}

	if (initialenemy_count > max_enemies) {   //for minimum  implementation
		initialenemy_count = max_enemies;
	}

	currentenemy_count = 0; //reset actual count before populating
	for (int i = 0; i < initialenemy_count; ++i) {
		if (currentenemy_count < max_enemies) { //ensure we don't exceed array bounds
			    enemies_ptr[currentenemy_count] = Enemy(); // creates a new enemy, defaults to linear_pattern
			    currentenemy_count++;
		}
	}

          // Player 1 k var
	x = 1; y = 1; dx = 0; dy = 0; //to start P1 inside top-left border
	score1 = 0;
	powerups1 = 0;
	moves1 = 0;
	isplayer1alive = true;
	isplayer1building = false;
	bonus_counter1 = 0;
	bonus_threshold1 = 10; //resetting to default, can be adjusted
	bonus_multiplier1 = 2; //  resetting to default
	nextpowerup_score1 = 50;
	lastdx1 = 0; lastdy1 = 1; //here we have initial last move to allow moving down/right

    // Player 2 k var
	if (setuptwoplayer) {
		x2 = N - 2; y2 = 1; dx2 = 0; dy2 = 0; // to start P2 inside top-right border
		score2 = 0;
		powerups2 = 0;
		moves2 = 0;
		isplayer2alive = true; //initialized assuming potential two-player start
		isplayer2building = false;
		
		bonus_counter2 = 0;
		bonus_threshold2 = 10; // idhar bhi same use hai to reset to the default
		bonus_multiplier2 = 2;
		nextpowerup_score2 = 50;
		lastdx2 = 0; lastdy2 = 1; //this is initial last move for P2
	} else {
		isplayer2alive = false; //marked as inactive for single player
		//still reset P2 variables for consistency if game mode changes later
		
		x2 = N - 2; y2 = 1; dx2 = 0; dy2 = 0;
		score2 = 0; powerups2 = 0; moves2 = 0;
		
		isplayer2building = false;
		bonus_counter2 = 0; 
		bonus_threshold2 = 10; 
		bonus_multiplier2 = 2;
		nextpowerup_score2 = 50;
		lastdx2 = 0; lastdy2 = 1;
	}

	gameactive = true;
	gameclock.restart();
}

int main(){

	srand(time(0));
	RenderWindow window(VideoMode(N * ts, M * ts + 100), "Our Gaming Studio");
	window.setFramerateLimit(60);
    
	if (!font.loadFromFile("Fonts/font2.ttf")) {
		cout << "Facing an error when loading font" <<endl;
	}
	
	if (!t1.loadFromFile("images/tiles.png")) { 
		cout << "There will have an error while loading tiles.png" << endl; 
	}
	
	if (!t3.loadFromFile("images/enemy.png")) { 
		cout << " An error comes when loading enemy.png" << endl; 
	}
	
	//loading sound effects
	if (!captureBuffer.loadFromFile("sounds/capture.wav")) {
		cout << "i have an error when loading sounds/capture.wav" << endl;
	}
	captureSound.setBuffer(captureBuffer);

	if (!bonusBuffer.loadFromFile("sounds/bonus.wav")) {
		cout << " i have an error when loading sounds/bonus.wav" << endl;
		//It plays if a player's capture (filledcount) is large enough to meet their current bonus_threshold, i.e. showing that bonus is active
	}
	bonusSound.setBuffer(bonusBuffer);		
	// Player 1 captures a large area (e.g., more than 10 tiles initially). The game calculates bonus points. If bonuspoints > 0, bonus.wav will play.
	
	
	if (!powerupGainBuffer.loadFromFile("sounds/powerup_gain.wav")) {
		cout << "Facing an error while loading sounds/powerup_gain.wav" << endl;
		//It plays when a player's score (score1 or score2) reaches or exceeds their next power-up threshold 
	}
	powerupGainSound.setBuffer(powerupGainBuffer);

	//Player 1's score is 45. They capture some tiles, and their score becomes 55. Since nextpowerup_score1 is initially 50, the condition score1 >= 	nextpowerup_score1 becomes true, powerups1 is incremented, and powerup_gain.wav plays. nextpowerup_score1 will then be updated for the next milestone.

	
	if (!powerupActivateBuffer.loadFromFile("sounds/powerup_activate.wav")) {
		cout << "Facing an error while loading sounds/powerup_activate.wav" << endl;
	}
	powerupActivateSound.setBuffer(powerupActivateBuffer);
	//player 1 has 1 power-up charge. They press 'P'. The powerup_activate.wav sound plays, and the game's freeze effect (stopping enemies) begins.

	
	if (!backgroundMusic.openFromFile("sounds/background_music.ogg")) { // Or .wav, .flac
		cout << "program has an error while loading sounds/background_music.ogg" << endl;
	} 
	else {
		backgroundMusic.setLoop(true);  // making the music loop
		backgroundMusic.setVolume(50);    // we can set volume (0-100), we can adjust it as needed
		backgroundMusic.play();       //start playing the music
	}
    
        if (!backgroundTexture.loadFromFile("images/background.jpg")) { 
        cout << "sHaving an error while loading  images/background.jpg" << endl;
	} 
	else {
		backgroundSprite.setTexture(backgroundTexture);
	//scaling the background to fit the window if its original size is different.This ensures it covers the entire window(N * ts wide, M *ts + 100 high)
		float scaleX = (float)(N * ts) / backgroundTexture.getSize().x;
		float scaleY = (float)(M * ts + 100) / backgroundTexture.getSize().y;
		backgroundSprite.setScale(scaleX, scaleY);
	}

	sTile.setTexture(t1);
	sEnemy.setTexture(t3);
	sEnemy.setOrigin(t3.getSize().x / 2.f, t3.getSize().y / 2.f);
	initializetext();
	loadscoreboard();
	// initialize the grid for the first time before potentially resetting
	initialize_gamegrid();
	Clock frameClock;
	current_state = state_main_menu;
	numplayers = 1;

	while (window.isOpen()){
		float deltatime = frameClock.restart().asSeconds();
		if (deltatime > 0.1f) deltatime = 0.1f;

		Event event;
		while (window.pollEvent(event)){
			if (event.type == Event::Closed) window.close();
			if (event.type == Event::KeyPressed) {
				
				switch (current_state) {// in this case i will see for the current state in the game
				
				case state_main_menu:// if the current state is the starting menu (the first one)
					if (event.key.code == Keyboard::Up) 
						menu_choice = (menu_choice + 2) % 3;
					else if (event.key.code == Keyboard::Down) 
						menu_choice = (menu_choice + 1) % 3;
					else if (event.key.code == Keyboard::Enter) {
						
						switch (menu_choice) {// after analyzing the menu option, now going for working respectively						
						case 0:
							current_state = state_select_mode;
							break;
						case 1:
							current_state = state_show_scoreboard;
							break;
						case 2:
							window.close();
							break;
						}
						menu_choice = 0;
						break;					
					} 
					else if (event.key.code == Keyboard::Escape) window.close();
				break;
				
				
				case state_select_mode:// if the current state is select mode wali
					if (event.key.code == Keyboard::Up || event.key.code == Keyboard::W) 
						menu_choice = (menu_choice + 1) % 2;
					else if (event.key.code == Keyboard::Down || event.key.code == Keyboard::S) 
						menu_choice = (menu_choice + 1) % 2;
					else if (event.key.code == Keyboard::Enter) {
						numplayers = (menu_choice == 0) ? 1 : 2;
						current_state = state_select_level;
						menu_choice = 0;
					} 
					else if (event.key.code == Keyboard::Escape) { 
						current_state = state_main_menu; 
						menu_choice = 0; 
					}
				break;
				 
				
				case state_select_level:// if the current state is of select level 
					if (event.key.code == Keyboard::Up || event.key.code == Keyboard::W) 
						menu_choice = (menu_choice + 3) % 4;
					else if (event.key.code == Keyboard::Down || event.key.code == Keyboard::S) 
						menu_choice = (menu_choice + 1) % 4;
					else if (event.key.code == Keyboard::Enter) {
						
						switch (menu_choice) {						
						case 0:
							currentdifficulty = difficulty_easy;
							break;
						case 1:
							currentdifficulty = difficulty_medium;
							break;
						case 2:
							currentdifficulty = difficulty_hard;
							break;
						case 3:
							currentdifficulty = difficulty_continuous;
							break;
						}
						resetgame(numplayers == 2);
						current_state = (numplayers == 1) ? state_playing_single : state_playing_two;
						break;
					} 
					else if (event.key.code == Keyboard::Escape) { 
						current_state = state_select_mode; 
						menu_choice = 0; 
					}
				break;
					
				
				case state_show_scoreboard:// if the current state is showing scoreboard
					if (event.key.code == Keyboard::Enter || event.key.code == Keyboard::Escape) { 
						current_state = state_main_menu; 
						menu_choice = 0; 
					}
				break;
					
				
				case state_gameover_single:// if the current state is gameover
				case state_gameover_two:
					if (event.key.code == Keyboard::Up || event.key.code == Keyboard::W) 
						menu_choice = (menu_choice + 2) % 3;
					else if (event.key.code == Keyboard::Down || event.key.code == Keyboard::S) 
						menu_choice = (menu_choice + 1) % 3;
					else if (event.key.code == Keyboard::Enter) {
						
						switch (menu_choice) {
						case 0:
							resetgame(numplayers == 2);
							current_state = (numplayers == 1) ? state_playing_single : state_playing_two;
							break;
						case 1:
							current_state = state_main_menu;
							menu_choice = 0;
							break;
						case 2:
							window.close();
							break;
						}
						break;
					} 
					else if (event.key.code == Keyboard::Escape) { 
						current_state = state_main_menu; 
						menu_choice = 0; 
					}
				break;
					
				
				case state_playing_single:
				case state_playing_two:
								
				// powerups are simply counted,they stack in powerups1
				// later,when the player activates one:
					
					if (event.key.code == Keyboard::P && powerups1 > 0 && !ispowerup_active) {
						ispowerup_active = true; 
						 poweruptimer = 0.f; 
						powerups1--; 
						playerusingpower = 1;
						        powerupActivateSound.play();

					}else if (numplayers == 2 && event.key.code == Keyboard::Q && isplayer2alive && powerups2 > 0 && !ispowerup_active) {
						ispowerup_active = true; 
						poweruptimer = 0.f; 
						powerups2--; 
						 playerusingpower = 2;
						powerupActivateSound.play();
					}else if (event.key.code == Keyboard::Escape) {
						gameactive = false; 
						current_state = state_main_menu; 
						menu_choice = 0;
					}
				break;
				}
			}
		}

		if ((current_state == state_playing_single || current_state == state_playing_two) && gameactive) {
			handleinput(window);
			updategame(deltatime);
		}

		window.clear(Color::Black);
		// the background color of our game, we can adjust it to any color (which are available only in SFML)like White, Magenta

		if (current_state == state_main_menu) 
			draw_mainmenu(window);

		else if (current_state == state_select_mode) 
			select_Modemenu(window);

		else if (current_state == state_select_level) 
			select_levelmenu(window);

		else if (current_state == state_show_scoreboard) 
			drawscoreboard_display(window);

		else if (current_state == state_playing_single || current_state == state_playing_two) 
			rendergame(window);

		else if (current_state == state_gameover_single || current_state == state_gameover_two) {

			rendergame(window); 
			make_gameover_menu(window);
		}
		window.display();
	}
	
	backgroundMusic.stop(); // stopping the background musci when game ends, so we placing it before return 0

	return 0;
}

// handle continuous input like movement keys controls swapped
void handleinput(RenderWindow &window) {
	if (!gameactive) return;// this ftn is used to assign the arrow keys to p1 and W,A,S,D to p2
	
	// player 0ne (Left) uses W,A,S,D keys present on the laptop
	if(isplayer1alive){
		int intendedDx1 = 0, intendedDy1 = 0; bool horizontalPressed1 = false;
		if (Keyboard::isKeyPressed(Keyboard::Left)) { 

			intendedDx1 = -1; 
			horizontalPressed1 = true; 
		}

		if (Keyboard::isKeyPressed(Keyboard::Right)) { 

			intendedDx1 = 1;  
			horizontalPressed1 = true; 
		}
		if (!horizontalPressed1) {

			if (Keyboard::isKeyPressed(Keyboard::Up)) 
			intendedDy1 = -1;
			if (Keyboard::isKeyPressed(Keyboard::Down)) 
			intendedDy1 = 1;
		}
		if (intendedDx1 != 0 || intendedDy1 != 0) {

			bool isReversal = isplayer1building && (intendedDx1 == -lastdx1 && intendedDx1 != 0 || intendedDy1 == -lastdy1 && intendedDy1 != 0);

			if (!isReversal && (intendedDx1 != dx || intendedDy1 != dy)) {
				dx = intendedDx1; 
				dy = intendedDy1; 
				lastdx1 = dx; 
				lastdy1 = dy;
			}
		}
	}

	// player tw0 (right) uses W,A,S,D keys present on the laptop
	if (numplayers == 2 && isplayer2alive) {

		int intendedDx2 = 0, intendedDy2 = 0; bool horizontalPressed2 = false;

		if (Keyboard::isKeyPressed(Keyboard::A))  { 

			intendedDx2 = -1; 
			horizontalPressed2 = true;
		}
		if (Keyboard::isKeyPressed(Keyboard::D)) { 

			intendedDx2 = 1;  
			horizontalPressed2 = true;
		}

		if (!horizontalPressed2) {

			if (Keyboard::isKeyPressed(Keyboard::W))    
				intendedDy2 = -1;
			if (Keyboard::isKeyPressed(Keyboard::S))  
				intendedDy2 = 1;
		}
		if (intendedDx2 != 0 || intendedDy2 != 0) {

			bool isReversal = isplayer2building && (intendedDx2 == -lastdx2 && intendedDx2 != 0 || intendedDy2 == -lastdy2 && intendedDy2 != 0);

			if (!isReversal && (intendedDx2 != dx2 || intendedDy2 != dy2)) {
				dx2 = intendedDx2; 
				dy2 = intendedDy2; 
				lastdx2 = dx2; 
				lastdy2 = dy2;
			}
		}
	}
}

void updategame(float deltatime) {
	if (!gameactive) return; //only check gameactive

	totalTime += deltatime;

	if (ispowerup_active) {
		poweruptimer += deltatime;
		if (poweruptimer >= powerup_duration) {
			ispowerup_active = false;
			poweruptimer = 0.f;
			playerusingpower = 0;
		}
	}

	const int continuous_enemy_cap = 10; 
	if (currentdifficulty == difficulty_continuous && currentenemy_count < continuous_enemy_cap && currentenemy_count < max_enemies) {
		time_sincelastenemyadd += deltatime;
		if (time_sincelastenemyadd >= 20.0f) {
			
			int addLimit = 2;
			int spacein_array = max_enemies - currentenemy_count;
			int spacein_modecap = continuous_enemy_cap - currentenemy_count;
			int enemiesto_add = addLimit;   //min logic for enemiesto_add
			if (spacein_array < enemiesto_add)
				enemiesto_add = spacein_array;
			if (spacein_modecap < enemiesto_add)
				enemiesto_add = spacein_modecap;
			if (enemiesto_add < 0)
				enemiesto_add = 0;

			//  loop for adding enemies
			for(int i = 0; i < enemiesto_add; ++i) {
				if (currentenemy_count >= max_enemies) 
					break; // Stop if array is full

				enemies_ptr[currentenemy_count] = Enemy(); // Create new enemy at the current index

				// ensure new enemies inherit speed factor
				if (currentenemy_count > 0 && currentenemy_count < max_enemies) { 
					// Checking index validity 
					enemies_ptr[currentenemy_count].current_speedfactor = enemies_ptr[0].current_speedfactor;
				}

				// If patterns are active, assign ZigZag or Circular to the new enemy
				if (patterns_activated) {
					// alternate based on the index where the enemy is being placed
					if (currentenemy_count % 2 != 0) { // Example: Odd indices get zigzag
						enemies_ptr[currentenemy_count].initializeSimpleZigZag();
					} else {                             // Even indices get circular
						enemies_ptr[currentenemy_count].initialize_circularpattern();
					}
				}
				//now that the enemy at currentenemy_count is set up, increment the count
				currentenemy_count++;
			}
			time_sincelastenemyadd = 0.f; //reset our timer outside the loop
		}
	}

	// increase Enemy Speed
	if (!ispowerup_active) {         //speed increase only if power-up is not active
		time_sinceSpeedup += deltatime;
		if (time_sinceSpeedup >= 20.0f) {
			for (int i = 0; i < currentenemy_count; ++i) {
				enemies_ptr[i].current_speedfactor += 0.1f;// every 20 seconds 
			}
			time_sinceSpeedup = 0.f;
		}
	}

    // activating advanced patterns (zigzag and circular) for some enemies after 30 seconds
	if (!ispowerup_active && !patterns_activated && totalTime >= 30.0f) {
		int enemies_to_switch = currentenemy_count / 2; // switch half the enemies
		if (enemies_to_switch > 0) {
			cout << "Activating zigzag/circular patterns for " << enemies_to_switch << " enemies after 30s." << endl;
			for (int i = 0; i < enemies_to_switch; ++i) {
                 if (i >= 0 && i < currentenemy_count) { // Safety check
				    // alternate patterns for the first half
				    if (i % 2 == 0) { // even indexed enemies get ZigZag
					    enemies_ptr[i].initializeSimpleZigZag();
				    } else {          // odd indexed enemies get Circular
					    enemies_ptr[i].initialize_circularpattern();
				    }
                 }
			}
			patterns_activated = true; // mark as activated for this game round
		}
	}

	// player m0vement timers
	const float P_move_delay = 0.06f;
	static float p1_movetimer = 0.f;
	static float p2_movetimer = 0.f;
	
	p1_movetimer += deltatime;
	if (numplayers == 2)
        	p2_movetimer += deltatime;

	// player 1 Update
	if (isplayer1alive&& (dx != 0 || dy != 0) && p1_movetimer >= P_move_delay) {
		p1_movetimer = 0.f;
		int nextX = x + dx; int nextY = y + dy;
		int targetCell = get_gridvalue(nextY, nextX);

		if (targetCell != tile_out_of_bounds) {
			// here we have death checks
			if (numplayers == 2 && targetCell == tile_trail_p2) // hit P2 trail
				isplayer1alive = false;
			else if (numplayers == 2 && nextX == x2 && nextY == y2 && isplayer2alive) // head-on with p2 sprite
				isplayer1alive = false;
			else if (targetCell == tile_trail_p1) // hit own trail
				isplayer1alive = false;

			if (isplayer1alive) {
				if (targetCell == tile_empty) {      //move into empty space
					if (!isplayer1building) { moves1++; isplayer1building = true; }
					setgridvalue(nextY, nextX, tile_trail_p1);
					x = nextX; y = nextY;
				}
				// hit border, own claimed area, OR opponent's claimed area
				else if (targetCell == tile_border || targetCell == claim_p1 || targetCell == claim_p2) { //use original claim constants
					if (isplayer1building) { // Was building
						// trigger fill only if hitting border or 0WN claimed area
						if (targetCell == tile_border || targetCell == claim_p1) { // Use original claim_p1
							int prevX = x, prevY = y;
							x = nextX; y = nextY; // move onto the safe tile before filling
							completefill(1, prevX, prevY);
						}
						// stop building regardless (border, own area, or opponent's area)
						isplayer1building = false;
						dx = dy = 0; // Stop movement
					} else { // not building
						// walk along border/claimed areas
						x = nextX; y = nextY;
					}
				}
			} 
		} 
	} // here Player 1 update block ends

	// player 2 Update
	if (isplayer2alive && numplayers == 2 && (dx2 != 0 || dy2 != 0) && p2_movetimer >= P_move_delay) {
		p2_movetimer = 0.f;
		int nextX2 = x2 + dx2; int nextY2 = y2 + dy2;
		int targetCell = get_gridvalue(nextY2, nextX2);

		if (targetCell != tile_out_of_bounds) { // use constant
			// ab idhar death checks hain
            		if (targetCell == tile_trail_p1) //if he hits p1 trail, died
				isplayer2alive = false;
			else if (nextX2 == x && nextY2 == y && isplayer1alive)     // head on with P1 sprite
				isplayer2alive = false;
			else if (targetCell == tile_trail_p2) // on hitting its own trail, he also dies
				isplayer2alive = false;

			if (isplayer2alive) {// toh agar woh nhi died
				if (targetCell == tile_empty) { // move into empty space
					if (!isplayer2building) { moves2++; isplayer2building = true; }
					setgridvalue(nextY2, nextX2, tile_trail_p2); // use constant
					x2 = nextX2; y2 = nextY2;
				}
				// hit border, own claimed area, or opponent's claimed area
				else if (targetCell == tile_border || targetCell == claim_p1 || targetCell == claim_p2) { //using original claim constants
					if (isplayer2building) { // was building
						// trigger fill only if hitting BORdeR or OwN claimed area
						if (targetCell == tile_border || targetCell == claim_p2) {   // using original claim_p2
							int prevX = x2, prevY = y2;
							x2 = nextX2; y2 = nextY2; // to move onto the safe tile before filling
							completefill(2, prevX, prevY);
						}
						// stop building regardless (border, own area, or opponent's area)
						isplayer2building = false;
						dx2 = dy2 = 0; // stop movement
					}
					else { // not building
						x2 = nextX2;     // walk along border/claimed areas
						y2 = nextY2;
					}
				}
			} 
		} 
	} // here the  player 2 update block ends

	// enemy m0veMent & c0llision
	if (!ispowerup_active) { // Enemies only move if powerup is not active
		for (int i = 0; i < currentenemy_count; ++i) {
			enemies_ptr[i].move(deltatime); // calls simplified move which calls pattern-specific or linear

			int exTile = static_cast<int>(enemies_ptr[i].x / ts); // again explicitly convert kr raha into (int)
			int eyTile = static_cast<int>(enemies_ptr[i].y / ts);   // idhar bhi saaame
			int tileUnderEnemy = get_gridvalue(eyTile, exTile);
			if (tileUnderEnemy == tile_trail_p1 && isplayer1building)
				isplayer1alive = false;
			if (numplayers == 2 && tileUnderEnemy == tile_trail_p2 && isplayer2building)
				isplayer2alive = false;
		}
	}

	// check game 0ver
	bool p1GameOver = !isplayer1alive;
	bool p2GameOver = (numplayers == 2 && !isplayer2alive);
	bool p2Inactive = (numplayers == 1);
	if (p1GameOver && (p2GameOver || p2Inactive)) {
		gameactive = false;
		if (numplayers == 1) {
			updatescoreboard(score1, totalTime);
			current_state = state_gameover_single;
		}
		else {
			updatescoreboard(score1, totalTime); //always update P1 score if game over
           		if(score2 > 0)
           			updatescoreboard(score2, totalTime);  // only update P2 if they scored
			current_state = state_gameover_two;
		}
		menu_choice = 0;
	}
}

void completefill(int playerid, int trailStartX, int trailStartY) {
	pair<int, int> protectedCoords[max_protected]; 
	int originalValues[max_protected];                  

	int protectedCount = 0;

	// 1# Protect enemies
	for(int i = 0; i < currentenemy_count; ++i) {
		const Enemy& enemy = enemies_ptr[i];
		int ex = static_cast<int>(enemy.x / ts);
		int ey = static_cast<int>(enemy.y / ts);
		int gridval = get_gridvalue(ey, ex);

		//here using defined constants including original claim_p1 and claim_p2
		if (gridval != tile_border && gridval != claim_p1 && gridval != claim_p2 && gridval != tile_out_of_bounds) {
			if (protectedCount < max_protected) {
				protectedCoords[protectedCount] = make_pair(ex, ey);
				originalValues[protectedCount] = gridval;
				setgridvalue(ey, ex, tile_temp_protected); protectedCount++;
			} 
		}
	}

	// 2)protect other player
	if (numplayers == 2) {
		int otherX = (playerid == 1) ? x2 : x;
		int otherY = (playerid == 1) ? y2 : y;
		bool otherAlive = (playerid == 1) ? isplayer2alive : isplayer1alive;
		int gridval = get_gridvalue(otherY, otherX);

		if (otherAlive && gridval != tile_border && gridval != claim_p1 && gridval != claim_p2 && gridval != tile_out_of_bounds) {
			if (protectedCount < max_protected) {
				protectedCoords[protectedCount] = make_pair(otherX, otherY);
				originalValues[protectedCount] = gridval;
				setgridvalue(otherY, otherX, tile_temp_protected); 
				protectedCount++;
			} 
		}
	}

	// 3]flood fill
	for (int i = 0; i < protectedCount; ++i) {
		int px = protectedCoords[i].first; int py = protectedCoords[i].second;
		int dx_fill[] = {0, 0, 1, -1}; int dy_fill[] = {1, -1, 0, 0};
		for(int j=0; j<4; ++j){
			if(get_gridvalue(py + dy_fill[j], px + dx_fill[j]) == tile_empty) {
				drop(py + dy_fill[j], px + dx_fill[j]);
			}
		}
	}

  // 4# claim Area & Score
	int filledcount = 0;
	int player_trail = (playerid == 1) ? tile_trail_p1 : tile_trail_p2;
	int other_trail = (playerid == 1) ? tile_trail_p2 : tile_trail_p1;
	int claimed_val = (playerid == 1) ? claim_p1 : claim_p2;

	for (int i = 1; i < M - 1; i++) {
		for (int j = 1; j < N - 1; j++) {
			int currentVal = get_gridvalue(i, j);
			if (currentVal == player_trail || (numplayers == 2 && currentVal == other_trail)) { // clear player trails
				setgridvalue(i, j, claimed_val);
			} else if (currentVal == tile_empty) {//here it is to fill empty visited areas
			
				setgridvalue(i, j, claimed_val);
				filledcount++;
			} else if (currentVal == tile_flood_visited) {
				setgridvalue(i, j, tile_empty);
			}
		}
	}
		
	//sound for (capturing Tiles
	if (filledcount > 0) {
		captureSound.play(); //sound for tiles Captured
	}
		
	// 5) restore protected
	for (int i = 0; i < protectedCount; ++i) {
		int px = protectedCoords[i].first; int py = protectedCoords[i].second;
		if (py >= 0 && py < M && px >= 0 && px < N && get_gridvalue(py, px) == tile_temp_protected) {
			setgridvalue(py, px, originalValues[i] == tile_flood_visited ? tile_empty : originalValues[i]);
		}
	}

	// 6)calculateing score & powerups 
	if (playerid == 1) {
		int basePoints = filledcount; int bonuspoints = 0;
		
		bool achievedBonusThisFillP1 = false;//flag for P1 bonus sound
				
		if (filledcount > bonus_threshold1) {//later, when evaluating a move    remember that, bonus threshold=10
			bonuspoints = basePoints * (bonus_multiplier1 - 1); //apply ×2 bonus 
			bonus_counter1++;                                  //track that bonus was earned		
			
			
			if (bonuspoints > 0 && !achievedBonusThisFillP1) { //here i am just checking if bonus points were actually added
				bonusSound.play(); // sound when bonus is achieved
				//BONUS FEATURE IMPLEMENTED BY OURSELF
				achievedBonusThisFillP1 = true;
			}
						
               		//iff we caapture more than 10 tiiles (filledcount > 10), we double 0ur points (points *= 2) and increment bonus_counter1.
			
			if (bonus_counter1 >= 5) { 
				bonus_threshold1 = 5;   // after 5 bonuses, threshold stays at 5
				bonus_multiplier1 = 4;  // multiplier jumps to ×4 
			}
			else if (bonus_counter1 >= 3) {  
				bonus_threshold1 = 5;  // after 3 bonuses, threshold lowers to 5
				bonus_multiplier1 = 2; // multiplier set to ×2
			}
		}
		score1 += (basePoints + bonuspoints);
				
		bool achievedPowerupThisFillP1 = false; // to play sound once per fill operation

		while (score1 >= nextpowerup_score1) {
			// here grant a power-up
			powerups1++;
			if (!achievedPowerupThisFillP1) { // checking this player's powerup gain flag
				powerupGainSound.play();    // Play sound
				achievedPowerupThisFillP1 = true; // set this player's powerup gain flag
			}
			//advancement to the next milestone
			if (nextpowerup_score1 == 50) {
				nextpowerup_score1 += 20;
			} else {
				nextpowerup_score1 += 30;
			}
		}	
	} 
	else { //player 2
		int basePoints = filledcount; int bonuspoints = 0;
		        bool achievedBonusThisFillP2 = false;//here we have flag for P2 bonus sound

		
		if (filledcount > bonus_threshold2) {
			bonuspoints = basePoints * (bonus_multiplier2 - 1); 			
			bonus_counter2++;
						 
			if (bonuspoints > 0 && !achievedBonusThisFillP2) { //checking if bonus points were actually added
				bonusSound.play(); // sound when bonus is achieved
				//bonus feature implemented by ourself
				
				achievedBonusThisFillP2 = true;
			}
					
			if (bonus_counter2 >= 5) { 
				bonus_threshold2 = 5;    // after 5 bonuses, threshold stays at 5
				bonus_multiplier2 = 4;   // multiplier jumps to ×4 
			}
			else if (bonus_counter2 >= 3) { 
				bonus_threshold2 = 5;      // after 3 bonuses, threshold lowers to 5
				bonus_multiplier2 = 2; // multiplier set to ×2
			}
		}
		score2 += (basePoints + bonuspoints);
	      
	        bool achievedpowerupthisfillP2 = false;  //specific flag for P2 power-up gain
		while (score2 >= nextpowerup_score2) {
			
			// then giving a power-up
			powerups2++;
			if (!achievedpowerupthisfillP2) { //checking this player's power-up gain flag
				powerupGainSound.play();    // Play sound
				achievedpowerupthisfillP2 = true; // Set this player's power-up gain flag
			}
			// advancement to the next milestone:
			if (nextpowerup_score2 == 50) {
				nextpowerup_score2 += 20;
			} else {
				//now,after 70, 100, 130, ... always +30
				nextpowerup_score2 += 30;// 70 to 100, 100  130,  130 to 160, etc.
			}		
		}
	}
}

void rendergame(RenderWindow &window) {

	// draw grid
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < N; j++) {

			int gridval = get_gridvalue(i,j);
			int texX = 18;    //default texture X offset (empty tile)
			Color tileColor = Color::White; // idhar ye color modulation default (no change)

			if (gridval == tile_border) 
				texX = 0;
			else if (gridval == tile_trail_p1) 
				texX = 54;
			else if (gridval == tile_trail_p2 && numplayers == 2) 
				texX = 18*5;
			else if (gridval == claim_p1) { 
				texX = 0; 
				tileColor = Color(100, 100, 255, 200); // assignning colors to the blocks of player 1
			} //here i am using original claim_p1 + Blue tint
			else if (gridval == claim_p2) { 
				texX = 0; 
				tileColor = Color(255, 100, 100, 200); // idhar ussi trah colors to the bl0cks of player 2
			} // use original claim_p2 + Red tint
			
			// waisay tile empty uses default texX=18 and tile_color=White

			sTile.setTextureRect(IntRect(texX, 0, ts, ts));
			sTile.setColor(tileColor); // apply color tint
			sTile.setPosition((float)(j * ts), (float)(i * ts));
			window.draw(sTile);
		}
	}
	sTile.setColor(Color::White); // reset color for subsequent draws (like players)

	// idhar draw players
	if (isplayer1alive) {
		sTile.setTextureRect(IntRect(36, 0, ts, ts)); // ye hai p1 ka sprite texture
		sTile.setColor(Color::White); // ensure player sprite is not tinted
		sTile.setPosition((float)(x * ts), (float)(y * ts));
		window.draw(sTile);
	}
	if (numplayers == 2 && isplayer2alive) {
		sTile.setTextureRect(IntRect(18*6, 0, ts, ts)); // p2 ka sprite texture 
		sTile.setColor(Color::White); 
		sTile.setPosition((float)(x2 * ts), (float)(y2 * ts));
		window.draw(sTile);
	}

	// aur yahan pr drawing enemies
	float rotationAngle = fmod(totalTime * 180.f, 360.f);
	for (int i = 0; i < currentenemy_count; ++i) {
		sEnemy.setPosition(enemies_ptr[i].x, enemies_ptr[i].y);
		sEnemy.setRotation(rotationAngle);
		window.draw(sEnemy);
	}

	// drawing UI idhar
	stringstream ss1, ss2, ssTime, ssMoves1, ssMoves2, ssPowerup1, ssPowerup2;
	
	ss1 << "P1 Score: " << score1;
	scoretext1.setString(ss1.str());
	window.draw(scoretext1);

        ssMoves1 << "Moves: " << moves1;
	movestext1.setString(ssMoves1.str());
	window.draw(movestext1);

        ssPowerup1 << "Freeze: " << powerups1 << " (P)";
	poweruptext1.setString(ssPowerup1.str());
	window.draw(poweruptext1);

	ssTime << "Time: " << format_time(totalTime);
	timetext.setString(ssTime.str());
	timetext.setOrigin(0.f, 0.f);

	float timeWidth = 0.f;
	if(!timetext.getString().isEmpty()) 
		timeWidth = timetext.findCharacterPos(timetext.getString().getSize()).x;
	
	timetext.setOrigin(timeWidth / 4.0f, 0.f); // correct centering / 2.0f
	timetext.setPosition(window.getSize().x / 2.0f, M * ts + 10);// yahan sae position set ki hai
	window.draw(timetext);

	if (numplayers == 2) {
		 ss2 << "P2 Score: " << score2;
		scoretext2.setString(ss2.str());
		window.draw(scoretext2);

		ssMoves2 << "Moves: " << moves2;
	        movestext2.setString(ssMoves2.str());
		window.draw(movestext2);

		ssPowerup2 << "Freeze: " << powerups2 << " (Q)";
		poweruptext2.setString(ssPowerup2.str());
	        window.draw(poweruptext2);
	}

	if (ispowerup_active) {
		Text freezeText;
		freezeText.setFont(font);
		freezeText.setCharacterSize(30);
		
		 freezeText.setFillColor(Color::Blue);
		freezeText.setOutlineColor(Color::White);
		freezeText.setOutlineThickness(1);
		freezeText.setString("FREEZE ACTIVE!"); freezeText.setOrigin(0.f, 0.f);

		float freezeWidth = 0.f; if(!freezeText.getString().isEmpty()) freezeWidth = freezeText.findCharacterPos(freezeText.getString().getSize()).x;
		freezeText.setOrigin(freezeWidth / 2.0f, 0.f); // correct centering / 2.0f
		freezeText.setPosition(N * ts / 2.0f, M * ts / 2.0f - freezeText.getCharacterSize() / 2.f);// correct position set 
		window.draw(freezeText);
	}
}

// ye aik helper hai for centering text horizontally
void drawCenteredtext(RenderWindow &window, Text &text, float posY) {
	text.setOrigin(0.f, 0.f);
	float textWidth = 0.f; 

	if(!text.getString().isEmpty()) textWidth = text.findCharacterPos(text.getString().getSize()).x;
	text.setOrigin(textWidth / 5.0f, 0.f);
	text.setPosition(window.getSize().x / 2.0f, posY);
	window.draw(text);
}

// menu drawing functions (simplified using drawCenteredtext)
void draw_mainmenu(RenderWindow &window) {
	
	if (backgroundTexture.getSize().x > 0) { // only draw if texture was loaded
		window.draw(backgroundSprite);
	}
	
	const int numOptions = 3; 
	string options[numOptions] = {"Start Game", "Scoreboard", "Exit"};

	float titleY = window.getSize().y / 2.0f - 60; 
	float optionsStartY = titleY + 60; 
	float promptY = window.getSize().y - 40;

	menutext.setString("X O N I X  G A M E "); 
	menutext.setCharacterSize(40); 
	menutext.setFillColor(Color::Cyan); 
	drawCenteredtext(window, menutext, titleY);

	menutext.setCharacterSize(24);

	for (int i = 0; i < numOptions; ++i) { 
		menutext.setString(options[i]); 
		menutext.setFillColor(i == menu_choice ? Color::Yellow : Color::White); 
		drawCenteredtext(window, menutext, optionsStartY + i * 40); 
	}
	prompt_text.setString("Use Up/Down Arrows and Enter"); 
	drawCenteredtext(window, prompt_text, promptY);
}

void select_Modemenu(RenderWindow &window) {
	
	if (backgroundTexture.getSize().x > 0) { //saame here only draw if texture was loaded
		window.draw(backgroundSprite);
	}	
	    
	const int numOptions = 2; string options[numOptions] = {"Single Player", "Two Players"};
        float titleY = window.getSize().y / 2.0f - 90; 
	float optionsStartY = titleY + 70; 
	float promptY = window.getSize().y - 40;

	menutext.setString("Select Mode:"); 
	menutext.setFillColor(Color::Cyan); 
	menutext.setCharacterSize(30); 
	drawCenteredtext(window, menutext, titleY);
        menutext.setCharacterSize(24);

	for (int i = 0; i < numOptions; ++i) { 
		menutext.setString(options[i]); 
		menutext.setFillColor(i == menu_choice ? Color::Yellow : Color::White); 
		drawCenteredtext(window, menutext, optionsStartY + i * 40); 
	}
	prompt_text.setString("Press Up/Down/Enter. Esc to go back."); 
	drawCenteredtext(window, prompt_text, promptY);
}

void select_levelmenu(RenderWindow &window) {
	
	if (backgroundTexture.getSize().x > 0) { //same case also used here that (only draw if texture was loaded
		window.draw(backgroundSprite);
	}

	const int numOptions = 4; 
	string options[numOptions] = {"Easy Level(2 Enemies)", "Medium Level(4 Enemies)", "Hard Level  (6 Enemies)", "Continuous (Start 2, +2/20s)"};

	float titleY = window.getSize().y / 2.0f - 100; 
	 float optionsStartY = titleY + 70; 
        float promptY = window.getSize().y - 40;

	menutext.setString("Select Difficulty:"); 
	menutext.setFillColor(Color::Cyan); 
        menutext.setCharacterSize(30); 
        drawCenteredtext(window, menutext, titleY);
	menutext.setCharacterSize(24);

	for (int i = 0; i < numOptions; ++i) { 
		menutext.setString(options[i]); 
		menutext.setFillColor(i == menu_choice ? Color::Yellow : Color::White); 
		drawCenteredtext(window, menutext, optionsStartY + i * 40); 
	}
	prompt_text.setString("Press Up/Down/Enter. Esc to go back."); 
	drawCenteredtext(window, prompt_text, promptY);
}

void drawscoreboard_display(RenderWindow &window) {

	float titleY = 50;
	float headerY = titleY + 50;
	float scoresStartY = headerY + 40;
	float promptY = window.getSize().y - 40;

        menutext.setString("High Scores");
	menutext.setCharacterSize(30);
        menutext.setFillColor(Color::Yellow);
	drawCenteredtext(window, menutext, titleY); // centered title
	
	menutext.setCharacterSize(22);
	menutext.setFillColor(Color::White); 
        Text scoretimetext = menutext; 
	menutext.setString("Rank   Score      Time"); 
	drawCenteredtext(window, menutext, headerY); //centered header
	float centerPos = window.getSize().x / 2.0f;

	for (int i = 0; i < sizeof_scoreboard; ++i) {


		if (highScores_scores[i] <= 0 && highScores_times[i] <= 0.0f) continue;

		stringstream ssRank, ssScore, ssTimeStr;
		ssRank << (i + 1) << ".";
		ssScore << setw(8) << highScores_scores[i];
		ssTimeStr << format_time(highScores_times[i]);

		float lineY = scoresStartY + i * 35;

		// At place of rank we do (left aligned relative to columns)
	        menutext.setString(ssRank.str());
		menutext.setOrigin(0.f, 0.f);
	       menutext.setPosition(centerPos - 130, lineY); // position left
		window.draw(menutext);

        //  score ko center aligned rakha 
	scoretimetext.setString(ssScore.str());
	scoretimetext.setOrigin(0.f, 0.f); // reset origin
	float scoreWidth = scoretimetext.findCharacterPos(scoretimetext.getString().getSize()).x;
	scoretimetext.setOrigin(scoreWidth / 12.0f, 0.f); //set origin
	 scoretimetext.setPosition(centerPos - 30, lineY); // position the origin
        window.draw(scoretimetext);
 
        // idhar time (ko right aligned)
	scoretimetext.setString(ssTimeStr.str());
	scoretimetext.setOrigin(0.f, 0.f); // reset origin
         float timeWidth = scoretimetext.findCharacterPos(scoretimetext.getString().getSize()).x;
        scoretimetext.setOrigin(timeWidth/4.f, 0.f); // align origin right top
        scoretimetext.setPosition(centerPos + 150, lineY); 
        window.draw(scoretimetext);

	}
	prompt_text.setString("Press Enter or Escape to return to Main Menu");
	drawCenteredtext(window, prompt_text, promptY); 
}
void make_gameover_menu(RenderWindow &window) {
    
	RectangleShape overlay(Vector2f((float)window.getSize().x, (float)window.getSize().y)); 
	overlay.setFillColor(Color(0, 0, 0, 180)); 
	window.draw(overlay);
     

	float gameOverY = window.getSize().y / 3.0f - 30;
	float scoreInfoY = gameOverY + 80;    //calculate scoreInfoY first
	float winnerY = scoreInfoY;              // now winnerY can use scoreInfoY
	float p1ScoreY_2P = winnerY + 50;     //this will now use the correctly initialized winnerY
	float p2ScoreY_2P = p1ScoreY_2P + 40;
	float singleScoreY = scoreInfoY;      //this was okay as scoreInfoY was used after its intended calculation line
	float optionsStartY = 0; 

	gameovertext.setString("GAME OVER"); 
	drawCenteredtext(window, gameovertext, gameOverY);

	if (current_state == state_gameover_single) {
		stringstream fs1; fs1 << "Final Score: " << score1; 
		finalscoretext.setString(fs1.str()); 
		finalscoretext.setFillColor(Color::White);

		if (score1 > 0 && score1 == highScores_scores[0] && abs(totalTime - highScores_times[0]) < 0.01f) { 

			finalscoretext.setFillColor(Color::Yellow); 
			finalscoretext.setString(fs1.str() + " (New High Score)"); 
		}
		drawCenteredtext(window, finalscoretext, singleScoreY); 
		optionsStartY = singleScoreY + 80;//setting optionsStartY for single playe

	} else if (current_state == state_gameover_two) {
		string winnerStr = "IT'S A DRAW"; 
		Color winnerColor = Color::Yellow; 

		if (score1 > score2) { 
			winnerStr = "Player 1 WINS"; 
			winnerColor = Color::Cyan;
		} 
		else if (score2 > score1) { 
			winnerStr = "Player 2 WiNs"; 
			winnerColor = Color::Magenta;
		}

		winnertext.setString(winnerStr); 
               winnertext.setFillColor(winnerColor); 
		drawCenteredtext(window, winnertext, winnerY);

		stringstream fs1, fs2; fs1 << "p1 Score: " << score1; 
		   fs2 << "p2 Score: " << score2;

		finalscoretext.setFillColor(Color::Cyan); 
		 finalscoretext.setString(fs1.str()); 
		drawCenteredtext(window, finalscoretext, p1ScoreY_2P);

		finalscoretext.setFillColor(Color::Magenta); 
		finalscoretext.setString(fs2.str()); 
		drawCenteredtext(window, finalscoretext, p2ScoreY_2P); 
		optionsStartY = p2ScoreY_2P + 80;//here we just (set optionsStartY for two players)
	}
	const int numOptions = 3; 
	string options[numOptions] = {"Restart", "Main Menu", "Exit Game"}; 
	menutext.setCharacterSize(24);

	for (int i = 0; i < numOptions; ++i) { 
		menutext.setString(options[i]); 
		menutext.setFillColor(i == menu_choice ? Color::Yellow : Color::White); 
		drawCenteredtext(window, menutext, optionsStartY + i * 40); 
	}    	
}
