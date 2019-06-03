#include <iostream>
#include <stdio.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <vector>

#define NUM_PLAYERS 3
#define NUM_CARDS 52
#define NUM_ROUNDS 3

using namespace std;

//Thread variables
pthread_mutex_t mutex_deck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_deck = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_dealer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_win = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_players = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_ready = PTHREAD_COND_INITIALIZER;
pthread_t player_thread[NUM_PLAYERS];
pthread_t dealer_thread;

//Other variables 
ofstream logFile;
bool win, done;
int current_round, turn, seed;
vector<int> deck;
vector< pair<int,int> > hands;

//Functions
void buildGame(char *argv[]);
void shuffleDeck();
void makeDeck();
void dealCards();
void useDeck();
void *dealerThreadFunction();
void *playerThreadFunction(void * id);
void run();
void playRound();
void useDeck(void * id, pair<int, int> hand);
void printDeck();
void discardPlayerHands();


int main(int argc, char *argv[]) {

	// Open a file
    logFile.open(argv[2]);

    // Seed the rand() function with given argument
    srand(atoi(argv[1]));

    // Run the game
    run();

    // Exit
    return 0;
}

// Shuffle the deck using the given seed
void shuffleDeck(){

	logFile << "DEALER: shuffle" << endl;

    for(int i = 0; i < NUM_CARDS; i++){
        int randPos = i + (rand() % (NUM_CARDS - i));
        int temp = deck[i];
        deck[i] = deck[randPos]; 
        deck[randPos] = temp;
    }
}

// Create the deck
void makeDeck(){

    int card_value = 1;
    for(int i = 0; i < NUM_CARDS; i++) {
        deck.push_back(card_value);
        card_value++;
        if(card_value == 14) {
            card_value = 1;
        }
    }
}

// Print the deck
void printDeck(){

	cout << "DECK: ";
	for (vector<int>::iterator it = deck.begin() ; it != deck.end(); ++it)
		cout << *it << " ";
	cout << endl << endl;
}

// Dealer thread function
void *dealerThreadFunction(void * id){

	useDeck(id, make_pair(0,0));

	// Tell the player threads that the round is ready to be played
	done = true;
	pthread_cond_broadcast(&condition_ready);
	
	// Lock the dealer until a victory
	pthread_mutex_lock(&mutex_dealer);
	// Wait for a victory
	while(win == false)
		pthread_cond_wait(&condition_win, &mutex_dealer);
	// Unlock the dealer to finish 
	pthread_mutex_unlock(&mutex_dealer);


    pthread_exit(NULL);
}

// Player thread function
void *playerThreadFunction(void * id){

	// Lock the players 
	pthread_mutex_lock(&mutex_players);
	// Wait for dealer to be done
	while(done == false)
		pthread_cond_wait(&condition_ready, &mutex_players);
	// Unlock the players to play the game
	pthread_mutex_unlock(&mutex_players);

	long player = (long)id;
	pair<int, int> hand = make_pair(0,0);

	// While the round isn't over
	while(win == false){

		// Give the players their appropriate hand
		if(current_round == 0){
			if(player == 1)
				hand = hands[0];
			else if(player == 2)
				hand = hands[1];
			else if(player == 3)
				hand = hands[2];
		}
		else if(current_round == 1){
			if(player == 2)
				hand = hands[0];
			else if(player == 3)
				hand = hands[1];
			else if(player == 1)
				hand = hands[2];
		}
		else if(current_round == 2){
			if(player == 3) 
				hand = hands[0];
			else if(player == 1)
				hand = hands[1];
			else if(player == 2)
				hand = hands[2];
		}

		// Lock the deck
		pthread_mutex_lock(&mutex_deck);

		// If the round isn't over and it is their turn, give the player access to the deck
		if(win == false && player == turn)
			useDeck(id, hand);

		// If the round isn't over and its not their turn, make the player wait
		while(win == false && player != turn)
			pthread_cond_wait(&condition_deck, &mutex_deck);

		// If the round is over the player tells us round is over
		if(win == true)
			logFile << "PLAYER " << player << ": round completed" << endl;

		// Unlock the deck
		pthread_mutex_unlock(&mutex_deck);
	}

	pthread_exit(NULL);
}

void run() {

	// Set the game variables
    current_round = 0;
    win = false;
    done = false;

    logFile << "Starting Game" << endl;

    // Initialize empty hands
    for(int i =0; i < NUM_PLAYERS; i++){
    	hands.push_back(make_pair(0,0));
    }

    // Create the deck of cards
    makeDeck();

    // Run the game for the desired number of rounds
    while(current_round < NUM_ROUNDS) {

    	// Set the starting turn (starting player) based on the round
    	if(current_round == 0){
    		turn = 1;
    	}
    	else if(current_round == 1){
    		turn = 2;

    		// Set all hands to empty before the round
    		for(int i =0; i < NUM_PLAYERS; i++){
    			hands[i].first = 0;
    			hands[i].second = 0;
    		}
    	}
    	else if(current_round == 2){
    		turn = 3;

    		// Set all hands to empty before the round
    		for(int i =0; i < NUM_PLAYERS; i++){
    			hands[i].first = 0;
    			hands[i].second = 0;
    		}
    	}

    	// Play round and increment round counter
        playRound();
        current_round++;

        // Reset values
        win = false;
        done = false;
    }

    deck.clear();

    logFile << "Game Over" << endl;

    //close the log file
    logFile.close();
}

