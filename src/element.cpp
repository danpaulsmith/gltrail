/***************************************************************************
 *   Copyright (C) 2008 by Erlend Simonsen                                 *
 *   mr@fudgie.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "element.h"
#include <iostream>
#include <stdlib.h>
#include <math.h>

using namespace std;

Element::Element(Host *h, QString name, QColor col)
{
  x = 1.0 - rand() % 1000 / 500.0;
  y = 1.0 - rand() % 1000 / 500.0;

  vx = 0.0;
  vy = 0.0;

  ax = 0.0000;
  ay = 0.0000;

  size = (rand() % 5000) / 1000.0 + 1.0;
  size = 1.0;

  messages = 0;
  rate = 0.0;

  host = h;
  m_name = name;
  color = col;
  radius = 0.1;

  //  std::cout << "[" << host->getDomain().toStdString() << "] ";
  //  cout << "Element [" << name.toStdString() << "] created." << endl;
}

Element::~Element()
{
}

void Element::add_link_in(Element *e) {
  if( e != NULL ) {
    if(!in.contains( e->name() ) ) {
      in[e->name()] = e;
      nodes_in.push_back(e);
      //      std::cout << "[" << host->getDomain().toStdString() << "] ";
      //      cout << "Rel [" << name().toStdString() << "] <- [" << e->name().toStdString() << "] created." << endl;
    }
    activities << e;
  }
  messages++;
}

void Element::add_link_out(Element *e) {
  if( e != NULL && !out.contains( e->name() ) ) {
    out[e->name()] = e;
    nodes_out.push_back(e);
    //    std::cout << "[" << host->getDomain().toStdString() << "] ";
    //    cout << "Rel [" << name().toStdString() << "] -> [" << e->name().toStdString() << "] created." << endl;
  }

  messages++;
}

void Element::update_stats(void) {

  rate = (rate * 299.0 + messages) / 300.0;
  messages = 0;

  switch( host->getGLWidget()->showSize() ) {
  case 0:
    size = rate * 60.0 / 8.0;
    break;
  case 1:
    size = in.size() * 0.2;
    break;
  case 2:
    size = out.size() * 0.2;
    break;
  case 3:
    size = (out.size() + in.size()) * 0.2;
    break;
  }

  realSize = size;

  float scale = 1.0;
  if( size > host->getMaxSize() ) {
    host->setMaxSize( size );
  } else {
    scale = size / host->getMaxSize();
  }

  size *= scale;

  if( size > 5.0 )
    size = 5.0;

  if( size < 1.0 )
    size = 1.0;

  radius = CUTOFF * size * 0.5;
}

void Element::update(GLWidget *gl) {

  vx += ax / SMOOTHING;
  vy += ay / SMOOTHING;

  vx *= DAMPENING;
  vy *= DAMPENING;

  x += vx / (size * size);
  y += vy / (size * size);

  ax = 0.0;
  ay = 0.0;

  if( x > 0.999 ) {
    x = 0.999;
    vx = -vx;
    ax = -ax;
  } else if( x < -0.999 ) {
    x = -0.999;
    vx = -vx;
    ax = -ax;
  }

  if( y > 0.999 ) {
    y = 0.999;
    vy = -vy;
    ay = -ay;
  } else if( y < -0.999 ) {
    y = -0.999;
    vy = -vy;
    ay = -ay;
  }

  minX = x - radius;
  maxX = x + radius;
  minY = y - radius;
  maxY = y + radius;

  return;

  if( fabs(lastX-x) < 0.001 && fabs(lastY-y) < 0.001 && lastSize >= size )
    return;

  float weight = (lastSize / 2) * CUTOFF / 2;

  int nx = toNode(lastX - weight);
  int stop_x = toNode(lastX + weight);

  if( stop_x < nx ) {
    int i = nx;
    nx = stop_x;
    stop_x = i;
  }

  int start_y = toNode(lastY - weight);
  int stop_y = toNode(lastY + weight);

  if( stop_y < start_y ) {
    int i = start_y;
    start_y = stop_y;
    stop_y = i;
  }

  while( nx <= stop_x ) {
    int ny = start_y;

    while( ny <= stop_y ) {
      gl->nodeMap[nx][ny].remove(this);
      ny++;
      //      gl->stats[STAT_NODE_MAP_UPDATES] += 1;
    }
    nx++;
  }

  weight = (size / 2) * CUTOFF / 2;

  nx = toNode(x - weight);
  stop_x = toNode(x + weight);

  if( stop_x < nx ) {
    int i = nx;
    nx = stop_x;
    stop_x = i;
  }

  start_y = toNode(y - weight);
  stop_y = toNode(y + weight);

  if( stop_y < start_y ) {
    int i = start_y;
    start_y = stop_y;
    stop_y = i;
  }

  while( nx <= stop_x ) {
    int ny = start_y;

    while( ny <= stop_y ) {
      gl->nodeMap[nx][ny].insert(this);
      ny++;
      gl->stats[STAT_NODE_MAP_UPDATES] += 1;
    }
    nx++;
  }

  lastX = x;
  lastY = y;
  lastSize = size;

}

void Element::render(GLWidget *gl) {
   GLfloat r = 0.004 + (size - 1.0) / 100;

   bool hover = fabs(gl->getX() - x) <= r*1.5f && fabs(gl->getY() - y) <= r * 1.5f;

   if(hover) {
     if( gl->getSelected() == NULL )
       gl->setSelected(this);
     glColor4f(1.0, 1.0, 1.0, 1.0);
   } else {
     gl->qglColor( color );
   }

   if( size == 1.0 ) {
     glTranslatef(x,y,0.0);
     glEnable(GL_LINE_SMOOTH);
     glCallList(gl->circle);
     glDisable(GL_LINE_SMOOTH);
     glTranslatef(-x,-y,0.0);
     gl->stats[STAT_LISTS] += 1;
   } else {
     glEnable(GL_LINE_SMOOTH);
     glBegin(GL_LINE_STRIP);

     GLfloat vy1 = y + r;
     GLfloat vx1 = x;

     for(GLfloat angle = 0.0f; angle <= (2.0f*M_PI); angle += 0.25f) {
       gl->stats[STAT_LINES] += 1;
       glVertex3f(vx1, vy1, 0.0);
       vx1 = x + r * sin(angle);
       vy1 = y + r * cos(angle);
     }
     gl->stats[STAT_LINES] += 2;
     glVertex3f(vx1, vy1, 0.0);
     glVertex3f(x, y+r, 0.0);

     glEnd();
     glDisable(GL_LINE_SMOOTH);
   }

//     glPointSize(size*2);
//     glBegin(GL_POINTS);
//     glVertex3f(x,y,0.0);
//     glEnd();



   if( gl->showLines() || hover ) {
     gl->qglColor( host->getColor().darker(300) );

     for(Elements::iterator it = in.begin(); it != in.end(); ++it) {
       gl->stats[STAT_LINES] += 1;
       glBegin(GL_LINES);
       glVertex3f(x,y,0.0);
       glVertex3f((*it)->x, (*it)->y, 0);
       glEnd();
     }

     for(Elements::iterator it = out.begin(); it != out.end(); ++it) {
       gl->stats[STAT_LINES] += 1;
       glBegin(GL_LINES);
       glVertex3f(x,y,0.0);
       glVertex3f((*it)->x, (*it)->y, 0);
       glEnd();
    }
  }

  if( hover ) {
    glColor4f(1.0, 1.0, 1.0, 1.0);
    int xi =  (int) ((1.0 + x) / 2.0 * gl->getWidth() );
    int xy =  (int) (( gl->getAspect() - y) / (2 * gl->getAspect()) * gl->getHeight() - r - 5.0);
    gl->renderText(xi,xy, QString("[%1] %2").arg( QString::number(realSize).left(5) ).arg(name()) );
  }

  if( activities.size() > 0 && rand() % 30 == 1) {
    Element *e = activities.takeFirst();
    gl->qglColor( host->getColor().lighter(120) );
    gl->stats[STAT_LINES] += 1;
    glBegin(GL_LINES);
    glVertex3f(x,y,0.0);
    glVertex3f(e->x, e->y, 0);
    glEnd();
  }

}

bool Element::contains(GLWidget *gl, Element *e) {
  //  gl->stats["Bounding Box Check"] += 1;
  return( e->x >= minX
          && e->x <= maxX
          && e->y >= minY
          && e->y <= maxY
          );
}

void Element::repulsive_check(GLWidget *gl, Element *e) {
  bool shown = false;
  float dx = (e->x - x) * 5;
  float dy = (e->y - y) * 5;

  float d2 = dx * dx + dy * dy;

  if(d2 < 0.001) {
    dx = rand() % 500 / 1000.0 * 0.05;
    dy = rand() % 500 / 1000.0 * 0.05;
    d2 = dx * dx + dy * dy;
  }

  if(d2 < 0.001) {
    d2 = 0.001;
  }

  double d = sqrt(d2);

  if( d < CUTOFF * size) {
    gl->stats[STAT_REPULSIVE_FORCE] += 1;
    repulsive_force(e, d, dx, dy);

    if( gl->showForces() ) {
      gl->stats[STAT_LINES] += 1;
      glColor3f(0.2, 0.2, 0.2);
      glBegin(GL_LINES);
      glVertex3f(x,y,0.0);
      glVertex3f(e->x, e->y, 0);
      glEnd();
      shown = true;
    }
  }
  if( d < CUTOFF * e->size) {
    gl->stats[STAT_REPULSIVE_FORCE] += 1;
    e->repulsive_force(this, d, -dx, -dy);

    if( gl->showForces() && !shown ) {
      gl->stats[STAT_LINES] += 1;
      glColor3f(0.2, 0.2, 0.2);
      glBegin(GL_LINES);
      glVertex3f(x,y,0.0);
      glVertex3f(e->x, e->y, 0);
      glEnd();
    }
  }
}

void Element::attractive_check(GLWidget *gl, Element *e) {
  float dx = (e->x - x);
  float dy = (e->y - y);

  float d2 = dx * dx + dy * dy;

  if(d2 < 0.001) {
    dx = rand() % 500 / 1000.0 * 0.05;
    dy = rand() % 500 / 1000.0 * 0.05;
    d2 = dx * dx + dy * dy;
  }

  if(d2 < 0.001) {
    d2 = 0.001;
  }

  double d = sqrt(d2);

  if( d > CUTOFF/4  ) {
    gl->stats[STAT_ATTRACTIVE_FORCE] += 1;
    attractive_force(e, d, dx, dy);
  }
}

void Element::repulsive_force(Element *e, double d, float dx, float dy) {
  float ed = (d - CUTOFF * size);
  ed *= log(size) * 0.5 + 1.0;

  float fx = ed / d * dx;
  float fy = ed / d * dy;

  e->ax -= fx;
  e->ay -= fy;

  ax += fx;
  ay += fy;

}

void Element::attractive_force(Element *e, double d, float dx, float dy) {
  float ed = d - ((CUTOFF*size)/4);
  ed *= log(size) * 0.5 + 1.0;

  float fx = dx / d;
  float fy = dy / d;

  fx *= K * ed;
  fy *= K * ed;

  e->ax -= fx;
  e->ay -= fy;

  ax += fx;
  ay += fy;
}