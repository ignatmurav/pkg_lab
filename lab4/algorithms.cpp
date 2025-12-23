#include "algorithms.h"
#include <cmath>
#include <algorithm>

std::vector<QPoint> step_line(int x0, int y0, int x1, int y1)
{
    std::vector<QPoint> pts;
    // Parametric step sampling with small step = 1 / max(|dx|,|dy|)
    int dx = x1 - x0;
    int dy = y1 - y0;
    int n = std::max(std::abs(dx), std::abs(dy));
    if (n == 0) { pts.emplace_back(x0, y0); return pts; }
    double step = 1.0 / n;
    for (double t = 0.0; t <= 1.0 + 1e-9; t += step) {
        double xf = x0 + t * dx;
        double yf = y0 + t * dy;
        int xi = int(std::round(xf));
        int yi = int(std::round(yf));
        pts.emplace_back(xi, yi);
    }
    // unique
    std::sort(pts.begin(), pts.end(), [](const QPoint &a, const QPoint &b){
        if (a.x() == b.x()) return a.y() < b.y();
        return a.x() < b.x();
    });
    pts.erase(std::unique(pts.begin(), pts.end(), [](const QPoint&a,const QPoint&b){ return a==b; }), pts.end());
    return pts;
}

std::vector<QPoint> dda_line(int x0, int y0, int x1, int y1)
{
    std::vector<QPoint> pts;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) { pts.emplace_back(x0,y0); return pts; }
    double xinc = dx / double(steps);
    double yinc = dy / double(steps);
    double x = x0, y = y0;
    for (int i = 0; i <= steps; ++i) {
        pts.emplace_back(int(std::round(x)), int(std::round(y)));
        x += xinc;
        y += yinc;
    }
    // remove duplicates
    pts.erase(std::unique(pts.begin(), pts.end(), [](const QPoint&a,const QPoint&b){ return a==b; }), pts.end());
    return pts;
}

std::vector<QPoint> bresenham_line(int x0, int y0, int x1, int y1)
{
    std::vector<QPoint> pts;
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = std::abs(y1 - y0);
    int err = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;
    for (int x = x0; x <= x1; ++x) {
        if (steep) pts.emplace_back(y, x); else pts.emplace_back(x, y);
        err -= dy;
        if (err < 0) {
            y += ystep;
            err += dx;
        }
    }
    return pts;
}

std::vector<QPoint> bresenham_circle(int xc, int yc, int r)
{
    std::vector<QPoint> pts;
    if (r < 0) return pts;
    int x = 0, y = r;
    int d = 3 - 2 * r;
    auto plot8 = [&](int xi, int yi) {
        pts.emplace_back(xc + xi, yc + yi);
        pts.emplace_back(xc - xi, yc + yi);
        pts.emplace_back(xc + xi, yc - yi);
        pts.emplace_back(xc - xi, yc - yi);
        pts.emplace_back(xc + yi, yc + xi);
        pts.emplace_back(xc - yi, yc + xi);
        pts.emplace_back(xc + yi, yc - xi);
        pts.emplace_back(xc - yi, yc - xi);
    };
    while (y >= x) {
        plot8(x,y);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
    // unique points
    std::sort(pts.begin(), pts.end(), [](const QPoint &a, const QPoint &b){
        if (a.x() == b.x()) return a.y() < b.y();
        return a.x() < b.x();
    });
    pts.erase(std::unique(pts.begin(), pts.end(), [](const QPoint&a,const QPoint&b){ return a==b; }), pts.end());
    return pts;
}


std::vector<QPoint> kastl_ptv_line(int x0, int y0, int x1, int y1)
{
    std::vector<QPoint> pts;



    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    int x = x0;
    int y = y0;

    // Основной цикл
    while (true) {
        pts.emplace_back(x, y);

        // Если достигли конечной точки, выходим
        if (x == x1 && y == y1) break;

        int e2 = 2 * err;

        // Коррекция по оси X
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }

        // Коррекция по оси Y
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    return pts;
}
