/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlRectangle - a cell that represents a orthogonal quadrilateral
// .SECTION Description
// vlRectangle is a concrete implementation of vlCell to represent a 2D
// orthogonal quadrilateral. Unlike vlQuad, the corners are at right angles,
// leading to large increases in computational efficiency.

#ifndef __vlRectangle_h
#define __vlRectangle_h

#include "Cell.hh"

class vlRectangle : public vlCell
{
public:
  vlRectangle() {};
  vlRectangle(const vlRectangle& r);
  char *GetClassName() {return "vlRectangle";};

  vlCell *MakeObject() {return new vlRectangle(*this);};
  int GetCellType() {return vlRECTANGLE;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 4;};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId);
  vlCell *GetFace(int faceId) {return 0;};

  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points, vlCellArray *verts, 
               vlCellArray *lines, vlCellArray *polys, vlFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);


  void ShapeFunctions(float pcoords[3], float sf[4]);
};

#endif


