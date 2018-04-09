#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "queue.h"
#include "p2.h"

#define BILLION 1E9

int dispatch(int last);
void print_time();

pthread_mutex_t track, pq;
pthread_cond_t ready, train_convar, crossed;

int start_bool = 0;
int number_of_trains;
struct timespec start, stop;
double accum;
Node* pq_east;
Node* pq_west;

// return character array for more readable output
char *direction(char s){
    if(s == 'e'|| s == 'E'){
        return "East";
    }
    else{
        return "West";
    }
}

void *Train(void *arg){
    t* train;
    train = (t*)arg;

    /* Busy wait to synchronize the starting (loading) time of all trains. This implimentation was chosen over a condition variable
        because a single train_convar requires locking and unlocking after the cond_wait, meaning the threads don't actually
        start at the same time. */
    //sleep(1);
    while(!start_bool){}
    usleep((useconds_t)train->loading_time * 100000);

    print_time();
    printf("Train %2d is ready to go %4s\n", train->train_number, direction(train->direction));

    pthread_mutex_lock(&pq);
    if(train->direction == 'E' || train->direction == 'e') {
        push(&pq_east, train);
    } else{
        push(&pq_west, train);
    }
    pthread_mutex_unlock(&pq);

    pthread_cond_signal(&ready);
    pthread_cond_wait(train->train_convar, &track);
    print_time();
    printf("Train %2d is ON the main track going %4s\n", train->train_number, direction(train->direction));
    usleep((useconds_t)train->crossing_time * 100000);
    print_time();
    printf("Train %2d is OFF the main track after going %4s\n", train->train_number, direction(train->direction));
    number_of_trains--;
    pthread_mutex_unlock(&track);
    pthread_cond_signal(&crossed);
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: Add path to text file\n");
        return 1;
    }

    FILE *fp, *fp_lines;
    fp = fopen(argv[1], "r");
    fp_lines = fopen(argv[1], "r");

    if(fp == NULL){
        perror("File opening error");
        return 1;
    }

    pthread_mutex_init(&track,NULL);
    pthread_mutex_init(&pq,NULL);

    // https://stackoverflow.com/questions/12733105/c-function-that-counts-lines-in-file
    int ch;
    number_of_trains = 0;
    while(!feof(fp_lines)) {
        ch = fgetc(fp_lines);
        if(ch == '\n') {
            number_of_trains++;
        }
    }

    pthread_t threads[number_of_trains];
    pthread_cond_t conditions[number_of_trains];
    pthread_cond_init(&train_convar, NULL);
    pthread_cond_init(&crossed, NULL);
    int i;
    for(i=0; i<number_of_trains;i++){
        pthread_cond_init(&conditions[i], NULL);
    }

    t *trains;
    trains = malloc(number_of_trains * sizeof(*trains));

    int train_no = 0;
    char direction[1], cross[10], load[10]; // figure out this char stuff if I have time lol
    while (fscanf(fp,"%s %s %s", direction, load, cross)==3) {
        int loading_time = atoi(load);
        trains[train_no].train_number = train_no;
        trains[train_no].direction = direction[0];
        trains[train_no].loading_time = loading_time;
        trains[train_no].crossing_time = atoi(cross);
        trains[train_no].train_convar = &conditions[train_no];
        train_no++;
    }

    int t;
    for(t=0; t<number_of_trains; t++){
        if(pthread_create(&threads[t], NULL, Train, (void *)&trains[t])) {
            perror("ERROR; pthread_create()");
        }
    }

    // quick sleep to ensure all threads are created and waiting before broadcasting anything
    sleep(2);

    start_bool = 1;
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
        perror( "clock gettime" );
        exit( EXIT_FAILURE );
    }

    /* GUIDE:
     * Send east train = 0
     * Send west train = 1
     * */
    int last = -1;
    while(number_of_trains != 0){

        // wait for signal from thread
        pthread_mutex_lock(&track);

        if (isEmpty(&pq_east) && isEmpty(&pq_west)) {
            pthread_cond_wait(&ready, &track);
        }

        last = dispatch(last);
        usleep(.0001 * 1000000);
        if (last == 0){
            pthread_cond_signal(peek(&pq_east)->train_convar);
            pthread_mutex_lock(&pq);
            pop(&pq_east);
            pthread_mutex_unlock(&pq);
        } else if (last == 1) {
            pthread_cond_signal(peek(&pq_west)->train_convar);
	         pthread_mutex_lock(&pq);
            pop(&pq_west);
            pthread_mutex_unlock(&pq);
        }

        // wait for signal from crossed thread
        pthread_cond_wait(&crossed, &track);
        pthread_mutex_unlock(&track);
    }

    fclose(fp);

    pthread_exit(0);
}

int dispatch(int last){
    if(!isEmpty(&pq_west) && !isEmpty(&pq_east)){
        // send higher priority train
        if(peek_priority(&pq_east) > peek_priority(&pq_west)){
            return 0;
        } else if (peek_priority(&pq_east) < peek_priority(&pq_west)){
            return 1;
        }

        // if they have equal priorities, send depending on which train went last (East is none previous)
        if(last == 1 || last == -1){
            return 0;
        } else {
            return 1;
        }
    }
    else if (!isEmpty(&pq_east) && isEmpty(&pq_west)){
        // dispatch the next east train
        return 0;
    }
    else if (isEmpty(&pq_east) && !isEmpty(&pq_west)){
        // dispatch the next west train
        return 1;
    }
    return -2;
}

void print_time(){
   if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
   }
   accum = ( stop.tv_sec - start.tv_sec ) + ( stop.tv_nsec - start.tv_nsec )/ BILLION;
   // my hacky way of getting minutes out of the seconds double
   double x = accum*10;
   x = (int)x%60;
   x = (float)x;
   printf("%02d:%02d:%04.1f ", (int)accum/3600, (int)accum/60, x/10);
}
