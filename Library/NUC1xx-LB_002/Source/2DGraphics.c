//
// 2D Graphics Driver
//
#include <stdio.h>
#include <string.h>
#include "NUC1xx.h"
#include "LCD.h"

void LineBresenham(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fgColor, uint16_t bgColor)
{
    int16_t dy = y2 - y1;
    int16_t dx = x2 - x1;
    int16_t stepx, stepy;

    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;        // dy is now 2*dy
    dx <<= 1;        // dx is now 2*dx

    draw_Pixel(x1,y1, fgColor, bgColor);
    if (dx > dy) 
    {
        int fraction = dy - (dx >> 1);  // same as 2*dy - dx
        while (x1 != x2) 
        {
           if (fraction >= 0) 
           {
               y1 += stepy;
               fraction -= dx;          // same as fraction -= 2*dx
           }
           x1 += stepx;
           fraction += dy;              // same as fraction -= 2*dy
           draw_Pixel(x1, y1, fgColor, bgColor);
        }
     } else {
        int fraction = dx - (dy >> 1);
        while (y1 != y2) {
           if (fraction >= 0) {
               x1 += stepx;
               fraction -= dy;
           }
           y1 += stepy;
           fraction += dx;
           draw_Pixel(x1, y1, fgColor, bgColor);
        }
     }
}

void LineOptimized(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fgColor, uint16_t bgColor)
{
    int16_t cx, cy,
        ix, iy,
        dx, dy, 
        ddx= x2-x1, ddy= y2-y1;
     
    if (!ddx) { //vertical line special case
        if (ddy > 0) {
            cy= y1;  
            while (cy<= y2) draw_Pixel(x1, cy++, fgColor, bgColor);
        } else {
            cy= y2;
            while (cy <= y1) draw_Pixel(x1, cy++, fgColor, bgColor);
        }
    }
    if (!ddy) { //horizontal line special case
        if (ddx > 0) {
            cx= x1;
            while (++cx <= x2) draw_Pixel(cx, y1, fgColor, bgColor);
        } else {
            cx= x2; 
            while (++cx <= x1) draw_Pixel(cx, y1, fgColor, bgColor);
        }
    }
    if (ddy < 0) { iy= -1; ddy= -ddy; }//pointing up
            else iy= 1;
    if (ddx < 0) { ix= -1; ddx= -ddx; }//pointing left
            else ix= 1;
    dx= dy= ddx*ddy;
    cy= y1, cx= x1; 
    if (ddx < ddy) { // < 45 degrees, a tall line    
        while (dx > 0) {
            dx-=ddy;
            while (dy >=dx) {
                draw_Pixel(cx, cy, fgColor, bgColor);
                cy+=iy, dy-=ddx;
            };
            cx+=ix;
        };
    } else { // >= 45 degrees, a wide line
        while (dy > 0) {
            dy-=ddx;
            while (dx >=dy) {
                draw_Pixel(cx, cy, fgColor, bgColor);
                cx+=ix, dx-=ddy;
            } ;
            cy+=iy;
        };
    }
}

void CircleBresenham(int16_t xc, int16_t yc, int16_t r, uint16_t fgColor, uint16_t bgColor)
{
    int16_t x = 0; 
    int16_t y = r; 
    int16_t p = 3 - 2 * r;
    if (!r) return;     
    while (y >= x) // only formulate 1/8 of circle
    {
        draw_Pixel(xc-x, yc-y, fgColor, bgColor);//upper left left
        draw_Pixel(xc-y, yc-x, fgColor, bgColor);//upper upper left
        draw_Pixel(xc+y, yc-x, fgColor, bgColor);//upper upper right
        draw_Pixel(xc+x, yc-y, fgColor, bgColor);//upper right right
        draw_Pixel(xc-x, yc+y, fgColor, bgColor);//lower left left
        draw_Pixel(xc-y, yc+x, fgColor, bgColor);//lower lower left
        draw_Pixel(xc+y, yc+x, fgColor, bgColor);//lower lower right
        draw_Pixel(xc+x, yc+y, fgColor, bgColor);//lower right right
        if (p < 0) p += 4*(x++) + 6; 
        else p += 4*((x++) - y--) + 10; 
     } 
}

