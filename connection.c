#include "connection.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void printconnectioninfo( Connection *cn )
{	printf( "key: 0x%X\n", cn->key );
	printf( "shmid: %d\n", cn->shmid );
	printf( "semid: %d\n", cn->semid );
	printf( "status: %d\n", cn->status );
	printf( "shm_addr: %p\n", cn->shm_addr );
}

//shuts down a given server connection
int shutdown( Connection *cn )
{	int ret = 0;
	if( shmdt( cn->shm_addr )  == -1 )
		ret = -1;
	 
	if( shmctl( cn->shmid, 0, IPC_RMID ) == -1 )
		ret = -1;

	if( semctl( cn->semid, 0, IPC_RMID ) == -1 )
		ret = -1;

	return ret;
}

//launches a server, along with shared memory of a specified size
Connection *launch( key_t key, int size, int no_sem, int flags )
{	Connection *cn = (Connection *) malloc( sizeof( Connection ) );
	cn->status = 0;
	cn->key = key;

	if( (cn->shmid = shmget( key, size, 0666 | IPC_CREAT | flags ) ) == -1 )
		cn->status |= SHMGET_ERROR;

	if( (cn->shm_addr = shmat( cn->shmid, 0, 0 ) ) == NULL )
		cn->status |= SHMAT_ERROR;

	if( (cn->semid = semget( key, no_sem, 0666 | IPC_CREAT ) ) == -1 )
		cn->status |= SEMAPHORE_ERROR;

	return cn;
}

//updates a semaphore
int updateSem( int semid, int op, int num )
{	struct sembuf buf;
	buf.sem_op = op;
	buf.sem_flg = SEM_UNDO;
	buf.sem_num = num;
	if( semop( semid, &buf, 1 ) == -1 )
		return -1;
	else
		return 0;
}

//trys connecting to a specified server
Connection *connect( key_t key )
{	Connection *cn = (Connection *) malloc( sizeof( Connection ) );
	cn->status = 0;
	cn->key = key;

	if( ( cn->shmid = shmget( key, 0, 0666 ) ) == -1 )
		cn->status |= SHMGET_ERROR;

	if( ( cn->shm_addr = shmat( cn->shmid, 0, 0 ) ) == NULL )
		cn->status |= SHMAT_ERROR;
	
	if( ( cn->semid = semget( key, 0, 0 ) ) == -1 )
		cn->status |= SEMAPHORE_ERROR;

	return cn;
}
