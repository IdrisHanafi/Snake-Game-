// Type the command below to play the game, remember it must be in linux
// gcc -W -Wall Lab10.c -o Lab10.exe -lncurses -lrt
// you win if your length is 15

#include <ncurses.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <getopt.h>

#define   TIMEOUTS       16
#define   TIMEOUT_SIGNAL (SIGRTMIN+0)

#define   TIMEOUT_USED   1
#define   TIMEOUT_ARMED  2
#define   TIMEOUT_PASSED 4

static timer_t               timeout_timer;
static volatile sig_atomic_t timeout_state[TIMEOUTS] = { 0 };
static struct timespec       timeout_time[TIMEOUTS];

#define STD_SPEED 100000
#define MAX_SPEED 700000
#define MIN_SPEED 7000
#define STEP_SPEED 5000

#define UP walk(1)
#define RIGHT walk(2)
#define DOWN walk(3)
#define LEFT walk(4)
#define MOVE walk(dir)

#define MSG_LEN 100 /* disk space to allocate for each temp meggage string */
#define MALLOC (struct part *)malloc(sizeof(struct part))

#define ATTR_PRINT(x) attrset(COLOR_PAIR(x)); addch(' ');
#define ATTR_PRINT_BORDER(x) addch('*');
#define ATTR_PRINT_TROPHY(x) addch(x);
#define ATTR_PRINT_SNAKE(x) addch('*');
#define PRINT_BG ATTR_PRINT(1)
#define PRINT_HEAD ATTR_PRINT_SNAKE(2)
#define PRINT_TAIL ATTR_PRINT_SNAKE(3)
#define PRINT_TROPHY(x) ATTR_PRINT_TROPHY(x)
#define PRINT_BORDER ATTR_PRINT_BORDER(5)
#define PRINT_SPLASH ATTR_PRINT(6)
#define DEL_CURSER move(0,0);

void walk(int way);
void finish(int sig);
void print_snake(void);
void remove_tip(void);
void make_trophy(void);
void gameOver(void);
void check_speed(void);
int timeout_done(void);
int timeout_init(void);
int timeout_done(void);
static void timeout_signal_handler(int signum __attribute__((unused)), siginfo_t *info, void *context __attribute__((unused)));
int timeout_set(const double);
static inline int timeout_unset(const int);
static inline int timeout_passed(const int);
static inline void timespec_set(struct timespec *const, const double);
static inline void timespec_add(struct timespec *const, const double);
static inline double timespec_diff(const struct timespec, const struct timespec);

struct part {
	int x;
	int y;
	struct part *next;
};
struct trophy {
	int x;
	int y;
};
struct part *tip; /* the tip of the snake tale */
struct part *cur; /* the 'working' part of the snake */
struct part *head; /* the head of the snake */
struct trophy app; /* allocate memmory for the trophy structure */
int dir; /* hold snake current dirrection */
unsigned long speed; /* speed is used to store the current speed */
int waiting = 0; /* number of snake parts waiting */
int rest = 0;
int stdspeed=STD_SPEED; /* ser the standard speed to fall back to */
int trophyNum = 0;
int timer = 0;
int snakeLength = 3;

