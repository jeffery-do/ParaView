/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitCell.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkExplicitCell, "$Revision$");

vtkExplicitCell::vtkExplicitCell()
{
  this->CellId = -1;
}

void vtkExplicitCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Cell Id: " << this->CellId << "\n";
}
