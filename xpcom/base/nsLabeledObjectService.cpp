#include "nsLabeledObjectService.h"
#include "nsIClassInfoImpl.h"

NS_IMPL_CLASSINFO(nsLabeledObjectService, nullptr, 0, LABELEDOBJECTSERVICE_CID)
NS_IMPL_ISUPPORTS_CI(nsLabeledObjectService, nsILabeledObjectService)

nsLabeledObjectService::nsLabeledObjectService() { }

nsLabeledObjectService::~nsLabeledObjectService() { }

nsresult nsLabeledObjectService::Init() { return NS_OK; }