/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Computational class for polylines.
//
#ifndef __vlPolyLine_h
#define __vlPolyLine_h

#include "Cell.hh"
#include "Points.hh"
#include "CellArr.hh"
#include "FNormals.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlPolyLine : public vlCell
{
public:
  vlPolyLine() {};
  char *GetClassName() {return "vlPolyLine";};

  int GenerateNormals(vlPoints *, vlCellArray *, vlFloatNormals *);

  int CellDimension() {return 1;};
  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points, vlCellArray *verts, 
               vlCellArray *lines, vlCellArray *polys, 
               vlFloatScalars *s);
  float EvaluatePosition(float x[3], int& subId, float pcoords[3]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3]);

};

#endif


