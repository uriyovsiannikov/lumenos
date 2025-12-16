#include "sgl.h"
#include "../drivers/vga/vga.h"
#include "../libs/print.h"
#define abs(x) (x < 0 ? 0 - x : x)
#define ceil(x) ((int)x + 1)
#define floor(x) (int)x
#define signbit(x) (x < 0.0 ? 1 : 0)
#define round(x) signbit(x) ? ceil(x - 0.5) : floor(x + 0.5)
#define swap(x, y) { (x)=(x)^(y); (y)=(x)^(y); (x)=(x)^(y); }
/********************************
 *      Private Functions       *
 ********************************/
static void circle(int xc, int yc, int x, int y, unsigned short color);
static void sglFillFlatBottomTriangle(int x0, int y0, int x1, int y1, int x2, int y2, unsigned short color);
static void sglFillFlatTopTriangle(int x0, int y0, int x1, int y1, int x2, int y2, unsigned short color);
/********************************
 *      Public SGL API          *
 ********************************/
void sglSwapBuffers() {
    xvSwapBuffers();
}
void sglClear(unsigned short color) {
    xvclroffscreen(color);
}
void sglPutPixel(int x, int y, unsigned short color) {
    xvPlotPixelf(x, y, color);
}
void sglDrawLine(int x0, int y0, int x1, int y1, unsigned short color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int step = (abs(dx) >= abs(dy)) ? abs(dx) : abs(dy);
    float xinc = dx / (float)(step);
    float yinc = dy / (float)(step);
    float x = x0;
    float y = y0;
    for (int i = 0; i < step; ++i) {
        sglPutPixel(round(x), round(y), color);
        x += xinc;
        y += yinc;
    }
}
void sglDrawRect(int x, int y, int width, int height, unsigned short color) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            sglPutPixel(x+i, y+j, color);
        }
    }
}
void sglDrawCircle(int xc, int yc, int r, unsigned short color) {
    int x = 0, y = r; 
    int d = 3 - 2 * r;
    circle(xc, yc, x, y, color); 
    while (y >= x) { 
        x++; 
        if (d > 0) { 
            y--;  
            d = d + 4 * (x - y) + 10; 
        } else {
            d = d + 4 * x + 6;
        }
        circle(xc, yc, x, y, color);
    }
}
void sglDrawTri(int x0, int y0, int x1, int y1, int x2, int y2, unsigned short color) {
    sglDrawLine(x0, y0, x1, y1, color);
    sglDrawLine(x1, y1, x2, y2, color);
    sglDrawLine(x2, y2, x0, y0, color);
}
/**
* Draw a filled triangle with the flat-top/flat-bottom method
* We split the original triangle in two, half flat-bottom and half flat-top
*
*
*          (x0,y0)
*            / \
*           /   \
*          /     \
*         /       \
*        /         \
*   (x1,y1)------(Mx,My)
*       \_           \
*          \_         \
*             \_       \
*                \_     \
*                   \    \
*                     \_  \
*                        \_\
*                           \
*                         (x2,y2)
*
*/
void sglDrawFilledTri(int x0, int y0, int x1, int y1, int x2, int y2, unsigned short color) {
    if (y0 > y1) {
        swap(y0, y1);
        swap(x0, x1);
    }
    if (y1 > y2) {
        swap(y1, y2);
        swap(x1, x2);
    }
    if (y0 > y1) {
        swap(y0, y1);
        swap(x0, x1);
    }
    if (y1 == y2) {
        sglFillFlatBottomTriangle(x0, y0, x1, y1, x2, y2, color);
    } else if (y0 == y1) {
        sglFillFlatTopTriangle(x0, y0, x1, y1, x2, y2, color);
    } else {
        int My = y1;
        int Mx = (((x2 - x0) * (y1 - y0)) / (y2 - y0)) + x0;
        sglFillFlatBottomTriangle(x0, y0, x1, y1, Mx, My, color);
        sglFillFlatTopTriangle(x1, y1, Mx, My, x2, y2, color);
    }
}
/**
* Draw a filled a triangle with a flat bottom
*
*
*        (x0,y0)
*          / \
*         /   \
*        /     \
*       /       \
*      /         \
*  (x1,y1)------(x2,y2)
*
*/
void sglFillFlatBottomTriangle(int x0, int y0, int x1, int y1, int x2, int y2, unsigned short color) {
    float invSlope1 = (float)(x1 - x0) / (y1 - y0);
    float invSlope2 = (float)(x2 - x0) / (y2 - y0);
    float xStart = x0;
    float xEnd = x0;
    for (int y = y0; y <= y2; ++y) {
        sglDrawLine(xStart, y, xEnd, y, color);
        xStart += invSlope1;
        xEnd += invSlope2;
    }
}
/**
* Draw a filled a triangle with a flat top
*
*
*  (x0,y0)------(x1,y1)
*      \         /
*       \       /
*        \     /
*         \   /
*          \ /
*        (x2,y2)
*
*/
void sglFillFlatTopTriangle(int x0, int y0, int x1, int y1, int x2, int y2, unsigned short color) {
    float invSlope1 = (float)(x2 - x0) / (y2 - y0);
    float invSlope2 = (float)(x2 - x1) / (y2 - y1);
    float xStart = x2;
    float xEnd = x2;
    for (int y = y2; y >= y0; --y) {
        sglDrawLine(xStart, y, xEnd, y, color);
        xStart -= invSlope1;
        xEnd -= invSlope2;
    }
}
void circle(int xc, int yc, int x, int y, unsigned short color) {
    sglPutPixel(xc+x, yc+y, color); 
    sglPutPixel(xc-x, yc+y, color); 
    sglPutPixel(xc+x, yc-y, color); 
    sglPutPixel(xc-x, yc-y, color); 
    sglPutPixel(xc+y, yc+x, color); 
    sglPutPixel(xc-y, yc+x, color); 
    sglPutPixel(xc+y, yc-x, color); 
    sglPutPixel(xc-y, yc-x, color); 
}
void sglDrawText(int x, int y, const char* text, unsigned short color) {
    static const unsigned char font_8x8[128][8] = {
        [32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 
        [33] = {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, 
        [48] = {0x3C, 0x66, 0x76, 0x6E, 0x66, 0x66, 0x3C, 0x00}, 
        [49] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, 
        [50] = {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00}, 
        [51] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}, 
        [52] = {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00}, 
        [53] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}, 
        [54] = {0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}, 
        [55] = {0x7E, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18, 0x00}, 
        [56] = {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}, 
        [57] = {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00}, 
        [65] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}, 
        [66] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, 
        [67] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}, 
        [68] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}, 
        [69] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00}, 
        [70] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00}, 
        [71] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00}, 
        [72] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, 
        [73] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, 
        [74] = {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}, 
        [75] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}, 
        [76] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, 
        [77] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, 
        [78] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00}, 
        [79] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, 
        [80] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}, 
        [81] = {0x3C, 0x66, 0x66, 0x66, 0x6A, 0x6C, 0x36, 0x00}, 
        [82] = {0x7C, 0x66, 0x66, 0x7C, 0x68, 0x6C, 0x66, 0x00}, 
        [83] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}, 
        [84] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, 
        [85] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, 
        [86] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, 
        [87] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, 
        [88] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}, 
        [89] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, 
        [90] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}, 
        [97] = {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00}, 
        [98] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00}, 
        [99] = {0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00}, 
        [100] = {0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00}, 
        [101] = {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00}, 
        [102] = {0x1C, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x30, 0x00}, 
        [103] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C}, 
        [104] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}, 
        [105] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00}, 
        [106] = {0x0C, 0x00, 0x1C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38}, 
        [107] = {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00}, 
        [108] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, 
        [109] = {0x00, 0x00, 0x76, 0x7F, 0x6B, 0x6B, 0x63, 0x00}, 
        [110] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}, 
        [111] = {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00}, 
        [112] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60}, 
        [113] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06}, 
        [114] = {0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00}, 
        [115] = {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00}, 
        [116] = {0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x1C, 0x00}, 
        [117] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00}, 
        [118] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, 
        [119] = {0x00, 0x00, 0x63, 0x6B, 0x6B, 0x7F, 0x36, 0x00}, 
        [120] = {0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00}, 
        [121] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C}, 
        [122] = {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}, 
    };
    int cursor_x = x;
    int cursor_y = y;
    while (*text) {
        unsigned char c = *text++;
        if (c < 32 || c > 126) {
            cursor_x += 8;
            continue;
        }
        for (int row = 0; row < 8; row++) {
            unsigned char row_data = font_8x8[c][row];
            for (int col = 0; col < 8; col++) {
                if (row_data & (1 << (7 - col))) {
                    sglPutPixel(cursor_x + col, cursor_y + row, color);
                }
            }
        }
        cursor_x += 8;
        if (cursor_x > 300 && c == ' ') {
            cursor_x = x;
            cursor_y += 10;
        }
    }
}
void sgl_test() {
    xvInitGfxMode(MODE13H);
    sglClear(BLACK);
    sglDrawRect(150, 10, 100, 50, GREEN);
    sglDrawLine(120, 14, 24, 30, RED);
	sglDrawText(150, 20, "TEST", RED);
    sglDrawCircle(100,100,50, CYAN);
    sglDrawFilledTri(100, 100, 50, 150, 120, 120, MAGENTA);
    sglDrawTri(200, 100, 150, 170, 250, 190, YELLOW);
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 100; j++) {
            sglPutPixel(i, 50 + j, i);
        }
    }
    sglSwapBuffers();
}
