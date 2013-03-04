#include <sys/types.h>

#define DISCONNECTED 1

#define SHMGET_ERROR 2
#define SHMAT_ERROR 4
#define SEMAPHORE_ERROR 8

#define Connection struct connection

//attempts a connection to a server with shared memory and returns a Connection structure
//with detailed information about the connection in return. NULL is returned on error
Connection *connect( key_t key );

//launches a server and its assosciated shared memory of specified size. Retruns a 
//Connection strucutre with all the relevant information. NULL is returned on error
Connection *launch( key_t key, int size, int sem_no, int flags );

//shuts down a given server connection. Returns 0 if everything shutdown well, -1 otherwise
int shutdown( Connection *cn );

//structure which contains important information about connections
struct connection
{	key_t key;	
	int shmid;
	int semid;
	int status;	//OK, DISCONNECTED or ERROR has occured (see above)
	void *shm_addr;
};

void printconnectioninfo( Connection *cn );

int updateSem( int semid, int op, int num );

#define releaseAccessToShm( CONNECTION, NUM )	updateSem( CONNECTION->semid, 1, NUM )

#define requestAccessToShm( CONNECTION, NUM )	updateSem( CONNECTION->semid, -1, NUM )