int main(int argc, char *argv[]) {
	int x, y, pause = 0;
	srandom((unsigned)time(NULL)); // random number generator with current time
	signal(SIGINT,finish); /* call finish() when ctrl+c is pressed */
	initscr(); /* init screen for the use of curses */
	nonl(); /* something about output */
	cbreak(); /* don't let getch() wait for a newline */
	noecho(); /* don't echo getch(); */
	keypad(stdscr,TRUE); /* make the _KEY def's work */
	nodelay(stdscr,TRUE); /* getch() will return ERR if no key is pressed */
	LINES--; /* start counting at 0, not 1 */
	COLS--; /* same as above */
	if(has_colors()) {
		start_color();
		init_pair(1, COLOR_BLACK,   COLOR_WHITE); // background 
		init_pair(2, COLOR_BLACK,    COLOR_WHITE); // head 
		init_pair(3, COLOR_BLACK,  COLOR_WHITE); // tail 
		init_pair(4, COLOR_BLACK,   COLOR_WHITE); // trophy 
		init_pair(5, COLOR_BLACK, COLOR_WHITE); // border 
	} else {
		printf("Color error\n");
		finish(1);
	}
	speed = stdspeed;
	if(rand()%4 < 2) {
		dir=2;
	} else {
		dir=3;
	} /* up=1, right=2, down=3, left=4 */

	for(x = 1; x < COLS; x++) {
		for(y=1;y<LINES;y++) {
			move(y,x);
			PRINT_BG;
		}
	}
	for(x = 0; x < COLS; x++) {
		move(0,x);
		PRINT_BORDER;
	}
	for(x = 0; x < COLS; x++) {
		move(LINES,x);
		PRINT_BORDER;
	}
	for(x = 0; x < LINES; x++) {
		move(x,0);
		PRINT_BORDER;
	}
	for(x = 0; x < LINES; x++) {
		move(x,COLS);
		PRINT_BORDER;
	}
	move(LINES,COLS);
	PRINT_BORDER;
	move(0,0);
	PRINT_BORDER;

	/* init snake */
	/* init tip of the tale */
	tip=MALLOC;
	tip->x=1;
	tip->y=1;
	// init middle part of the snake 
	tip->next=MALLOC; // find memory for first part of the tale 
	cur=tip->next; // store memory address og first part of the tale in cur 
	cur->x=1;
	cur->y=1;
	/* init snake head */
	cur->next=MALLOC; /* find memory for head */
	cur=cur->next;
	cur->x=1; /* set x cordinat for head */
	cur->y=1; /* set y cordinat for head */
	cur->next=NULL;
	print_snake();
	make_trophy();
	while(1) {
		if(!pause) {
			x=getch();
			usleep(speed);
			switch(x) {
				case KEY_UP:
					if(dir==3) {	
						gameOver();
					} else {
						UP;
					}
					break;
				case KEY_RIGHT:
					if(dir==4) {	
						gameOver();
					} else {
						RIGHT;
					}
					break;
				case KEY_DOWN:
					if(dir==1) {	
						gameOver();
					} else {
						DOWN;
					}
					break;
				case KEY_LEFT:
					if(dir==2) {	
						gameOver();
					} else {
						LEFT;
					}
					break;
				case ERR:
					MOVE;
					break;
				default:
					MOVE;
					break;
			}
			DEL_CURSER;
			refresh();
		} else {
			usleep(STD_SPEED);
			refresh();
		}
		
	}
	finish(0);
}

void walk(int way) {
	if((way==1 && dir==3) || (way==2 && dir==4) || (way==3 && dir==1) || (way==4 && dir==2)) {
		MOVE;
		return;
	}
	if(waiting==0)
		remove_tip();
	else
		waiting--;
	cur=tip;
	while(cur->next != NULL)
		cur=cur->next;
	cur->next=MALLOC;
	head=cur->next;
	head->next=NULL;
	switch(way) {
		case 1: /* up */
			head->x=cur->x;
			head->y=cur->y-1;
			break;
		case 2: /* right */
			head->x=cur->x+1;
			head->y=cur->y;
			break;
		case 3: /* down */
			head->x=cur->x;
			head->y=cur->y+1;
			break;
		case 4: /* left */
			head->x=cur->x-1;
			head->y=cur->y;
			break;
	}
	dir=way;
	move(cur->y,cur->x);
	PRINT_TAIL;
	move(head->y,head->x);
	PRINT_HEAD;
	if(head->x==app.x && head->y==app.y) {
		waiting = waiting + trophyNum;	// increase by the number
		snakeLength = snakeLength + trophyNum;
		make_trophy();
	}
	if(snakeLength >= 15) {
		nodelay(stdscr,TRUE);
		winning(0);
	}
	if (timeout_passed(timer)) {
		timeout_unset(timer);
		if (timeout_done()) {
			fprintf(stderr, "timeout_done(): %s.\n", strerror(errno));
			return 1;
	    	}
		move(app.y, app.x);
		addch(' ');
		make_trophy();
	}
	if((head->x == 0)||(head->x == COLS)||(head->y == 0)||(head->y == LINES))
		gameOver();
	cur=tip;
	while(cur->next != NULL) {
		if(head->x==cur->x && head->y==cur->y)
			gameOver();
		cur=cur->next;
	}
}