void CircleMidpoint(int16_t xc, int16_t yc, int16_t r, uint16_t fgColor, uint16_t bgColor)
{
    int16_t x= 0, y= r;
    int   d= 1-r;
    int   dE= 3;
    int   dSE= 5 - 2*r;

    if (!r) return;
    draw_Pixel(xc-r, yc, fgColor, bgColor);
    draw_Pixel(xc+r, yc, fgColor, bgColor);
    draw_Pixel(xc, yc-r, fgColor, bgColor);
    draw_Pixel(xc, yc+r, fgColor, bgColor);
      
    while (y > x)    //only formulate 1/8 of circle
    {
        if (d < 0) 
        {
            d+= dE;
            dE+=2, dSE+=2;
        } else {
            d+=dSE;
            dE+=2, dSE+=4;
            y--;
        }
        x++;
 
        draw_Pixel(xc-x, yc-y, fgColor, bgColor);//upper left left
        draw_Pixel(xc-y, yc-x, fgColor, bgColor);//upper upper left
        draw_Pixel(xc+y, yc-x, fgColor, bgColor);//upper upper right
        draw_Pixel(xc+x, yc-y, fgColor, bgColor);//upper right right
        draw_Pixel(xc-x, yc+y, fgColor, bgColor);//lower left left
        draw_Pixel(xc-y, yc+x, fgColor, bgColor);//lower lower left
        draw_Pixel(xc+y, yc+x, fgColor, bgColor);//lower lower right
        draw_Pixel(xc+x, yc+y, fgColor, bgColor);//lower right right
     }
}

void CircleOptimized(int16_t xc, int16_t yc, int16_t r, uint16_t fgColor, uint16_t bgColor)
{
    uint16_t x= r, y= 0;//local coords     
    int          cd2= 0;    //current distance squared - radius squared

    if (!r) return; 
    draw_Pixel(xc-r, yc, fgColor, bgColor);
    draw_Pixel(xc+r, yc, fgColor, bgColor);
    draw_Pixel(xc, yc-r, fgColor, bgColor);
    draw_Pixel(xc, yc+r, fgColor, bgColor);
 
    while (x > y)    //only formulate 1/8 of circle
    {
        cd2-= (--x) - (++y);
        if (cd2 < 0) cd2+=x++;

        draw_Pixel(xc-x, yc-y, fgColor, bgColor);//upper left left
        draw_Pixel(xc-y, yc-x, fgColor, bgColor);//upper upper left
        draw_Pixel(xc+y, yc-x, fgColor, bgColor);//upper upper right
        draw_Pixel(xc+x, yc-y, fgColor, bgColor);//upper right right
        draw_Pixel(xc-x, yc+y, fgColor, bgColor);//lower left left
        draw_Pixel(xc-y, yc+x, fgColor, bgColor);//lower lower left
        draw_Pixel(xc+y, yc+x, fgColor, bgColor);//lower lower right
        draw_Pixel(xc+x, yc+y, fgColor, bgColor);//lower right right
     } 
}

void RectangleDraw(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t fgColor, uint16_t bgColor)
{
	int16_t x,y, tmp;
	if (x0>x1) { tmp = x1; x1 = x0; x0 = tmp; }
	if (y0>y1) { tmp = y1; y1 = y0; y0 = tmp; }
  for (x=x0; x<=x1; x++) draw_Pixel(x,y0,fgColor, bgColor);
	for (y=y0; y<=y1; y++) draw_Pixel(x0,y,fgColor, bgColor);
	for (x=x0; x<=x1; x++) draw_Pixel(x,y1,fgColor, bgColor);
 	for (y=y0; y<=y1; y++) draw_Pixel(x1,y,fgColor, bgColor);		
}

void RectangleFill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t fgColor, uint16_t bgColor)
{
	int16_t x,y, tmp;
	if (x0>x1) { tmp = x1; x1 = x0; x0 = tmp; }
	if (y0>y1) { tmp = y1; y1 = y0; y0 = tmp; } 
    for (x=x0; x<=x1; x++) {
		for (y=y0; y<=y1; y++) 	{
			draw_Pixel(x,y, fgColor, bgColor);	
			}
		}
}

void Triangle (int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fgColor, uint16_t bgColor)
{
	LineBresenham(x0, y0, x1, y1, fgColor, bgColor);
	LineBresenham(x1, y1, x2, y2, fgColor, bgColor);
	LineBresenham(x0, y0, x2, y2, fgColor, bgColor);
}
