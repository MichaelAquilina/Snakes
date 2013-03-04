#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "game.h"

void refreshAll( struct game *gm )
{	int i;
	for( i=0; i<( gm->no_players ); i++ )
		gm->players[i].refresh = TRUE;
}

//generates a fruit at a random location in the game
Bodypart *generateFruit( struct game *gm, int dimy, int dimx )
{	
	refreshAll( gm );
	gm->fruit.x = 1+ ( rand() % (dimx-2) );
	gm->fruit.y = 1+ ( rand() % (dimy-2) );
	return &(gm->fruit);
}

//creates a new player in a given game
struct player *newPlayer( struct game *gm )
{	struct player *pl = &(gm->players[ gm->no_players ] );
	initialisePlayer( pl );	
	gm->no_players++;
	gm->no_lplayers++;
	return pl;
}

//checks if the head of sn1 intersects any of the body of snake 2
//returns 1 if true, 2 if both snakes hit at the head, 0 if they dont intersect
int snakeHit( Snake *sn1, Snake *sn2 )
{		
	/*further optimisation: it is impossible for the same snake to hit the first
	  three of this body parts, start calculating from the 4th */
	int i = (sn1==sn2)? 3: 0;	//if it is the same snake, ignore the head	
	int x = sn1->buf[sn1->head].x;
	int y = sn1->buf[sn1->head].y;

	for( ; i<(sn2->length); i++ )
		if( x==getPartX( sn2, i ) && y==getPartY( sn2, i ) )
			return (i==0)? 2: 1;

	return 0;
}

int checkBorder( struct game *gm, int dimy, int dimx, void (*hit)( int ) )
{	int i;
	int hits;	
	Snake *sn;

	for( i=0; i< (gm->no_players); i++ )
	{	if( gm->players[i].killed )
			continue;

		sn = &( gm->players[i].sn );
		if( getSnakeX( sn )<=0 || getSnakeY( sn )<=0 || getSnakeX( sn )>=dimx-1 || getSnakeY( sn )>=dimy-1 )
		{
			refreshAll( gm );
			hits++;
			(*hit)( i );
		}
	}
	return hits;
}

//checks whether any collisions occured in the game
int checkCollisions( struct game *gm, void (*hit)( int ) )
{	int i;
	int j;
	int r;
	int hits = 0;

	Snake *sn1;

	for( i=0; i < (gm->no_players); i++ )
	{	//if this player is killed, then ignore him	
		if( gm->players[i].killed )
			continue;	
		
		sn1 = &( gm->players[i].sn );
		//compare to every player
		for( j = 0; j < (gm->no_players); j++ )
		{	//if the player is killed then ignore him	
			if( gm->players[j].killed )
				continue;
			
			if( r = snakeHit( sn1, &(gm->players[j].sn) ) )
			{	
				refreshAll( gm );			
				hits++;
				(*hit)( i );
				//if the collission was a head-on collission, terminate both clients				
				if( r==2 )
				{
					hits++;
					(*hit)( j );
				}
			}
		}
	}
	return hits;
}

//get the specified bodypart from the snake
Bodypart *getPart( Snake *sn, int part ) 
{	int index = ( sn->head + part ) % WIN_LENGTH;
	return &( sn->buf[index] );
}

//moves the given snake by 1 depending on the direction stored in its structure
int moveSnake( Snake *sn )
{	
	int px;
	int py;

	if( sn==NULL )
		return -1;

	//check if last element in the list contains
	if( getPartType( sn, sn->length -1 ) == SNAKE_EATEN )	
	{
		setPartType( sn, sn->length -1, SNAKE_PART );
		sn->length++;
	}
	//get last recorded head coordinates
	px = sn->buf[sn->head].x;
	py = sn->buf[sn->head].y;

	//move the head back
	sn->head--;
	if( sn->head < 0 )
		sn->head = WIN_LENGTH - 1;

	//update the position of the new head
	sn->buf[sn->head].y = py;
	sn->buf[sn->head].x = px;
	switch( sn->dir )
	{	case UP   : sn->buf[sn->head].y--;
				break;
		case LEFT : sn->buf[sn->head].x--;
				break;
		case DOWN : sn->buf[sn->head].y++;
				break;
		case RIGHT: sn->buf[sn->head].x++;
				break;	
	}
	return 0;
}

//initialises a snakes structure and all its components
void initialiseSnake( Snake *sn, int y, int x, int length, char dir )
{	int i;
	sn->length = length;
	sn->actual_length = length;
	sn->head = 0;
	sn->dir = dir;
	
	//determine the positionns of the leading body parts
	for( i=0; i<WIN_LENGTH; i++ )
	{	int index = ( sn->head + i ) % WIN_LENGTH;
		sn->buf[ index ].x = x;
		sn->buf[ index ].y = y;
		sn->buf[ index ].type = SNAKE_PART;		
		switch( dir ) 
		{	case LEFT : sn->buf[index].x += i;
					break;
			case RIGHT: sn->buf[index].x -= i;
					break;
			case UP   : sn->buf[index].y += i;
					break;
			case DOWN : sn->buf[index].y -= i;
					break;
		}		
	}
}

//takes a given player structure and initialises
void initialisePlayer( struct player *pl )
{	pl->score = 0;
	pl->valid = FALSE;
	pl->killed = TRUE;
	pl->refresh = TRUE;
	pl->pid = getpid();
}

//takes a given game structure and initalises its components
void initialiseGame( struct game *gm )
{	gm->no_players = 0;
	gm->no_lplayers = 0;
	gm->fruit.x = 0;
	gm->fruit.y = 0;
	gm->fruit.type = SNAKE_FRUIT;	
	gm->serverpid = getpid ();
}

//creates a game and iniatilises its components
struct game *createGame( )
{	struct game *gm = (struct game *) malloc( sizeof( struct game ) );
	initialiseGame( gm );
	return gm;
}

//create a new Snake structure and initialises all its elements
Snake *createSnake( int x, int y, int length, char dir )
{	
	Snake *sn = (Snake *) malloc( sizeof( Snake ) );
	initialiseSnake( sn, x, y, length, dir );
	return sn;
}

//prints information about a given snake structure
void printsnakeinfo( Snake *sn )
{	int i;	
	printf( "length: %d\n", sn->length );
	printf( "actual_length: %d\n", sn->actual_length );
	printf( "dir: %c\n", sn->dir );
	printf( "head: %d\n", sn->head );
	
	for( i=0; i<WIN_LENGTH; i++ )
	{	int index = (sn->head + i) % WIN_LENGTH;
		printf( "%d->%d: (%d,%d) : %c \n",i, index, sn->buf[index].x, sn->buf[index].y, sn->buf[index].type );
	}
}

//print player information to the screen
void printplayerinfo( struct player *pl, BOOL printSnake )
{
	printf( "pid: %d\n", pl->pid );
	printf( "score: %d\n", pl->score );
	printf( "killed: %d\n", pl->killed );
	printf( "valid: %d\n", pl->valid );	
	if( printSnake )
		printsnakeinfo( &(pl->sn) );
}

//print game information to the screen
void printgameinfo( struct game *gm, BOOL printSnake )
{	int i;
	printf( "framerate: %d\n", 1000/GAME_SPEED );
	printf( "no_players: %d\n", gm->no_players );
	printf( "no_lplayers: %d\n", gm->no_lplayers );
	printf( "serverpid: %d\n", gm->serverpid );

	for( i=0; i<(gm->no_players); i++ )
	{
		printf( "===Player %d===\n", i );
		printplayerinfo( &(gm->players[i]), printSnake );
	}
}
