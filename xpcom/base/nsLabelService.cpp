#include "nsLabelService.h"
#include "nsIClassInfoImpl.h"

NS_IMPL_CLASSINFO(nsLabelService, nullptr, 0, LABELSERVICE_CID)
NS_IMPL_ISUPPORTS_CI(nsLabelService, nsILabelService)

nsLabelService::nsLabelService() { }

nsLabelService::~nsLabelService() { }

nsresult nsLabelService::Init() { return NS_OK; }
