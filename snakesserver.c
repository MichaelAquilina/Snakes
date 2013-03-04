#include "connection.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>
#include <ncurses.h>
#include <errno.h>

/*Server performs all the data operations, does not perform any form of I/O interaction with the user directly*/

#define debug if( debugmode ) printf

//server time out of 10 seconds
#define SERVER_TIMEOUT 10
#define OFFLINE 0
#define ONLINE 1

bool debugmode = FALSE;
Connection *cn;
struct game *gm;

void killclient( int playerno );
//triggered by a SIGCONT signal
void startserver( int signal );
//triggered by SIGINT or if no clients are available
void closeserver( int signal );
//triggered by a SIGALRM after a server timeout
void checkserver( int signal );
//generates fruit at a new location
void regenerate( int signal );
//swaps two player objects
void swap( struct player *p1, struct player *p2 );
//Restarts the server and performs clean up on the current player list
void Restart( void );


//ncurses screen bounds, should be dynamic, but for debugging keep them here
#define maxx 80
#define maxy 24

int status = OFFLINE;

int main( int argc, char *argv[] )
{
	//declare all variables in the beginning (ISO Standard)
	int i;
	int result;
	unsigned short array[MAX_PLAYERS+1];
	Snake *sn;

	//check if debug mode is enabled
	if( argc>1 )
	{
		if( strcmp( argv[1], "-debug")==0 )
			debugmode = TRUE;
	}

	debug( "Debug mode enabled!\n" );
	debug( "Snakes server started\n", argv[0] );
	debug( "Max Players = %d\n", MAX_PLAYERS );
	debug( "Winning Length = %d\n", WIN_LENGTH );

	signal( SIGINT, closeserver );
	signal( SIGCONT, startserver );
	signal( SIGALRM, closeserver );	
	debug( "Attempting to Launch Server\n" );
	cn = launch( STDGAME, sizeof( struct game ), MAX_PLAYERS+1, IPC_EXCL );
	
	if( cn->status > 0 )
	{	perror( "Error launching server" );
		debug( "Error code: %d\n", cn->status );
		closeserver( SIGINT );
	}

	//initialise the array
	for( i=0; i<MAX_PLAYERS+1; i++ )
		array[i]=1;
	
	//set the semaphore values
	if( semctl( cn->semid, 0, SETALL, array ) ==-1 )
	{	perror( "Error Initialising Semaphore" );
		closeserver( SIGINT );
	}
	gm = cn->shm_addr;
	
	debug( "maxy = %d\n", maxy );
	debug( "maxx = %d\n", maxx );

	//set a seed for the random function
	srand( getpid() );
	initialiseGame( gm );
	generateFruit( gm, maxy, maxx );

	debug( "Server Launch Succesful\n" );
	
	debug( "Waiting %d seconds for a client\n", SERVER_TIMEOUT );
	//specify a timeout period before the server terminates	
	alarm( SERVER_TIMEOUT );
	//tell the client that the server is ready (if the client spawned the server)
	kill( getppid(), SIGCONT );	
	//wait for a signal from the client
	pause();

	signal( SIGALRM, regenerate );
	//regenerate( SIGALRM );
	debug( "A Game has Started\n" );
	do {
		//if there are no clients available then shutdown the server
		if( gm->no_lplayers == 0 )
		{  	debug( "All players have died\n" );
			closeserver( SIGINT );
		}		
		
		//for every live player in the game
		for( i=0; i<(gm->no_players); i++ )			
			if( !(gm->players[i].killed) )
			{
				//ping every client to check if they are still alive, if not, remove them
				if( kill( gm->players[i].pid, 0 ) == - 1 && errno == ESRCH )
				{	
					debug( "A zombie process with pid %d was found. Removing...\n", gm->players[i].pid );
					requestAccessToShm( cn, 0 );	
					gm->players[i].killed = TRUE;				
					gm->no_lplayers--;
					releaseAccessToShm( cn, 0 );
					continue;
				}
				sn = &(gm->players[i].sn);				
				//move the snake one space in the direction he is facing
				moveSnake( sn );
				//check if the snake moved over a fruit
				if( getSnakeX( sn )==gm->fruit.x && getSnakeY( sn )==gm->fruit.y )
				{
					//update player data
					gm->players[i].score += FRUIT_POINTS;
					gm->players[i].sn.actual_length ++;	
					if( debugmode )
						printplayerinfo( &(gm->players[i]), TRUE );

					//check if the snake is long enough to win the game	
					if( sn->actual_length==WIN_LENGTH )
					{	
						debug( "Player %d has won the game\n", i );
						Restart();
					}	
					else
						setSnakeType( sn, SNAKE_EATEN );
					regenerate( SIGALRM );
				}
			}

		//check snake and border collissions in the game
		checkCollisions( gm, killclient );
		checkBorder( gm, maxy, maxx, killclient );

		//sleep a constant time, depending on the game speed
		usleep( GAME_SPEED*1000 );
	} while( 1 );
	
	closeserver( SIGINT );
}

//regenerate the snake fruit after a preset amount of time
void regenerate( int sig )
{
	int seconds = 3 + (rand() % 6);
	signal( SIGALRM, regenerate );
	generateFruit( gm, maxy, maxx );
	debug( "Generated new fruit at: %d, %d\n", gm->fruit.x, gm->fruit.y );	
	alarm( seconds );
}

//kills the specified player
void killclient( int playerno )
{
	debug( "Terminating player number %d with pid %d\n", playerno, gm->players[ playerno ].pid );
	gm->players[ playerno ].killed = TRUE;
}

//generated by SIGALRM while the server is waiting for a client connection
void checkserver( int sig )
{	if( status==OFFLINE )
		closeserver( sig );
}

//sets the server as ONLINE
void startserver( int sig )
{	status = ONLINE;	
	debug( "Server found a client\n" );
}

//shutdown the server and all its connected users
void closeserver( int sig )
{	int i;	
	signal( SIGINT, closeserver );

	debug( "Shutting down server\n" );
	//kill off all players
	if( gm!=NULL )
	{	for( i=0; i<(gm->no_players); i++ )
			if( !gm->players[i].killed )
				killclient( i );	
	}

	//shutdown the connection if open
	if( cn!=NULL )
		shutdown( cn );
	exit( 0 );
}

void Restart()
{
	int i;
	debug( "Restarting the server\n" );
	requestAccessToShm( cn, 0 );

	for( i=0; i<(gm->no_players); i++ )
	{
		requestAccessToShm( cn, i+1);
		
		initialiseSnake( &((gm->players)[i].sn), 2+i, 1, INITIAL_LENGTH, RIGHT );
		(gm->players)[i].refresh = TRUE;
		if( debugmode )
			printplayerinfo( &((gm->players)[i]), TRUE );

		releaseAccessToShm( cn, i+1);
	}

	releaseAccessToShm( cn, 0 );
}

//swaps the values of two player objects
void swap( struct player *p1, struct player *p2 )
{
	//memory stored on the stack
	struct player dummy = *p1;
	
	//i dont think its swapping well: UPDATE: it is swapping well
	*p1 = *p2;
	*p2 = dummy;
}
