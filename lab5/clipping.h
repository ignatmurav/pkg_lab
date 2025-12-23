#pragma once
#include <QPointF>
#include <QVector>
#include "mainwindow.h"

// Implementations:
bool computeCohenOutcode(const QPointF &p, const Rect &clip, int &outcode);
bool cohenSutherlandClipSegment(const QPointF &p0, const QPointF &p1, const Rect &clip, QPointF &out0, QPointF &out1);

// Sutherland-Hodgman
QVector<QPointF> sutherlandHodgmanClip(const QVector<QPointF> &poly, const Rect &clip);
