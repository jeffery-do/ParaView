/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.


     THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,719,585
     "Dividing Cubes System and Method for the Display of Surface Structures
     Contained Within the Interior Region of a Solid Body".
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
#include "vtkDividingCubes.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkVoxel.h"

vtkCxxRevisionMacro(vtkDividingCubes, "$Revision$");
vtkStandardNewMacro(vtkDividingCubes);

// Description:
// Construct object with Value=0.0, Distance=0.1, and Increment=1.
vtkDividingCubes::vtkDividingCubes()
{
  this->Value = 0.0;
  this->Distance = 0.1;
  this->Increment = 1;
  this->Count = 0;
  this->SubVoxelPts = vtkIdList::New(); this->SubVoxelPts->SetNumberOfIds(8);
  this->SubVoxel = vtkVoxel::New();
  this->SubVoxelScalars = vtkDoubleArray::New(); 
  this->SubVoxelScalars->SetNumberOfTuples(8);
  this->SubVoxelNormals = vtkDoubleArray::New(); 
  this->SubVoxelNormals->SetNumberOfComponents(3);
  this->SubVoxelNormals->SetNumberOfTuples(8);
}

vtkDividingCubes::~vtkDividingCubes()
{
  this->SubVoxelPts->Delete();
  this->SubVoxel->Delete();
  this->SubVoxelScalars->Delete();
  this->SubVoxelNormals->Delete();
}

static double Normals[8][3]; //voxel normals
static vtkPoints *NewPts; //points being generated
static vtkDoubleArray *NewNormals; //points being generated
static vtkCellArray *NewVerts; //verts being generated
static vtkDoubleArray *SubNormals; //sub-volume normals
static vtkDoubleArray *SubScalars; //sub-volume scalars
static int SubSliceSize;

void vtkDividingCubes::Execute()
{
  int i, j, k, idx;
  vtkDataArray *inScalars;
  vtkIdList *voxelPts;
  vtkDoubleArray *voxelScalars;
  double origin[3], x[3], ar[3], h[3];
  int dim[3], jOffset, kOffset, sliceSize;
  int above, below, vertNum, n[3];
  vtkImageData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<< "Executing dividing cubes...");

  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }

  //
  // Initialize self; check input; create output objects
  //
  this->Count = 0;

  // make sure we have scalar data
  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to contour");
    return;
    }

  // just deal with volumes
  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro("Bad input: only treats 3D structured point datasets");
    return;
    }
  input->GetDimensions(dim);
  input->GetSpacing(ar);
  input->GetOrigin(origin);

  
  // creating points
  NewPts = vtkPoints::New(); 
  NewPts->Allocate(500000,500000);
  NewNormals = vtkDoubleArray::New(); 
  NewNormals->SetNumberOfComponents(3);
  NewNormals->Allocate(1500000,1500000);
  NewVerts = vtkCellArray::New(); 
  NewVerts->Allocate(500000,500000);
  NewVerts->InsertNextCell(0); //temporary cell count
  //
  // Loop over all cells checking to see which straddle the specified value. Since
  // we know that we are working with a volume, can create appropriate data directly.
  //
  sliceSize = dim[0] * dim[1];
  for (i=0; i<3; i++) //compute subvoxel widths and subvolume dimensions
    {
    n[i] = ((int) ceil((double)ar[i]/this->Distance)) + 1;
    h[i] = (double)ar[i]/(n[i]-1);
    }

  SubSliceSize = n[0] * n[1];
  SubNormals = vtkDoubleArray::New();
  SubNormals->SetNumberOfComponents(3);
  SubNormals->SetNumberOfTuples(SubSliceSize*n[2]);

  SubScalars = vtkDoubleArray::New();
  SubScalars->SetNumberOfTuples(SubSliceSize*n[2]);
  voxelPts = vtkIdList::New(); voxelPts->SetNumberOfIds(8);
  voxelScalars = vtkDoubleArray::New(); voxelScalars->SetNumberOfTuples(8);
  

  for ( k=0; k < (dim[2]-1); k++)
    {
    kOffset = k*sliceSize;
    x[2] = origin[2] + k*ar[2];

    for ( j=0; j < (dim[1]-1); j++)
      {
      jOffset = j*dim[0];
      x[1] = origin[1] + j*ar[1];

      for ( i=0; i < (dim[0]-1); i++)
        {
        idx  = i + jOffset + kOffset;
        x[0] = origin[0] + i*ar[0];

        // get point ids of this voxel
        voxelPts->SetId(0, idx);
        voxelPts->SetId(1, idx + 1);
        voxelPts->SetId(2, idx + dim[0]);
        voxelPts->SetId(3, idx + dim[0] + 1);
        voxelPts->SetId(4, idx + sliceSize);
        voxelPts->SetId(5, idx + sliceSize + 1);
        voxelPts->SetId(6, idx + sliceSize + dim[0]);
        voxelPts->SetId(7, idx + sliceSize + dim[0] + 1);

        // get scalars of this voxel
        inScalars->GetTuples(voxelPts,voxelScalars);

        // loop over 8 points of voxel to check if cell straddles value
        for ( above=below=0, vertNum=0; vertNum < 8; vertNum++ )
          {
          if ( voxelScalars->GetComponent(vertNum,0) >= this->Value )
            {
            above = 1;
            }
          else if ( voxelScalars->GetComponent(vertNum,0) < this->Value )
            {
            below = 1;
            }

          if ( above && below ) // recursively generate points
            { //compute voxel normals and subdivide
            input->GetPointGradient(i,j,k, inScalars, Normals[0]);
            input->GetPointGradient(i+1,j,k, inScalars, Normals[1]);
            input->GetPointGradient(i,j+1,k, inScalars, Normals[2]);
            input->GetPointGradient(i+1,j+1,k, inScalars, Normals[3]);
            input->GetPointGradient(i,j,k+1, inScalars, Normals[4]);
            input->GetPointGradient(i+1,j,k+1, inScalars, Normals[5]);
            input->GetPointGradient(i,j+1,k+1, inScalars, Normals[6]);
            input->GetPointGradient(i+1,j+1,k+1, inScalars, Normals[7]);

            this->SubDivide(x, n, h, voxelScalars->GetPointer(0));
            }
          }
        }
      }
    }

  NewVerts->UpdateCellCount(NewPts->GetNumberOfPoints());
  vtkDebugMacro(<< "Created " << NewPts->GetNumberOfPoints() << " points");
  //
  // Update ourselves and release memory
  //
  SubNormals->Delete();
  SubScalars->Delete();
  voxelPts->Delete();
  voxelScalars->Delete();
  
  output->SetPoints(NewPts);
  NewPts->Delete();

  output->SetVerts(NewVerts);
  NewVerts->Delete();

  output->GetPointData()->SetNormals(NewNormals);
  NewNormals->Delete();

  output->Squeeze();
}

