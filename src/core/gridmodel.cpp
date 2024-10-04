/***************************************************************************
  gridmodel.cpp - GridModel

 ---------------------
 begin                : 4.10.2024
 copyright            : (C) 2024 by Mathieu Pellerin
 email                : mathieu@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "gridmodel.h"

GridModel::GridModel( QObject *parent )
  : QObject( parent )
{
}

void GridModel::setMapSettings( QgsQuickMapSettings *mapSettings )
{
  if ( mMapSettings == mapSettings )
  {
    return;
  }

  if ( mMapSettings )
  {
    disconnect( mMapSettings, &QgsQuickMapSettings::visibleExtentChanged, this, &GridModel::update );
  }

  mMapSettings = mapSettings;
  emit mapSettingsChanged();


  if ( mMapSettings )
  {
    connect( mMapSettings, &QgsQuickMapSettings::visibleExtentChanged, this, &GridModel::update );
  }
}

void GridModel::setXInterval( double interval )
{
  if ( mXInterval == interval )
    return;

  mXInterval = interval;
  emit xIntervalChanged();

  update();
}

void GridModel::setYInterval( double interval )
{
  if ( mYInterval == interval )
    return;

  mYInterval = interval;
  emit yIntervalChanged();

  update();
}

void GridModel::setXOffset( double offset )
{
  if ( mXOffset == offset )
    return;

  mXOffset = offset;
  emit xOffsetChanged();

  update();
}

void GridModel::setYOffset( double offset )
{
  if ( mYOffset == offset )
    return;

  mYOffset = offset;
  emit yOffsetChanged();

  update();
}

void GridModel::update()
{
  if ( !mMapSettings )
  {
    return;
  }

  bool hadLines = !mLines.isEmpty();
  mLines.clear();
  mAnnotations.clear();

  const QgsRectangle visibleExtent = mMapSettings->visibleExtent();
  if ( mXInterval / mMapSettings->mapUnitsPerPoint() < 3 || mYInterval / mMapSettings->mapUnitsPerPoint() < 3 )
  {
    if ( hadLines )
    {
      emit linesChanged();
      emit annotationsChanged();
    }
    return;
  }

  QList<QPointF> line;
  QPointF intersectionPoint;

  double xPos = visibleExtent.xMinimum() - std::fmod( visibleExtent.xMinimum(), mXInterval ) + mXOffset;
  const QLineF topBorder( QPointF( 0, 0 ), QPointF( mMapSettings->outputSize().width(), 0 ) );
  const QLineF bottomBorder( QPointF( 0, mMapSettings->outputSize().height() ), QPointF( mMapSettings->outputSize().width(), mMapSettings->outputSize().height() ) );
  while ( xPos <= visibleExtent.xMaximum() )
  {
    const QLineF currentLine( mMapSettings->coordinateToScreen( QgsPoint( xPos, visibleExtent.yMinimum() ) ), mMapSettings->coordinateToScreen( QgsPoint( xPos, visibleExtent.yMaximum() ) ) );
    if ( currentLine.intersects( topBorder, &intersectionPoint ) )
    {
      mAnnotations << GridAnnotation( GridAnnotation::Top, intersectionPoint, mLocale.toString( xPos, 'f', 0 ) );
    }
    if ( currentLine.intersects( bottomBorder, &intersectionPoint ) )
    {
      mAnnotations << GridAnnotation( GridAnnotation::Bottom, intersectionPoint, mLocale.toString( xPos, 'f', 0 ) );
    }

    line << currentLine.p1() << currentLine.p2();
    mLines << line;
    line.clear();
    xPos += mXInterval;
  }

  double yPos = visibleExtent.yMinimum() - std::fmod( visibleExtent.yMinimum(), mYInterval ) + mYOffset;
  const QLineF leftBorder( QPointF( 0, 0 ), QPointF( 0, mMapSettings->outputSize().height() ) );
  const QLineF rightBorder( QPointF( mMapSettings->outputSize().width(), 0 ), QPointF( mMapSettings->outputSize().width(), mMapSettings->outputSize().height() ) );
  while ( yPos <= visibleExtent.yMaximum() )
  {
    const QLineF currentLine( mMapSettings->coordinateToScreen( QgsPoint( visibleExtent.xMinimum(), yPos ) ), mMapSettings->coordinateToScreen( QgsPoint( visibleExtent.xMaximum(), yPos ) ) );
    if ( currentLine.intersects( leftBorder, &intersectionPoint ) )
    {
      mAnnotations << GridAnnotation( GridAnnotation::Left, intersectionPoint, mLocale.toString( yPos, 'f', 0 ) );
    }
    if ( currentLine.intersects( rightBorder, &intersectionPoint ) )
    {
      mAnnotations << GridAnnotation( GridAnnotation::Right, intersectionPoint, mLocale.toString( yPos, 'f', 0 ) );
    }

    line << currentLine.p1() << currentLine.p2();
    mLines << line;
    line.clear();
    yPos += mYInterval;
  }

  emit linesChanged();
  emit annotationsChanged();
}
