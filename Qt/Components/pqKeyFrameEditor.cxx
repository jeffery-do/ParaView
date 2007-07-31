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

#include "pqKeyFrameEditor.h"
#include "ui_pqKeyFrameEditor.h"

#include <QPointer>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QLineEdit>
#include <QDialog>
#include <QItemDelegate>
#include <QStandardItem>
#include <QComboBox>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqSMAdaptor.h"
#include "pqApplicationCore.h"
#include "pqUndoStack.h"
#include "pqKeyFrameTypeWidget.h"
#include "pqSignalAdaptorKeyFrameType.h"
#include "pqSMProxy.h"
#include "pqPropertyLinks.h"


// editor dialog that comes and goes for editing a single key frame
// interpolation type
pqKeyFrameTypeDialog::pqKeyFrameTypeDialog(QWidget* p, QWidget* child)
  : QDialog(p)
{
  this->Child = child;
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setWindowTitle(tr("Key Frame Interpolation"));
  this->setModal(true);
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(this->Child, 0,0);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

  l->addStretch();
  l->addWidget(buttons, 1, 0);
  this->Child->show();
}

pqKeyFrameTypeDialog::~pqKeyFrameTypeDialog()
{
  // disconnect child
  this->Child->setParent(NULL);
  this->Child->hide();
}

// item model for putting a key frame interpolation widget in the model
class pqKeyFrameInterpolationItem : public QStandardItem
{
public:
  pqKeyFrameInterpolationItem() : Widget()
    {
    }
  // get data from combo box on key frame editor
  QVariant data(int role) const
    {
    int idx = this->Widget.typeComboBox()->currentIndex();
    QAbstractItemModel* comboModel = this->Widget.typeComboBox()->model();
    return comboModel->data(comboModel->index(idx, 0), role);
    }
  pqKeyFrameTypeWidget Widget;
};

// delegate for creating editors for model items
class pqKeyFrameEditorDelegate : public QItemDelegate
{
public:
  pqKeyFrameEditorDelegate(QObject* p) : QItemDelegate(p) {}
  QWidget* createEditor(QWidget* p, const QStyleOptionViewItem&, 
                        const QModelIndex & index ) const
    {
    const QStandardItemModel* model = 
      qobject_cast<const QStandardItemModel*>(index.model());

    if(index.column() == 0)
      {
      return new QLineEdit(p);
      }
    else if(index.column() == 1)
      {
      pqKeyFrameInterpolationItem* item = 
        static_cast<pqKeyFrameInterpolationItem*>(model->item(index.row(), 1));
      if(item)
        {
        return new pqKeyFrameTypeDialog(p, &item->Widget);
        }
      return NULL;
      }
    else if(index.column() == 2)
      {
      // TODO create the widget that auto panels would create?
      // HMMM pqAnimationPanel doesn't do this, nor does it
      //      support all property types.
      return new QLineEdit(p);
      }
    return NULL;
    }
  void updateEditorGeometry(QWidget* editor, 
                            const QStyleOptionViewItem& option,
                            const QModelIndex & index) const
    {
    if(qobject_cast<pqKeyFrameTypeDialog*>(editor))
      {
      int w = 300;
      int h = 100;
      QWidget* p = editor->parentWidget();
      QRect prect = p->geometry();
      QRect newRect(p->mapToGlobal(prect.center()), QSize(w, h));
      newRect.adjust(-w/2, -h, -w/2, -h);
      editor->setGeometry(newRect);
      }
    else
      {
      QItemDelegate::updateEditorGeometry(editor, option, index);
      }
    }
};

class pqKeyFrameEditor::pqInternal : public Ui::pqKeyFrameEditor
{
public:
  pqInternal()
    {
    }
  QPointer<pqAnimationCue> Cue;
  QPointer<pqAnimationScene> Scene;
  QStandardItemModel Model;
  QPair<double,double> TimeRange;

  double normalizedTime(double t)
    {
    // zero span?
    double span = this->TimeRange.second - this->TimeRange.first;
    return (t - this->TimeRange.first) / span;
    }
  double realTime(double t)
    {
    double span = this->TimeRange.second - this->TimeRange.first;
    return t * span + this->TimeRange.first;
    }

  QStandardItem* newTimeItem(int row)
    {
    QStandardItem* item = new QStandardItem();
    int count = this->Model.rowCount();
    
    QVariant time = this->TimeRange.first;
    if(count == row && row != 0)
      {
      time = this->TimeRange.second;
      }
    else if(row > 0)
      {
      // average time between rows
      QModelIndex tidx = this->Model.index(row,0);
      time = this->Model.data(tidx).toDouble();
      tidx = this->Model.index(row-1,0);
      time = time.toDouble() - this->Model.data(tidx).toDouble();
      time = time.toDouble()/2.0;
      }
    item->setData(time, Qt::DisplayRole);
    return item;
    }
  QStandardItem* newInterpolationItem(int row)
    {
    QStandardItem* item = NULL;
    int count = this->Model.rowCount();
    if(count != row || row == 0)
      {
      item = new pqKeyFrameInterpolationItem();
      }
    return item;
    }
  QStandardItem* newValueItem(int /*row*/)
    {
    QStandardItem* item = new QStandardItem();
    // TODO initialize  -- would like to share code with pqAnimationPanel...
    return item;
    }
};

