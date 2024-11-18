#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include "crackme.h"

#define ASCII_START 0 // Don't touch.
#define ASCII_END 127 // Don't touch.
#define WORK_TAG 0
#define FOUND_TAG 1
#define EXIT_TAG 2
#define REQUEST_WORK_TAG 3
#define DONE_TAG 4

// Calculating block size for a process to handle.
unsigned long calculate_block(unsigned long combinations, int processes)
{
	// By dividing combinations with amount processes.
	unsigned long block = combinations / processes;

	return block;
}

struct QueueWork
{
	// Start block range
	unsigned long start;

	// End block range
	unsigned long end;

	// Worker that got work. Keeping track on which worker rank has the job
	int worker;

	// Status of the work
	int done;

	// Size of the queue.
	int size;
};

typedef struct QueueWork QueueWork_t;

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Wrong usage: %s <size of password to generate>\n", argv[0]);
		exit(1);
	}
	int sizePass = atoi(argv[1]);

	// Process rank
	int world_rank;

	// Number of processes
	int world_size;

	// Intializes MPI (For starting it up)
	MPI_Init(&argc, &argv); // Passes inn argc and argv.

	// This gets the rank among all processes
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	// This gets the amount of processes
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Great to have for debugging errors.
	MPI_Status status; // MPI_SOURCE, MPI_TAG and MPI_ERROR

	// Allocating memory for my text string guess.
	char *guess = (char *)malloc(sizeof(char) * sizePass + 1);
	if (guess == NULL)
	{
		fprintf(stderr, "No memory allocated for guess\n");
		return 1;
	}
	// Null terminate string.
	guess[sizePass] = '\0';

	for (int i = 0; i < sizePass; i++)
		guess[i] = ASCII_START;

	// Calculating amount guess combinations that exist.
	unsigned long combinations = pow((ASCII_END + 1), sizePass);
	// printf("combinations that exist: %ld\n", combinations);
	// Calculating block size for the guess combination. For the size that can split each process in for fair distribution. -1 we dont count master in.
	unsigned long block_size = calculate_block(combinations, (world_size - 1));

	// This is my master process (world_rank 0) that generates and distributes work, rest above rank 0 are the workers that works for the master.
	if (!world_rank)
	{
		// Calculating start block and end block. Each process will have their own start block and end block to check guess.
		unsigned long start_block = 0;
		// Amount workers.
		int working_processes = world_size - 1;

		// Allocating memory for my work queue array.
		QueueWork_t *queue = (QueueWork_t *)malloc(sizeof(QueueWork_t) * world_size);
		if (queue == NULL)
		{
			fprintf(stderr, "No memory allocated for queue\n");
			return 1;
		}
		// Intitialize queue size as 0
		queue->size = 0;

		// Weights used for making uneven distribution. The weights must allways sum to 100
		double weights[working_processes];
		// Switch case based on amount workers.
		switch ((working_processes))
		{
		case 1:
			weights[0] = 1;
			break;
		case 2:
			weights[0] = 0.40;
			weights[1] = 0.60;
			break;

		case 3:
			weights[0] = 0.25;
			weights[1] = 0.30;
			weights[2] = 0.45;

			break;

		case 4:
			weights[0] = 0.10;
			weights[1] = 0.20;
			weights[2] = 0.30;
			weights[3] = 0.40;
			break;

		case 5:
			weights[0] = 0.08;
			weights[1] = 0.10;
			weights[2] = 0.20;
			weights[3] = 0.28;
			weights[4] = 0.34;

			break;

		case 6:
			weights[0] = 0.04;
			weights[1] = 0.06;
			weights[2] = 0.10;
			weights[3] = 0.19;
			weights[4] = 0.25;
			weights[5] = 0.36;

			break;
		case 7:
			weights[0] = 0.02;
			weights[1] = 0.03;
			weights[2] = 0.06;
			weights[3] = 0.12;
			weights[4] = 0.19;
			weights[5] = 0.28;
			weights[6] = 0.30;
			break;
		}

		// Creating work and putting it into work queue. Work creation loop.
		while (queue->size < working_processes)
		{

			// Multiply the weight with combination as the block size we want for the next block size to increment with.
			unsigned long next_block_size = (unsigned long)(combinations * weights[queue->size]);

			// Making end range numbers based on the growing block size
			unsigned long next_block = start_block + next_block_size;

			// Last rank/process has the last block which is max combinations.
			// To avoid not having all possibilities due to floating values calculation on block size when world size(process) is odd numbers or when multiplying with float values. (Value not rounding up)
			if (queue->size == working_processes - 1) // Since array starts from 0, -1
			{
				// Calculating reminder to add for the last jobb in queue.
				unsigned long remainder = combinations % next_block;

				// Adding the remainder on the next block, which is the last block that gets assigned to a worker.
				next_block += remainder;
				if (remainder)
					printf("Remainder: %lu\n", remainder);
			}

			// Never know what can happen, this is nice fail safe mechanism for setting the max value.
			// Shouldnt be nescarry since using weights and the sums are 100. Just another layer of fail safe.
			if (next_block > combinations)
			{
				next_block = combinations;
			}

			// Putting start,end worker and done status in queue array.
			// Where the index i use is based of the queue size.
			queue[queue->size].start = start_block;
			queue[queue->size].end = next_block;
			// Marking the job for that index as unassigned. -1 means not assigned yet to any worker_rank
			queue[queue->size].worker = -1;
			// Marking the job as not done. 1 means done.
			queue[queue->size].done = 0;
			// Incrementing queue size.
			queue->size++;
			printf("queuesize: %d, startb: %lu, nextb: %lu, DIFF: %lu > blocksize/4: %lu, combo: %lu \n", queue->size, start_block, next_block, (next_block - start_block), (block_size / 4), combinations);

			// Setting a new start block value for next block.
			start_block = next_block;
		}
		// Count when worker is completly done with their range.
		int workers_done = 0;
		// Here master distributes work to worker and steals work.
		while (workers_done < working_processes)
		{
			// Receiving messages from the worker with any tags. For deciding what to do next.
			// This will receive tags such as request work or found.
			MPI_Recv(guess, (sizePass + 1), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			// Which rank we received message from.
			int worker_rank = status.MPI_SOURCE;
			// Which tag the rank brings in here.
			int worker_tag = status.MPI_TAG;

			// Checking the tag to decide outcome
			// If the status received is found tag means correct guess is found by a worker.
			if (worker_tag == FOUND_TAG)
			{
				printf("Master: Received password from worker %d: %s\n", worker_rank, guess);

				// Appending the correct guess to the solution file.
				FILE *fp = fopen("solution.txt", "a");
				fprintf(fp, "PAR Guess found: %s with size: %d\n", guess, sizePass);
				fclose(fp);

				// After getting the guess from the worker now time to send exit signal to other workers.
				// Loop to iter through all workers.
				for (int i = 1; i < world_size; i++)
				{

					printf("Master: Sent exit tag signal to worker %d\n", i);
					// Sending signal to the workers to stop working.
					MPI_Send(NULL, 0, MPI_BYTE, i, EXIT_TAG, MPI_COMM_WORLD);
					queue->size--;
					workers_done++;
				}

				break;
			}

			// If the status received is request work tag means a worker wants work.
			// This case is true in beginning when master distributes first jobs from the queue to worker.
			// And this case will be also true when a worker finishes it's job and steal job from another worker which isn't done.
			else if (worker_tag == REQUEST_WORK_TAG)
			{
				// First thing is to check for undone work and unassigned from the queue
				// And retrieve the index where it reside in the work queue.
				int index = -1;
				for (int i = 0; i < queue->size; i++)
				{
					// Checking for work that isn't done and that is unassigned.
					if (queue[i].done != 1 && queue[i].worker == -1)
					{
						// When found the block in the work queue assign it to index.
						index = i;
						break;
					}
				}

				// Checking that the index is correct. If correct we assign that index from the work queue to worker.
				if (index > -1)
				{
					// Assigning the work to the worker rank. Now it's not unassigned!
					queue[index].worker = worker_rank;

					// Sending the work block to worker that requested work.
					printf("Master gives worker %d, start_block: %lu, next_block: %lu\n", worker_rank, queue[index].start, queue[index].end);
					MPI_Send(&queue[index].start, 1, MPI_UNSIGNED_LONG, worker_rank, WORK_TAG, MPI_COMM_WORLD);
					MPI_Send(&queue[index].end, 1, MPI_UNSIGNED_LONG, worker_rank, WORK_TAG, MPI_COMM_WORLD);
				}
				// If undone work and no unassigned work left. Worker tries to steal from another worker.
				else
				{
					// To store slow worker rank
					int slow_worker_rank = -1;
					// Time to steal work. Need to get the index from the work queue.
					for (int i = 0; i < queue->size; i++)
					{
						// Steal work that is assigned to a worker and that isn't the work itself had (Don't steal same job i completed to my self again).
						// And difference is bigger then the block size. Don't wanna steal a work that is small range.
						if (queue[i].worker > 0 && queue[i].worker != worker_rank && queue[i].done != 1 && (queue[i].end - queue[i].start) > block_size / 4)
						{
							// Getting the rank of the slow worker.
							slow_worker_rank = queue[i].worker;

							// Found the block in work queue and assigning it to index.
							index = i;

							break;
						}
					}
					// Rank only above 0 is belonging to workers.
					if (slow_worker_rank > 0)
					{
						// Stealing the work from the slow worker.

						// Slow worker start. Instead of stealing the whole start of the slow worker, we steal the half way of it.
						// That way the worker that steals, he begins in the middle of the slow worker work.
						unsigned long slow_worker_start = queue[index].start + (queue[index].end - queue[index].start) / 2;
						// Slow worker end
						unsigned long slow_worker_end = queue[index].end;

						// Setting the start in queue to the slow worker end.
						// This makes the next steal go next. Without this we keep steal same block due to the loop above starts from 0 every time worker calls steal.
						queue[index].start = slow_worker_end;

						// Sending the work block to worker that want to steal.
						printf("Master steals work from worker: %d, and gives to worker: %d, start_block: %lu, next_block: %lu\n", slow_worker_rank, worker_rank, slow_worker_start, slow_worker_end);
						MPI_Send(&slow_worker_start, 1, MPI_UNSIGNED_LONG, worker_rank, WORK_TAG, MPI_COMM_WORLD);
						MPI_Send(&slow_worker_end, 1, MPI_UNSIGNED_LONG, worker_rank, WORK_TAG, MPI_COMM_WORLD);
					}
					// All work is assigned and nothing more to steal since all work done.
					// This case won't be executed every run time, since our program finds the guess and terminates before it gets here.
					else
					{
						// Find the worker index to mark it as done and send exit signal to it.
						int done_index;
						for (int i = 0; i < queue->size; i++)
						{ // Finding the right worker by matching it with the worker rank and the assigned work which worker it was assigned to.
							if (queue[i].worker == worker_rank && queue[i].done != 1)
							{
								done_index = i;
								break;
							}
						}

						// Marking it done in the work queue.
						queue[done_index].done = 1;
						printf("Worker: %d, is done with: %lu-%lu\n", worker_rank, queue[done_index].start, queue[done_index].end);
						// Sending exit singnal to that worker.
						printf("Master: Sent exit tag signal to worker %d\n", worker_rank);
						MPI_Send(NULL, 0, MPI_BYTE, worker_rank, EXIT_TAG, MPI_COMM_WORLD);
						queue->size--;
						workers_done++;
					}
				}
			}
			// If worker is done with his range, mark the range he worked in the queue as done.
			else if (worker_tag == DONE_TAG)
			{
				// Find the worker index to mark it as done and send exit signal to it.
				int done_index;
				for (int i = 0; i < queue->size; i++)
				{ // Finding the right worker by matching it with the worker rank and the assigned work which worker it was assigned to.
					if (queue[i].worker == worker_rank && queue[i].done != 1)
					{
						done_index = i;
						break;
					}
				}

				queue[done_index].done = 1;
				printf("Worker: %d, is done with: %lu-%lu. Worker finds new work\n", worker_rank, queue[done_index].start, queue[done_index].end);
			}
		}
		// Free up in the end.
		free(queue);
	}
	else
	{
		// Received start and end block from master.
		unsigned long start_blockr;
		unsigned long end_blockr;

		// Flag for nonblock prob.
		int flag;
		// Found for when the right guess is found.
		int found = 0;
		int master_tag;

		while (!found)
		{
			// Worker request work from master first to start.
			MPI_Send(NULL, 0, MPI_BYTE, 0, REQUEST_WORK_TAG, MPI_COMM_WORLD);

			// This receive will handle based on which tag it receives and can receive 2 types, work or exit from master.
			// Receive the first start work block range. Means there might exist job, depends on the tag status.
			MPI_Recv(&start_blockr, 1, MPI_UNSIGNED_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			master_tag = status.MPI_TAG;

			// If the master sent work, worker receives the end block range it will begin and work on it.
			if (master_tag == WORK_TAG)
			{
				// Receiving work from master on block range end for the process.
				MPI_Recv(&end_blockr, 1, MPI_UNSIGNED_LONG, 0, WORK_TAG, MPI_COMM_WORLD, &status);
				printf("Worker: %d, received start_block: %lu, end_block: %lu \n", world_rank, start_blockr, end_blockr);

				// // int count;
				// // MPI_Get_count(&status, MPI_UNSIGNED_LONG, &count);
				// // printf("count: %d\n", count);

				// Index value inside a block range.
				unsigned long index;
				for (unsigned long i = start_blockr; i < end_blockr; i++)
				{

					// Waiting for the exiting process tag. Flag will be true when message is waiting on be received. Non blocking method.
					MPI_Iprobe(0, EXIT_TAG, MPI_COMM_WORLD, &flag, &status);
					// Break out of the processes so it doesn't continue looping when the guess is found by an other process (worker).
					if (flag)
					{
						// Receive the exit tag msg from master.
						MPI_Recv(NULL, 0, MPI_BYTE, 0, EXIT_TAG, MPI_COMM_WORLD, &status);
						printf("Worker exit: %d\n", world_rank);
						// int count;
						// MPI_Get_count(&status, MPI_INT, &count);
						// printf("count: %d\n", count);

						// Found
						found = 1;
						break;
					}

					// Index value where i is at in the block range.
					index = i;

					// Loop which increases the numeric value for char* from right to left.
					// Where each col will be the guess string index.
					for (int col = 0; col < sizePass; col++)
					{
						// Calculating the "column" the string we want to generate a numeric value for (sizePass - col - 1)
						// And also the character to put in the guess (ASCII_START + (index % (ASCII_END + 1) by caluclating the remainder of index which gives us the character to use from the position in ASCII range.
						// Example if ASCII start is 33(!) is 126(~) and we are on index 0 we get: 0 % (ASCII_END +1) = 0 and the char is then "!".
						guess[sizePass - col - 1] = (index % (ASCII_END + 1));
						// printf("index: %lu, col: %d,char: %c \n", index, col, guess[sizePass - col - 1]);

						// Divding the index with the ASCII range to find the next index value.
						// This will get the next index for the character, moving to next digit number.
						index = index / (ASCII_END + 1);
						// printf("division index: %lu\n", index);
					}

					// Checking the guess.
					if (p(sizePass, guess) == 0)
					{
						printf("Success worker %d, in: %lu-%lu, found the guess: %s\n", world_rank, start_blockr, end_blockr, guess);
						// When guess found sending it to master for writing it to file.
						MPI_Send(guess, (sizePass + 1), MPI_CHAR, 0, FOUND_TAG, MPI_COMM_WORLD);

						// Found exit out of the loop.
						found = 1;
						break;
					}
				}
				// We get here when worker has done it range and we want, to inform master that this part is done.
				MPI_Send(NULL, 0, MPI_BYTE, 0, DONE_TAG, MPI_COMM_WORLD);
			}
			// If master sends exit tag to worker means to exit.
			else if (status.MPI_TAG == EXIT_TAG)
			{
				found = 1;
				break;
			}
		}
	}

	// Free up in the end.
	free(guess);

	// Cleans and shuts down MPI.
	MPI_Finalize();

	return 0;
}
