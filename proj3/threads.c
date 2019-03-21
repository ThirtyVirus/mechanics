/*
** threads.c: Space-Invadors like game that utilizes multi-threading
** and the ncurses library.
**
** Author: Brandon Calabrese (bmc1340)
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include <pthread.h>

/*
** Declare non-magic values to facilitate program functions
*/
size_t BUFSIZE = 80;
#define BUFSIZE_DECLARE 80
#define DEFAULT_PADDLE_HEIGHT 14
#define DEFAULT_MAX_CONCURRENT 8
#define MISSILE_SLEEP 300000
#define ATTACKER_SLEEP 900000

/*
** Runtime data to be used during gameplay
*/
char defenseForce[BUFSIZE_DECLARE];
char attackForce[BUFSIZE_DECLARE];
int maxMissles = 0;

int *cityLayout;
int citySize = 0;
int maxCitySize = 1;
int height, width = 0; //window size

int paddleX = 0;
int paddleY = DEFAULT_PADDLE_HEIGHT; //default is 14, can be more w/ buildings higher than 13

int maxConcurrentMissles = DEFAULT_MAX_CONCURRENT;

static pthread_mutex_t ATTACK = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t DEFENSE = PTHREAD_MUTEX_INITIALIZER;

int end = 0;

/*
** safeAdd: Safely adds value to cityLayout and changes the size of the "array"
** to handle storing all values. Initializes memory allocation as well.
** Also sets appropriate paddle height.
**  
** parameter:
**  toAdd the value to be added to the city layout
** 
** returns:
** void
*/
void safeAdd(int toAdd) {
    
    //get highest building for paddle height
    if (toAdd >= paddleY) {
        paddleY = toAdd + 1;
    }
    
    //initial memory allocation
    if (citySize == 0) {
        cityLayout = (int *)calloc(maxCitySize, sizeof(int));
    }
    
    //realloc space as needed
    if (citySize + 1 >= maxCitySize) {
        maxCitySize *= 2;
        cityLayout = (int *)realloc(cityLayout, sizeof(int) * maxCitySize);
    }
    
    //initialize new allocated memory for cityLayout
    for (int counter = citySize; counter < maxCitySize; counter++) {
        cityLayout[counter] = 2;
    }
    
    //add element to "array"
    cityLayout[citySize] = toAdd;
    citySize++;
}

/*
** readConfig: Reads the given configuration file and collects
** the needed data for program execution.
** handles formatting errors
**  
** parameter:
** file the config file to read from
** 
** returns:
** void
*/
int readConfig(FILE *file) {
    
    char *buffer = NULL;
    
    int argsParsed = 0;
    int currentLine = 0;
    int commentsRead = 0;
    while (argsParsed < 3 && commentsRead < 100) {
        
        //get next line
        getline(&buffer, &BUFSIZE, file);
        
        //remove trailing newline character
        buffer[strcspn(buffer, "\n")] = 0;
        
        //ignore comments and empty lines
        if (buffer[0] == '#' || buffer[0] == '\n') {
            commentsRead++;
            continue;
        }
        
        //defense force
        if (argsParsed == 0) {
            strcpy(defenseForce, buffer);
            argsParsed++;
        }
        //attack force
        else if (argsParsed == 1) {
            strcpy(attackForce, buffer);
            argsParsed++;
        }
        //max missles
        else if (argsParsed == 2) {
            
            //assure that input here is a number
            if (!isdigit(*buffer) && atoi(buffer) == 0) {
                fprintf(stderr, "Error: missing attacker name.\n");
                free(buffer);
                fclose(file);
                return 1;
            }
            
            maxMissles = atoi(buffer);
            if (maxMissles == 0) {
                maxMissles = -1;
            }
            argsParsed++;
            getline(&buffer, &BUFSIZE, file);
        }
        
        currentLine++;
    }
    
    //verify that there is a defender and attacker name
    int defenseExists = 0;
    int attackExists = 0;
    for (int counter = 0; counter < BUFSIZE_DECLARE; counter++) {
        if (isdigit(defenseForce[counter]) || isalpha(defenseForce[counter])) {
            defenseExists = 1;
        }
        if (isdigit(attackForce[counter]) || isalpha(attackForce[counter])) {
            attackExists = 1;
        }
    }
    if (!defenseExists) {
        fprintf(stderr, "Error: missing defender name.\n");
        free(buffer);
        fclose(file);
        return 1;
    }
    if (!attackExists) {
        fprintf(stderr, "Error: missing attacker name.\n");
        free(buffer);
        fclose(file);
        return 1;
    }
    
    //verify that file contained enough information so far
    if (currentLine < 3) {
        fprintf(stderr, "Error: missing attacker name.\n");
        free(buffer);
        fclose(file);
        return 1;
    }
    
    //city layout (reads remaining lines in file)
    char* token;
    while(getline(&buffer, &BUFSIZE, file) >= 0) {
        
        //ignore comments
        if (buffer[0] == '#') {
            continue;
        }
        
        //add all values on line to city layout
        token = strtok(buffer, " ");
        while (token != NULL) {
            
            //assure that input here is a number
            
            //if (atoi(token) == 0) {
            //    printf("Test: 4. '%d'\n", *token);
            //    fprintf(stderr, "Invalid Config!\n");
            //    free(buffer);
            //    fclose(file);
            //    return 1;
            //}
            
            //add indevidual value to sity layout
            safeAdd(atoi(token));
            token = strtok(NULL, " ");
        }
        
    }
    //verify that file contained cityscape info
    if (citySize == 0) {
        printf("Test: 4\n");
        fprintf(stderr, "Invalid Config!\n");
        free(buffer);
        fclose(file);
        return 1;
    }
    
    //free up buffer to prevent memory problems
    if(buffer != NULL) {
        free(buffer);
    }
    fclose(file);
    
    return 0;
}

