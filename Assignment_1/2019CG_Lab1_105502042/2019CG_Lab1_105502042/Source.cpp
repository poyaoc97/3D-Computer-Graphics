#include <GL/glut.h>
#include <array>
#include <vector>
#include <cmath>

using namespace std;
using Coord = array<int, 2>;
constexpr int POINTSIZE { 1 };	// the number of pixels a point maps to
constexpr int WIDTH { 800 };
constexpr int HEIGHT { 600 };
unsigned char keyPressed;		// the last key being pressed

struct ops {					// ops stands for operations
    unsigned char op;
    vector<Coord> mouseCoords;
};
vector<Coord> mouseCoords;	// cleaned up after every draw and upon key press
vector<ops>   drawLog;		// records all drawing activities
vector<Coord> polyCoords;	// used for drawing polygons exclusively

inline void drawAPoint(Coord target, bool log = false) {
    auto [x, y] { target };	// unpacking
    glVertex2i(x, y);

    if (log) {
        drawLog.push_back({ 'd',{target} });
    }
}

void drawALine(Coord endpoint1, Coord endpoint2, bool log = false) {
    // unpack data
    auto [x1, y1] { endpoint1 };		// structured binding,requires C++17. didn't know u could do this, cool.
    auto [x2, y2] { endpoint2 };
    // drawing logic begins
    if (x2 < x1) {	// let (x1, y1) be the point on the left
        swap(x1, x2);
        swap(y1, y2);
    }
    glVertex2i(x1, y1);			// draw the first point no matter wut

    //preprocessing
    bool negativeSlope { y1 > y2 };
    if (negativeSlope) {		// see if the slope is negative
        y2 += 2 * (y1 - y2);	// mirror the line with respect to y = y1 for now, and voila, a positive slope line!
    }							// we draw this line as if the slope was positive in our head and draw it "upside down" on the screen in the while loop
    bool mGreaterThanOne { (y2 - y1) > (x2 - x1) };
    if (mGreaterThanOne) {		// slope greater than 1, swap x and y,  mirror the line with respect to y = x for now, mirror it again when drawing
        swap(x1, y1);
        swap(x2, y2);
    }
    int x { x1 };
    int y { y1 };
    int a { y2 - y1 };
    int b { x1 - x2 };
    int d { 2 * a + b };
    while (x < x2) {		// draw from left to right (recall that we make x2 be always on the right)
        if (d <= 0) {		// choose E
            d += 2 * a;
            if (mGreaterThanOne) {							// sort of "mirror" the negative slope line back to where it's supposed to be
                glVertex2i(y, !negativeSlope ? (++x) : (2 * x1 - ++x)); // slope > 1 y is actually x and vice versa
            } else {
                glVertex2i(++x, !negativeSlope ? (y) : (2 * y1 - y));
            }
        } else {				// choose NE
            d += 2 * (a + b);
            if (mGreaterThanOne) {
                glVertex2i(++y, !negativeSlope ? (++x) : (2 * x1 - ++x));
            } else {
                glVertex2i(++x, !negativeSlope ? (++y) : (2 * y1 - ++y));
            }
        }
    }

    if (log) {
        drawLog.push_back({ 'l', {endpoint1, endpoint2} });
    }
}

inline void drawAPolygon(Coord vertex, bool log = false, bool lastEdge = false) {	// draw polygon edge by edge
    if (lastEdge) {
        drawALine(polyCoords.back(), polyCoords.front());
        if (log) {
            drawLog.push_back({ 'p', polyCoords });
        }
        polyCoords.clear();
        return;
    }

    if (polyCoords.size()) {	// draw only if there are at least one vertex in vector
        drawALine(polyCoords.back(), vertex);
    }
    polyCoords.push_back(vertex);
}

inline void drawPolyAtOnce(const vector<Coord>& V, bool log = false) {
    drawALine(V.front(), V[1]);
    drawALine(V.front(), V.back());
    for (auto it = V.cbegin() + 1; it != V.cend(); it++) {
        drawALine(*(it - 1), *it);
    }
    if (log) {
        drawLog.push_back({ 'p', V });
    }
}

