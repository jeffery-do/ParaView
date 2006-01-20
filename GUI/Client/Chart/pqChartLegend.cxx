/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqChartLabel.h"
#include "pqChartLegend.h"

#include <vtkstd/vector>

#include <QPainter>

static const int padding = 5;
static const int line_width = 15;

class pqChartLegend::pqImplementation
{
public:
  QRect Bounds;
  
  vtkstd::vector<QPen> Pens;
  vtkstd::vector<pqChartLabel*> Labels;
};

pqChartLegend::pqChartLegend(QObject* parent) :
  QObject(parent),
  Implementation(new pqImplementation())
{
}

pqChartLegend::~pqChartLegend()
{
  this->clear();
  delete Implementation;
}

void pqChartLegend::clear()
{
  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    delete this->Implementation->Labels[i];
    
  this->Implementation->Pens.clear();
  this->Implementation->Labels.clear();
}

void pqChartLegend::addEntry(QPen& pen, pqChartLabel* label)
{
  if(!label)
    return;
      
  this->Implementation->Pens.push_back(pen);
  this->Implementation->Labels.push_back(label);
}

const QRect pqChartLegend::getSizeRequest()
{
  QRect result(0, 0, 0, 0);

  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    {
    QRect request = this->Implementation->Labels[i]->getSizeRequest();
    result.setBottom(result.bottom() + request.height());
    result.setWidth(vtkstd::max(result.width(), request.width()));
    }
  
  result.setWidth(result.width() + line_width + (2 * padding));
  result.setHeight(result.height() + (2 * padding));
  
  return result;
}

void pqChartLegend::setBounds(const QRect& bounds)
{
  this->Implementation->Bounds = bounds;
  
  if(this->Implementation->Labels.size())
    {
    const int entry_height = (bounds.height() - padding - padding) / this->Implementation->Labels.size();
    for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
      {
      this->Implementation->Labels[i]->setBounds(
        QRect(
          bounds.left() + line_width + padding,
          bounds.top() + padding + (i * entry_height),
          bounds.width() - line_width - padding - padding,
          entry_height));
      }
    }
  
  emit repaintNeeded();
}

void pqChartLegend::draw(QPainter& painter, const QRect& area)
{
  if(this->Implementation->Labels.empty())
    return;
    
  painter.save();
  painter.drawRect(this->Implementation->Bounds);
  
  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    {
    painter.setPen(this->Implementation->Pens[i]);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    const QRect label_bounds = this->Implementation->Labels[i]->getBounds();
    
    painter.drawLine(
      this->Implementation->Bounds.left() + padding,
      label_bounds.top(),
      this->Implementation->Bounds.left() + line_width,
      label_bounds.bottom());
    }
  
  for(unsigned int i = 0; i != this->Implementation->Labels.size(); ++i)
    {
    this->Implementation->Labels[i]->draw(painter, area);
    }
  
  painter.restore();
}