#define VTK_POINTS_PER_POLY_VERTEX 10000

void vtkDividingCubes::SubDivide(double origin[3], int dim[3], double h[3],
                                 double values[8])
{
  int i, j, k, ii, vertNum, id;
  double s;
  int kOffset, jOffset, idx, above, below;
  double p[3], w[8], n[3], normal[3], offset[3];

  // Compute normals and scalars on subvoxel array
  for (k=0; k < dim[2]; k++)
    {
    kOffset = k * SubSliceSize;
    p[2] = k * h[2];
    for (j=0; j < dim[1]; j++)
      {
      jOffset = j * dim[0];
      p[1] = j * h[1];
      for (i=0; i < dim[0]; i++)
        {
        idx = i + jOffset + kOffset;
        p[0] = i * h[0];

        this->SubVoxel->InterpolationFunctions(p,w);
        s = n[0] = n[1] = n[2] = 0.0;
        for (ii=0; ii<8; ii++)
          {
          s += values[ii]*w[ii];
          n[0] += Normals[ii][0]*w[ii];
          n[1] += Normals[ii][1]*w[ii];
          n[2] += Normals[ii][2]*w[ii];
          }
        SubScalars->SetComponent(idx,0,s);
        SubNormals->SetTuple(idx,n);
        }
      }
    }

  // loop over sub-volume determining whether contour passes through subvoxels.
  // If so, generate center point in subvoxel.
  for (i=0; i<3; i++)
    {
    offset[i] = origin[i] + (h[i] / 2.0);
    }
  for (k=0; k < (dim[2]-1); k++)
    {
    kOffset = k * SubSliceSize;
    p[2] = offset[2] + k * h[2];
    for (j=0; j < (dim[1]-1); j++)
      {
      jOffset = j * dim[0];
      p[1] = offset[1] + j * h[1];
      for (i=0; i < (dim[0]-1); i++)
        {
        idx = i + jOffset + kOffset;
        p[0] = offset[0] + i * h[0];

        // get point ids of this voxel
        this->SubVoxelPts->SetId(0, idx);
        this->SubVoxelPts->SetId(1, idx + 1);
        this->SubVoxelPts->SetId(2, idx + dim[0]);
        this->SubVoxelPts->SetId(3, idx + dim[0] + 1);
        this->SubVoxelPts->SetId(4, idx + SubSliceSize);
        this->SubVoxelPts->SetId(5, idx + SubSliceSize + 1);
        this->SubVoxelPts->SetId(6, idx + SubSliceSize + dim[0]);
        this->SubVoxelPts->SetId(7, idx + SubSliceSize + dim[0] + 1);

        // get scalars of this voxel
        SubScalars->GetTuples(this->SubVoxelPts,this->SubVoxelScalars);

        // loop over 8 points of voxel to check if cell straddles value
        for ( above=below=0, vertNum=0; vertNum < 8; vertNum++ )
          {
          if ( this->SubVoxelScalars->GetComponent(vertNum,0) >= this->Value )
            {
            above = 1;
            }
          else if ( this->SubVoxelScalars->GetComponent(vertNum,0) < this->Value )
            {
            below = 1;
            }
          }

        if ( (above && below) && !(this->Count++ % this->Increment) )
          { //generate center point
          SubNormals->GetTuples(this->SubVoxelPts,this->SubVoxelNormals);
          for (n[0]=n[1]=n[2]=0.0, vertNum=0; vertNum < 8; vertNum++)
            {
            this->SubVoxelNormals->GetTuple(vertNum, normal);
            n[0] += normal[0];
            n[1] += normal[1];
            n[2] += normal[2];
            }
          vtkMath::Normalize(n);

          id = NewPts->InsertNextPoint(p);
          NewVerts->InsertCellPoint(id);
          NewNormals->InsertTuple(id,n);

          if ( !(NewPts->GetNumberOfPoints() % VTK_POINTS_PER_POLY_VERTEX) )
            {
            vtkDebugMacro(<<"point# "<<NewPts->GetNumberOfPoints());
            }
          }
        }
      }
    }
}

void vtkDividingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Value: " << this->Value << "\n";
  os << indent << "Distance: " << this->Distance << "\n";
  os << indent << "Increment: " << this->Increment << "\n";
}


