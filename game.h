#include <sys/types.h>

#define STDGAME	ftok( "/usr/bin/gcc", 1989 )

#define SNAKES_SERVER "snakesserver"

#define GAME_SPEED 80

#define MAX_PLAYERS 8	//for now keep a hard limit of 8 due to restrictions of heads up display (HUD)

//if you want to change the winning limit, change from here
#define WIN_LENGTH 15

#define INITIAL_LENGTH 3

#define SNAKE_PART ' '

#define DEFAULT_HIT	(void (*) () )0

#define FRUIT_POINTS 100

#define SNAKE_EATEN '#'

#define SNAKE_FRUIT '%'

#define Snake struct snake
#define Bodypart struct bodypart
#define Game struct game
#define Player struct player

#ifndef BOOL
	#define BOOL char
	#define TRUE 1
	#define FALSE 0
#endif

//movement keys
//change the down key from here
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'

struct bodypart
{	int x;
	int y;
	char type;
};

struct snake
{	int head;
	char dir;
	int length;
	int actual_length;
	struct bodypart buf[ WIN_LENGTH ];
};

struct player
{	int pid;
	int score;
	BOOL valid;
	BOOL killed;
	BOOL refresh;
	struct snake sn;
};

struct game
{	pid_t serverpid;
	int no_players;
	int no_lplayers;
	struct bodypart fruit;	
	struct player players[ MAX_PLAYERS ];
};

//tells all players in the game to refresh their screen
void refreshAll( struct game *gm );

//generates a fruit in the game structure at a random location, given bounds dimy and dimx. 
//Returns the location of the fruit in a Bodypart structure.
Bodypart *generateFruit( struct game *gm, int dimy, int dimx );

//creates a new player in the given game and returns a pointer to the player profile
struct player *newPlayer( struct game *gm );

//initialise a snake structure with the give information
void initialiseSnake( Snake *sn, int x, int y, int length, char dir );

//initialise a player structure. Note that it is up to the user to set killed to FALSE at his/her own time
void initialisePlayer( struct player *pl );

//initialise a game strucutre
void initialiseGame( struct game *gm );

//creates a game and initialises all its components
struct game *createGame();

//checks if the head of snake 1 (sn1) hit and part of snake 2 (sn2)
//returns 1 if a hit is detected and a 0 otherwise
int snakeHit( Snake *sn1, Snake *sn2 );

//creates a new snake and initialises all its parts
Snake *createSnake( int y, int x, int length, char dir );

//moves the given snake by 1 space depending on the direction it is facing
int moveSnake( Snake *sn );

//checks if any of the snakes in a specified game are out of boundaries given the
//dimensions dimy and dimx. If a snake is out of boundaries, the specified hit function
//is run with the players pid passed in the arguments. Returns the number of out of
//boundary snakes detected.
int checkBorder( struct game *gm, int dimy, int dimx, void (*hit)( int ) );

//checks possible collisions in the game and executes the hit function declared
//for every hit snake. If DEFAULT_HIT is specified in hit, then the default function is used
//which terminates the collided client. Returns the number of collissions detected
int checkCollisions( struct game *gm, void (*hit)( int ) );

//prints debug information about the given snake
void printsnakeinfo( Snake *sn );

//prints debug information about the given player. If printSnake is set to TRUE then
//snake debug information is also printed
void printplayerinfo( struct player *pl, BOOL printSnake );

//prints debug information about the given game and its players. If printSnake is set 
//to TRUE then snake debgug information is also printed
void printgameinfo( struct game *gm, BOOL printSnake );

//gets the bodypart of the specified index (0 being the head and everything else trailing 
//after it )
//#define getPart( SNAKE, INDEX )	SNAKE->buf[ ( SNAKE->head + INDEX ) % SNAKE->length]
Bodypart *getPart( Snake *sn, int index );

//wrappers to quickly get the x and y values of a given body part
#define getPartType( SNAKE, PART )	getPart( SNAKE, PART )->type
#define getPartX( SNAKE, PART )	getPart( SNAKE, PART )->x
#define getPartY( SNAKE, PART )	getPart( SNAKE, PART )->y

//wrappers to quickly set the x and y values of a given snake body part
#define setPartType( SNAKE, PART, TYPE )	getPart( SNAKE, PART )->type = TYPE
#define setPartX( SNAKE, PART, X )	getPart( SNAKE, PART )->x = X
#define setPartY( SNAKE, PART, Y ) getPart( SNAKE, PART )->y = Y

//gives the x and y position of the head of the snake
#define getSnakeX( SNAKE )	getPart( SNAKE, 0 )->x
#define getSnakeY( SNAKE )	getPart( SNAKE, 0 )->y
#define getSnakeType( SNAKE )	getPart( SNAKE, 0 )->type

#define setSnakeType( SNAKE, TYPE )	getPart( SNAKE, 0 )->type =  TYPE
#define setSnakeX( SNAKE, X )	getPart( SNAKE, 0 )->x = X
#define setSnakeY( SNAKE, Y )	getPart( SNAKE, 0 )->y = Y
