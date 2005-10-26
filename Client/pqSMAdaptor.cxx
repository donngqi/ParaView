
#include "pqSMAdaptor.h"

#include <assert.h>
#include <map>

#include <QString>
#include <QVariant>
#include <QByteArray>
#include <QSignalMapper>

#include "vtkEventQtSlotConnect.h"

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMVectorProperty.h"


namespace {
  // store SM side of connection information
  class SMGroup
    {
  public:
    SMGroup(vtkSMProxy* a, vtkSMProperty* b, int c)
      : Proxy(a), Property(b), Index(c) {}
    SMGroup& operator=(const SMGroup& copy)
      {
      this->Proxy = copy.Proxy;
      this->Property = copy.Property;
      this->Index = copy.Index;
      return *this;
      }
    bool operator<(SMGroup const& other) const
      {
      if(this->Proxy < other.Proxy)
        return true;
      else if(this->Proxy > other.Proxy)
        return false;

      if(this->Property < other.Property)
        return true;
      else if(this->Property > other.Property)
        return false;

      return this->Index < other.Index;
      }
    vtkSMProxy* Proxy;
    vtkSMProperty* Property;
    int Index;
    };
  // store Qt side of connection information
  typedef vtkstd::pair<QObject*, QByteArray> QtGroup;
}


class pqSMAdaptorInternal
{
public:
  pqSMAdaptorInternal()
    {
    VTKConnections = vtkEventQtSlotConnect::New();
    QtConnections = new QSignalMapper;
    }
  ~pqSMAdaptorInternal()
    {
    VTKConnections->Delete();
    delete QtConnections;
    }

  // managed links
  typedef vtkstd::multimap<SMGroup, QtGroup> LinkMap;
  LinkMap SMLinks;
  LinkMap QtLinks;

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  
  // handle changes from the Qt side
  QSignalMapper* QtConnections;
};


pqSMAdaptor::pqSMAdaptor()
{
  this->Internal = new pqSMAdaptorInternal;
  QObject::connect(this->Internal->QtConnections, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(QtLinkedPropertyChanged(QWidget*)));
}

pqSMAdaptor::~pqSMAdaptor()
{
  delete this->Internal;
}

void pqSMAdaptor::SetProperty(vtkSMProperty* Property, QVariant QtProperty)
{
  QList<QVariant> props;
  if(QtProperty.type() == QVariant::List)
    props = QtProperty.toList();
  else
    props.push_back(QtProperty);
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  assert(VectorProperty != NULL);
  assert(props.size() <= VectorProperty->GetNumberOfElements());
  for(int i=0; i<props.size(); i++)
    {
    this->SetProperty(Property, i, props[i]);
    }
}

QVariant pqSMAdaptor::GetProperty(vtkSMProperty* Property)
{
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  assert(VectorProperty != NULL);
  int numElems = VectorProperty->GetNumberOfElements();
  if(numElems == 1)
    {
    return this->GetProperty(Property, 0);
    }
  QList<QVariant> props;
  for(int i=0; i<numElems; i++)
    {
    props.push_back(this->GetProperty(Property, i));
    }
  return props;
}

void pqSMAdaptor::SetProperty(vtkSMProperty* Property, int Index, QVariant QtProperty)
{
  vtkSMPropertyAdaptor* adapter = vtkSMPropertyAdaptor::New();
  adapter->SetProperty(Property);
  if(QtProperty.type() == QVariant::Bool) // bools expand to "true" or "false" instead of "1" or "0"
    QtProperty = QtProperty.toInt();
  adapter->SetRangeValue(Index, QtProperty.toString().toAscii().data());
  adapter->Delete();
}

QVariant pqSMAdaptor::GetProperty(vtkSMProperty* Property, int Index)
{
  vtkSMPropertyAdaptor* adapter = vtkSMPropertyAdaptor::New();
  adapter->SetProperty(Property);
  QVariant var = adapter->GetRangeValue(Index);
  adapter->Delete();
  return var;
}


