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
#include "vtkImageSkeleton2D.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageSkeleton2D, "$Revision$");
vtkStandardNewMacro(vtkImageSkeleton2D);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageSkeleton2D fitler.
vtkImageSkeleton2D::vtkImageSkeleton2D()
{
  this->Prune = 0;
}

//----------------------------------------------------------------------------
void vtkImageSkeleton2D::SetNumberOfIterations(int num)
{
  this->vtkImageIterateFilter::SetNumberOfIterations(num);
}


//----------------------------------------------------------------------------
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageSkeleton2D::ComputeInputUpdateExtent(int inExt[6], 
                                                  int outExt[6])
{
  int idx;
  int *wholeExtent;
  
  wholeExtent = this->GetInput()->GetWholeExtent();

  inExt[4] = outExt[4];
  inExt[5] = outExt[5];
  
  for (idx = 0; idx < 2; ++idx)
    {
    inExt[idx*2] = outExt[idx*2] - 1;
    inExt[idx*2+1] = outExt[idx*2+1] + 1;
    
    // If the expanded region is out of the IMAGE Extent (grow min)
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }
    // If the expanded region is out of the IMAGE Extent (shrink max)      
    if (inExt[idx*2+1] > wholeExtent[idx*2+1])
      {
      // shrink the required region extent
      inExt[idx*2+1] = wholeExtent[idx*2+1];
      }
    }
}




