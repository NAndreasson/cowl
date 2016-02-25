#ifndef nsLabeledObjectService_h__
#define nsLabeledObjectService_h__

#pragma once

#include "nsXPCOM.h"
#include "nsTArray.h"
#include "nsILabeledObjectService.h"
#include "mozilla/dom/LabeledObject.h"

class nsLabeledObjectService : public nsILabeledObjectService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILABELEDOBJECTSERVICE

  nsLabeledObjectService();

  nsresult Init();


protected: friend class mozilla::dom::LabeledObject;
  nsTArray<RefPtr<mozilla::dom::LabeledObject> > mLabeledObjectList;
  virtual ~nsLabeledObjectService();
};

#endif