pqKeyFrameEditor::pqKeyFrameEditor(pqAnimationScene* scene, 
                                   pqAnimationCue* cue, QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  this->Internal->Scene = scene;
  this->Internal->Cue = cue;
  this->Internal->TimeRange = scene->getClockTimeRange();

  this->Internal->tableView->setModel(&this->Internal->Model);
  this->Internal->tableView->horizontalHeader()->setStretchLastSection(true);

  this->Internal->tableView->setItemDelegate(
    new pqKeyFrameEditorDelegate(this->Internal->tableView));

  this->readKeyFrameData();

  connect(this->Internal->pbNew, SIGNAL(clicked(bool)),
          this, SLOT(newKeyFrame()));
  connect(this->Internal->pbDelete, SIGNAL(clicked(bool)),
          this, SLOT(deleteKeyFrame()));
  connect(this->Internal->pbDeleteAll, SIGNAL(clicked(bool)),
          this, SLOT(deleteAllKeyFrames()));

}

pqKeyFrameEditor::~pqKeyFrameEditor()
{
  delete this->Internal;
}

void pqKeyFrameEditor::readKeyFrameData()
{
  this->Internal->Model.clear();

  if(!this->Internal->Cue)
    {
    return;
    }

  int numberKeyFrames = this->Internal->Cue->getNumberOfKeyFrames();

  this->Internal->Model.setColumnCount(3);
  this->Internal->Model.setRowCount(numberKeyFrames);

  // set the header data
  QStringList headerLabels;
  headerLabels << tr("Time") << tr("Interpolation") << tr("Value");
  this->Internal->Model.setHorizontalHeaderLabels(headerLabels);

  for(int i=0; i<numberKeyFrames; i++)
    {
    pqSMProxy keyFrame = this->Internal->Cue->getKeyFrame(i);
    
    QModelIndex idx = this->Internal->Model.index(i, 0);
    QVariant keyTime =
      pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyTime"));
    double realKeyTime = this->Internal->realTime(keyTime.toDouble());
    this->Internal->Model.setData(idx, realKeyTime, Qt::DisplayRole);

    if(i < numberKeyFrames-1)
      {
      pqKeyFrameInterpolationItem* item = new pqKeyFrameInterpolationItem();
      this->Internal->Model.setItem(i, 1, item);
      
      // intialize gui with adaptor
      pqPropertyLinks links;
      pqSignalAdaptorKeyFrameType adaptor(&item->Widget, &links);
      adaptor.setKeyFrameProxy(keyFrame);
      }
    
    idx = this->Internal->Model.index(i, 2);
    this->Internal->Model.setData(idx,
      pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyValues")),
      Qt::DisplayRole);
    }

}

void pqKeyFrameEditor::writeKeyFrameData()
{
  if(!this->Internal->Cue)
    {
    return;
    }

  int oldNumber = this->Internal->Cue->getNumberOfKeyFrames();
  int newNumber = this->Internal->Model.rowCount();

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();

  if(stack)
    {
    stack->beginUndoSet("Edit Keyframes");
    }

  for(int i=0; i<oldNumber-newNumber; i++)
    {
    this->Internal->Cue->insertKeyFrame(0);
    }
  for(int i=0; i<newNumber-oldNumber; i++)
    {
    this->Internal->Cue->deleteKeyFrame(0);
    }

  for(int i=0; i<newNumber; i++)
    {
    vtkSMProxy* keyFrame = this->Internal->Cue->getKeyFrame(i);

    QModelIndex idx = this->Internal->Model.index(i, 0);
    QVariant newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    double nTime = this->Internal->normalizedTime(newData.toDouble());
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"), nTime);
      
    pqKeyFrameInterpolationItem* item = 
      static_cast<pqKeyFrameInterpolationItem*>(this->Internal->Model.item(i, 1));
    if(item)
      {
      pqSMAdaptor::setEnumerationProperty(keyFrame->GetProperty("Type"),
        item->Widget.type());
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Base"),
        item->Widget.base());
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("StartPower"),
        item->Widget.startPower());
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("EndPower"),
        item->Widget.endPower());
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Phase"),
        item->Widget.phase());
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Offset"),
        item->Widget.offset());
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Frequency"),
        item->Widget.frequency());
      }
    
    idx = this->Internal->Model.index(i, 2);
    newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyValues"), newData);
    keyFrame->UpdateVTKObjects();
    }
  
  if(stack)
    {
    stack->endUndoSet();
    }
}

void pqKeyFrameEditor::newKeyFrame()
{
  // insert before selection, or insert 2nd to last
  int row = 0;
  int count = this->Internal->Model.rowCount();

  QModelIndex idx = 
    this->Internal->tableView->selectionModel()->currentIndex();
  if(idx.isValid())
    {
    row = idx.row();
    }
  else
    {
    row = count != 0 ? count-1 : 0;
    }

  QList<QStandardItem*> items;
  items.append(this->Internal->newTimeItem(row));
  items.append(this->Internal->newInterpolationItem(row));
  items.append(this->Internal->newValueItem(row));
  this->Internal->Model.insertRow(row, items);
  
  // add one more
  if(count == 0)
    {
    items.clear();
    items.append(this->Internal->newTimeItem(1));
    items.append(this->Internal->newInterpolationItem(1));
    items.append(this->Internal->newValueItem(1));
    this->Internal->Model.insertRow(1, items);
    }
}

void pqKeyFrameEditor::deleteKeyFrame()
{
  QModelIndex idx = 
    this->Internal->tableView->selectionModel()->currentIndex();
  if(idx.isValid())
    {
    this->Internal->Model.removeRow(idx.row());
    }
}

void pqKeyFrameEditor::deleteAllKeyFrames()
{
  // remove all rows
  this->Internal->Model.removeRows(0, this->Internal->Model.rowCount());
}


