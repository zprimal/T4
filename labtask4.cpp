// Lab Task
// Run with 4 replicants and 4 clients
// Initially, it works according to the following:
// - Client read or write data from a randomly selected replicant
//
// - Client's Operations :
//   # Client 1 : read x, x=x+5, write x, read y, y=y+10, write y
//   # Client 2 : read x, read y, x=x+y, write x, y=y*2, write y
//   # Client 3 : read y, y=y+20, write y, read y, y=y/2, write y
//   # Client 4 : read y, read x, y=y-x, write y, x=0, write x

#include <mpi.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

#define REQUEST_TAG		0
#define REPLY_TAG		1
#define UPDATE_X		1
#define UPDATE_Y		2

using namespace std;

// The struct that holds the request message
typedef struct {
	char op;
	char data;
	int value;
} requestData;


int main (int argc, char* argv[]) {
    MPI_Init (NULL, NULL);

    int world_size;
    MPI_Comm_size (MPI_COMM_WORLD, &world_size);
    int rank;
    MPI_Status stat;
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	// Line 41 to 49 is to create a new data type to be used by MPI_Send/MPI_Recv
	int structCount = 3;				// struct has 3 members
	int structLength[3] = {1, 1, 1};	// the number of element for each member

	MPI_Aint structOffset[3] = {0, 1, 2};	// memory address offset for each member
	MPI_Datatype structDataType[3] = {MPI_CHAR, MPI_CHAR, MPI_INT};	// The basic data type of each member

	MPI_Datatype REQ_TYPE;	// The name of our new data type
	MPI_Type_struct (structCount, structLength, structOffset, structDataType, &REQ_TYPE);	// create the new data type
	MPI_Type_commit (&REQ_TYPE);

	// Process rank 0 to 3 are replicants
	if (rank < 4) {
		MPI_Request req;
		int x = 0, y = 0;
		requestData request;
		int reply;
		int source;
		int flag, count;
		MPI_Barrier (MPI_COMM_WORLD);
		while (1) {
			MPI_Irecv (&request, 1, REQ_TYPE, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &req);
			flag = 0;
			count = 0;
			do {
				usleep (50000);	// wait for 0.05 second before retry
				count++;
				// Cancel receive and quit after more than 0.5 second of no activity
				if (count > 10) {
					MPI_Cancel (&req);
					break;
				}
				// Non-blocking test to check if a reply has arrived
				// if got reply, flag = true
				MPI_Test (&req, &flag, &stat);
			} while (!flag);

			if (flag) {
				// client 1 is process rank 4, etc.
				// Request recieved
				source = stat.MPI_SOURCE;


				if (request.op == 'R') {
					if (request.data == 'x') {
						reply = x;
					} else if (request.data == 'y') {
						reply = y;
					}
					MPI_Send (&reply, 1, MPI_INT, stat.MPI_SOURCE, REPLY_TAG, MPI_COMM_WORLD);
				}

				if (request.op == 'W') {
					request.op = 'U';
					if (request.data == 'x' && rank == UPDATE_X) {
						// If this replicants is the update replicants
					} else if (request.data == 'y' && rank == UPDATE_Y) {
						// If this replicants is the update replicants
					} else if (request.data == 'x') {
						MPI_Send (&request, 1, REQ_TYPE, UPDATE_X, REQUEST_TAG, MPI_COMM_WORLD);
					} else if (request.data == 'y') {
						MPI_Send (&request, 1, REQ_TYPE, UPDATE_Y, REQUEST_TAG, MPI_COMM_WORLD);
					}
				}

				if (request.op == 'U') {
					if (request.data == 'x') {
						x = request.value;
						reply = x;
						if (rank == UPDATE_X) {
							//If is X primary replicant send update command to other replicants
							if (source < 4) {
								cout << "\033[31m[R" << UPDATE_X+1 << "] Updating replicant minors (REQ from [R" << source+1 << "])" << endl;
							} else {
								cout << "\033[31m[R" << UPDATE_X+1 << "] Updating replicant minors (REQ from [C" << source+1 << "])" << endl;
							}

							MPI_Send (&request, 1, REQ_TYPE, 0, REQUEST_TAG, MPI_COMM_WORLD);
							MPI_Send (&request, 1, REQ_TYPE, 2, REQUEST_TAG, MPI_COMM_WORLD);
							MPI_Send (&request, 1, REQ_TYPE, 3, REQUEST_TAG, MPI_COMM_WORLD);
						}
					} else if (request.data == 'y') {
						y = request.value;
						reply = y;
						if (rank == UPDATE_Y) {
							//If is Y primary replicant send update command to other replicants
							if (source < 4) {
								cout << "\033[31m[R" << UPDATE_Y+1 << "] Updating replicant minors (REQ from [R" << source+1 << "])" << endl;
							} else {
								cout << "\033[31m[R" << UPDATE_Y+1 << "] Updating replicant minors (REQ from [C" << source+1 << "])" << endl;
							}

							MPI_Send (&request, 1, REQ_TYPE, 0, REQUEST_TAG, MPI_COMM_WORLD);
							MPI_Send (&request, 1, REQ_TYPE, 1, REQUEST_TAG, MPI_COMM_WORLD);
							MPI_Send (&request, 1, REQ_TYPE, 3, REQUEST_TAG, MPI_COMM_WORLD);
						}
					}
				}

				//If it is not from a replicant server..
				if (source > 3) {
					for (int i = 0; i < 4; i++) {
						if (i != rank) {
							// Don't send to yourself
							MPI_Send (&request, 1, REQ_TYPE, i, REQUEST_TAG, MPI_COMM_WORLD);
							// cout << "Updating replicant " << i << endl;
						}
					}
					// cout << "Replying to client " << source << endl;
					MPI_Send (&reply, 1, MPI_INT, stat.MPI_SOURCE, REPLY_TAG, MPI_COMM_WORLD);
				} else {
					// cout << "Updated by replicant " << source << endl;
				}


				// MPI_Send (&reply, 1, MPI_INT, stat.MPI_SOURCE, REPLY_TAG, MPI_COMM_WORLD);
			}

			// replicant print out its x and y value before terminate
			else {
				cout << "\033[32m" << "[Replicant " << rank+1 << "] The final value is x = " << x << " and y = " << y << endl;
				break;
			}
		}

	}

	// Client 1
	else if (rank == 4) {
		MPI_Barrier (MPI_COMM_WORLD);	//make sure replicants are ready before clients start
		int d_store;
		requestData request ;
		int reply;
		int local_x, local_y;

		srand(rank);

		// Read x
		request.op = 'R';
		request.data = 'x';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C1] Send read request of x to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C1] Receive x = " << reply << " from replicant " << d_store+1 << endl;
		local_x = reply;

		// Write x
		local_x = local_x + 5;
		request.op = 'W';
		request.data = 'x';
		request.value = local_x;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C1] Send write request of x to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C1] Receive x = " << reply << " from replicant " << d_store+1 << endl;

		// Read y
		request.op = 'R';
		request.data = 'y';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C1] Send read request of y to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C1] Receive y = " << reply << " from replicant " << d_store+1 << endl;
		local_y = reply;

		// Write y
		local_y = local_y + 10;
		request.op = 'W';
		request.data = 'y';
		request.value = local_y;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C1] Send write request of y to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C1] Receive y = " << reply << " from replicant " << d_store+1 << endl;

	}

	// Client 2
	else if (rank == 5) {
		MPI_Barrier (MPI_COMM_WORLD);
		int d_store;
		requestData request ;
		int reply;
		int local_x, local_y;

		srand(rank);

		// Read x
		request.op = 'R';
		request.data = 'x';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C2] Send read request of x to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C2] Receive x = " << reply << " from replicant " << d_store+1 << endl;
		local_x = reply;

		// Read y
		request.op = 'R';
		request.data = 'y';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C2] Send read request of y to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C2] Receive y = " << reply << " from replicant " << d_store+1 << endl;
		local_y = reply;

		// Write x
		local_x = local_x + local_y;
		request.op = 'W';
		request.data = 'x';
		request.value = local_x;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C2] Send write request of x to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C2] Receive x = " << reply << " from replicant " << d_store+1 << endl;

		// Write y
		local_y = local_y * 2;
		request.op = 'W';
		request.data = 'y';
		request.value = local_y;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C2] Send write request of y to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C2] Receive y = " << reply << " from replicant " << d_store+1 << endl;

	}

	// Client 3
	else if (rank == 6) {
		MPI_Barrier (MPI_COMM_WORLD);
		int d_store;
		requestData request ;
		int reply;
		int local_x, local_y;

		srand(rank);

		// Read y
		request.op = 'R';
		request.data = 'y';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C3] Send read request of y to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C3] Receive y = " << reply << " from replicant " << d_store << endl;
		local_y = reply;

		// Write y
		local_y = local_y + 20;
		request.op = 'W';
		request.data = 'y';
		request.value = local_y;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C3] Send write request of y to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C3] Receive y = " << reply << " from replicant " << d_store << endl;

		// Read y
		request.op = 'R';
		request.data = 'y';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C3] Send read request of y to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C3] Receive y = " << reply << " from replicant " << d_store << endl;
		local_y = reply;

		// Write y
		local_y = local_y / 2;
		request.op = 'W';
		request.data = 'y';
		request.value = local_y;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C3] Send write request of y to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C3] Receive y = " << reply << " from replicant " << d_store << endl;

	}

	// Client 4
	else if (rank == 7) {
		MPI_Barrier (MPI_COMM_WORLD);
		int d_store;
		requestData request ;
		int reply;
		int local_x, local_y;

		srand(rank);

		// Read y
		request.op = 'R';
		request.data = 'y';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C4] Send read request of y to replicant " << d_store+1 << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C4] Receive y = " << reply << " from replicant " << d_store+1 << endl;
		local_y = reply;

		// Read x
		request.op = 'R';
		request.data = 'x';
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C4] Send read request of x to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C4] Receive x = " << reply << " from replicant " << d_store << endl;
		local_x = reply;

		// Write y
		local_y = local_y - local_x;
		request.op = 'W';
		request.data = 'y';
		request.value = local_y;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C4] Send write request of y to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C4] Receive y = " << reply << " from replicant " << d_store << endl;

		// Write x
		local_x = 0;
		request.op = 'W';
		request.data = 'x';
		request.value = local_x;
		d_store = rand()%4;
		MPI_Send (&request, 1, REQ_TYPE, d_store, REQUEST_TAG, MPI_COMM_WORLD);
		cout << "\033[3" << rank << "m" << "[C4] Send write request of x to replicant " << d_store << endl;
		MPI_Recv (&reply, 1, MPI_INT, d_store, REPLY_TAG, MPI_COMM_WORLD, &stat);
		cout << "\033[3" << rank << "m" << "[C4] Receive x = " << reply << " from replicant " << d_store << endl;

	}
	else
		{}

	cout << "\033[0m";
    // Finalize the MPI environment.
    MPI_Finalize();
}
