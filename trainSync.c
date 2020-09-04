#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

struct station
{
    pthread_mutex_t lock;
    pthread_cond_t full;
    pthread_cond_t arrive;
	pthread_cond_t seated;
};

volatile int passengers;
volatile int train_seats;
volatile int seated_passengers;

void
station_load_train(struct station *station, int count)
{
    pthread_mutex_lock(&(station->lock));

    while (passengers > 0 && train_seats > 0)
    {
        pthread_cond_signal(&(station->arrive));
        train_seats--;
		pthread_cond_wait(&(station->seated), &(station->lock));
    }
    pthread_cond_signal(&(station->full));
	
    pthread_mutex_unlock(&(station->lock));
}

void station_wait_for_train(struct station *station)
{
    pthread_mutex_lock(&(station->lock));
    pthread_cond_wait(&(station->arrive), &(station->lock));

    passengers--;
    seated_passengers++;
	
	pthread_cond_signal(&(station->seated));

    pthread_mutex_unlock(&(station->lock));
}

void station_on_board(struct station *station)
{
    pthread_mutex_lock(&(station->lock));

    pthread_cond_wait(&(station->full), &(station->lock));
	
	printf("remaining passengers: %d, seated passengers: %d\n",passengers, seated_passengers);
    seated_passengers = 0;

    pthread_mutex_unlock(&(station->lock));
}

void station_init(struct station *station)
{
    pthread_mutex_init(&station->lock, NULL);
    pthread_cond_init(&station->full, NULL);
    pthread_cond_init(&station->arrive, NULL);
	pthread_cond_init(&station->seated, NULL);
}


void *passenger(void *args)
{
    struct station* station = (struct station *) args;
    station_wait_for_train(station);
}

void *train(void *args)
{
    struct station *station = (struct station *) args;
    station_load_train(station, train_seats);
}

int main(int argc, char **argv)
{
    struct station station;
    int i;
    station_init(&station);
    passengers = rand() % 500;
	printf("passengers in the station waiting: %d\n",passengers);
    pthread_t tid;
    pthread_attr_t attr;
    for (i = 0; i < passengers; i++)
    {
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, passenger, &station);
    }

    while (passengers > 0)
    {
        train_seats = rand() % 120;
		printf("train arrived with %d seats available\n",train_seats);
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, train, &station);
        station_on_board(&station);
    }
}