void check_speed(void) {
	if(speed>MAX_SPEED)
		speed=MAX_SPEED;
	else if(speed<MIN_SPEED)
		speed=MIN_SPEED;
}

void gameOver(void) {
	nodelay(stdscr,TRUE);
	finish(0);
}

void print_snake(void) {
	int x=1;
	cur=tip;
	while(x) {
		move(cur->y,cur->x);
		if(cur->next==NULL) {
			PRINT_HEAD;
			x=0;
		} else {
			PRINT_TAIL;
			cur=cur->next;
		}
	}
}

void remove_tip(void) {
	cur=tip;             /* store memmory location of the current tip in cur */
	tip=tip->next;       /* make tip point at the next part of the tale */
	move(cur->y,cur->x); /* move to cur */
	PRINT_BG;            /* print background on cur */
	free(cur);           /* free what's in cur from memmory */
}

void finish(int sig) {
	endwin();
	exit(sig);
}

void winning(int sig) {
	endwin();
	printf("YOU WIN!!!!!\n\n");
	exit(sig);
}

void make_trophy(void) {
	if (timeout_init()) {
        	fprintf(stderr, "timeout_init(): %s.\n", strerror(errno));
        	return 1;
    	}
	timer = timeout_set(9.0);
	int run=1;
	speed = speed - STEP_SPEED;
	while(run) {
		run=0;
		app.x=random()%(COLS-2)+1;
		app.y=random()%(LINES-2)+1;
		cur=tip;
		while(cur->next!=NULL) {
			if((cur->x==app.x) && (cur->y==app.y))
				run=1;
			cur=cur->next;
		}
	}
	move(app.y,app.x);
	trophyNum = (rand()%8)+1;
	PRINT_TROPHY((char)(((int)'0')+(trophyNum)));
}





static inline double timespec_diff(const struct timespec after, const struct timespec before) {
 return (double)(after.tv_sec - before.tv_sec) + (double)(after.tv_nsec - before.tv_nsec) / 1000000000.0;
}

static inline void timespec_add(struct timespec *const to, const double seconds) {
    if (to && seconds > 0.0) {
        long  s = (long)seconds;
        long  ns = (long)(0.5 + 1000000000.0 * (seconds - (double)s));

        if (ns < 0L)
            ns = 0L;
        else
        if (ns > 999999999L)
            ns = 999999999L;

        to->tv_sec += (time_t)s;
        to->tv_nsec += ns;

        if (to->tv_nsec >= 1000000000L) {
            to->tv_nsec -= 1000000000L;
            to->tv_sec++;
        }
    }
}

static inline void timespec_set(struct timespec *const to, const double seconds) {
    if (to) {
        if (seconds > 0.0) {
            const long  s = (long)seconds;
            long       ns = (long)(0.5 + 1000000000.0 * (seconds - (double)s));

            if (ns < 0L)
                ns = 0L;
            else
            if (ns > 999999999L)
                ns = 999999999L;

            to->tv_sec = (time_t)s;
            to->tv_nsec = ns;

        } else {
            to->tv_sec = (time_t)0;
            to->tv_nsec = 0L;
        }
    }
}

static inline int timeout_passed(const int timeout) {
    if (timeout >= 0 && timeout < TIMEOUTS) {
        const int  state = __sync_or_and_fetch(&timeout_state[timeout], 0);

        if (!(state & TIMEOUT_USED))
            return -1;

        if (!(state & TIMEOUT_ARMED))
            return -1;

        return (state & TIMEOUT_PASSED) ? 1 : 0;

    } else {
        return -1;
    }
}

static inline int timeout_unset(const int timeout) {
    if (timeout >= 0 && timeout < TIMEOUTS) {
        const int  state = __sync_and_and_fetch(&timeout_state[timeout], ~TIMEOUT_PASSED);

        if (!(state & TIMEOUT_USED))
            return -1;

        if (!(state & TIMEOUT_ARMED))
            return -1;

        return (state & TIMEOUT_PASSED) ? 1 : 0;

    } else {
        return -1;
    }
}


