#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <string.h>
#include <wchar.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))
#define MAX 1024

struct termios info;    //Used to take keystrokes immediately
struct winsize w;   //Used to see terminal size

//The actual textfile array
unsigned char file[MAX][MAX];
int cursor[]={9,2};
int bottomline=2;

int run=1;  //variable used to check if program is running or not
int athome=1;

void* printClock(void* vargp)
{
    struct tm* ptr;
    time_t t;

    while(run)
    {
        t = time(NULL);
        ptr = localtime(&t);

        gotoxy(0,0);    //Go to the top of the scree
        printf(asctime(ptr)); //Write down the time
        gotoxy(9,cursor[1]);    //Go to the cursor
        if(!athome && cursor[1] >= w.ws_row)
            printf("\033[1A"); //Moves the cursor up
        for(int x=9; x<cursor[0]; x++)    //Redraw the current line
            printf("%c" , file[cursor[1]][x]);
        fflush(stdout);

        sleep(1);
    }
}

void printscreen()
{
    gotoxy(0,0);

    //Prints the time at the top of the screen
    struct tm* ptr;
    time_t t;
    t = time(NULL);
    ptr = localtime(&t);
    printf(asctime(ptr));

    for(int y=(w.ws_row-cursor[1] < 0) ? cursor[1]-w.ws_row +1 : 1; y<=cursor[1] && y<bottomline; y++)
    {
        gotoxy(0, y);
        for(int x=MAX-3; x>8; x--)  //Puts "spaces" instead of char 0
            if(file[y][x] == 0 && file[y][x+1] != 0)
                file[y][x] = ' ';

        switch(file[y][6])
            {
                case ':' :  //Change the line to Normal
                    printf("\033[%sm", "0");
                    break;
                case '|' :  //Change the line to White
                    printf("\033[%sm", "1;37");
                    break;
                case '!' :  //Change the line to Yellow
                    printf("\033[%sm", "1;33");
                    break;
                case '$' :  //Change the line to Green
                    printf("\033[%sm", "1;32");
                    break;
                case '?' :  //Change the line to Blue
                    printf("\033[%sm", "1;34");
                    break;
                case '#' :  //Change the line to Red
                    printf("\033[%sm", "1;31");
                    break;
            }
        printf("\033[2K"); //Clears the line 
        for(int x=0; x<w.ws_col; x++)    //Redraw the line
            printf("%c" , file[y][x]);
    }
    gotoxy(0,0);

    printf("\033[2K"); //Clears the line 
    t = time(NULL);
    ptr = localtime(&t);
    printf(asctime(ptr)); //Prints the time at the top of the screen again

    gotoxy(0,cursor[1]);
    gotoxy(cursor[0],cursor[1]);
    printf("\033[1A"); //Moves the cursor up
}

