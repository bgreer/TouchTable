#include "tracker.h"


Tracker::Tracker ()
{

}

Tracker::~Tracker ()
{

}

float Tracker::distanceBetween (TrackerObject *t1, TrackerObject *t2)
{
  return (t1->xpos - t2->xpos)*(t1->xpos - t2->xpos) + 
         (t1->ypos - t2->ypos)*(t1->ypos - t1->ypos);
}

std::vector<TrackerObject*> *Tracker::getPoints (void)
{
  double t = getTime();

  std::vector<TrackerObject*> *ret = new std::vector<TrackerObject*> ();
  for (int ii=0; ii<objs.size(); ii++)
  {
    if (t - objs[ii]->t_start > obj_mintime)
      ret->push_back(objs[ii]);
  }

  return ret;
}

void Tracker::update (std::vector<KeyPoint> *keys)
{
  int ii, ij, ind;
  double t = getTime();
  double dt, dr, minr;
  float xpred, ypred, scale;

  // copy info into local array
  for (ii=0; ii<keys->size(); ii++)
  {
    newx.push_back((*keys)[ii].pt.x);
    newy.push_back((*keys)[ii].pt.y);
  }

  // reset object flags
  for (ii=0; ii<objs.size(); ii++)
    objs[ii]->updated = false;

  // loop through each new point
  for (ii=0; ii<newx.size(); ii++)
  {
    // try to match to existing point
    ind = -1;
    minr = 1e9;
    scale = 1.0;
    // find obj with closest predicted position
    for (ij=0; ij<objs.size(); ij++)
    {
      dt = t - objs[ij]->t_lastupdate;
      xpred = objs[ij]->xpos + objs[ij]->xvel*dt;
      ypred = objs[ij]->ypos + objs[ij]->yvel*dt;
      dr = pow(xpred - newx[ii],2.0) + pow(ypred - newy[ii],2.0);
      if (dr < minr)
      {
        minr = dr;
        ind = ij;
        scale = 1.0 + 0.001*(objs[ij]->xvel*objs[ij]->xvel + objs[ij]->yvel*objs[ij]->yvel);
        if (scale > 3.0) scale = 3.0;
      }
    }

    // if we have found a good match, update it
    if (ind > -1 && minr < obj_max_dr*scale && objs[ind]->updated == false)
    {
      objs[ind]->xvel = -(objs[ind]->xpos - newx[ii])/dt;
      objs[ind]->yvel = -(objs[ind]->ypos - newy[ii])/dt;
      objs[ind]->xpos = newx[ii];
      objs[ind]->ypos = newy[ii];
      // TOUCH_MOVE
      objs[ind]->updated = true;
      objs[ind]->t_lastupdate = t;
    } else {
      // new point!
      objs.push_back(new TrackerObject(newx[ii],newy[ii]));
      objs[objs.size()-1]->updated = true;
      // TOUCH_DOWN
    }
  }

  // check to see if any objs were not updated
  for (ii=0; ii<objs.size(); ii++)
  {
    if (objs[ii]->updated == false)
    {
      dt = t - objs[ii]->t_lastupdate;
      if ( dt > obj_timeout)
      {
        delete objs[ii];
        objs.erase(objs.begin() + ii);
        // TOUCH_UP
        ii--;
      } else {
        // predict?
        objs[ii]->xvel *= pow(0.5,(int)(dt/0.01));
        objs[ii]->yvel *= pow(0.5,(int)(dt/0.01));
        objs[ii]->xpos += objs[ii]->xvel*dt;
        objs[ii]->yvel += objs[ii]->yvel*dt;
        // TOUCH_MOVE
        objs[ii]->updated = true;
      }
    }
  }

  // clear arrays
  newx.clear();
  newy.clear();
}