//----------------------------------------------------------------------------
// This method contains the second switch statement that calls the correct
// templated function for the mask types.
// This is my best attempt at skeleton.  The rules are a little hacked up,
// but it is the only way I can think of to get the 
// desired results with a 3x3 kernel.
template <class T>
void vtkImageSkeleton2DExecute(vtkImageSkeleton2D *self,
                               vtkImageData *inData, T *inPtr, 
                               vtkImageData *outData, int *outExt, 
                               T *outPtr, int id)
{
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2, numComps;
  int idx0, idx1, idx2, idxC;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  T *outPtr0, *outPtr1, *outPtr2;
  int wholeMin0, wholeMax0, wholeMin1, wholeMax1, wholeMin2, wholeMax2;
  int prune = self->GetPrune();
  float n[8];
  int countFaces, countCorners;
  unsigned long count = 0;
  unsigned long target;
  int erodeCase;

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2); 
  outData->GetIncrements(outInc0, outInc1, outInc2); 
  outMin0 = outExt[0];  outMax0 = outExt[1];
  outMin1 = outExt[2];  outMax1 = outExt[3];
  outMin2 = outExt[4];  outMax2 = outExt[5];
  self->GetInput()->GetWholeExtent(wholeMin0, wholeMax0, wholeMin1, wholeMax1,
                                   wholeMin2, wholeMax2);
  numComps = inData->GetNumberOfScalarComponents();
  
  target = (unsigned long)(numComps*(outMax2-outMin2+1)*
                           (outMax1-outMin1+1)/50.0);
  target++;

  inPtrC = inPtr;
  for (idxC = 0; idxC < numComps; ++idxC)
    {
    inPtr2 = inPtrC;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      // erode input
      inPtr1 = inPtr2;
      for (idx1 = outMin1; !self->AbortExecute && idx1 <= outMax1; ++idx1)
        {
        if (!id) 
          {
          if (!(count%target))
            {
            self->UpdateProgress(0.9*count/(50.0*target));
            }
          count++;
          }
        inPtr0 = inPtr1;
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          // Center pixel has to be on.
          if (*inPtr0)
            {
            // neighbors independant of boundaries
            n[0] = (idx0>wholeMin0) ? (float)*(inPtr0-inInc0) : 0;
            n[1] = (idx0>wholeMin0)&&(idx1>wholeMin1) 
              ? (float)*(inPtr0-inInc0-inInc1) : 0;
            n[2] = (idx1>wholeMin1) ? (float)*(inPtr0-inInc1) : 0;
            n[3] = (idx1>wholeMin1)&&(idx0<wholeMax0) 
              ? (float)*(inPtr0-inInc1+inInc0) : 0;
            n[4] = (idx0<wholeMax0) ? (float)*(inPtr0+inInc0) : 0;
            n[5] = (idx0<wholeMax0)&&(idx1<wholeMax1) 
              ? (float)*(inPtr0+inInc0+inInc1) : 0;
            n[6] = (idx1<wholeMax1) ? (float)*(inPtr0+inInc1) : 0;
            n[7] = (idx1<wholeMax1)&&(idx0>wholeMin0) 
              ? (float)*(inPtr0+inInc1-inInc0) : 0;
            
            // Lets try a case table. (shifting bits would be faster)
            erodeCase = 0;
            if (n[7] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[6] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[5] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[4] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[3] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[2] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[1] > 0) {++erodeCase;} 
            erodeCase *= 2;
            if (n[0] > 0) {++erodeCase;}
            
            if (erodeCase == 54 || erodeCase == 216)
              { // erode
              // 54 top part of diagonal / double thick line
              // 216 bottom part of diagonal \ double thick line
              *inPtr0 = 1;
              }
            else if (erodeCase == 99 || erodeCase == 141)
              { // No errosion
              // 99 bottom part of diagonal / double thick line
              // 141 top part of diagonal \ double thick line
              }
            else
              {
              // old heuristic method
              countFaces = (n[0]>0)+(n[2]>0)+(n[4]>0)+(n[6]>0);
              countCorners = (n[1]>0)+(n[3]>0)+(n[5]>0)+(n[7]>0);
              
              // special case to void split dependent results.
              // (should we just have a case table?)
              if (countFaces == 2 && countCorners == 0 &&
                  n[2] > 0 && n[4] > 0)
                {
                *inPtr0 = 1;
                }
              
              // special case
              if (prune > 1 && ((countFaces + countCorners) <= 1))
                {
                *inPtr0 = 1;
                }
              
              // one of four face neighbors has to be off
              if (n[0] == 0 || n[2] == 0 ||
                  n[4] == 0 || n[6] == 0)
                {
                // Special condition not to prune diamond corners
                if (prune > 1 || countFaces != 1 || countCorners != 2 ||
                    ((n[1]==0 || n[2]==0 || n[3]==0) && 
                     (n[3]==0 || n[4]==0 || n[5]==0) && 
                     (n[5]==0 || n[6]==0 || n[7]==0) && 
                     (n[7]==0 || n[0]==0 || n[1]==0)))
                  {
                  // special condition (making another prune level)
                  // pruning 135 degree corners
                  if (prune || countFaces != 2 || countCorners != 2 ||
                      ((n[1]==0 || n[2]==0 || n[3]==0 || n[4]) && 
                       (n[0]==0 || n[1]==0 || n[2]==0 || n[3]) && 
                       (n[7]==0 || n[0]==0 || n[1]==0 || n[2]) && 
                       (n[6]==0 || n[7]==0 || n[0]==0 || n[1]) && 
                       (n[5]==0 || n[6]==0 || n[7]==0 || n[0]) && 
                       (n[4]==0 || n[5]==0 || n[6]==0 || n[7]) && 
                       (n[3]==0 || n[4]==0 || n[5]==0 || n[6]) && 
                       (n[2]==0 || n[3]==0 || n[4]==0 || n[5])))
                    {
                    // remaining pixels need to be connected.
                    // do not break corner connectivity
                    if ((n[1] == 0 || n[0] > 1 || n[2] > 1) &&
                        (n[3] == 0 || n[2] > 1 || n[4] > 1) &&
                        (n[5] == 0 || n[4] > 1 || n[6] > 1) &&
                        (n[7] == 0 || n[6] > 1 || n[0] > 1))
                      {
                      // opposite faces 
                      // (special condition so double thick lines
                      // will not be completely eroded)
                      if ((n[0] == 0 || n[4] == 0 || n[2] > 1 || n[6] > 1) &&
                          (n[2] == 0 || n[6] == 0 || n[0] > 1 || n[4] > 1))
                        {
                        // check to stop pruning (sort of a hack huristic)
                        if (prune > 1 || (countFaces > 2) || 
                            ((countFaces == 2) && (countCorners > 1)))
                          {
                          *inPtr0 = 1;
                          }
                        }
                      }
                    }
                  }
                }
              }
            
            }
          inPtr0 += inInc0;
          }
        inPtr1 += inInc1;
        }
      inPtr2 += inInc2;
      }
    ++inPtrC;
    }


  // copy to output
  for (idxC = 0; idxC < numComps; ++idxC)
    {
    outPtr2 = outPtr;
    inPtr2 = inPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
        {
        outPtr0 = outPtr1;
        inPtr0 = inPtr1;
        for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
          {
          if (*inPtr0 <= 1)
            {
            *outPtr0 = (T)(0.0);
            }
          else
            {
            *outPtr0 = *inPtr0;
            }
      
          inPtr0 += inInc0;
          outPtr0 += outInc0;      
          }
        inPtr1 += inInc1;
        outPtr1 += outInc1;      
        }
      inPtr2 += inInc2;
      outPtr2 += outInc2;      
      }
    ++inPtr;
    ++outPtr;
    }
}





//----------------------------------------------------------------------------
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void vtkImageSkeleton2D::ThreadedExecute(vtkImageData *inData, 
                                         vtkImageData *outData, 
                                         int outExt[6], int id)
{
  void *inPtr;
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  vtkImageData *tempData;
  int inExt[6];
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
      << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  this->ComputeInputUpdateExtent(inExt, outExt); 

  // Make a temporary copy of the input data
  tempData = vtkImageData::New();
  tempData->SetScalarType(inData->GetScalarType());
  tempData->SetExtent(inExt);
  tempData->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
  tempData->CopyAndCastFrom(inData, inExt);

  inPtr = tempData->GetScalarPointerForExtent(outExt);
  switch (tempData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageSkeleton2DExecute, this, tempData,
                      (VTK_TT *)(inPtr), outData, outExt, 
                      (VTK_TT *)(outPtr), id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      tempData->Delete();
      return;
    }

  tempData->Delete();
}

void vtkImageSkeleton2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Prune: " << (this->Prune ? "On\n" : "Off\n");

}

