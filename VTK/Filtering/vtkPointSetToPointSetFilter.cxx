/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPointSetToPointSetFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
// Construct object.
vtkPointSetToPointSetFilter::vtkPointSetToPointSetFilter()
{
  this->NumberOfRequiredInputs = 1;
}

//----------------------------------------------------------------------------
vtkPointSetToPointSetFilter::~vtkPointSetToPointSetFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPointSetToPointSetFilter::SetInput(vtkPointSet *input)
{
  vtkPointSet *oldInput = this->GetInput();
  
  if (oldInput != NULL)
    {
    if (input == NULL || 
	oldInput->GetDataObjectType() != input->GetDataObjectType())
      {
      vtkWarningMacro("Changing input type.  Deleting output");
      this->SetOutput(NULL);
      }
    }
  
  if (input != NULL && this->vtkSource::GetOutput(0) == NULL)
    {
    this->vtkSource::SetNthOutput(0, input->MakeObject());
    this->Outputs[0]->ReleaseData();
    this->Outputs[0]->Delete();
    }
  
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPointSet *vtkPointSetToPointSetFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkPointSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// Get the output of this filter. If output is NULL then input hasn't been set
// which is necessary for abstract objects.
vtkPointSet *vtkPointSetToPointSetFilter::GetOutput()
{
  if (this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before output can be retrieved");
    return NULL;
    }

  return this->vtkPointSetSource::GetOutput();
}

   
//----------------------------------------------------------------------------
// Get the output as vtkPolyData.
vtkPolyData *vtkPointSetToPointSetFilter::GetPolyDataOutput() 
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_POLY_DATA)
    {
    return (vtkPolyData *)ds;
    }
  return NULL;
}

//----------------------------------------------------------------------------
// Get the output as vtkStructuredGrid. Performs run-time checking.
vtkStructuredGrid *vtkPointSetToPointSetFilter::GetStructuredGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    return (vtkStructuredGrid *)ds;
    }
  return NULL;
}

//----------------------------------------------------------------------------
// Get the output as vtkUnstructuredGrid. Performs run-time checking.
vtkUnstructuredGrid *vtkPointSetToPointSetFilter::GetUnstructuredGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
    return (vtkUnstructuredGrid *)ds;
    }
  return NULL;
}















