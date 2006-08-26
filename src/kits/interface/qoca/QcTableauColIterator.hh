// $Id: QcTableauColIterator.hh,v 1.7 2000/11/29 01:58:42 pmoulder Exp $

//============================================================================//
// Originally written by Sitt Sen Chok
//----------------------------------------------------------------------------//
// The QOCA implementation is free software, but it is Copyright (C)          //
// 1994-1999 Monash University.  It is distributed under the terms of the GNU //
// General Public License.  See the file COPYING for copying permission.      //
//                                                                            //
// The QOCA toolkit and runtime are distributed under the terms of the GNU    //
// Library General Public License.  See the file COPYING.LIB for copying      //
// permissions for those files.                                               //
//                                                                            //
// If those licencing arrangements are not satisfactory, please contact us!   //
// We are willing to offer alternative arrangements, if the need should arise.//
//                                                                            //
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#ifndef __QcTableauColIteratorH
#define __QcTableauColIteratorH

#ifdef qcRealTableauCoeff
# include "qoca/QcSparseMatrixColIterator.hh"
#else
# include "qoca/QcNullIterator.H"
#endif

class QcTableauColIterator
#ifdef qcRealTableauCoeff
  : public QcSparseMatrixColIterator
#else
  : public QcNullIterator
#endif
{
public:
  QcTableauColIterator (const QcLinEqTableau &tab, unsigned col);
};


inline QcTableauColIterator::QcTableauColIterator(const QcLinEqTableau &tab,
						  unsigned col)
#ifdef qcRealTableauCoeff
  : QcSparseMatrixColIterator (tab.GetCoreTableau().fSF.getCoeffs(), col)
#endif
{
  qcAssertPre (col < tab.getNColumns());
}


#endif /* !__QcTableauColIteratorH */
