/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlaneWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVPlaneWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// This widget creates and manages its own vtkPlane on each process.
// I could not descide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPlaneWidget_h
#define __vtkPVPlaneWidget_h

#include "vtkPVImplicitPlaneWidget.h"

class vtkPVSource;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWLabel;

class VTK_EXPORT vtkPVPlaneWidget : public vtkPVImplicitPlaneWidget
{
public:
  static vtkPVPlaneWidget* New();
  vtkTypeRevisionMacro(vtkPVPlaneWidget, vtkPVImplicitPlaneWidget);

  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // This method sets the input to the 3D widget and places the widget.
  virtual void ActualPlaceWidget();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVPlaneWidget* ClonePrototype(vtkPVSource* pvSource,
                                   vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  void SetCenter(float,float,float);
  virtual void SetCenter(float f[3]) { this->SetCenter(f[0], f[1], f[2]); }
  void SetNormal(float,float,float);
  virtual void SetNormal(float f[3]) { this->SetNormal(f[0], f[1], f[2]); }

  // Description:
  // For saving the widget into a VTK tcl script.
  // Saves a plane (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

protected:
  vtkPVPlaneWidget();
  ~vtkPVPlaneWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVPlaneWidget(const vtkPVPlaneWidget&); // Not implemented
  void operator=(const vtkPVPlaneWidget&); // Not implemented
};

#endif