int main(int argc, char const *argv[])
{
    //Sets the terminal into non-canonical mode:
    tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin */
    info.c_lflag &= ~(ICANON | ECHO);/* disable canonical mode */ //Also disable the Keystroke echo
    info.c_cc[VMIN] = 1;          /* wait until at least one keystroke available */
    info.c_cc[VTIME] = 0;         /* no timeout */
    tcsetattr(0, TCSANOW, &info); /* set immediately */  

    //Prints the clock at the top of the screen
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, printClock, NULL);
    //pthread_join(thread_id, NULL);

    //Fills the "file" variable with 0's
    for(int y=0;y<MAX;y++)
        for(int x=0;x<MAX;x++)
            file[y][x] =  0;

    //Defines the buffer
    unsigned char ch;

    //Set the timer
    char timer[]= "00:00 : ";
    char currenttime[]= "00:00";
    struct tm* ptr;
    time_t t;
    t = time(NULL);
    ptr = localtime(&t);

    char originalTime[24];  //Takes note of the opening time
    for(int x=0;x<24;x++)
        originalTime[x] = asctime(ptr)[x];

    for(int x=11;x<16;x++)
        timer[x-11] = asctime(ptr)[x]; //Set the time
    for(int x=0;x<9;x++)    
        file[cursor[1]][x] = timer[x]; //Write the time down

    printf("\033[2J"); //Clears the screen
    gotoxy(0,0);
    printf(asctime(ptr));
    printf("%s" , timer); // Print the first timer
    file[0][0] = '\n';

    while(run)
    {
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);   //Get window size (w.ws_row / w.ws_col);
        ch = getchar(); //Takes the keystroke being pressed

        if (ch == '\033')    
        {     
            getchar(); // skip the [
            ch = 0; //empties the char

            switch(getchar()) { // The real value...
                case '5'://Page Up
                    getchar(); //removes the ~
                    if(!athome && cursor[1] > 2)
                    {
                        char symbol = file[cursor[1]][6];
                        cursor[1]--;
                        file[cursor[1]][6] = symbol;    //Change the symbol of the line on top                                 
                        printscreen();

                        for(int x=MAX-3; x>8; x--)  //Puts the Cursor at the end of the line
                            if(file[cursor[1]][x] == 0)
                                cursor[0] = x;
                    }
                    break;
                case '6'://Page Down
                    getchar();  //removes the ~
                    if(!athome && cursor[1] < MAX && cursor[1] < bottomline-1)
                    {
                        char symbol = file[cursor[1]][6];
                        cursor[1]++;
                        file[cursor[1]][6] = symbol;    //Change the symbol of the line bellow
                        printscreen();

                        for(int x=MAX-3; x>8; x--)  //Puts the Cursor at the end of the line
                            if(file[cursor[1]][x] == 0)
                                cursor[0] = x;
                    }  
                    break;
                case '1':
                    getchar();
                case 'H':   //Home key
                    cursor[0] = 9;      //Put the cursor at the start of the line
                    gotoxy(cursor[0],cursor[1]);
                    if(cursor[1]!=bottomline)
                        printf("\033[1A"); // Move up 1 column
                    break;
                case '4':
                    getchar();
                case 'F':   //End Key, set the cursor to the end of file
                    athome = 1;
                    printf("\033[2J"); //Clears the screen
                    printscreen();

                    t = time(NULL); //get the time again
                    ptr = localtime(&t);
                    for(int x=11;x<16;x++) timer[x-11] = asctime(ptr)[x]; //Set the time

                    cursor[0]=9;
                    cursor[1]=bottomline; //Put the cursor at the end of the file!

                    for(int x=MAX-3; x>8; x--)  //Puts the Cursor at the end of the line
                        if(file[cursor[1]][x] == 0)
                            cursor[0] = x;
                    
                    gotoxy(0,cursor[1]);
                    //gotoxy(0,w.ws_row); //Go to the end of the screen
                    for(int x=0; x<MAX; x++)    //Redraw the line
                        printf("%c" , file[cursor[1]][x]);
                    break;
                case '2': //Insert key
                    getchar();

                    for(int x=MAX-2; x>cursor[0]; x--)
                        file[cursor[1]][x] = file[cursor[1]][x-1];
                    file[cursor[1]][cursor[0]] = ' ';

                    printscreen();
                    if(athome)
                    {
                        gotoxy(0,cursor[1]);
                        printf("\033[2K"); //Clears the line 
                        for(int x=0; x<MAX-1; x++)    //Redraw the line
                            printf("%c" , file[cursor[1]][x]);
                    }
                    
                    gotoxy(cursor[0],cursor[1]);
                    break;
                case '3':   //Delete Key
                    getchar();

                    for(int x=cursor[0]; x<MAX-1; x++)
                    {
                        if(x==MAX-2)
                            file[cursor[1]][x] = 0;
                        else
                            file[cursor[1]][x] = file[cursor[1]][x+1];
                    }

                    printscreen();
                    if(athome)
                    {
                        gotoxy(0,cursor[1]);
                        printf("\033[2K"); //Clears the line 
                        for(int x=0; x<MAX-1; x++)    //Redraw the line
                            printf("%c" , file[cursor[1]][x]);
                    }
                    
                    gotoxy(cursor[0],cursor[1]); 
                    break;
                case 27:     //Escape Key (x3), ends the program
                    run=0;
                default:
                    break;
                case 'A':
                    // code for arrow up
                    if(cursor[1] > 2)
                    {
                        if(athome)
                            printf("\033[2J"); //Clears the screen
                        athome=0;

                        if(cursor[1] != bottomline)
                        {
                            gotoxy(0,cursor[1]-1);
                            printf("\033[2K"); //Clears the line 
                            for(int x=0; x<MAX; x++)    //Redraw the line
                                printf("%c" , file[cursor[1]][x]);
                        }

                        cursor[1]--;                               
                        printscreen();

                        for(int x=MAX-3; x>8; x--)  //Puts the Cursor at the end of the line
                            if(file[cursor[1]][x] == 0)
                                cursor[0] = x;
                    }
                    break;
                case 'B':
                    // code for arrow down
                    if(cursor[1] < MAX && cursor[1] < bottomline-1)
                    {
                        if(athome)
                            printf("\033[2J"); //Clears the screen
                        athome=0;

                        gotoxy(0,cursor[1]-1);
                        printf("\033[2K"); //Clears the line 
                        for(int x=0; x<MAX; x++)    //Redraw the line
                            printf("%c" , file[cursor[1]][x]);

                        cursor[1]++;            
                        printscreen();

                        for(int x=MAX-3; x>8; x--)  //Puts the Cursor at the end of the line
                            if(file[cursor[1]][x] == 0)
                                cursor[0] = x;
                    }                   
                    break;
                case 'C':
                    // code for arrow right
                    if(cursor[0] < MAX)
                    {
                        int endline;
                        for(int x=MAX-3; x>8; x--)  //Check the end of the line
                            if(file[cursor[1]][x] == 0)
                                endline = x;
                        if(cursor[0] < endline)
                        {
                            printf("\033[1C"); // Move right X column;
                            cursor[0]++;
                        }
                    }
                    break;
                case 'D':
                    // code for arrow left
                    if(cursor[0] > 9)
                    {
                        printf("\033[1D"); // Move left X column;
                        cursor[0]--;
                    }
                    break;
            }
        }
        else if(ch == 127 || ch=='\b')  //Backspace
        {
            if(cursor[0] > 9)
            {
                printf("\b \b");    //returns, put a "space", then returns again
                cursor[0]--;
                file[cursor[1]][cursor[0]]= ' ';
                ch = 0;
            }
        }
        else if(ch == '\n') //When ENTER is pressed
        {
            if(athome)
            {
                t = time(NULL); //get the time again
                ptr = localtime(&t);
                for(int x=11;x<16;x++) timer[x-11] = asctime(ptr)[x]; //Set the time

                printf("\n%s" , timer);//Goes to the next line
                file[cursor[1]][MAX-1] = ch; //set the last char to "\n"

                for(int x=0;x<9;x++)    //Write the time down
                    file[cursor[1]][x] = timer[x];

                cursor[1]++;    //Change the curser's place
                cursor[0]=9;       
                
                if(cursor[1]>=bottomline)
                {
                    for(int x=0;x<8;x++)    //Write the time down again on the next line
                        file[cursor[1]][x] = ' ';
                    file[cursor[1]][6] = timer[6];
                    bottomline=cursor[1];
                }

                if(cursor[1] == bottomline)
                {
                    //athome = 1;
                    printf("\033[2J"); //Clears the screen
                    printscreen();

                    cursor[0]=9;
                    cursor[1]=bottomline; //Put the cursor at the end of the file!

                    printf("\n");

                    for(int x=MAX-3; x>8; x--)  //Puts the Cursor at the end of the line
                        if(file[cursor[1]][x] == 0)
                            cursor[0] = x;
                    
                    gotoxy(0,cursor[1]);
                    for(int x=0; x<MAX; x++)    //Redraw the line
                        printf("%c" , file[cursor[1]][x]);
                }
            }
            else if(cursor[1]<bottomline)
            {
                for(int y=MAX-1; y>cursor[1]+1; y--)  //Insert a new line
                    for(int x=0; x<MAX; x++)
                        file[y][x] = file[y-1][x];

                cursor[1]++;
                bottomline++;

                for(int x=0;x<8; x++)
                    file[cursor[1]][x]= ' ';
                file[cursor[1]][6]=file[cursor[1]-1][6];
                for(int x=8;x<MAX-1;x++)
                    file[cursor[1]][x] =  0;   

                file[cursor[1]][MAX-1] = ch; //set the last char to "\n"          

                printf("\033[2J"); //Redraws the screen
                printscreen();

                cursor[0] = 9;      //Put the cursor at the start of the line
                gotoxy(cursor[0],cursor[1]);
                printf("\033[1A"); // Move up 1 column
            }
        }
        else if(ch == '\t') //Change the color when tab is pressed
        {             
            char action[4];     //used to change the colors
            char symbol;      //used to change the sybol

            switch(file[cursor[1]][6])
            {
                default:
                case ':' :  //Change the char to White (|)
                    strcpy(action, "1;37");
                    symbol = '|';
                    break;

                case '|' :  //Change the char to Yellow (!)
                    strcpy(action, "1;33");
                    symbol = '!';
                    break;
                case '!' :  //Change the char to Green ($)
                    strcpy(action, "1;32");
                    symbol = '$';
                    break;
                case '$' :  //Change the char to Blue (?)
                    strcpy(action, "1;34");
                    symbol = '?';
                    break;
                case '?' :  //Change the char to Red (#)
                    strcpy(action, "1;31");
                    symbol = '#';
                    break;
                case '#' :  //Change the char to Normal (:)
                    strcpy(action, "0000");
                    symbol = ':';
                    break;
            }

            gotoxy(0,cursor[1]);
            
            printf("\033[%sm", action); //Change the color

            gotoxy(7,cursor[1]);    //Set the color in the textfile

            printf("%c", symbol);
            file[cursor[1]][6] = symbol;    //Change the symbol of the line
            timer[6] = symbol;

            gotoxy(cursor[0],cursor[1]);
        }
        else
        {
            printf("%c", ch);   //Prints the character
            file[cursor[1]][cursor[0]] = ch;
            cursor[0]++;
        }

        if(athome)  //Write down the time if AT HOME
        {
            t = time(NULL); //get the time again
            ptr = localtime(&t); //Set the time
            for(int x=11;x<16;x++) currenttime[x-11] = asctime(ptr)[x];
            //gotoxy(0,w.ws_row); //Moves to the bottom of the screen
            gotoxy(0,cursor[1]); //Moves to home
            printf("%s" , currenttime);//Goes to the next line   
        }
                 
        if(cursor[1]!=bottomline)
        {
            gotoxy(0,cursor[1]+1);
            printf("\033[1A"); // Move up 1 column
            for(int x=0; x<=cursor[0]; x++)    //Redraw the line
                printf("%c" , file[cursor[1]][x]);

            int endline;
            for(int x=MAX-3; x>8; x--)  //Check the end of the line
                if(file[cursor[1]][x] == 0)
                    endline = x;
            if(cursor[0] < endline)
                printf("\033[1D"); // Move left 1 column
        }
        else
        {
            gotoxy(7,cursor[1]);
                printf("%c" , file[cursor[1]][6]);
            gotoxy(9,cursor[1]);
            for(int x=9; x<cursor[0]; x++)    //Redraw the line
                printf("%c" , file[cursor[1]][x]);
        }
    }

    printf("\033[2J"); //Clears the screen

    //Saves the entire document
    freopen("ttrack.txt","a",stdout);  //write in the file

    printf("\n\n");   //Jumps two lines
    printf(originalTime);//Prints the original time
    printf("\n");

    for(int y=0;y<MAX;y++)
        for(int x=0;x<MAX;x++)
            if(file[y][x] >0)
                printf("%c", file[y][x]);   //Saves the *file[][] variable to the text file

    fclose(stdout); //Close the file

    //Puts the terminal back to canonical mode
    tcgetattr(0, &info);
    info.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &info);

    return 0;
}
