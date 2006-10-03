/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqHistogramListModel.h
/// \date 8/15/2006

#ifndef _pqHistogramListModel_h
#define _pqHistogramListModel_h


#include "QtChartExport.h"
#include "pqHistogramModel.h"

class pqChartValue;
class pqHistogramListModelInternal;


/// \class pqHistogramListModel
/// \brief
///   The pqHistogramListModel class is a generic histogram model.
class QTCHART_EXPORT pqHistogramListModel : public pqHistogramModel
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a histogram list model object.
  /// \param parent The parent object.
  pqHistogramListModel(QObject *parent=0);
  virtual ~pqHistogramListModel();

  /// \name pqHistogramModel Methods
  //@{
  virtual int getNumberOfBins() const;
  virtual void getBinValue(int index, pqChartValue &bin) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// \name Data Setup Methods
  //@{
  /// Removes all the histogram data.
  void clearBinValues();

  /// \brief
  ///   Adds a bin value to the end of the histogram.
  /// \param bin The new histogram value.
  void addBinValue(const pqChartValue &bin);

  /// \brief
  ///   Inserts a new bin value into the histogram.
  /// \param index Where to insert the bin.
  /// \param bin The new histogram value.
  void insertBinValue(int index, const pqChartValue &bin);

  /// \brief
  ///   Removes the specified bin value from the histogram.
  /// \param index The index of the bin to remove.
  void removeBinValue(int index);

  void setRangeX(const pqChartValue &min, const pqChartValue &max);
  void setMinimumX(const pqChartValue &min);
  void setMaximumX(const pqChartValue &max);
  //@}

private:
  pqHistogramListModelInternal *Internal; ///< Stores the histogram data.
};

#endif
