#include "clipping.h"
#include <algorithm>

// Outcode bits for Cohen-Sutherland
const int INSIDE = 0; // 0000
const int LEFT_ = 1;  // 0001
const int RIGHT_ = 2; // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

bool computeCohenOutcode(const QPointF &p, const Rect &clip, int &outcode){
    outcode = INSIDE;
    if (p.x() < clip.xmin) outcode |= LEFT_;
    else if (p.x() > clip.xmax) outcode |= RIGHT_;
    if (p.y() < clip.ymin) outcode |= BOTTOM;
    else if (p.y() > clip.ymax) outcode |= TOP;
    return true;
}

bool cohenSutherlandClipSegment(const QPointF &p0, const QPointF &p1, const Rect &clip, QPointF &out0, QPointF &out1)
{
    QPointF a = p0, b = p1;
    int outcode0 = 0, outcode1 = 0;
    computeCohenOutcode(a, clip, outcode0);
    computeCohenOutcode(b, clip, outcode1);

    bool accept = false;
    while (true){
        if ((outcode0 | outcode1) == 0){
            // both inside
            accept = true; break;
        } else if (outcode0 & outcode1){
            // trivial reject
            break;
        } else {
            // choose a point outside
            int outcodeOut = outcode0 ? outcode0 : outcode1;
            double x, y;
            // find intersection with clip boundary
            if (outcodeOut & TOP){
                x = a.x() + (b.x()-a.x()) * (clip.ymax - a.y()) / (b.y()-a.y());
                y = clip.ymax;
            } else if (outcodeOut & BOTTOM){
                x = a.x() + (b.x()-a.x()) * (clip.ymin - a.y()) / (b.y()-a.y());
                y = clip.ymin;
            } else if (outcodeOut & RIGHT_){
                y = a.y() + (b.y()-a.y()) * (clip.xmax - a.x()) / (b.x()-a.x());
                x = clip.xmax;
            } else { // LEFT_
                y = a.y() + (b.y()-a.y()) * (clip.xmin - a.x()) / (b.x()-a.x());
                x = clip.xmin;
            }

            // replace outside point with intersection
            if (outcodeOut == outcode0){
                a.setX(x); a.setY(y);
                computeCohenOutcode(a, clip, outcode0);
            } else {
                b.setX(x); b.setY(y);
                computeCohenOutcode(b, clip, outcode1);
            }
        }
    }
    if (accept){
        out0 = a; out1 = b;
        return true;
    }
    return false;
}

// Sutherland-Hodgman helpers: clip against one edge
static inline bool insideEdge(const QPointF &p, int edge, const Rect &clip){
    // edge: 0 = left, 1 = right, 2 = bottom, 3 = top
    switch(edge){
    case 0: return p.x() >= clip.xmin;
    case 1: return p.x() <= clip.xmax;
    case 2: return p.y() >= clip.ymin;
    case 3: return p.y() <= clip.ymax;
    }
    return true;
}

static inline QPointF intersectEdge(const QPointF &a, const QPointF &b, int edge, const Rect &clip){
    double x1=a.x(), y1=a.y(), x2=b.x(), y2=b.y();
    double x=0,y=0;
    double dx = x2 - x1, dy = y2 - y1;
    if (edge==0){ // left x = xmin
        x = clip.xmin;
        if (dx != 0) y = y1 + dy * (clip.xmin - x1) / dx;
        else y = y1;
    } else if (edge==1){ // right x = xmax
        x = clip.xmax;
        if (dx != 0) y = y1 + dy * (clip.xmax - x1) / dx;
        else y = y1;
    } else if (edge==2){ // bottom y = ymin
        y = clip.ymin;
        if (dy != 0) x = x1 + dx * (clip.ymin - y1) / dy;
        else x = x1;
    } else { // top y = ymax
        y = clip.ymax;
        if (dy != 0) x = x1 + dx * (clip.ymax - y1) / dy;
        else x = x1;
    }
    return QPointF(x,y);
}

QVector<QPointF> sutherlandHodgmanClip(const QVector<QPointF> &poly, const Rect &clip)
{
    QVector<QPointF> output = poly;
    // four edges: left(0), right(1), bottom(2), top(3)
    for (int edge=0; edge<4; ++edge){
        QVector<QPointF> input = output;
        output.clear();
        if (input.isEmpty()) break;
        QPointF S = input.back();
        for (const QPointF &E : input){
            bool Ein = insideEdge(E, edge, clip);
            bool Sin = insideEdge(S, edge, clip);
            if (Ein){
                if (Sin){
                    // both in
                    output.append(E);
                } else {
                    // S out, E in => add intersection then E
                    QPointF I = intersectEdge(S, E, edge, clip);
                    output.append(I);
                    output.append(E);
                }
            } else {
                if (Sin){
                    // S in, E out => add intersection
                    QPointF I = intersectEdge(S, E, edge, clip);
                    output.append(I);
                } else {
                    // both out -> nothing
                }
            }
            S = E;
        }
    }
    return output;
}
