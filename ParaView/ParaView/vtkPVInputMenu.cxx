/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputMenu.cxx
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
#include "vtkPVInputMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataSet.h"
#include "vtkSource.h"
#include "vtkPVApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVInputProperty.h"
#include "vtkPVDataInformation.h"
#include "vtkPVPart.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputMenu);
vtkCxxRevisionMacro(vtkPVInputMenu, "1.40");


//----------------------------------------------------------------------------
vtkPVInputMenu::vtkPVInputMenu()
{
  this->InputName = NULL;
  this->Sources = NULL;
  this->CurrentValue = NULL;

  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkPVInputMenu::~vtkPVInputMenu()
{
  this->SetInputName(NULL);
  this->Sources = NULL;

  this->Label->Delete();
  this->Label = NULL;
  this->Menu->Delete();
  this->Menu = NULL;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetLabel(const char* label)
{
  this->Label->SetLabel(label);
  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->Menu->SetParent(this);
  this->Menu->Create(app, "");
  this->Script("pack %s -side left", this->Menu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::AddSources(vtkPVSourceCollection *sources)
{
  vtkObject *o;
  vtkPVSource *source;
  int currentFound = 0;
  vtkPVInputProperty *inProp;
  
  if (sources == NULL)
    {
    return;
    }

  inProp = this->GetInputProperty();

  this->ClearEntries();
  sources->InitTraversal();
  while ( (o = sources->GetNextItemAsObject()) )
    {
    source = vtkPVSource::SafeDownCast(o);
    if (this->AddEntry(source) && source == this->CurrentValue)
      {
      currentFound = 1;
      }
    }
  // Input should be initialized in VTK and reset will initialze the menu.
  if ( ! currentFound)
    {
    // Set the default source.
    sources->InitTraversal();
    this->SetCurrentValue((vtkPVSource*)(sources->GetNextItemAsObject()));
    this->ModifiedCallback();
    }

  if (this->CurrentValue)
    {
    this->Menu->SetValue(this->CurrentValue->GetName());
    }
  else
    {
    this->Menu->SetValue("");
    }

}

//----------------------------------------------------------------------------
int vtkPVInputMenu::AddEntry(vtkPVSource *pvs)
{
  vtkPVData *pvd;
  if (pvs == this->PVSource || pvs == NULL)
    {
    return 0;
    }

  // Have to have the same number of parts as last input.
  pvd = pvs->GetPVOutput();
  if (pvd == NULL)
    {
    return 0;
    }
  if (this->CurrentValue)
    {
    vtkPVData *currentData = this->CurrentValue->GetPVOutput();
    if (currentData != NULL)
      {
      if (pvd->GetNumberOfPVParts() != currentData->GetNumberOfPVParts())
        {
        return 0;
        }
      }
    }

  // Has to meet all requirments from XML filter description.
  if ( ! this->GetInputProperty()->GetIsValidInput(pvs->GetPVOutput(), 
                                                   this->PVSource))
    {
    return 0;
    }

  char methodAndArgs[1024];
  sprintf(methodAndArgs, "MenuEntryCallback %s", pvs->GetTclName());

  this->Menu->AddEntryWithCommand(pvs->GetName(), 
                                  this, methodAndArgs);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::MenuEntryCallback(vtkPVSource *pvs)
{
  if (pvs == this->CurrentValue)
    {
    return;
    }
  if ( this->CheckForLoop(pvs) )
    {
    vtkKWMessageDialog::PopupMessage(
        this->Application, this->GetPVApplication()->GetMainWindow(),
        "ParaView Error", 
        "This operation would result in a loop in the pipeline. "
        "Since loops in the pipeline can result in infinite loops, "
        "this operation is prohibited.",
        vtkKWMessageDialog::ErrorIcon);
    this->Menu->SetValue(this->CurrentValue->GetName());
    return;
    }
  this->CurrentValue = pvs;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetCurrentValue(vtkPVSource *pvs)
{
  if (pvs == this->CurrentValue)
    {
    return;
    }

  this->CurrentValue = pvs;
  if (this->Application == NULL)
    {
    return;
    }
  if (pvs)
    {
    this->Menu->SetValue(pvs->GetName());
    }
  else
    {
    this->Menu->SetValue("");
    }
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
int vtkPVInputMenu::CheckForLoop(vtkPVSource *pvs)
{
  if ( !pvs )
    {
    return 0;
    }
  vtkPVSource* source = this->GetPVSource();
  if ( pvs == source )
    {
    return 1;
    }
  int cc;
  int res = 0;
  for ( cc = 0; cc < pvs->GetNumberOfPVInputs(); cc ++ )
    {
    vtkPVData* data = pvs->GetPVInput(cc);
    if ( data && data->GetPVSource() )
      {
      res += this->CheckForLoop(data->GetPVSource());
      }
    }
  return res;
}
//----------------------------------------------------------------------------
vtkPVData *vtkPVInputMenu::GetPVData()
{
  vtkPVSource *pvs = this->GetCurrentValue();

  if (pvs == NULL)
    {
    return NULL;
    }
  return pvs->GetPVOutput();
}

//----------------------------------------------------------------------------
// vtkPVSource handles this now.
void vtkPVInputMenu::SaveInBatchScript(ofstream*)
{
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::ModifiedCallback()
{
  this->vtkPVWidget::ModifiedCallback();
}

//----------------------------------------------------------------------------
// We should probably cache this value.
// compute it once when the InputName is set ...
int vtkPVInputMenu::GetPVInputIndex()
{
  int num, idx;

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource must be set before translation.");
    return 0;
    }
  num = this->PVSource->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    if (strcmp(this->InputName, 
               this->PVSource->GetInputProperty(idx)->GetName()) == 0)
      {
      return idx;
      }
    }

  vtkErrorMacro("Cound not find VTK input name: " << this->InputName);
  return 0;
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVInputMenu::GetInputProperty()
{
  return this->PVSource->GetInputProperty(this->InputName);
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::AcceptInternal(const char* sourceTclName)
{
  // Since this is done on PVSource and not vtk source,
  // We can ignore the sourceTclName and return after the first call.
  if (this->ModifiedFlag == 0)
    {
    return;
    }
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  if (this->CurrentValue)
    {
    this->Script("%s SetPVInput %d %s", this->PVSource->GetTclName(), 
                 this->GetPVInputIndex(),
                 this->CurrentValue->GetPVOutput()->GetTclName());
    }
  else
    {
    this->Script("%s SetPVInput %d {}", this->PVSource->GetTclName(), 
                 this->GetPVInputIndex());
    }

  // ??? used to be set to one ???
  this->ModifiedFlag=0;
}


//---------------------------------------------------------------------------
void vtkPVInputMenu::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  if (this->CurrentValue && this->CurrentValue->InitializeTrace(file))
    {
    *file << "$kw(" << this->GetTclName() << ") SetCurrentValue "
          << "$kw(" << this->CurrentValue->GetTclName() << ")\n";
    }
  else
    {
    *file << "$kw(" << this->GetTclName() << ") SetCurrentValue "
          << "{}\n";
    }
}



//----------------------------------------------------------------------------
void vtkPVInputMenu::ResetInternal(const char* sourceTclName)
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  // The list of possible inputs could have changed.
  this->AddSources(this->Sources);

  // Set the current value.  The catch is here because GlyphSource is
  // initially NULL which causes an error.
  this->Script("catch {%s SetCurrentValue [[%s GetPVInput %d] GetPVSource]}", 
               this->GetTclName(), this->PVSource->GetTclName(), 
               this->GetPVInputIndex());    

  // Only turn modified off if the SetCurrentValue was successful.
  if (this->GetIntegerResult(this->Application) == 0)
    {
    this->ModifiedFlag = 0;
    }
  else
    {
    this->ModifiedFlag = 1;
    }
}

//----------------------------------------------------------------------------
vtkPVInputMenu* vtkPVInputMenu::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVInputMenu::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVInputMenu* pvim = vtkPVInputMenu::SafeDownCast(clone);
  if (pvim)
    {
    pvim->SetLabel(this->Label->GetLabel());
    pvim->SetInputName(this->InputName);
    pvim->SetSources(this->GetSources());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVInputMenu.");
    }
}

//----------------------------------------------------------------------------
int vtkPVInputMenu::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->Label->SetLabel(label);  
  
  // Setup the InputName.
  const char* input_name = element->GetAttribute("input_name");
  if(input_name)
    {
    this->SetInputName(input_name);
    }
  else
    {
    this->SetInputName("Input");
    }
    
  vtkPVWindow* window = this->GetPVWindowFormParser(parser);
  const char* source_list = element->GetAttribute("source_list");
  if(source_list)
    {
    this->SetSources(window->GetSourceList(source_list));
    }
  else
    {
    this->SetSources(window->GetSourceList("Sources"));
    }
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVInputMenu::GetLabel() 
{
  return this->Label->GetLabel();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetSources(vtkPVSourceCollection *sources) 
{
  this->Sources = sources;
}

//----------------------------------------------------------------------------
vtkPVSourceCollection *vtkPVInputMenu::GetSources() 
{
  return this->Sources;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::ClearEntries() 
{ 
  this->Menu->ClearEntries();
}


//----------------------------------------------------------------------------
void vtkPVInputMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputName: " << (this->InputName?this->InputName:"none") 
     << endl;
}