/*
** cleanup: Frees memory used by application
** Does other end-of-application tasks
**  
** returns:
** void
*/
void cleanup() {
    endwin(); //closes ncurses
    
    free(cityLayout);
}

/*
** Initialize: initialize curses environment and cityscape
** 
** returns:
** void
*/
void initialize() {
    
    //init random number generator
    time_t t; srand((unsigned) time(&t));
    
    //initialize curses environment
    initscr(); //creates stdscr
    cbreak(); //allows CTRL-X to exit in testing environment
    //raw() //can replace cbreak when done testing
    
    //initialize screen size and (centered) paddle location
    getmaxyx(stdscr, height, width);
    paddleX = (width / 2) - 2;
    
    //resize city layout to fit screen
    if (citySize < width) {
        maxCitySize = width;
        cityLayout = (int *)realloc(cityLayout, sizeof(int) * maxCitySize);

        //initialize new allocated memory for cityLayout
        for (int counter = citySize - 1; counter < maxCitySize; counter++) {
            cityLayout[counter] = 2;
        }
        citySize = maxCitySize;
    }
    
    //initial draw of cityscape
    for (int counter = 0; counter < citySize; counter++) {
        
        //draw buildings
        if (cityLayout[counter] > 2) {
            
            //draw top of buildings
            mvaddch(height - cityLayout[counter], counter, '_');
            
            //draw sides of buildings
            if (counter > 0) { //left
                if (cityLayout[counter - 1] < cityLayout[counter]) {
                    mvaddch(height - cityLayout[counter], counter, ' ');
                    for (int i=height-cityLayout[counter]+1;i<=height-2;i++) {
                        mvaddch(i, counter, '|');
                    }
                }
            }
            if (counter < citySize - 1) { //right
                if (cityLayout[counter + 1] < cityLayout[counter]) {
                    mvaddch(height - cityLayout[counter], counter, ' ');
                    for (int i=height-cityLayout[counter]+1;i<=height-2;i++) {
                        mvaddch(i, counter, '|');
                    }
                }
            }
        } else {
            //draw ground
            mvaddch(height - cityLayout[counter], counter, '_');
        }
    }
    
    //initial draw of paddle
    mvprintw(height - paddleY, paddleX - 3, " ##### ");
    
    //draw top label
    mvprintw(0, 16, "Enter 'q' to quit at end of attack, or control-C");
    refresh();
}

/*
** runDefense: The defense thread. Handles user
** controls for paddle and draws it to screen.
** also checks for quitting game
**  
** parameter:
** arg does nothing
** 
** returns:
** void
*/
void *runDefense(void *arg) {
    if (arg) {} //mute compiler warning for not using arg
    
    //get user input
    while (end == 0) {
        
        //controls
        pthread_mutex_lock(&DEFENSE);
        char input = getch();
        pthread_mutex_unlock(&DEFENSE);
        
        //move paddle left
        if (input == 0x44 && paddleX > 2) {
            paddleX--;
        }
        
        //move paddle right
        if (input == 0x43 && paddleX < width - 3) {
            paddleX++;
        }
        
        //quit game
        if (input == 'q') {
            end = 1;
        }
        
        //draw paddle (with assumption that paddle is within bounds)
        if (paddleX == 2) {
            pthread_mutex_lock(&DEFENSE);
            mvprintw(height - paddleY, paddleX - 2, "##### ");
            pthread_mutex_unlock(&DEFENSE);
        } else {
            pthread_mutex_lock(&DEFENSE);
            mvprintw(height - paddleY, paddleX - 3, " ##### ");
            pthread_mutex_unlock(&DEFENSE);
        }
        pthread_mutex_lock(&DEFENSE);
        move(height - paddleY, paddleX);
        pthread_mutex_unlock(&DEFENSE);
        
        pthread_mutex_lock(&DEFENSE);
        refresh();
        pthread_mutex_unlock(&DEFENSE);
    }
    
    return NULL;
}