int timeout_set(const double seconds) {
    struct timespec   now, then;
    struct itimerspec when;
    double            next;
    int               timeout, i;

    if (seconds <= 0.0)
        return -1;

    if (clock_gettime(CLOCK_REALTIME, &now))
        return -1;

    then = now;
    timespec_add(&then, seconds);

    for (timeout = 0; timeout < TIMEOUTS; timeout++)
        if (!(__sync_fetch_and_or(&timeout_state[timeout], TIMEOUT_USED) & TIMEOUT_USED))
            break;

    if (timeout >= TIMEOUTS)
        return -1;

    __sync_and_and_fetch(&timeout_state[timeout], TIMEOUT_USED);

    timeout_time[timeout] = then;

    __sync_or_and_fetch(&timeout_state[timeout], TIMEOUT_ARMED);

    next = seconds;
    for (i = 0; i < TIMEOUTS; i++)
        if ((__sync_fetch_and_or(&timeout_state[i], 0) & (TIMEOUT_USED | TIMEOUT_ARMED | TIMEOUT_PASSED)) == (TIMEOUT_USED | TIMEOUT_ARMED)) {
            const double secs = timespec_diff(timeout_time[i], now);
            if (secs >= 0.0 && secs < next)
                next = secs;
        }

    timespec_set(&when.it_value, next);
    when.it_interval.tv_sec = 0;
    when.it_interval.tv_nsec = 0L;

    if (timer_settime(timeout_timer, 0, &when, NULL)) {
        __sync_and_and_fetch(&timeout_state[timeout], 0);
        return -1;
    }

    return timeout;
}


static void timeout_signal_handler(int signum __attribute__((unused)), siginfo_t *info, void *context __attribute__((unused)))
{
    struct timespec   now;
    struct itimerspec when;
    int               saved_errno, i;
    double            next;

    if (!info || info->si_code != SI_TIMER)
        return;

    saved_errno = errno;

    if (clock_gettime(CLOCK_REALTIME, &now)) {
        errno = saved_errno;
        return;
    }

    next = -1.0;

    for (i = 0; i < TIMEOUTS; i++)
        if ((__sync_or_and_fetch(&timeout_state[i], 0) & (TIMEOUT_USED | TIMEOUT_ARMED | TIMEOUT_PASSED)) == (TIMEOUT_USED | TIMEOUT_ARMED)) {
            const double  seconds = timespec_diff(timeout_time[i], now);
            if (seconds <= 0.0) {
                __sync_or_and_fetch(&timeout_state[i], TIMEOUT_PASSED);

            } else
            if (next <= 0.0 || seconds < next) {
                next = seconds;
            }
        }

    timespec_set(&when.it_value, next);
    when.it_interval.tv_sec = 0;
    when.it_interval.tv_nsec = 0L;
    timer_settime(timeout_timer, 0, &when, NULL);

    errno = saved_errno;
}


int timeout_init(void) {
    struct sigaction  act;
    struct sigevent   evt;
    struct itimerspec arm;

    sigemptyset(&act.sa_mask);
    act.sa_sigaction = timeout_signal_handler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(TIMEOUT_SIGNAL, &act, NULL))
        return errno;

    evt.sigev_notify = SIGEV_SIGNAL;
    evt.sigev_signo = TIMEOUT_SIGNAL;
    evt.sigev_value.sival_ptr = NULL;
    if (timer_create(CLOCK_REALTIME, &evt, &timeout_timer))
        return errno;

    arm.it_value.tv_sec = 0;
    arm.it_value.tv_nsec = 0L;
    arm.it_interval.tv_sec = 0;
    arm.it_interval.tv_nsec = 0L;
    if (timer_settime(timeout_timer, 0, &arm, NULL))
        return errno;

    return 0;
}

int timeout_done(void) {
    struct sigaction  act;
    struct itimerspec arm;
    int               errors = 0;

    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(TIMEOUT_SIGNAL, &act, NULL))
        if (!errors) errors = errno;

    arm.it_value.tv_sec = 0;
    arm.it_value.tv_nsec = 0L;
    arm.it_interval.tv_sec = 0;
    arm.it_interval.tv_nsec = 0;
    if (timer_settime(timeout_timer, 0, &arm, NULL))
        if (!errors) errors = errno;

    if (timer_delete(timeout_timer))
        if (!errors) errors = errno;

    if (errors)
        errno = errors;

    return errors;
}
