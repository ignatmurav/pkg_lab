#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <vector>
#include <QPoint>

// All functions return a vector of integer grid points (QPoint: x,y)

std::vector<QPoint> step_line(int x0, int y0, int x1, int y1);
std::vector<QPoint> dda_line(int x0, int y0, int x1, int y1);
std::vector<QPoint> bresenham_line(int x0, int y0, int x1, int y1);
std::vector<QPoint> bresenham_circle(int xc, int yc, int r);
std::vector<QPoint> kastl_ptv_line(int x0, int y0, int x1, int y1);

#endif // ALGORITHMS_H