// Play a single round of the game
void playRound() {

    cout << endl << "Starting Round " << current_round+1 << endl << endl;
    logFile << endl << "Starting Round " << current_round+1 << endl << endl; 

    // Create the dealer thread and give it the dealer function to run
    pthread_create(&dealer_thread, NULL, dealerThreadFunction, (void *)0);

    // Create the player threads and give them the player function to run
    for(long i = 1; i <= NUM_PLAYERS; i++){
        pthread_create(&player_thread[i], NULL, playerThreadFunction, (void *)i);
    }

    // Join the threads together so they can be synchronized
    pthread_join(dealer_thread, NULL);
    for(long i = 1; i <= NUM_PLAYERS; i++){
        pthread_join(player_thread[i], NULL);
    }
}

// Discard the hands back into the deck
void discardPlayerHands(){

	for(int i = 0; i < NUM_PLAYERS; i++){
		if(hands[i].first != 0){
			deck.push_back(hands[i].first);
			hands[i].first = 0;
		}
		if(hands[i].second != 0){
			deck.push_back(hands[i].second);
			hands[i].second = 0;
		}
	}
}

// Dictates what threads can do when they are given access to the deck
void useDeck(void * id, pair<int, int> hand){

	// If its the dealer using the deck
	if((long)id == 0){

		shuffleDeck();

		// Deal 1 card to each player
		for(int i = 0; i < NUM_PLAYERS; i++){
			hands[i].first = deck.front();
			deck.erase(deck.begin());
		}
	}

	// If its a player using the deck
	else{

		logFile << "PLAYER " << (long)id << ": hand " << hand.first << endl;

		// Draw a card
		hand.second = deck.front();
		deck.erase(deck.begin());
		logFile << "PLAYER " << (long)id << ": draws " << hand.second << endl;

		logFile << "PLAYER " << (long)id << ": hand " << hand.first << " " << hand.second << endl;

		// If they match you win
		if(hand.first == hand.second){

			// Updates the hands with the changes
			if(current_round == 0){
				if((long)id == 1)
					hands[0] = hand;
				else if((long)id == 2)
					hands[1] = hand;
				else if((long)id == 3)
					hands[2] = hand;
			}
			else if(current_round == 1){
				if((long)id == 2)
					hands[0] = hand;
				else if((long)id == 3)
					hands[1] = hand;
				else if((long)id == 1)
					hands[2] = hand;
			}
			else if(current_round == 2){
				if((long)id == 3) 
					hands[0] = hand;
				else if((long)id == 1)
					hands[1] = hand;
				else if((long)id == 2)
					hands[2] = hand;
			}

			logFile << "PLAYER " << (long)id << ": wins" << endl << endl;
			cout << "HAND: " << hand.first << " " << hand.second << endl;
			cout << "WIN yes" << endl;

			printDeck();

			discardPlayerHands();

			win = true;

			// Tell the dealer the round is over
			pthread_cond_signal(&condition_win);
		}
		else if(hand.first != hand.second){

			// Randomly choose card to put back
			int discard = rand()%2;
			if(discard == 0){
				logFile << "PLAYER " << (long)id << ": discards " << hand.first << endl << endl;
				deck.push_back(hand.first);
				hand.first = hand.second;
				hand.second = 0;
			}
			else if(discard == 1){
				logFile << "PLAYER " << (long)id << ": discards " << hand.second << endl << endl;
				deck.push_back(hand.second);
				hand.second = 0;
			}

			cout << "HAND: " << hand.first << endl;
			cout << "WIN no" << endl;

			// Updates the hands with the changes
			if(current_round == 0){
				if((long)id == 1)
					hands[0] = hand;
				else if((long)id == 2)
					hands[1] = hand;
				else if((long)id == 3)
					hands[2] = hand;
			}
			else if(current_round == 1){
				if((long)id == 2)
					hands[0] = hand;
				else if((long)id == 3)
					hands[1] = hand;
				else if((long)id == 1)
					hands[2] = hand;
			}
			else if(current_round == 2){
				if((long)id == 3) 
					hands[0] = hand;
				else if((long)id == 1)
					hands[1] = hand;
				else if((long)id == 2)
					hands[2] = hand;
			}

			printDeck();
		}

		turn++;
		if(turn > NUM_PLAYERS)
			turn = 1;
	}

	// Let the other threads know they can use the deck
	pthread_cond_broadcast(&condition_deck);
}
