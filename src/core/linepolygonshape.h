/***************************************************************************
              linepolygonshape.h
               ----------------------------------------------------
              date                 : 21.11.2023
              copyright            : (C) 2023 by Mathieu Pellerin
              email                : mathieu at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LINEPOLYGONSHAPE_H
#define LINEPOLYGONSHAPE_H

#include "qgsgeometrywrapper.h"
#include "qgsquickmapsettings.h"

#include <QtQuick/QQuickItem>

class QgsGeometry;

/**
 * @brief The LinePolygonShape class is used to provide the shape data to draw geometries
 * on the map canvas using the QML Shape item.
 */
class LinePolygonShape : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY( QColor color READ color WRITE setColor NOTIFY colorChanged )
    Q_PROPERTY( float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged )
    Q_PROPERTY( QgsQuickMapSettings *mapSettings READ mapSettings WRITE setMapSettings NOTIFY mapSettingsChanged )
    Q_PROPERTY( QgsGeometryWrapper *geometry READ geometry WRITE setGeometry NOTIFY qgsGeometryChanged )

    Q_PROPERTY( QList<QPolygonF> polylines READ polylines NOTIFY polylinesChanged )
    Q_PROPERTY( Qgis::GeometryType polylinesType READ polylinesType NOTIFY polylinesTypeChanged )

  public:
    explicit LinePolygonShape( QQuickItem *parent = nullptr );

    QgsGeometryWrapper *geometry() const;
    void setGeometry( QgsGeometryWrapper *geometry );

    QgsQuickMapSettings *mapSettings() const;
    void setMapSettings( QgsQuickMapSettings *mapSettings );

    QColor color() const;
    void setColor( const QColor &color );

    float lineWidth() const;
    void setLineWidth( float width );

    QList<QPolygonF> polylines() const { return mPolylines; }

    Qgis::GeometryType polylinesType() const { return mPolylinesType; }

  signals:
    void colorChanged();
    void lineWidthChanged();
    void mapSettingsChanged();
    void qgsGeometryChanged();
    void updated();
    void polylinesChanged();
    void polylinesTypeChanged();

  private slots:
    void rotationChanged();
    void mapCrsChanged();
    void visibleExtentChanged();
    void makeDirty();

  protected:
    QSGNode *updatePaintNode( QSGNode *n, QQuickItem::UpdatePaintNodeData * ) override;

  private:
    void updateTransform();
    void createPolylines();

    QColor mColor;
    float mWidth = 0;
    bool mDirty = false;
    QgsQuickMapSettings *mMapSettings = nullptr;
    QgsGeometryWrapper *mGeometry = nullptr;
    QgsPoint mGeometryCorner;
    double mGeometryMUPP = 0.0;
    QList<QPolygonF> mPolylines;
    Qgis::GeometryType mPolylinesType = Qgis::GeometryType::Null;
};

#endif // LINEPOLYGONSHAPE_H