/*
** runMissle: The missle thread. Controls the descent of each missile
** launched by *runAttacl(). Destroys buildings, leaves craters
** in ground, and draws effects to the screen.
**  
** parameter:
** arg does nothing
** 
** returns:
** void
*/
void *runMissile(void * arg) {
    if (arg) {} //mute compiler warning for not using arg
    
    //initialize missile coordinates
    int x = rand() % width;
    int y = height - 3;
    
    //loop to run until ground hit
    while (1) {
        
        //draw missle (and remove ghost characters)
        pthread_mutex_lock(&ATTACK);
        mvaddch(height - y - 1, x, ' '); move(height - paddleY, paddleX);
        pthread_mutex_unlock(&ATTACK);
        
        pthread_mutex_lock(&ATTACK);
        mvaddch(height - y, x, '|'); move(height - paddleY, paddleX);
        pthread_mutex_unlock(&ATTACK);
        
        //process collision
        if (cityLayout[x] == y) {
            //break building
            if (y > 2) {
                cityLayout[x] -= 1;
                
                pthread_mutex_lock(&ATTACK);
                mvaddch(height - cityLayout[x] - 1, x, ' '); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

                pthread_mutex_lock(&ATTACK);
                mvaddch(height - cityLayout[x], x, '?'); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

                pthread_mutex_lock(&ATTACK);
                mvaddch(height - cityLayout[x] + 1, x, '*'); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);
            }
            //break floor
            else {
                pthread_mutex_lock(&ATTACK);
                mvaddch(height - cityLayout[x] - 1, x, ' '); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

                pthread_mutex_lock(&ATTACK);
                mvaddch(height - cityLayout[x], x, '?'); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

                pthread_mutex_lock(&ATTACK);
                mvaddch(height - cityLayout[x] + 1, x, '*'); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

            }
            pthread_mutex_unlock(&ATTACK);
            return NULL;
        }
        //collision with paddle
        else if (y == paddleY){
            if (x > paddleX - 3 && x < paddleX + 3) {
                pthread_mutex_lock(&ATTACK);
                mvaddch(height - paddleY - 1, x, '?'); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

                pthread_mutex_lock(&ATTACK);
                mvaddch(height - paddleY, x, '#'); move(height - paddleY, paddleX);
                pthread_mutex_unlock(&ATTACK);

                return NULL;
            }
        }
        pthread_mutex_lock(&ATTACK);
        refresh();
        pthread_mutex_unlock(&ATTACK);
        
        //process random speed descent
        int sleepTime = rand() % MISSILE_SLEEP;
        usleep(sleepTime);
        y --;
        
    }
}

/*
** runAttack: The attack thread. Spawns missile threads
** until out of them. Spawns them in waves.
**  
** parameter:
** arg does nothing
** 
** returns:
** void
*/
void *runAttack(void *arg) {
    if (arg) {} //mute compiler warning for not using arg
    
    //handle spawning of missles
    while ((maxMissles > 0 || maxMissles == -1) && end == 0) {
        
        //spawn new wave of missles
        //enforce maximum missles even if less than concurrent remain
        int effectiveMissles = maxConcurrentMissles;
        if (maxMissles < maxConcurrentMissles && maxMissles != -1) {
            effectiveMissles = maxMissles;
        }
        
        //declare and run missile threads
        pthread_t missiles[effectiveMissles];
        for (int counter = 0; counter < effectiveMissles; counter++) {
            pthread_t missile;
            pthread_create(&missile, NULL, runMissile, NULL);
            missiles[counter] = missile;
            if (maxMissles > 0) {
                maxMissles -= 1;
            }
            
            int sleepTime = rand() % ATTACKER_SLEEP;
            usleep(sleepTime);
        }
        
        //join threads such that attack waits for concurrent missles to end
        for (int counter = 0; counter < effectiveMissles; counter++) {
            pthread_join(missiles[counter], NULL);
        }
        
    }
    return NULL;
}

/*
** main: handles opening of the config file, initialization, and
** the eventual running and termination of the 'game'
** 
** parameters:
**  argc  number of arguments
**  argv  collection of arguments
**  
** returns:
** 0 OR 1
*/
int main (int argc, char *argv[]) {
    
    //check for proper format / number of args
    if (argc != 2) {
        fprintf(stderr, "Usage: ./threads config-file\n");
        return 1;
    }
    
    //open config file
    FILE * fp = NULL;
    fp = fopen(argv[1], "r");
    //if found, read file and collect data
    if (fp) {
        int failed = readConfig(fp);
        if (failed == 1) { return 1; }
    //if not found, exit program with error
    } else {
        fprintf(stderr, "File not found!\n");
        return 1;
    }
    
    //initialize game
    initialize();
    
    //start defense thread
    pthread_t defense;
    pthread_create(&defense, NULL, runDefense, NULL);
    
    //start attack thread
    pthread_t attack;
    pthread_create(&attack, NULL, runAttack, NULL);
    
    //join threads (wait until they conclude)
    pthread_join(attack, NULL);
    pthread_join(defense, NULL);
    
    //show user endgame screen
    mvprintw(1, 16, "Game Complete, press any key to exit");
    getch();
    
    //cleanup and prepare for program termination
    cleanup();
    
    return 0;
}