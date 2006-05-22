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

#include "pqObjectInspectorWidget.h"

// Qt includes
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QTabWidget>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>

// vtk includes
#include "QVTKWidget.h"

// paraview includes

// paraq includes
#include "pqApplicationCore.h"
#include "pqAutoGeneratedObjectPanel.h"
#include "pqClipPanel.h"
#include "pqCutPanel.h"
#include "pqLoadedFormObjectPanel.h"
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"

pqObjectInspectorWidget::pqObjectInspectorWidget(QWidget *p)
  : QWidget(p)
{
  this->setObjectName("ObjectInspectorWidget");

  this->TabWidget = 0;
  this->CurrentAutoPanel = 0;
  this->CurrentCustomPanel = 0;

  this->TabWidget = new QTabWidget(this);
  QScrollArea* s = new QScrollArea();
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  s->setWidgetResizable(true);
  s->setObjectName("ScrollView");
  QVBoxLayout *boxLayout = new QVBoxLayout(s);
  boxLayout->setMargin(0);

  this->TabWidget->addTab(s, "");
  this->TabWidget->setObjectName("TabWidget");
  this->TabWidget->hide();
  
  // main layout
  boxLayout = new QVBoxLayout(this);
  boxLayout->setMargin(0);

  QBoxLayout* buttonlayout = new QHBoxLayout();
  boxLayout->addLayout(buttonlayout);
  this->AcceptButton = new QPushButton(this);
  this->AcceptButton->setObjectName("Accept");
  this->AcceptButton->setText(tr("Accept"));
  this->ResetButton = new QPushButton(this);
  this->ResetButton->setObjectName("Reset");
  this->ResetButton->setText(tr("Reset"));
  buttonlayout->addWidget(this->AcceptButton);
  buttonlayout->addWidget(this->ResetButton);

  boxLayout->addStretch();
  
  this->connect(this->AcceptButton, SIGNAL(pressed()), SLOT(accept()));
  this->connect(this->ResetButton, SIGNAL(pressed()), SLOT(reset()));

  this->AcceptButton->setEnabled(false);
  this->ResetButton->setEnabled(false);


  this->connect(pqApplicationCore::instance()->getPipelineData(), 
                SIGNAL(sourceUnRegistered(vtkSMProxy*)),
                SLOT(removeProxy(vtkSMProxy*)));
 
}

pqObjectInspectorWidget::~pqObjectInspectorWidget()
{
  // delete all queued panels
  foreach(pqObjectPanel* p, this->QueuedCustomPanels)
    {
    delete p;
    }
  
  foreach(pqObjectPanel* p, this->QueuedAutoPanels)
    {
    delete p;
    }
}