void pqSMAdaptor::LinkPropertyTo(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                                        QObject* qObject, const char* qProperty)
{
  if(!Property || !qObject)
    return;

  // set the property on the QObject, so they start in-sync
  qObject->setProperty(qProperty, this->GetProperty(Property, Index));

  pqSMAdaptorInternal::LinkMap::iterator iter = this->Internal->SMLinks.find(SMGroup(Proxy, Property, Index));
  bool found = iter != this->Internal->SMLinks.end();
    
  iter = this->Internal->SMLinks.insert(iter, 
                               pqSMAdaptorInternal::LinkMap::value_type(
                                 SMGroup(Proxy, Property, Index), QtGroup(qObject, qProperty)));


  if(!found)
    {
    // connect SM property changed to QObject set property
    this->Internal->VTKConnections->Connect(Property, vtkCommand::ModifiedEvent,
                                            this, SLOT(SMLinkedPropertyChanged(vtkObject*, unsigned long, void*)),
                                            (void*)&(iter->first));
    }
}

void pqSMAdaptor::UnlinkPropertyFrom(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                                            QObject* qObject, const char* qProperty)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  
  Iters iters = this->Internal->SMLinks.equal_range(SMGroup(Proxy, Property, Index));

  bool all = true;

  pqSMAdaptorInternal::LinkMap::iterator qiter;
  for(qiter = iters.first; qiter != iters.second; )
    {
    if((qObject == NULL || qiter->second.first == qObject) &&
       (qProperty == NULL || qiter->second.second == qProperty))
      {
      this->Internal->SMLinks.erase(qiter++);
      }
    else
      {
      ++qiter;
      all = false;
      }
    }
  
  if(all)
    {
    this->Internal->VTKConnections->Disconnect(Property, vtkCommand::ModifiedEvent, this);
    }
}

void pqSMAdaptor::LinkPropertyTo(QObject* qObject, const char* qProperty, const char* signal,
                                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject)
    return;

  qObject->setProperty(qProperty, this->GetProperty(Property, Index));

  pqSMAdaptorInternal::LinkMap::iterator iter = this->Internal->QtLinks.insert(this->Internal->QtLinks.end(), 
                               pqSMAdaptorInternal::LinkMap::value_type(
                                 SMGroup(Proxy, Property, Index), QtGroup(qObject, qProperty)));

  QObject::connect(qObject, signal, this->Internal->QtConnections, SLOT(map()));
  this->Internal->QtConnections->setMapping(qObject, reinterpret_cast<QWidget*>(&*iter));

}

void pqSMAdaptor::UnlinkPropertyFrom(QObject* qObject, const char* qProperty, const char* signal,
                                            vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  
  Iters iters = this->Internal->QtLinks.equal_range(SMGroup(Proxy, Property, Index));

  bool all = true;

  pqSMAdaptorInternal::LinkMap::iterator qiter;
  for(qiter = iters.first; qiter != iters.second; )
    {
    if((qObject == NULL || qiter->second.first == qObject) &&
       (qProperty == NULL || qiter->second.second == qProperty))
      {
      QObject::disconnect(qiter->second.first, signal, this->Internal->QtConnections, SLOT(map()));
      this->Internal->QtConnections->removeMappings(qiter->second.first);
      this->Internal->QtLinks.erase(qiter++);
      }
    else
      {
      ++qiter;
      all = false;
      }
    }
}

void pqSMAdaptor::SMLinkedPropertyChanged(vtkObject*, unsigned long, void* data)
{

  // TODO : handle domains
  //        there is no vtkSMPropertyAdaptor like thing to handle domains

  SMGroup* d = static_cast<SMGroup*>(data);
  
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  pqSMAdaptorInternal::LinkMap::iterator iter;
  
  // is there a way to not do a lookup?
  Iters iters = this->Internal->SMLinks.equal_range(*d);
  
  QVariant var = this->GetProperty(d->Property, d->Index);
  
  for(iter = iters.first; iter != iters.second; ++iter)
    {
    QVariant old = iter->second.first->property(iter->second.second.data());
    if(old != var)
      iter->second.first->setProperty(iter->second.second.data(), var);
    }
}

void pqSMAdaptor::QtLinkedPropertyChanged(QWidget* data)
{
  // map::value_type is masked as a QWidget
  pqSMAdaptorInternal::LinkMap::value_type* iter = 
    reinterpret_cast<pqSMAdaptorInternal::LinkMap::value_type*>(data);
  
  QVariant prop = iter->second.first->property(iter->second.second.data());
  QVariant old = this->GetProperty(iter->first.Property, iter->first.Index);
  if(prop != old)
    {
    this->SetProperty(iter->first.Property, iter->first.Index, prop);
    iter->first.Proxy->UpdateVTKObjects();
    iter->first.Proxy->MarkConsumersAsModified();
    }
}


