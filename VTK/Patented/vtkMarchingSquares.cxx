/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.


     THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
     "System and Method for the Display of Surface Structures Contained
     Within the Interior Region of a Solid Body".
     Application of this software for commercial purposes requires 
     a license grant from GE. Contact:

         Carl B. Horton
         Sr. Counsel, Intellectual Property
         3000 N. Grandview Blvd., W-710
         Waukesha, WI  53188
         Phone:  (262) 513-4022
         E-Mail: Carl.Horton@med.ge.com

     for more information.

=========================================================================*/
#include "vtkMarchingSquares.h"

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMarchingSquaresCases.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkStructuredPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <math.h>

vtkCxxRevisionMacro(vtkMarchingSquares, "$Revision$");
vtkStandardNewMacro(vtkMarchingSquares);

// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkMarchingSquares::vtkMarchingSquares()
{
  this->ContourValues = vtkContourValues::New();

  this->ImageRange[0] = 0; this->ImageRange[1] = VTK_LARGE_INTEGER;
  this->ImageRange[2] = 0; this->ImageRange[3] = VTK_LARGE_INTEGER;
  this->ImageRange[4] = 0; this->ImageRange[5] = 0;

  this->Locator = NULL;
}

vtkMarchingSquares::~vtkMarchingSquares()
{
  this->ContourValues->Delete();
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkMarchingSquares::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkMarchingSquares::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}



