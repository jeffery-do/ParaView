/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGeoSampleArcs.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCoordinate.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGeoMath.h"
#include "vtkGlobeSource.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTimerLog.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkCxxRevisionMacro(vtkGeoSampleArcs, "$Revision$");
vtkStandardNewMacro(vtkGeoSampleArcs);

vtkGeoSampleArcs::vtkGeoSampleArcs()
{
  this->GlobeRadius = vtkGeoMath::EarthRadiusMeters();
  this->MaximumDistanceMeters = 100000.0;
}

vtkGeoSampleArcs::~vtkGeoSampleArcs()
{
}

int vtkGeoSampleArcs::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Traverse input lines, adding a circle for each line segment.
  vtkCellArray* lines = input->GetLines();
  vtkCellArray* newLines = vtkCellArray::New();
  vtkPoints* points = input->GetPoints();
  if ( !points )
    {
    return 0;
    }
  float* pointsPtr = static_cast<float*>(points->GetVoidPointer(0));
  vtkPoints* newPoints = vtkPoints::New();
  lines->InitTraversal();
  for (vtkIdType i = 0; i < lines->GetNumberOfCells(); i++)
    {
    vtkIdType npts;
    vtkIdType* pts;
    lines->GetNextCell(npts, pts);
    
    double lastPoint[3];
    double curPoint[3];
    double lastPtLL[2];
    double curPtLL[2];
    curPoint[0] = pointsPtr[3*pts[0]+0];
    curPoint[1] = pointsPtr[3*pts[0]+1];
    curPoint[2] = pointsPtr[3*pts[0]+2];
    vtkGlobeSource::ComputeLatitudeLongitude(
      curPoint, curPtLL[0], curPtLL[1]);
    
    for (vtkIdType p = 1; p < npts; ++p)
      {
      // Advance point
      for (int c = 0; c < 3; ++c)
        {
        lastPoint[c] = curPoint[c];
        }
      lastPtLL[0] = curPtLL[0];
      lastPtLL[1] = curPtLL[1];
      curPoint[0] = pointsPtr[3*pts[p]+0];
      curPoint[1] = pointsPtr[3*pts[p]+1];
      curPoint[2] = pointsPtr[3*pts[p]+2];
      vtkGlobeSource::ComputeLatitudeLongitude(
        curPoint, curPtLL[0], curPtLL[1]);

      double dist = sqrt(vtkMath::Distance2BetweenPoints(lastPoint, curPoint));

      // Calculate the number of subdivisions.
      vtkIdType numDivisions = static_cast<vtkIdType>(dist / this->MaximumDistanceMeters + 0.5) + 1;
      if (numDivisions < 2)
        {
        numDivisions = 2;
        }

      // Create the new cell
      newLines->InsertNextCell(numDivisions);
      
      for (vtkIdType s = 0; s < numDivisions; ++s)
        {
        // Interpolate in lat-long.
        double interpPtLL[2];
        double frac = static_cast<double>(s) / (numDivisions - 1);
        for (int c = 0; c < 2; ++c)
          {
          interpPtLL[c] = frac*curPtLL[c] + (1.0 - frac)*lastPtLL[c];
          }
        // Convert lat-long to world;
        double interpPt[3];
        vtkGlobeSource::ComputeGlobePoint(interpPtLL[0], interpPtLL[1], this->GlobeRadius, interpPt);
        vtkIdType newPt = newPoints->InsertNextPoint(interpPt);
        newLines->InsertCellPoint(newPt);        
        }

      }
    }

  // Send the data to output.
  output->SetLines(newLines);
  output->SetPoints(newPoints);

  // Clean up.
  newLines->Delete();
  newPoints->Delete();

  return 1;
}

void vtkGeoSampleArcs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GlobeRadius: " << this->GlobeRadius << endl;
  os << indent << "MaximumDistanceMeters: " << this->MaximumDistanceMeters << endl;
}