void pqObjectInspectorWidget::setProxy(vtkSMProxy *proxy)
{
  // do nothing if this proxy is already current
  if(this->CurrentAutoPanel && this->CurrentAutoPanel->proxy() == proxy)
    {
    return;
    }

  if (this->CurrentAutoPanel)
    {
    this->CurrentAutoPanel->unselect();
    }
  if (this->CurrentCustomPanel)
    {
    this->CurrentCustomPanel->unselect();
    }

  // we have a proxy with pending changes
  if(this->AcceptButton->isEnabled())
    {
    // save the panels
    if(this->CurrentAutoPanel)
      {
      this->CurrentAutoPanel->setObjectName("");
      this->QueuedAutoPanels.insert(this->CurrentAutoPanel->proxy(),
        this->CurrentAutoPanel);
      }

    if(this->CurrentCustomPanel)
      {
      this->CurrentCustomPanel->setObjectName("");
      this->QueuedCustomPanels.insert(this->CurrentCustomPanel->proxy(),
        this->CurrentCustomPanel);
      }
    }
  else
    {
    // delete the panels
    if(this->CurrentAutoPanel)
      {
      delete this->CurrentAutoPanel;
      }

    if(this->CurrentCustomPanel)
      {
      delete this->CurrentCustomPanel;
      }
    }

  this->CurrentCustomPanel = NULL;
  this->CurrentAutoPanel = NULL;

  // make sure proxy is built
  if(proxy)
    {
    proxy->UpdateVTKObjects();
    proxy->UpdatePropertyInformation();
    }
  
  // search for a custom form for this proxy with pending changes
  QMap<pqSMProxy, pqObjectPanel*>::iterator iter;
  iter = this->QueuedCustomPanels.find(proxy);
  if(iter != this->QueuedCustomPanels.end())
    {
    this->CurrentCustomPanel = iter.value();
    }

  if(proxy && !this->CurrentCustomPanel)
    {
    if(QString(proxy->GetXMLName()) == "Clip")
      {
      this->CurrentCustomPanel = new pqClipPanel(NULL);
      this->CurrentCustomPanel->setProxy(proxy);
      }
    else if(QString(proxy->GetXMLName()) == "Cut")
      {
      this->CurrentCustomPanel = new pqCutPanel(NULL);
      this->CurrentCustomPanel->setProxy(proxy);
      }
    else
      {
      // try to find a custom form in our pqWidgets resources
      QString proxyui = QString(":/pqWidgets/") + QString(proxy->GetXMLName()) + QString(".ui");
      pqLoadedFormObjectPanel* panel = new pqLoadedFormObjectPanel(proxyui, NULL);
      if(!panel->isValid())
        {
        delete panel;
        panel = NULL;
        }
        this->CurrentCustomPanel = panel;
      }
    
    if(this->CurrentCustomPanel)
      {
      this->CurrentCustomPanel->setProxy(proxy);
      
      QObject::connect(this->CurrentCustomPanel->getPropertyManager(), 
        SIGNAL(canAcceptOrReject(bool)), 
        this->AcceptButton, SLOT(setEnabled(bool)));
      QObject::connect(this->CurrentCustomPanel->getPropertyManager(), 
        SIGNAL(canAcceptOrReject(bool)), 
        this->ResetButton, SLOT(setEnabled(bool)));

      }
    }

  // search for an auto generated form for this proxy with pending changes
  QMap<pqSMProxy, pqAutoGeneratedObjectPanel*>::iterator jter;
  jter = this->QueuedAutoPanels.find(proxy);
  if(jter != this->QueuedAutoPanels.end())
    {
    this->CurrentAutoPanel = jter.value();
    }

  if(this->CurrentAutoPanel == NULL)
    {
    this->CurrentAutoPanel = new pqAutoGeneratedObjectPanel;
    this->CurrentAutoPanel->setProxy(proxy);
      
    QObject::connect(this->CurrentAutoPanel->getPropertyManager(), 
      SIGNAL(canAcceptOrReject(bool)), 
      this->AcceptButton, SLOT(setEnabled(bool)));
    QObject::connect(this->CurrentAutoPanel->getPropertyManager(), 
      SIGNAL(canAcceptOrReject(bool)), 
      this->ResetButton, SLOT(setEnabled(bool)));
    }

  // the current auto panel always has the name "Editor"
  if(this->CurrentAutoPanel)
    {
    this->CurrentAutoPanel->setObjectName("Editor");
    }

  // set up layout for with or without custom form
  if(this->CurrentCustomPanel && this->TabWidget->isHidden())
    {
    QLayoutItem* item = this->layout()->takeAt(1);
    if(item && item->widget())
      {
      item->widget()->hide();
      item->widget()->setParent(NULL);
      }

    this->TabWidget->addTab(this->CurrentAutoPanel, "Advanced");

    this->layout()->addWidget(this->TabWidget);
    QScrollArea* s = qobject_cast<QScrollArea*>(this->TabWidget->widget(0));
    s->setWidget(this->CurrentCustomPanel);
    this->TabWidget->setTabText(0, proxy->GetXMLName());
    this->TabWidget->show();
    this->CurrentCustomPanel->show();
    }
  else if(this->CurrentCustomPanel && !this->TabWidget->isHidden())
    {
    // put new custom panel in place
    this->TabWidget->setTabText(0, proxy->GetXMLName());
    QScrollArea* s = qobject_cast<QScrollArea*>(this->TabWidget->widget(0));
    QWidget* lastform = s->takeWidget();
    if(lastform)
      {
      lastform->hide();
      lastform->setParent(NULL);
      }
    
    // put new auto panel in place
    lastform = this->TabWidget->widget(1);
    if(lastform)
      {
      lastform->hide();
      }
    this->TabWidget->removeTab(1);
    this->TabWidget->addTab(this->CurrentAutoPanel, "Advanced");
    this->CurrentCustomPanel->show();
    }
  else if(!this->CurrentCustomPanel && !this->TabWidget->isHidden())
    {
    // we don't have a custom form, make sure we don't show one, if we did previously
    QScrollArea* s = qobject_cast<QScrollArea*>(this->TabWidget->widget(0));
    s->takeWidget();
    this->layout()->removeWidget(this->TabWidget);
    this->TabWidget->removeTab(1);
    this->TabWidget->hide();
    
    this->layout()->addWidget(this->CurrentAutoPanel);
    this->CurrentAutoPanel->show();
    }
  else if(!this->CurrentCustomPanel && this->TabWidget->isHidden())
    {
    QLayoutItem* item = this->layout()->takeAt(1);
    if(item && item->widget())
      {
      item->widget()->hide();
      item->widget()->setParent(NULL);
      }
    this->layout()->addWidget(this->CurrentAutoPanel);
    this->CurrentAutoPanel->show();
    }

  if (this->CurrentAutoPanel)
    {
    this->CurrentAutoPanel->select();
    }
  if (this->CurrentCustomPanel)
    {
    this->CurrentCustomPanel->select();
    }
}