void vtkMarchingSquares::SetImageRange(int imin, int imax, int jmin, int jmax, 
                                       int kmin, int kmax)
{
  int dim[6];

  dim[0] = imin;
  dim[1] = imax;
  dim[2] = jmin;
  dim[3] = jmax;
  dim[4] = kmin;
  dim[5] = kmax;

  this->SetImageRange(dim);
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkMarchingSquares::GetMTime()
{
  unsigned long mTime=this->vtkPolyDataSource::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  if (this->Locator)
    {
    mTime2=this->Locator->GetMTime();
    mTime = ( mTime2 > mTime ? mTime2 : mTime );
    }

  return mTime;
}

//
// Contouring filter specialized for images
//
template <class T>
void vtkContourImage(T *scalars, vtkDataArray *newScalars, int roi[6], int dir[3],
                     int start[2], int end[2], int offset[3], double ar[3], 
                     double origin[3], double *values, int numValues, 
                     vtkPointLocator *p, vtkCellArray *lines)
{
  int i, j;
  vtkIdType ptIds[2];
  double t, *x1, *x2, x[3], xp, yp;
  double pts[4][3], min, max;
  int contNum, jOffset, idx, ii, jj, index, *vert;
  static int CASE_MASK[4] = {1,2,8,4};  
  vtkMarchingSquaresLineCases *lineCase, *lineCases;
  static int edges[4][2] = { {0,1}, {1,3}, {2,3}, {0,2} };
  EDGE_LIST  *edge;
  double value, s[4];

  lineCases = vtkMarchingSquaresLineCases::GetCases();
//
// Get min/max contour values
//
  if ( numValues < 1 )
    {
    return;
    }
  for ( min=max=values[0], i=1; i < numValues; i++)
    {
    if ( values[i] < min )
      {
      min = values[i];
      }
    if ( values[i] > max )
      {
      max = values[i];
      }
    }

  //assign coordinate value to non-varying coordinate direction
  x[dir[2]] = origin[dir[2]] + roi[dir[2]*2]*ar[dir[2]];

  // Traverse all pixel cells, generating line segements using marching squares.
  for ( j=roi[start[1]]; j < roi[end[1]]; j++ )
    {

    jOffset = j * offset[1];
    pts[0][dir[1]] = origin[dir[1]] + j*ar[dir[1]];
    yp = origin[dir[1]] + (j+1)*ar[dir[1]];

    for ( i=roi[start[0]]; i < roi[end[0]]; i++)
      {
       //get scalar values
      idx = i * offset[0] + jOffset + offset[2];
      s[0] = scalars[idx];
      s[1] = scalars[idx+offset[0]];
      s[2] = scalars[idx + offset[1]];
      s[3] = scalars[idx+offset[0] + offset[1]];

      if ( (s[0] < min && s[1] < min && s[2] < min && s[3] < min) ||
      (s[0] > max && s[1] > max && s[2] > max && s[3] > max) )
        {
        continue; // no contours possible
        }

      //create pixel points
      pts[0][dir[0]] = origin[dir[0]] + i*ar[dir[0]];
      xp = origin[dir[0]] + (i+1)*ar[dir[0]];

      pts[1][dir[0]] = xp;
      pts[1][dir[1]] = pts[0][dir[1]];

      pts[2][dir[0]] = pts[0][dir[0]];
      pts[2][dir[1]] = yp;

      pts[3][dir[0]] = xp;
      pts[3][dir[1]] = yp;

      // Loop over contours in this pixel
      for (contNum=0; contNum < numValues; contNum++)
        {
        value = values[contNum];

        // Build the case table
        for ( ii=0, index = 0; ii < 4; ii++)
          {
          if ( s[ii] >= value )
            {
            index |= CASE_MASK[ii];
            }
          }
        if ( index == 0 || index == 15 )
          {
          continue; //no lines
          }

        lineCase = lineCases + index;
        edge = lineCase->edges;

        for ( ; edge[0] > -1; edge += 2 )
          {
          for (ii=0; ii<2; ii++) //insert line
            {
            vert = edges[edge[ii]];
            t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
            x1 = pts[vert[0]];
            x2 = pts[vert[1]];
            for (jj=0; jj<2; jj++) //only need to interpolate two values
              {
              x[dir[jj]] = x1[dir[jj]] + t * (x2[dir[jj]] - x1[dir[jj]]);
              }
            if ( p->InsertUniquePoint(x, ptIds[ii]) )
              {
              newScalars->InsertComponent(ptIds[ii],0,value);
              }
            }
          
          if ( ptIds[0] != ptIds[1] ) //check for degenerate line
            {
            lines->InsertNextCell(2,ptIds);
            }

          }//for each line
        }//for all contours
      }//for i
    }//for j
}

//
// Contouring filter specialized for images (or slices from images)
//
void vtkMarchingSquares::Execute()
{
  vtkImageData *input = this->GetInput();
  vtkPointData *pd;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkDataArray *inScalars;
  vtkDataArray *newScalars = NULL;
  int i, dims[3], roi[6], dataSize, dim, plane=0;
  int *ext;
  double origin[3], ar[3];
  vtkPolyData *output = this->GetOutput();
  int start[2], end[2], offset[3], dir[3], estimatedSize;
  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();

  vtkDebugMacro(<< "Executing marching squares");
//
// Initialize and check input
//
  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }
  pd=input->GetPointData();
  if (pd ==NULL)
    {
    vtkErrorMacro(<<"PointData is NULL");
    return;
    }
  inScalars=pd->GetScalars();
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }
//
// Check dimensionality of data and get appropriate form
//
  input->GetDimensions(dims);
  ext = input->GetExtent();
  input->GetOrigin(origin);
  input->GetSpacing(ar);
  dataSize = dims[0] * dims[1] * dims[2];

  if ( input->GetDataDimension() != 2 )
    {
    for (i=0; i < 6; i++)
      {
      roi[i] = this->ImageRange[i];
      }
    }
  else
    {
    input->GetExtent(roi);
    }

  // check the final region of interest to make sure its acceptable
  for ( dim=0, i=0; i < 3; i++ )
    {
    if ( roi[2*i+1] > ext[2*i+1] )
      {
      roi[2*i+1] = ext[2*i+1];
      }
    else if ( roi[2*i+1] < ext[2*i] )
      {
      roi[2*i+1] = ext[2*i];
      }

    if ( roi[2*i] > roi[2*i+1] )
      {
      roi[2*i] = roi[2*i+1];
      }
    else if ( roi[2*i] < ext[2*i] )
      {
      roi[2*i] = ext[2*i];
      }

    if ( (roi[2*i+1]-roi[2*i]) > 0 )
      {
      dim++;
      }
    else
      {
      plane = i;
      }
    }

  if ( dim != 2 )
    {
    vtkErrorMacro(<<"Marching squares requires 2D data");
    return;
    }