void drawACircle(Coord center, Coord pointOnCircle, bool log = false) {
    // unpack data
    auto [x1, y1] { center };
    auto [x2, y2] { pointOnCircle };
    // drawing logic begins
    int R { (int)sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2)) };
    int x { 0 };
    int y { R };
    int d { 1 - R };
    int incE { 3 };
    int incSE { -2 * R + 5 };
    glVertex2i(0 + x1, R + y1);
    glVertex2i(R + x1, 0 + y1);
    glVertex2i(-R + x1, 0 + y1);
    glVertex2i(0 + x1, -R + y1);
    while (x < y) {
        if (d < 0) {
            d += incE;
            incE += 2;
            incSE += 2;
            ++x;
        } else {
            d += incSE;
            incE += 2;
            incSE += 4;
            ++x;
            --y;
        }
        glVertex2i(x + x1, y + y1);	// shift the circle from (0, 0) to (x1, y1)
        glVertex2i(y + x1, x + y1);
        glVertex2i(x + x1, -y + y1);
        glVertex2i(y + x1, -x + y1);
        glVertex2i(-x + x1, y + y1);
        glVertex2i(-y + x1, x + y1);
        glVertex2i(-x + x1, -y + y1);
        glVertex2i(-y + x1, -x + y1);
    }

    if (log) {
        drawLog.push_back({ 'o', {center, pointOnCircle} });
    }
}

inline void restore() {
    glPointSize(POINTSIZE);
    glBegin(GL_POINTS);
    for (auto&[op, mouseCoords] : drawLog)
        switch (op) {
        case 'd':
            drawAPoint(mouseCoords.front());
            break;
        case 'l':
            drawALine(mouseCoords.front(), mouseCoords.back());
            break;
        case 'p':
            drawPolyAtOnce(mouseCoords);
            break;
        case 'o':
            drawACircle(mouseCoords.front(), mouseCoords.back());
        }
    glEnd();
    glFlush();	// flush only after restoring all those objects
}

void mouseHandler(int button, int state, int x, int y) {
    y = HEIGHT - y;	// treat the bottom left corner as the origin for mouse coordinate
    glPointSize(POINTSIZE);
    glBegin(GL_POINTS);
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            switch (keyPressed) {
            case 'd':
            case 'D':
                drawAPoint({ x, y }, true);
                break;
            case 'p':
            case 'P':
                drawAPolygon({ x, y }, true);
                break;
            default:
                mouseCoords.push_back({ x, y });	// push only if none of the above applies, which eliminates push overhead for the above cases
            }
        }
        if (state == GLUT_UP) {
            switch (keyPressed) {
            case 'l':
            case 'L':
                if (mouseCoords.size() == 2) {
                    drawALine(mouseCoords.front(), mouseCoords.back(), true);
                    mouseCoords.clear();
                }
                break;
            case 'o':
            case 'O':
                drawACircle(mouseCoords.front(), { x, y }, true);
                mouseCoords.clear();
            }
        }
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP && keyPressed == 'p' && polyCoords.size() >= 3) {	// Finish the polygon
        drawAPolygon({ 0, 0 }, true, true);
    }
    glEnd();
    glFlush();
}

void keyboardHandler(unsigned char key, int x, int y) {
    mouseCoords.clear();
    keyPressed = key;
    switch (key) {
    case 'q':
    case 'Q':
        exit(0);
    case 'c':
    case 'C':
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();
        break;
    case 'r':
    case 'R':
        restore();
    }
}

void displayFunc() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WIDTH, 0.0, HEIGHT);
    glFlush();
}

int main(int argc, char** argv) {
    // init GLUT and create Window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Your First GLUT Window!");
    // displayFunc is called whenever there is a need to redisplay the
    // window, e.g. when the window is exposed from under another window or
    // when the window is de-iconified
    glutDisplayFunc(displayFunc);
    glutMouseFunc(mouseHandler);
    glutKeyboardFunc(keyboardHandler);
    // set bargckground color
    glClearColor(0.0, 0.0, 0.0, 0.0); // set the bargckground
    glClear(GL_COLOR_BUFFER_BIT); // clear the buffer
    // enter GLUT event processing cycle
    glutMainLoop();
    return 0;
}
