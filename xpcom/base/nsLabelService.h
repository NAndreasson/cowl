#ifndef nsLabelService_h__
#define nsLabelService_h__

#pragma once

#include "nsXPCOM.h"
#include "nsTArray.h"
#include "nsILabelService.h"
#include "mozilla/dom/Label.h"

class nsLabelService : public nsILabelService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILABELSERVICE

  nsLabelService();

  nsresult Init();


protected:
  friend class mozilla::dom::Label;
  friend class mozilla::dom::Privilege;
  nsTArray<RefPtr<mozilla::dom::Label> > mLabelList;
  virtual ~nsLabelService();
};

#endif