void pqObjectInspectorWidget::accept()
{
  emit this->preaccept();

  // accept all queued panels
  foreach(pqObjectPanel* p, this->QueuedCustomPanels)
    {
    p->getPropertyManager()->accept();
    delete p;
    }
  this->QueuedCustomPanels.clear();
  
  foreach(pqObjectPanel* p, this->QueuedAutoPanels)
    {
    p->getPropertyManager()->accept();
    delete p;
    }
  this->QueuedAutoPanels.clear();
  
  if(this->CurrentAutoPanel)
    {
    this->CurrentAutoPanel->getPropertyManager()->accept();
    }
 
  pqApplicationCore::instance()->getActiveRenderModule()->render();

  /* For now just render the active window, later we will
     render all the windows to which this source belongs.
  // cause the screen to update
  pqPipelineSource *source = 
    pqServerManagerModel::instance()->getPQSource(proxy);
  if(source)
    {
    // FIXME
    // source->getDisplay()->UpdateWindows();
    }
    */
  
  emit this->accepted();
  emit this->postaccept();
}

void pqObjectInspectorWidget::reset()
{
  emit this->prereject();

  // delete all queued panels
  foreach(pqObjectPanel* p, this->QueuedCustomPanels)
    {
    delete p;
    }
  this->QueuedCustomPanels.clear();
  
  foreach(pqObjectPanel* p, this->QueuedAutoPanels)
    {
    delete p;
    }
  this->QueuedAutoPanels.clear();

  if(this->CurrentAutoPanel)
    {
    this->CurrentAutoPanel->getPropertyManager()->reject();
    }
  if(this->CurrentCustomPanel)
    {
    this->CurrentCustomPanel->getPropertyManager()->reject();
    }

  emit postreject();
}

QSize pqObjectInspectorWidget::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = 20 * (qMax(fm.lineSpacing(), 14));
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), this));
}

void pqObjectInspectorWidget::removeProxy(vtkSMProxy* proxy)
{
  QMap<pqSMProxy, pqObjectPanel*>::iterator iter;
  iter = this->QueuedCustomPanels.find(proxy);
  if(iter != this->QueuedCustomPanels.end())
    {
    delete iter.value();
    this->QueuedCustomPanels.erase(iter);
    }
  
  QMap<pqSMProxy, pqAutoGeneratedObjectPanel*>::iterator jter;
  jter = this->QueuedAutoPanels.find(proxy);
  if(jter != this->QueuedAutoPanels.end())
    {
    delete jter.value();
    this->QueuedAutoPanels.erase(jter);
    }

  if(this->CurrentCustomPanel && this->CurrentCustomPanel->proxy() == proxy)
    {
    delete this->CurrentCustomPanel;
    this->CurrentCustomPanel = NULL;
    }
  if(this->CurrentAutoPanel && this->CurrentAutoPanel->proxy() == proxy)
    {
    delete this->CurrentAutoPanel;
    this->CurrentAutoPanel = NULL;
    }
}

