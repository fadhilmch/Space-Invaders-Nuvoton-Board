#ifndef __2D_Graphic_Driver_H__
#define __2D_Graphic_Driver_H__
     
extern void LineBresenham(int x1, int y1, int x2, int y2, uint16_t fg_color, uint16_t bg_color);
extern void LineOptimized(int x1, int y1, int x2, int y2, uint16_t fg_color, uint16_t bg_color);

extern void CircleBresenham(int xc, int yc, int r, uint16_t fg_color, uint16_t bg_color);
extern void CircleMidpoint(int xc, int yc, int r, uint16_t fg_color, uint16_t bg_color);
extern void CircleOptimized(int xc, int yc, int r, uint16_t fg_color, uint16_t bg_color);  

extern void RectangleDraw(int x0, int y0, int x1, int y1, uint16_t fg_color, uint16_t bg_color);
extern void RectangleFill(int x0, int y0, int x1, int y1, uint16_t fg_color, uint16_t bg_color);

extern void Triangle (int x0, int y0, int x1, int y1, int x2, int y2, uint16_t fg_color, uint16_t bg_color);

#endif
