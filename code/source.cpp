/* TO DO:
   [x] clean up: rename some vars for better readability
   [x] make two arrays; one for viewport coordinates, another for end NDC coordinates.
   [x] unions
   [x] parametrize obstacles.
   [x] add floor and ceiling to collisions
   [x] abstract collision function
   [x] add circular collision - ended up remaking collision. Took 10 minutes after got me head out the ass. As usual.
   [x] 2D distance function
   [x] write a new player shader
   [x] exclude player coords from the float arrays
   [x] new write player function
   [x] switch player from screensized polygon to a strict quad
   [x] draw the player with a texture
   [ ] pause button. Just for practice.

   later
   [ ] generate levels randomly. Have an array for like 100 numbers.
       - before 100, rand and write a number. When at 100, go back to 0.
   [x] enforce FPS
   [ ] text &
   [ ] buttons? NAH, just "tap to tralala" although, I'll have to do buttons on Android though, won't I... continue/exit one.
 */

/* TO DO: 14.10.16
   [x] pause functionality
   - [x] teach the pause to sleep the CPU
   [ ] on collision,
   - [ ] flash red for X time...
   -- [\] time thing-line something?
   -- [ ] demetrispanos: state variable on the bird "frames_red = X", frames_red--.
   - [ ] collision position correction
 */


#if defined (__APPLE_CC__)
#include <unistd.h>
#else
#include <windows.h>
#endif

// C++ libs
#include <iostream>
#include <cmath>
#include <stdint.h>

// GL libs
#if defined (__APPLE_CC__)
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#else
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#endif

#include "types.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader.h"

#if defined (__APPLE_CC__)
#define SLEEP(x) usleep(x * 1000)
#else
#define SLEEP(x) Sleep(x)
#endif

#define PRINT(...) std::cout << __VA_ARGS__ << std::endl;

// function prototypes
void glClearColor(float color[4]);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow *window, int button, int action, int mods);



