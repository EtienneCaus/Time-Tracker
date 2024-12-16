#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <string.h>
#include <wchar.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))
#define MAX 1024

struct termios info;    //Used to take keystrokes immediately
struct winsize w;   //Used to see terminal size

//The actual textfile array
unsigned char file[MAX][MAX];
int cursor[]={9,2};
int bottomline=2;

void printscreen()
{
    gotoxy(0,0);

    for(int y=(w.ws_row-cursor[1] < 0) ? cursor[1]-w.ws_row +1 : 1; y<=cursor[1] && y<bottomline; y++)
    {
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
        for(int x=0; x<MAX; x++)    //Redraw the line
            printf("%c" , file[y][x]);
    }
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

    //Fills the "file" variable with 0's
    for(int y=0;y<MAX;y++)
        for(int x=0;x<MAX;x++)
            file[y][x] =  0;

    //Defines the buffer
    unsigned char ch;
    int athome=1;
    int run=1;

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
                    }  
                    break;
                case 'H':
                    cursor[0] = 9;      //Put the cursor at the start of the line
                    gotoxy(cursor[0],cursor[1]);
                    if(cursor[1]!=bottomline)
                        printf("\033[1A"); // Move up 1 column
                    break;
                case 'F':   //End Key, set the cursor to the end of file
                    athome = 1;
                    printf("\033[2J"); //Clears the screen
                    printscreen();

                    t = time(NULL); //get the time again
                    ptr = localtime(&t);
                    for(int x=11;x<16;x++) timer[x-11] = asctime(ptr)[x]; //Set the time

                    cursor[0]=9;
                    cursor[1]=bottomline; //Put the cursor at the end of the file!
                    gotoxy(0,cursor[1]);
                    for(int x=0; x<MAX; x++)    //Redraw the line
                        printf("%c" , file[cursor[1]][x]);
                    break;
                case '2': //Insert key
                    getchar();

                    for(int x=MAX-2; x>cursor[0]; x--)
                        file[cursor[1]][x] = file[cursor[1]][x-1];
                    file[cursor[1]][cursor[0]] = '~';

                    printf("\033[2K"); //Clears the line
                    gotoxy(0,cursor[1]);

                    if(cursor[1]!=bottomline)
                        printf("\033[1A"); // Move up 1 column

                    for(int x=0; x<MAX; x++)    //Redraw the line
                        printf("%c" , file[cursor[1]][x]);
                    
                    gotoxy(cursor[0],cursor[1]); 
                    if(cursor[1]!=bottomline)
                        printf("\033[1A"); // Move up 1 column
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
                    printf("\033[2K"); //Clears the line
                    gotoxy(0,cursor[1]);

                    if(cursor[1]!=bottomline)
                        printf("\033[1A"); // Move up 1 column

                    for(int x=0; x<MAX; x++)    //Redraw the line
                        printf("%c" , file[cursor[1]][x]);
                    
                    gotoxy(cursor[0],cursor[1]); 
                    if(cursor[1]!=bottomline)
                        printf("\033[1A"); // Move up 1 column
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

                        cursor[1]--;                                        
                        printscreen();
                    }
                    break;
                case 'B':
                    // code for arrow down
                    if(cursor[1] < MAX && cursor[1] < bottomline-1)
                    {
                        if(athome)
                            printf("\033[2J"); //Clears the screen
                        athome=0;

                        cursor[1]++;            
                        printscreen();
                    }                   
                    break;
                case 'C':
                    // code for arrow right
                    if(cursor[0] < MAX)
                    {
                        printf("\033[1C"); // Move right X column;
                        cursor[0]++;
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
                    for(int x=0;x<9;x++)    //Write the time down again on the next line
                        file[cursor[1]][x] = timer[x];
                    bottomline=cursor[1];
                }
            }
            else if(cursor[1]<bottomline-1)
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
            if(cursor[1]!=bottomline)
                printf("\033[1A"); // Move up 1 column

            printf("\033[%sm", action); //Change the color
            for(int x=0; x<MAX; x++)    //Redraw the line
                printf("%c" , file[cursor[1]][x]);

            gotoxy(7,cursor[1]);    //Set the color in the textfile
            if(cursor[1]!=bottomline)
                printf("\033[1A"); // Move up 1 column

            printf("%c", symbol);
            file[cursor[1]][6] = symbol;    //Change the symbol of the line
            timer[6] = symbol;

            gotoxy(cursor[0],cursor[1]); 
            if(cursor[1]!=bottomline)
                printf("\033[1A"); // Move up 1 column
        }
        else
        {
            printf("%c", ch);   //Prints the character
            file[cursor[1]][cursor[0]] = ch;
            cursor[0]++;
        }

        t = time(NULL); //get the time again
        ptr = localtime(&t); //Set the time
        for(int x=11;x<16;x++) currenttime[x-11] = asctime(ptr)[x];
        gotoxy(0,w.ws_row); //Moves to the bottom of the screen
        printf("%s" , currenttime);//Goes to the next line
        gotoxy(cursor[0],cursor[1]);
        if(cursor[1]!=bottomline)
                printf("\033[1A"); // Move up 1 column
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
