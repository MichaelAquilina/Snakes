#include "connection.h"
#include "game.h"
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <ncurses.h>

/*The client for the snakes game represents a dumb terminal whose only role is to perform simple input and ouput*/

#define STARTPOSITION playerno+2
#define debug if( debugmode ) printf

Connection *cn;
struct game *gm;
struct player *pl;
Snake *sn;
int playerno;

char debugmode = FALSE;

int maxx;
int maxy;

void closeclient( int signal );
void verifyconnection( int signal );
void RestartPlayer();

void drawSnake( Snake *sni )
{	int i;
	int index;
	if( sni==sn )
		attron( COLOR_PAIR(3) | A_REVERSE );
	else
		attron( COLOR_PAIR(2) | A_REVERSE );
	for( i=0; i<=(sni->length); i++ )	
	{
		//turn color differently for the body
		if( i==1 )
			attron( COLOR_PAIR( 1 ) );
		//remove last part
		if( i==sni->length )
			attroff( A_REVERSE );
		
		move( getPartY( sni, i ), getPartX( sni, i ) );
		addch( getPartType( sni, i ) );
	}
}


void bomb( const char *message )
{	endwin();
	puts( message );
	exit( 0 );
}

void forkserver( void );

int main( int argc, char **argv )
{
	char c;	
	int i;
	cn = connect( STDGAME );	

	//check if debug mode is enabled
	if( argc>1 )
	{
		if( strcmp( argv[1], "-debug")==0 )
			debugmode = TRUE;
	}
		
	signal( SIGCONT, verifyconnection);
	//check if an error occured during connect
	if( cn->status>0 )
	{	
		if( errno == ENOENT )
			forkserver();
		else
		{	debug( "Error code: %d\n", cn->status );	
			perror( "Error connecting to game" );
			return 1;
		}
	}
	gm = (struct game *) cn->shm_addr;

	//test connection to the listed server
	if( kill( gm->serverpid, 0 )==-1 && errno==ESRCH )
	{	//server is not available, but shared memory is still here
		debug( "WARNING: Server was not shutdown proporly last time\n" );
		//perform clean up
		shmctl( cn->shmid, 0, IPC_RMID );
		semctl( cn->semid, 0, IPC_RMID );
		//set the connection as invalid	
		cn->status = DISCONNECTED;
		forkserver();
		gm = (struct game *) cn->shm_addr;
	}

	signal( SIGINT, closeclient );

	//determine if server is full
	playerno =  gm->no_players;
	if( playerno>=MAX_PLAYERS )
	{	debug( "Server is full! Max=%d\n", MAX_PLAYERS );
		closeclient( SIGINT );
	}

	//request access to semaphore, then request a new Player addition
	requestAccessToShm( cn, 0 );
	pl = newPlayer( gm );
	releaseAccessToShm( cn , 0 );

	debug( "Running client\n" );

	//start drawing the game screen
	initscr();
	getmaxyx( stdscr, maxy, maxx );

	if( !has_colors() )
		bomb( "Terminal does not support colors\n" );
	if( start_color() != OK )
		bomb( "Error starting color mode\n" );

	//initialise player snake
	sn = &(pl->sn);

	//possible workaround: make the player head spawn just inside the ring, the rest of the body can be outside
	requestAccessToShm( cn, playerno+1 );	
	initialiseSnake( sn, STARTPOSITION , 1, INITIAL_LENGTH, RIGHT );
	pl->killed = FALSE;	
	pl->valid = TRUE;
	releaseAccessToShm( cn, playerno+1 );
	
	//tell the server that the process is ready
	kill( gm->serverpid, SIGCONT );

	init_pair( 1, COLOR_WHITE, COLOR_BLACK );
	init_pair( 2, COLOR_YELLOW, COLOR_BLACK );
	init_pair( 3, COLOR_RED, COLOR_BLACK );
	
	noecho();
	cbreak();
	nodelay( stdscr, TRUE );

	do {
		requestAccessToShm( cn, playerno+1 );

		if( pl->killed )
			closeclient( SIGINT );

		attron( COLOR_PAIR(1) );
		//does the screen need total refreshing?
		if( pl->refresh) 
		{
			clear();
			box( stdscr, 0, 0 );			
			pl->refresh = FALSE;		
		}		

		//draw the fruit found in the game on the screen
		move( gm->fruit.y, gm->fruit.x );
		addch( SNAKE_FRUIT );

		//draw border
		box( stdscr, 0, 0 );
		//draw all the snakes registered on the screen
		for( i=0; i<(gm->no_players); i++ )
		{	if( !(gm->players[i].killed) )	
			{	drawSnake( &(gm->players[i].sn) );
				attron( COLOR_PAIR( 1 ) );
			}
			else
				attron( COLOR_PAIR( 3 ) );
			
			//draw the HUD (Heads Up Display)
			move( (maxy-1)*( (int) i/4 ), 2+(20*(i%4)) );
			printw( "Player %d: %d (%d)", i+1, gm->players[i].score, gm->players[i].sn.actual_length );
		}

		move( 0, 0 );
		refresh();

		//perform input checking
		switch( tolower( c = getch() ) )
		{	case LEFT : if( getPartX( sn, 1 ) >= getPartX( sn, 0 ) )
						sn->dir = LEFT;
					break;
			case RIGHT: if( getPartX( sn, 1 ) <= getPartX( sn, 0 ) )
						sn->dir = RIGHT;
					break;
			case UP   : if( getPartY( sn, 1 ) >= getPartY( sn, 0 ) )
						sn->dir = UP;
					break;
			case DOWN : if( getPartY( sn, 1 ) <= getPartY( sn, 0 ) )	
						sn->dir = DOWN;
					break;
		}
	
		releaseAccessToShm( cn, playerno+1);
		napms( (int) GAME_SPEED/2 );
	} while( 1 );

	//never reached
	closeclient( SIGINT );
	endwin();
}

void forkserver()
{	if( fork()==0 )
	{	debug( "Server not found\n" );
		execl( SNAKES_SERVER, "", NULL );
	}
	else
	{	//if the connection hasnt already been verified, wait	
		if( cn->status>0 )
			pause();
		debug( "Reconnecting to new server\n" );
		cn = connect( STDGAME );
	}
}

void verifyconnection( int sig )
{	
	signal( SIGUSR1, verifyconnection );
	debug( "Client connection verified\n" );
	if( cn != NULL )
		cn->status = 0;
}

void closeclient( int sig )
{
	signal( SIGINT, closeclient );

	//close ncurses mode
	endwin();
	printf( "You have lost the game!\n" );
	debug( "This client is shutting down\n" );

	//dont attempt to access public data without using semaphores
	requestAccessToShm( cn,0 );
	gm->no_lplayers--;
	pl->killed = TRUE;
	pl->valid = FALSE;
	releaseAccessToShm( cn,0 );
	
	exit( 0 );
}