GLfloat BackgroundColor[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
//GLfloat color1[4] = { 1.0f, 0.5f, 0.2f, 1.0f };
//GLfloat color2[4] = { 1.0f, 0.0f, 0.0f, 1.0f };


const GLuint WIDTH = 480, HEIGHT = 800; // TO DO: this will have to be "get screen size from system".
const GLfloat screenWidth  = (float)WIDTH;
const GLfloat screenHeight = (float)HEIGHT;
GLfloat screenRatio  = screenWidth/screenHeight;


// TO DO: move everything into a global struct. A game state of sorts.
float playerX;
float playerY;
const float playerSize = screenHeight / 20.0f;
float playerRadius = playerSize / 2.0f;

float playerVerSpeed;
const float playerJumpSpeed = 500.0f;


const float obstacleWidth  = screenWidth / 4.8f;
float obstacleLeftX;
float obstacleRightX;
const float obstacleGap = screenWidth;

int sequencePointer = 0;
int gapLocationLeft, gapLocationRight;
const int sequenceSize = 10;
//int sequence[sequenceSize] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
int sequence[sequenceSize] = { 3, 4, 5, 7, 2, 6, 5, 3, 5, 8 };

const float gravity = 1500.0f;
const float obstacleHorSpeed = 200.0f;


bool collisionFlag = false;


float adjustmentX;
float adjustmentY;
bool adjusted;
int adjustment_step;
float precision = 0.01f;

// ----- -----
//     DATAS
// ----- -----

GLuint obstacleIndexes[] =
{
    0, 1, 2,   0, 2, 3,
    4, 5, 6,   4, 6, 7,
    8, 9, 10,  8, 10,11,
    12,13,14,  12,14,15,
    16,17,18,  16,18,19,// floor
    20,21,22,  20,22,23// ceiling
};

GLuint playerIndexes[] =
{
    0,1,2, 0,2,3
};
GLfloat texCoords[] =
{
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f
};


/*
  access to
  - all of the coordinates,
  - coordinates structured as points,
  - -//- as rectangles,
  - geometric description.
 */
typedef struct
{
    float x, y;
} Point;
typedef union
{
    struct
    {
        Point p0, p1, p2, p3;
        Point t0, t1, t2, t3;
    };
    struct
    {
        //float left, top, left, bottom, right, bottom, right, top; // you can't do this though...
        float left, top, pad, bottom, right; // can do this though!
    };
    float dump[16];
} TexQuad;

TexQuad VP_player;
TexQuad NDC_player;

// TO DO: add more directional description maybe?
typedef union
{
    struct
    {
        Point p0, p1, p2, p3;
    };
    struct
    {
        Point ULcorner, LLcorner, LRcorner, URcorner;
    };
    struct
    {
        float left, top, pad, bottom, right;
    };
} Rect;
typedef union
{
    struct
    {
        Rect r0, r1, r2, r3, r4, r5;
    };
    struct
    {
        //Rect UL, LL, LR, UR;    // ain't the correct order.
        Rect UL, LL, UR, LR, F, C;
    };
    float dump[48];
} SixRect;

SixRect VP_obstacles;
SixRect NDC_obstacles;


// for coordinates
float viewportToNDC(float v, float d) // value and dimension
{
    return (v - d/2.0f) / (d/2.0f);
}
// for values
float NDCvalue(float v, float d)
{
    return v * 2.0f/d;
}


// GOLD.
void WritePlayer(float y, float tx[])
{
    float left   = playerX - playerSize / 2.0f;
    float right  = playerX + playerSize / 2.0f;
    float top    = y + playerSize / 2.0f;
    float bottom = y - playerSize / 2.0f;

    float x1 = left;
    float x2 = left;
    float x3 = right;
    float x4 = right;
    float y1 = top;
    float y2 = bottom;
    float y3 = bottom;
    float y4 = top;

    VP_player.p0.x = x1;
    VP_player.p0.y = y1;
    VP_player.p1.x = x2;
    VP_player.p1.y = y2;
    VP_player.p2.x = x3;
    VP_player.p2.y = y3;
    VP_player.p3.x = x4;
    VP_player.p3.y = y4;

    VP_player.t0.x = tx[0];
    VP_player.t0.y = tx[1];
    VP_player.t1.x = tx[2];
    VP_player.t1.y = tx[3];
    VP_player.t2.x = tx[4];
    VP_player.t2.y = tx[5];
    VP_player.t3.x = tx[6];
    VP_player.t3.y = tx[7];
}

const float gapSize = 2.0f * screenHeight / 10.0f;
// GOLD.
void WriteObstacle(float lx, float rx, int gapPositionL, int gapPositionR)
{
    float offsetBottom = screenHeight / 8.0f;
    float offsetTop    = screenHeight / 8.0f * 7.0f;
    float scale     = offsetTop - offsetBottom;
    float gapJump   = scale / 10.0f;
    float vertBorders = screenHeight / 30.0f;
    float floor   = vertBorders;
    float ceiling = screenHeight - vertBorders;

    float gapCenter1 = offsetBottom + (gapJump / 2.0f) + (gapPositionL * gapJump);
    float gapCenter2 = offsetBottom + (gapJump / 2.0f) + (gapPositionR * gapJump);


    float left1  = lx - obstacleWidth / 2.0f;
    float right1 = lx + obstacleWidth / 2.0f;

    float topU1    = screenHeight;                // will have to replace with "ceiling"
    float bottomU1 = gapCenter1 + gapSize / 2.0f;
    float topL1    = gapCenter1 - gapSize / 2.0f;
    float bottomL1 = 0.0f;                        // and "floor".
    // I mean if you go crazy about meaningless improvements again, you OCD prick.

    VP_obstacles.r0.p0.x = left1;
    VP_obstacles.r0.p0.y =  topU1;
    VP_obstacles.r0.p1.x = left1;
    VP_obstacles.r0.p1.y =  bottomU1;
    VP_obstacles.r0.p2.x = right1;
    VP_obstacles.r0.p2.y =  bottomU1;
    VP_obstacles.r0.p3.x = right1;
    VP_obstacles.r0.p3.y =  topU1;

    VP_obstacles.r1.p0.x = left1;
    VP_obstacles.r1.p0.y =  topL1;
    VP_obstacles.r1.p1.x = left1;
    VP_obstacles.r1.p1.y =  bottomL1;
    VP_obstacles.r1.p2.x = right1;
    VP_obstacles.r1.p2.y =  bottomL1;
    VP_obstacles.r1.p3.x = right1;
    VP_obstacles.r1.p3.y =  topL1;


    float left2  = rx - obstacleWidth / 2.0f;
    float right2 = rx + obstacleWidth / 2.0f;

    float topU2    = screenHeight;
    float bottomU2 = gapCenter2 + gapSize / 2.0f;
    float topL2    = gapCenter2 - gapSize / 2.0f;
    float bottomL2 = 0.0f;

    VP_obstacles.r2.p0.x = left2;
    VP_obstacles.r2.p0.y =  topU2;
    VP_obstacles.r2.p1.x = left2;
    VP_obstacles.r2.p1.y =  bottomU2;
    VP_obstacles.r2.p2.x = right2;
    VP_obstacles.r2.p2.y =  bottomU2;
    VP_obstacles.r2.p3.x = right2;
    VP_obstacles.r2.p3.y =  topU2;

    VP_obstacles.r3.p0.x = left2;
    VP_obstacles.r3.p0.y =  topL2;
    VP_obstacles.r3.p1.x = left2;
    VP_obstacles.r3.p1.y =  bottomL2;
    VP_obstacles.r3.p2.x = right2;
    VP_obstacles.r3.p2.y =  bottomL2;
    VP_obstacles.r3.p3.x = right2;
    VP_obstacles.r3.p3.y =  topL2;


    // floor and ceiling
    float _left  = 0.0f;
    float _right = screenWidth;

    VP_obstacles.F.p0.x = _left;
    VP_obstacles.F.p0.y = floor;
    VP_obstacles.F.p1.x = _left;
    VP_obstacles.F.p1.y = 0.0f;
    VP_obstacles.F.p2.x = _right;
    VP_obstacles.F.p2.y = 0.0f;
    VP_obstacles.F.p3.x = _right;
    VP_obstacles.F.p3.y = floor;

    VP_obstacles.C.p0.x = _left;
    VP_obstacles.C.p0.y = screenHeight;
    VP_obstacles.C.p1.x = _left;
    VP_obstacles.C.p1.y = ceiling;
    VP_obstacles.C.p2.x = _right;
    VP_obstacles.C.p2.y = ceiling;
    VP_obstacles.C.p3.x = _right;
    VP_obstacles.C.p3.y = screenHeight;
}


void ConvertCircle(TexQuad *from, TexQuad *to)
{
    to->dump[0] = viewportToNDC(from->dump[0], screenWidth);
    to->dump[1] = viewportToNDC(from->dump[1], screenHeight);
    to->dump[2] = viewportToNDC(from->dump[2], screenWidth);
    to->dump[3] = viewportToNDC(from->dump[3], screenHeight);
    to->dump[4] = viewportToNDC(from->dump[4], screenWidth);
    to->dump[5] = viewportToNDC(from->dump[5], screenHeight);
    to->dump[6] = viewportToNDC(from->dump[6], screenWidth);
    to->dump[7] = viewportToNDC(from->dump[7], screenHeight);

    to->dump[8]  = from->dump[8];
    to->dump[9]  = from->dump[9];
    to->dump[10] = from->dump[10];
    to->dump[11] = from->dump[11];
    to->dump[12] = from->dump[12];
    to->dump[13] = from->dump[13];
    to->dump[14] = from->dump[14];
    to->dump[15] = from->dump[15];
}

void ConvertRects(SixRect *from, SixRect *to)
{
    to->dump[0]  = viewportToNDC(from->dump[0],  screenWidth);
    to->dump[1]  = viewportToNDC(from->dump[1],  screenHeight);
    to->dump[2]  = viewportToNDC(from->dump[2],  screenWidth);
    to->dump[3]  = viewportToNDC(from->dump[3],  screenHeight);
    to->dump[4]  = viewportToNDC(from->dump[4],  screenWidth);
    to->dump[5]  = viewportToNDC(from->dump[5],  screenHeight);
    to->dump[6]  = viewportToNDC(from->dump[6],  screenWidth);
    to->dump[7]  = viewportToNDC(from->dump[7],  screenHeight);

    to->dump[8]  = viewportToNDC(from->dump[8],  screenWidth);
    to->dump[9]  = viewportToNDC(from->dump[9],  screenHeight);
    to->dump[10] = viewportToNDC(from->dump[10], screenWidth);
    to->dump[11] = viewportToNDC(from->dump[11], screenHeight);
    to->dump[12] = viewportToNDC(from->dump[12], screenWidth);
    to->dump[13] = viewportToNDC(from->dump[13], screenHeight);
    to->dump[14] = viewportToNDC(from->dump[14], screenWidth);
    to->dump[15] = viewportToNDC(from->dump[15], screenHeight);

    to->dump[16] = viewportToNDC(from->dump[16], screenWidth);
    to->dump[17] = viewportToNDC(from->dump[17], screenHeight);
    to->dump[18] = viewportToNDC(from->dump[18], screenWidth);
    to->dump[19] = viewportToNDC(from->dump[19], screenHeight);
    to->dump[20] = viewportToNDC(from->dump[20], screenWidth);
    to->dump[21] = viewportToNDC(from->dump[21], screenHeight);
    to->dump[22] = viewportToNDC(from->dump[22], screenWidth);
    to->dump[23] = viewportToNDC(from->dump[23], screenHeight);

    to->dump[24] = viewportToNDC(from->dump[24], screenWidth);
    to->dump[25] = viewportToNDC(from->dump[25], screenHeight);
    to->dump[26] = viewportToNDC(from->dump[26], screenWidth);
    to->dump[27] = viewportToNDC(from->dump[27], screenHeight);
    to->dump[28] = viewportToNDC(from->dump[28], screenWidth);
    to->dump[29] = viewportToNDC(from->dump[29], screenHeight);
    to->dump[30] = viewportToNDC(from->dump[30], screenWidth);
    to->dump[31] = viewportToNDC(from->dump[31], screenHeight);

    to->dump[32] = viewportToNDC(from->dump[32], screenWidth);
    to->dump[33] = viewportToNDC(from->dump[33], screenHeight);
    to->dump[34] = viewportToNDC(from->dump[34], screenWidth);
    to->dump[35] = viewportToNDC(from->dump[35], screenHeight);
    to->dump[36] = viewportToNDC(from->dump[36], screenWidth);
    to->dump[37] = viewportToNDC(from->dump[37], screenHeight);
    to->dump[38] = viewportToNDC(from->dump[38], screenWidth);
    to->dump[39] = viewportToNDC(from->dump[39], screenHeight);

    to->dump[40] = viewportToNDC(from->dump[40], screenWidth);
    to->dump[41] = viewportToNDC(from->dump[41], screenHeight);
    to->dump[42] = viewportToNDC(from->dump[42], screenWidth);
    to->dump[43] = viewportToNDC(from->dump[43], screenHeight);
    to->dump[44] = viewportToNDC(from->dump[44], screenWidth);
    to->dump[45] = viewportToNDC(from->dump[45], screenHeight);
    to->dump[46] = viewportToNDC(from->dump[46], screenWidth);
    to->dump[47] = viewportToNDC(from->dump[47], screenHeight);
}


bool kk_pause = false;
float redbird = 0.0f;
void Reset()
{
    redbird = 0.0f;
    collisionFlag = false;
    kk_pause = false;              // ?

    playerX = screenWidth / 2.0f;
    playerY = screenHeight / 2.0f;
    WritePlayer(playerY, texCoords);
    playerVerSpeed = 0.0f;

    obstacleLeftX = 100.0f;
    obstacleRightX = obstacleLeftX + obstacleGap;

    sequencePointer = 0;
    gapLocationLeft  = sequence[sequencePointer++];
    gapLocationRight = sequence[sequencePointer++];

    WriteObstacle(obstacleLeftX, obstacleRightX, gapLocationLeft, gapLocationRight);


    // reset the adjustment vars here
    adjustmentX = 0;
    adjustmentY = 0;
    adjusted = false;
    adjustment_step = 0;
}


// correction should be here. Basically adjust playerY based on... data.
// Why this indirection?
void Collision()
{
    collisionFlag = true;
    redbird = 1.0f;
}


// TO DO: move these to a math thing
float HorDist(float x1, float x2)
{
    return std::abs(x1 - x2);
}
float VerDist(float y1, float y2)
{
    return std::abs(y1 - y2);
}
float Dist(float x1, float y1, float x2, float y2)
{
    return sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
}
float Dist(Point p1, Point p2)
{
    return Dist(p1.x, p1.y, p2.x, p2.y);
}

float Max(float f1, float f2)
{
    return f1 > f2 ? f1 : f2;
}
float Min(float f1, float f2)
{
    return f1 < f2 ? f1 : f2;
}



int64 GlobalPerfCountFrequency;

/*
LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}
real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = (real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency;
    return Result;
}
*/

// these could be made more general, with some stride trickery... just a "print me a struct"
void DebugPrintout_TexQuad(TexQuad tq)
{
    printf("-quad:    [%5.1f, %5.1f] [%5.1f, %5.1f] \n", tq.p0.x, tq.p0.y, tq.p3.x, tq.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", tq.p1.x, tq.p1.y, tq.p2.x, tq.p2.y);

    printf("-texture: [%5.1f, %5.1f] [%5.1f, %5.1f] \n", tq.t0.x, tq.t0.y, tq.t3.x, tq.t3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", tq.t1.x, tq.t1.y, tq.t2.x, tq.t2.y);
}
void DebugPrintout_SixRect(SixRect fr)
{
    printf("-rect0:   [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r0.p0.x, fr.r0.p0.y, fr.r0.p3.x, fr.r0.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r0.p1.x, fr.r0.p1.y, fr.r0.p2.x, fr.r0.p2.y);

    printf("-rect1:   [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r1.p0.x, fr.r1.p0.y, fr.r1.p3.x, fr.r1.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r1.p1.x, fr.r1.p1.y, fr.r1.p2.x, fr.r1.p2.y);

    printf("-rect2:   [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r2.p0.x, fr.r2.p0.y, fr.r2.p3.x, fr.r2.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r2.p1.x, fr.r2.p1.y, fr.r2.p2.x, fr.r2.p2.y);

    printf("-rect3:   [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r3.p0.x, fr.r3.p0.y, fr.r3.p3.x, fr.r3.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r3.p1.x, fr.r3.p1.y, fr.r3.p2.x, fr.r3.p2.y);

    printf("-ceiling: [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r5.p0.x, fr.r5.p0.y, fr.r5.p3.x, fr.r5.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r5.p1.x, fr.r5.p1.y, fr.r5.p2.x, fr.r5.p2.y);

    printf("-floor:   [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r4.p0.x, fr.r4.p0.y, fr.r4.p3.x, fr.r4.p3.y);
    printf("          [%5.1f, %5.1f] [%5.1f, %5.1f] \n", fr.r4.p1.x, fr.r4.p1.y, fr.r4.p2.x, fr.r4.p2.y);
}

// So here's the final piece of the puzzle: this is a rectangular check for the bird QUAD.
//  Need to write a new one for the circle, to check against the center!
bool Collision_CheckObstacleQUAD(Rect O, TexQuad P)
{
    bool result = false;
    if(O.left < P.right && O.right > P.left && O.bottom < P.top && O.top > P.bottom)
        result = true;

    return result;
}
bool Collision_CheckObstacle(Rect O, Point P)
{
    bool result = false;
    /*
      Demetris: "if any of the corners are within radius distance of the player center, you're done"
     */
    if(HorDist(O.left, P.x) < playerRadius)
        if(O.top > P.y && P.y > O.bottom)
            if(P.x + playerRadius > O.left)
                result = true;

    if(VerDist(O.top, playerY) < playerRadius)
        if(O.left < P.x && P.x < O.right)
            result = true;

    if(VerDist(O.bottom, playerY) < playerRadius)
        if(O.left < P.x && P.x < O.right)
            result = true;

    return result;
}
bool Collision_CheckObstaclesALL(SixRect O, Point P)
{
    bool result = false;

    if(Collision_CheckObstacle(O.UL, P))
        result = true;
    else if(Collision_CheckObstacle(O.LL, P))
        result = true;
    else if(Collision_CheckObstacle(O.LR, P))
        result = true;
    else if(Collision_CheckObstacle(O.UR, P))
        result = true;

    return result;
}

bool Collision_CheckCorner(Point O, Point P, float R)
{
    bool result = false;
    if(Dist(O, P) < R)
        result = true;

    return result;
}
bool Collision_CheckCornersALL(SixRect O, Point P, float R)
{
    bool result = false;

    if(Collision_CheckCorner(O.UR.LLcorner, P, R))
        result = true;
    else if(Collision_CheckCorner(O.LR.ULcorner, P, R))
        result = true;
    else if(Collision_CheckCorner(O.UL.LLcorner, P, R))
        result = true;
    else if(Collision_CheckCorner(O.LL.ULcorner, P, R))
        result = true;

    return result;
}

int main()
{
    // --- --- ---
    //    PREPARATIONS of bullshit
    // --- --- ---
    PRINT("Starting GLFW context, OpenGL 3.3");

    // init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // activate GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    // ---

    // init GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    GLfloat ratio = (float)WIDTH/(float)HEIGHT;


    // --- --- ---
    //    PREPARATIONS of stuff
    // --- --- ---

    Shader obstacleShader = Shader("obstacles.vsh", "obstacles.fsh");
    Shader playerShader   = Shader("texture.vsh", "texture.fsh");
    //Shader playerShader   = Shader("square.vsh", "square.fsh");




    // --- --- ---
    //    GPU WRANGLING
    // --- --- ---

    // --- player
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // do we need to do this here though?

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(playerIndexes), playerIndexes, GL_STATIC_DRAW);

    GLuint playerTexture;
    glGenTextures(1, &playerTexture);
    glBindTexture(GL_TEXTURE_2D, playerTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int playerWidth, playerHeight, components;
    unsigned char *image = stbi_load("playerTexture.png", &playerWidth, &playerHeight, &components, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, playerWidth, playerHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    // ---


    // --- obstacles
    GLuint VBO1;
    glGenBuffers(1, &VBO1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO1);

    GLuint VAO1;
    glGenVertexArrays(1, &VAO1);
    glBindVertexArray(VAO1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);


    GLuint EBO1;
    glGenBuffers(1, &EBO1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obstacleIndexes), obstacleIndexes, GL_STATIC_DRAW);
    // ---


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // --- --- ---
    //    TIME
    // --- --- ---
#if defined (__APPLE_CC__)
	real32 deltaTime = 1.0f/60.0f;
#else
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    int RefreshRate = 60;
    real32 TargetSecondsPerFrame = 1.0f / (real32)RefreshRate;

    LARGE_INTEGER LastCounter = Win32GetWallClock();
    uint64 LastCycleCount = __rdtsc();

    real32 deltaTime = 1.0f/(real32)RefreshRate;
#endif

    bool printed = false;
    Reset();

    while(!glfwWindowShouldClose(window))
    {
        // --- --- ---
        //    INPUT
        // --- --- ---
        glfwPollEvents();

        // what do we need?
        bool collisionFlag_obstacle = false;
        bool collisionFlag_corner = false;

        adjustmentX = 0;
        adjustmentY = 0;
        adjusted = false;
        adjustment_step = 0;
        precision = 0.0001f;

        if(kk_pause)
            SLEEP(1);            // for how long? For as long as is negligible for a player, but not CPU.
        else
        {
            // --- --- ---
            //    PROCESSING
            // --- --- ---

            // ---
            //   collision
            // ---

            // Idea: set corner and usual collision checks as different ones and check them differently.
            //  Should be enough, because I see quadratic collision being corrected as linear.

			Point p = {playerX, playerY};
#if 1
            if(Collision_CheckCornersALL(VP_obstacles, p, playerRadius))
            {
                collisionFlag_corner = true;
            }
            else
                #endif
            {
                if(Collision_CheckObstaclesALL(VP_obstacles, p))
                    collisionFlag_obstacle = true;
            }

            // Catch a collision fact and...
#if 1
            if(collisionFlag_corner)
            {
                while(!adjusted)
                {
                    adjusted = true;
                    if(Collision_CheckCornersALL(VP_obstacles, p, playerRadius))
                        adjusted = false;

                    // could pull this out too for readability.
                    adjustment_step++;
                    adjustmentX = adjustment_step * obstacleHorSpeed * (deltaTime * precision);
                    adjustmentY = adjustment_step * playerVerSpeed   * (deltaTime * precision);

                    playerY += adjustmentY;
                    obstacleLeftX  += adjustmentX;
                    obstacleRightX += adjustmentX;

                    WriteObstacle(obstacleLeftX, obstacleRightX, gapLocationLeft, gapLocationRight);
                    WritePlayer(playerY, texCoords);
                }
                Collision();
            }
            else
#endif
                if(collisionFlag_obstacle)
            {
                while(!adjusted)
                {
                    adjusted = true;
                    if(Collision_CheckObstaclesALL(VP_obstacles, p))
                        adjusted = false;

                    // could pull this out too for readability.
                    adjustment_step++;
                    adjustmentX = adjustment_step * obstacleHorSpeed * (deltaTime * precision);
                    adjustmentY = adjustment_step * playerVerSpeed   * (deltaTime * precision);

                    playerY += adjustmentY;
                    obstacleLeftX  += adjustmentX;
                    obstacleRightX += adjustmentX;

                    WriteObstacle(obstacleLeftX, obstacleRightX, gapLocationLeft, gapLocationRight);
                    WritePlayer(playerY, texCoords);
                }
                Collision();
            }
            else//(!collisionFlag_corner && !collisionFlag_obstacles) // not sure if this expression is correct. OR, just else it.
            {
                // OBSTACLE MOTION
                obstacleLeftX  -= obstacleHorSpeed * deltaTime;
                obstacleRightX -= obstacleHorSpeed * deltaTime;

                // PLAYER MOTION
                playerVerSpeed += gravity * deltaTime;
                playerY -= playerVerSpeed * deltaTime;

                // ACTUALLY WRITE THE COORDINATE CHANGE.
                WriteObstacle(obstacleLeftX, obstacleRightX, gapLocationLeft, gapLocationRight);
                WritePlayer(playerY, texCoords);
            }


            // CEILING-FLOOR CHECK
            // add it to the obstacles check... or something.
            if(VP_player.top >= VP_obstacles.C.bottom)
            {
                playerY -= (VP_player.top - VP_obstacles.C.bottom);
                WritePlayer(playerY, texCoords);
                Collision();
            }
            if(VP_player.bottom <= VP_obstacles.F.top)
            {
                playerY += (VP_obstacles.F.top - VP_player.bottom);
                WritePlayer(playerY, texCoords);
                Collision();
            }


            // OBSTACLE RESET
            // TO DO: cook some up to start the obstacles to the right of the player.
            if(obstacleLeftX <= -(obstacleWidth/2.0f))
            {
                obstacleLeftX   = obstacleRightX + obstacleGap;
                gapLocationLeft = sequence[sequencePointer++];

                if(sequencePointer == sequenceSize)
                    sequencePointer = 0;
            }
            if(obstacleRightX <= -(obstacleWidth/2.0f))
            {
                obstacleRightX   = obstacleLeftX + obstacleGap;
                gapLocationRight = sequence[sequencePointer++];

                if(sequencePointer == sequenceSize)
                    sequencePointer = 0;
            }




            // --- --- ---
            //    RENDERING
            // --- --- ---
            ConvertRects(&VP_obstacles, &NDC_obstacles);
            ConvertCircle(&VP_player, &NDC_player);


            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);


            obstacleShader.Use();

            glBindBuffer(GL_ARRAY_BUFFER, VBO1);
            glBufferData(GL_ARRAY_BUFFER, sizeof(NDC_obstacles.dump), NDC_obstacles.dump, GL_STATIC_DRAW);

            glBindVertexArray(VAO1);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);


            playerShader.Use();

            glUniform1f(2, redbird);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(NDC_player.dump), NDC_player.dump, GL_STATIC_DRAW);

            glBindTexture(GL_TEXTURE_2D, playerTexture);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


            glfwSwapBuffers(window);


            // --- --- ---
            //    TIME
            // --- --- ---

#if defined (__APPLE_CC__)
#else
            LARGE_INTEGER WorkCounter = Win32GetWallClock();
            real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

            real32 SecondsElapsedForFrame = WorkSecondsElapsed;
            if(SecondsElapsedForFrame < TargetSecondsPerFrame)
            {
                if(SleepIsGranular)
                {
                    DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                    if(SleepMS > 0)
                        Sleep(SleepMS);
                }

                real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                //Assert(TestSecondsElapsedForFrame < TargetSecondsPerFrame);
                while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
            }

            LARGE_INTEGER EndCounter = Win32GetWallClock();
            real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
            LastCounter = EndCounter;

            uint64 EndCycleCount = __rdtsc();
            uint64 CyclesElapsed = EndCycleCount - LastCycleCount;

            LastCycleCount = EndCycleCount;
            // Can the time code be taken out?
#endif

            // --- --- ---
            //    "POST-PROCESSING"
            // --- --- ---
            if(collisionFlag)
            {
                SLEEP(500);
                Reset();
            }

        }
    }
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &EBO);

    glDeleteBuffers(1, &VBO1);
    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &EBO1);


    glfwTerminate();
    PRINT("All good o7");
    return 0;
}

void PlayerJump()
{
    playerVerSpeed = -playerJumpSpeed;
}
void Pause()
{
    if(kk_pause)
        kk_pause = false;
    else
        kk_pause = true;
}

void glClearColor(float color[4])
{
    glClearColor(color[0], color[1], color[2], color[3]);
}
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if(key == GLFW_KEY_ENTER && action == GLFW_PRESS)
        Pause();
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        PlayerJump();
    if(key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
        Reset();
}
void mouse_callback(GLFWwindow *window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        PlayerJump();
}
