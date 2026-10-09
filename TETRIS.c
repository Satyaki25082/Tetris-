#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>


#define WIDTH 10
#define HEIGHT 20

int field[HEIGHT][WIDTH] = {0}; // Game field
int score = 0;                  // Scoreboard

// Tetromino shapes (4x4)
int shapes[7][4][4] = {
    {{1,1,1,1},{0,0,0,0},{0,0,0,0},{0,0,0,0}}, // I
    {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}, // O
    {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // T
    {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // L
    {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}, // J
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}, // S
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}  // Z
};

int currentShape[4][4];
int x = WIDTH/2 - 2, y = 0;

// -------- START SCREEN --------
void showControls() {
    printf("\033[2J"); // clear screen
    printf("=========== TETRIS GAME ===========\n\n");
    printf("Controls:\n");
    printf(" Left Arrow -> Move Left\n");
    printf(" Right Arrow -> Move Right\n");
    printf(" Down Arrow -> Move Down\n");
    printf(" R-> Rotate Tetromino\n");
    printf(" SPACE -> Instant Drop\n\n");
    printf("Press ENTER to start the game...");
    getchar(); // wait for Enter
}

// -------- INPUT --------
int kbhit() {
    struct termios oldt, newt; //                                                     oldt -> old terminal settings, newt -> new modified settings
    int ch;//  character read 
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt); //     Save current keyboard settings (backup)
    newt = oldt;//make a copy so that we can modify safely
    newt.c_lflag &= ~(ICANON | ECHO);// Turn off enter requirement and turns off character display
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);//Activate new input mode automatically
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);//save current input mode (blocking)
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);  // getchar() will not wait and returns immediately if no input
    ch = getchar();// reads a character
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);//Bring the terminal back to normal
    fcntl(STDIN_FILENO, F_SETFL, oldf);//This restores the file flags & back to blocking mode
    if (ch != EOF) {
        ungetc(ch, stdin);//Return a character to input buffer so it can be read later
        return 1;
    }
    return 0;
}

int getch() { //Will return a key pressed
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// -------- GAME --------
void copyShape(int index) {
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            currentShape[i][j] = shapes[index][i][j];
}

void draw() {
    printf("\033[H"); // move cursor to top left corner
    printf("Score: %d\n", score);
    for(int i=0;i<HEIGHT;i++) {
        for(int j=0;j<WIDTH;j++) {
            if(field[i][j])
                printf("#");
            else if(i>=y && i<y+4 && j>=x && j<x+4 && currentShape[i-y][j-x])//checks if the shapes are inside the 4x4 box & the last datatype converts the screen into shape coordinates
                printf("#");
            else
                printf(".");
        }
        printf("\n");
    }
}

int collision(int newX, int newY) { //newx-> new horizontal position , newy -> new vertical position
    for(int i=0;i<4;i++) {
        for(int j=0;j<4;j++) {
            if(currentShape[i][j]) {
                int fx = newX + j;
                int fy = newY + i;
                if(fx < 0 || fx >= WIDTH || fy >= HEIGHT) return 1;
                if(fy >= 0 && field[fy][fx]) return 1;
            }
        }
    }
    return 0;
}

void placeShape() {
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            if(currentShape[i][j]) {
                if(y+i >= 0) field[y+i][x+j] = 1;
            }
}

void clearLines() {
    for(int i=0;i<HEIGHT;i++) {
        int full = 1;
        for(int j=0;j<WIDTH;j++) {
            if(!field[i][j]) {
                full = 0;
                break;
            }
        }
        if(full) {
            for(int k=i;k>0;k--)
                for(int j=0;j<WIDTH;j++)
                    field[k][j] = field[k-1][j];
            for(int j=0;j<WIDTH;j++)
                field[0][j] = 0;
            score += 100; // add points per line cleared
        }
    }
}

// -------- ROTATION --------
void rotateShape() {
    int temp[4][4];
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            temp[j][3-i] = currentShape[i][j];
    for(int i=0;i<4;i++) {
        for(int j=0;j<4;j++) {
            if(temp[i][j]) {
                int fx = x + j;
                int fy = y + i;
                if(fx < 0 || fx >= WIDTH || fy >= HEIGHT || (fy >= 0 && field[fy][fx])) {
                    return;
                }
            }
        }
    }
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            currentShape[i][j] = temp[i][j];
}

// -------- MAIN --------
int main() {
    srand(time(0));
    showControls(); // SHOW CONTROLS FIRST
    printf("\033[2J"); // clear screen
    copyShape(rand()%7);
    while(1) {
        draw();
        usleep(300000);
        if(kbhit()) {
            int ch = getch();
            if(ch == 'r' || ch == 'R') rotateShape();
            if(ch == ' ') {
                while(!collision(x, y+1)) y++;
            }
            if(ch == 27) {
                if(getch() == 91) {
                    ch = getch();
                    if(ch == 68 && !collision(x-1,y)) x--;
                    if(ch == 67 && !collision(x+1,y)) x++;
                    if(ch == 66 && !collision(x,y+1)) y++;
                }
            }
        }
        if(!collision(x,y+1)) {
            y++;
        } else {
            placeShape();
            clearLines();
            x = WIDTH/2 - 2;
            y = 0;
            copyShape(rand()%7);
            if(collision(x,y)) {
                printf("Game Over!\nFinal Score: %d\n", score);
                break;
            }
        }
    }
    return 0;
}