//
// Setup indices and offsets (since can have x-y or z-plane)
//
  if ( plane == 0 ) //x-plane
    {
    start[0] = 2; end[0] = 3;
    start[1] = 4; end[1] = 5;
    offset[0] = dims[0];
    offset[1] = dims[0]*dims[1];
    offset[2] = (roi[0]-ext[0]);
    dir[0] = 1; dir[1] = 2; dir[2] = 0;
    }
  else if ( plane == 1 ) //y-plane
    {
    start[0] = 0; end[0] = 1;
    start[1] = 4; end[1] = 5;
    offset[0] = 1;
    offset[1] = dims[0]*dims[1];
    offset[2] = (roi[2]-ext[2])*dims[0];
    dir[0] = 0; dir[1] = 2; dir[2] = 1;
    }
  else //z-plane
    {
    start[0] = 0; end[0] = 1;
    start[1] = 2; end[1] = 3;
    offset[0] = 1;
    offset[1] = dims[0];
    offset[2] = (roi[4]-ext[4])*dims[0]*dims[1];
    dir[0] = 0; dir[1] = 1; dir[2] = 2;
    }
//
// Allocate necessary objects
//
  estimatedSize = (int) (numContours * sqrt((double)dims[0]*dims[1]));
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(estimatedSize,2));

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, this->GetInput()->GetBounds());
  //
  // Check data type and execute appropriate function
  //
  if (inScalars->GetNumberOfComponents() == 1 )
    {
    switch (inScalars->GetDataType())
      {
      case VTK_CHAR:
        {
        char *scalars = static_cast<vtkCharArray *>(inScalars)->GetPointer(0);
        newScalars = vtkCharArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_UNSIGNED_CHAR:
        {
        unsigned char *scalars = static_cast<vtkUnsignedCharArray *>(inScalars)->GetPointer(0);
        newScalars = vtkUnsignedCharArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_SHORT:
        {
        short *scalars = static_cast<vtkShortArray *>(inScalars)->GetPointer(0);
        newScalars = vtkShortArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_UNSIGNED_SHORT:
        {
        unsigned short *scalars = static_cast<vtkUnsignedShortArray *>(inScalars)->GetPointer(0);
        newScalars = vtkUnsignedShortArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_INT:
        {
        int *scalars = static_cast<vtkIntArray *>(inScalars)->GetPointer(0);
        newScalars = vtkIntArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_UNSIGNED_INT:
        {
        unsigned int *scalars = static_cast<vtkUnsignedIntArray *>(inScalars)->GetPointer(0);
        newScalars = vtkUnsignedIntArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
        break;
      case VTK_LONG:
        {
        long *scalars = static_cast<vtkLongArray *>(inScalars)->GetPointer(0);
        newScalars = vtkLongArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_UNSIGNED_LONG:
        {
        unsigned long *scalars = static_cast<vtkUnsignedLongArray *>(inScalars)->GetPointer(0);
        newScalars = vtkUnsignedLongArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_FLOAT:
        {
        float *scalars = 
          static_cast<vtkFloatArray *>(inScalars)->GetPointer(0);
        newScalars = vtkFloatArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      case VTK_DOUBLE:
        {
        double *scalars = 
          static_cast<vtkDoubleArray *>(inScalars)->GetPointer(0);
        newScalars = vtkDoubleArray::New();
        newScalars->Allocate(5000,25000);
        vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                        values,numContours,this->Locator,newLines);
        }
      break;
      }//switch
    }

  else //multiple components - have to convert
    {
    vtkDoubleArray *image = vtkDoubleArray::New();
    image->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    image->SetNumberOfTuples(dataSize);
    inScalars->GetTuples(0,dataSize,image);
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(5000,25000);
    double *scalars = image->GetPointer(0);
    vtkContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
                    values,numContours,this->Locator,newLines);
    image->Delete();
    }

  vtkDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newLines->GetNumberOfCells() << " lines");
  //
  // Update ourselves.  Because we don't know up front how many lines
  // we've created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  this->Locator->Initialize();
  output->Squeeze();
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkMarchingSquares::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator)
    {
    return;
    }

  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  
  if ( locator )
    {
    locator->Register(this);
    }
  
  this->Locator = locator;
  this->Modified();
}

void vtkMarchingSquares::CreateDefaultLocator()
{
  if ( this->Locator == NULL)
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkMarchingSquares::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Image Range: ( " 
     << this->ImageRange[0] << ", "
     << this->ImageRange[1] << ", "
     << this->ImageRange[2] << ", "
     << this->ImageRange[3] << ", "
     << this->ImageRange[4] << ", "
     << this->ImageRange[5] << " )\n";

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}


