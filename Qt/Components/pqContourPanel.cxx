/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

#include "pqApplicationCore.h"
#include "pqContourPanel.h"
#include "pqNamedWidgets.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqSampleScalarWidget.h"
#include "pqProxySelectionWidget.h"

#include "ui_pqContourControls.h"

#include <pqCollapsedGroup.h>

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QCheckBox>
#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqContourPanel::pqImplementation

class pqContourPanel::pqImplementation
{
public:
  pqImplementation() :
    SampleScalarWidget(false)
  {
  }

  /// Provides a container for Qt controls
  QWidget ControlsContainer;
  /// Provides the Qt controls for the panel
  Ui::pqContourControls Controls;
  /// Controls the number and values of contours
  pqSampleScalarWidget SampleScalarWidget;
};

pqContourPanel::pqContourPanel(pqProxy* object_proxy, QWidget* p) :
  base(object_proxy, p),
  Implementation(new pqImplementation())
{
  this->Implementation->Controls.setupUi(
    &this->Implementation->ControlsContainer);

  pqCollapsedGroup* const group1 = new pqCollapsedGroup(this);
  group1->setTitle(tr("Contour"));
  QVBoxLayout* l = new QVBoxLayout(group1);
  this->Implementation->ControlsContainer.layout()->setMargin(0);
  l->addWidget(&this->Implementation->ControlsContainer);
  
  // begin: the locator --- a proxy property (incremental_point_locators)
  // 
  // make the locator a sibling of the contour sub-panel and a child of 'this'
  pqProxySelectionWidget* locator = new pqProxySelectionWidget
    ( object_proxy->getProxy(), "Locator",
      this->proxy()->GetProperty( "Locator" )->GetXMLLabel(), this );
  locator->layout()->setMargin( 0 );      
  locator->setObjectName( "Locator" );
  
  // link this proxy (the GUI element) with "Locator" (the underlying property
  // of the contour filter)
  this->propertyManager()
      ->registerLink( locator, "proxy", SIGNAL(proxyChanged(pqSMProxy)),
                      this->proxy(), this->proxy()->GetProperty("Locator") );
  
  // exchange signals beween the contour and the locator
  QObject::connect( this, SIGNAL(viewChanged(pqView*) ),
                    locator, SLOT(setView(pqView*)) );
  QObject::connect( this, SIGNAL(onaccept()), locator, SLOT(accept()) );
  QObject::connect( this, SIGNAL(onreset()),  locator, SLOT(reset())  );
  QObject::connect( this, SIGNAL(onselect()), locator, SLOT(select()) );
  QObject::connect( this, SIGNAL(ondeselect()), locator, SLOT(deselect()) );
  QObject::connect( locator, SIGNAL(modified()), this, SLOT(setModified()) );
  QObject::connect( locator, SIGNAL(proxyChanged(pqSMProxy)), 
                    this,    SLOT(setModified()) );
  //
  // end: the locator --- a proxy property (incremental_point_locators) 

  pqCollapsedGroup* const group2 = new pqCollapsedGroup(this);
  group2->setTitle(tr(this->proxy()->GetProperty("ContourValues")->GetXMLLabel()));
  l = new QVBoxLayout(group2);
  this->Implementation->SampleScalarWidget.layout()->setMargin(0);
  l->addWidget(&this->Implementation->SampleScalarWidget);
  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->addWidget(group1);
  panel_layout->addWidget(group2);
  panel_layout->addWidget(locator);
  panel_layout->addStretch();

  connect(this->propertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->propertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));

  // Setup the sample scalar widget ...
  this->Implementation->SampleScalarWidget.setDataSources(
    this->proxy(),
    vtkSMDoubleVectorProperty::SafeDownCast(this->proxy()->GetProperty("ContourValues")),
    this->proxy()->GetProperty("SelectInputScalars"));

  // Link SampleScalarWidget's qProperty to vtkSMProperty
  this->propertyManager()->registerLink(
    &this->Implementation->SampleScalarWidget, "samples",
    SIGNAL(samplesChanged()), this->proxy(),
    this->proxy()->GetProperty("ContourValues"));

  pqNamedWidgets::link(
    &this->Implementation->ControlsContainer, this->proxy(), this->propertyManager());
}

pqContourPanel::~pqContourPanel()
{
  delete this->Implementation;
}

void pqContourPanel::onAccepted()
{
  this->Implementation->SampleScalarWidget.accept();
}

void pqContourPanel::onRejected()
{
  this->Implementation->SampleScalarWidget.reset();
}
