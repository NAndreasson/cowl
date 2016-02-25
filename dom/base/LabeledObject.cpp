 // -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/LabeledObject.h"
#include "mozilla/dom/LabeledObjectBinding.h"
#include "nsLabeledObjectService.h"
#include "mozilla/dom/COWL.h"
#include "StructuredCloneTags.h"
#include "xpcprivate.h"
#include "nsContentUtils.h"


namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(LabeledObject)
NS_IMPL_CYCLE_COLLECTING_ADDREF(LabeledObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE(LabeledObject)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(LabeledObject)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

LabeledObject::LabeledObject(const nsAString& blob, Label& confidentiality, Label& integrity)
  : mConfidentiality(&confidentiality)
  , mIntegrity(&integrity)
  , mBlob(blob)
{

}

already_AddRefed<LabeledObject>
LabeledObject::Constructor(const GlobalObject& global,
                         JSContext* cx,
                         const nsAString& blob,
                         const CILabel& labels,
                         ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  COWL::Enable(global, cx);

  RefPtr<Label> confidentiality;

  // TODO make prettier...
  if (labels.mConfidentiality.WasPassed()) {
    confidentiality = labels.mConfidentiality.Value();
  } else {
    confidentiality = COWL::GetConfidentiality(global, cx, aRv);
    if (aRv.Failed())
      return nullptr;
    if (!confidentiality) {
      COWL::JSErrorResult(cx, aRv, "Failed to get current confidentiality label.");
      return nullptr;
    }
  }

  RefPtr<Label> integrity;

  if (labels.mIntegrity.WasPassed()) {
    integrity = labels.mIntegrity.Value();
  } else {
    integrity = COWL::GetIntegrity(global, cx, aRv);
    if (aRv.Failed())
      return nullptr;
    if (!integrity) {
      COWL::JSErrorResult(cx, aRv, "Failed to get current integrity label.");
      return nullptr;
    }
  }

  JSCompartment *compartment = js::GetContextCompartment(cx);
  MOZ_ASSERT(compartment);

  // if (MOZ_UNLIKELY(!xpc::cowl::IsCompartmentConfined(compartment)))
  //   xpc::cowl::EnableCompartmentConfinement(compartment);

  RefPtr<Label> privs = xpc::cowl::GetCompartmentPrivileges(compartment);

  // current compartment label must flow to label of blob
  if (!xpc::cowl::GuardWrite(compartment, *confidentiality, *integrity, privs)) {
    COWL::JSErrorResult(cx, aRv,
        "Label of blob is not above current label or below current clearance.");
    return nullptr;
  }

  // see label and so on which should be contained in CILABELS
  // perform a write check
  RefPtr<LabeledObject> labeledObject = new LabeledObject(blob, *confidentiality, *integrity);

  // RefPtr<Label> privacy = COWL::GetPrivacyLabel(global, cx, aRv);
  // return Constructor(global, cx, blob, *privacy, *trust, aRv);
  return labeledObject.forget();
}

already_AddRefed<Label>
LabeledObject::Confidentiality() const
{
  // should these be clones? Labels should be immutable so it shouln't matter?
  RefPtr<Label> confidentiality = mConfidentiality;
  return confidentiality.forget();
}

already_AddRefed<Label>
LabeledObject::Integrity() const
{
  // should these be clones? Labels should be immutable so it shouln't matter?
  RefPtr<Label> integrity = mIntegrity;
  return integrity.forget();
}

already_AddRefed<LabeledObject>
LabeledObject::Clone(const CILabel& labels, ErrorResult &aRv) const
{
  RefPtr<LabeledObject> labeledObject = new LabeledObject(mBlob, *mConfidentiality, *mIntegrity);

  return labeledObject.forget();
}

void
LabeledObject::GetProtectedObject(JSContext* cx, nsString& aRetVal, ErrorResult& aRv) const
{
  aRv.MightThrowJSException();
  JSCompartment *compartment = js::GetContextCompartment(cx);
  MOZ_ASSERT(compartment);

  if (MOZ_UNLIKELY(!xpc::cowl::IsCompartmentConfined(compartment)))
    xpc::cowl::EnableCompartmentConfinement(compartment);

  RefPtr<Label> privs = xpc::cowl::GetCompartmentPrivileges(compartment);

  // get compartment confidentiality label
  // do a mConfidentiality.and(compConf).Downgrade(privs)
  // set to comparment label to corresponding...
  // get compartment integrity label
  //

  // current compartment label must flow to label of target
  // raise it if need be
  // TODO: raise label if the following check does not hold
  if (!xpc::cowl::GuardRead(compartment, *mConfidentiality,*mIntegrity, privs, cx)) {
    // raise ...
    // get current conf and integrity...
    //
    COWL::JSErrorResult(cx, aRv, "Cannot inspect object...");
    return;
  }

  aRetVal = mBlob;
}

bool
LabeledObject::WriteStructuredClone(JSContext* cx,
                                  JSStructuredCloneWriter* writer)
{
  nsresult rv;
  nsCOMPtr<nsILabeledObjectService> ilbs(do_GetService(LABELEDOBJECTSERVICE_CID, &rv));
  if (NS_FAILED(rv)) {
    return false;
  }
  nsLabeledObjectService* lbs = static_cast<nsLabeledObjectService*>(ilbs.get());
  if (!lbs) {
    return false;
  }
  if (JS_WriteUint32Pair(writer, SCTAG_DOM_LABELEDOBJECT,
                                 lbs->mLabeledObjectList.Length())) {
    lbs->mLabeledObjectList.AppendElement(this);
    return true;
  }
  return false;
}

JSObject*
LabeledObject::ReadStructuredClone(JSContext* cx,
                                 JSStructuredCloneReader* reader, uint32_t idx)
{
  nsresult rv;
  nsCOMPtr<nsILabeledObjectService> ilbs(do_GetService(LABELEDOBJECTSERVICE_CID, &rv));
  if (NS_FAILED(rv)) {
    return nullptr;
  }
  nsLabeledObjectService* lbs = static_cast<nsLabeledObjectService*>(ilbs.get());
  if (!lbs) {
    return nullptr;
  }
  if(idx >= lbs->mLabeledObjectList.Length()) {
    return nullptr;
  }
  RefPtr<LabeledObject> labeledObject = lbs->mLabeledObjectList[idx];
  lbs->mLabeledObjectList.RemoveElementAt(idx);

  ErrorResult aRv;
  //RefPtr<File> Object       = labeledObject->Object();
  RefPtr<Label> confidentiality   = labeledObject->Confidentiality();
  RefPtr<Label> integrity     = labeledObject->Integrity();

  confidentiality = confidentiality->Clone(aRv);
  if (aRv.Failed()) return nullptr;
  integrity = integrity->Clone(aRv);
  if (aRv.Failed()) return nullptr;

  RefPtr<LabeledObject> b  =
    new LabeledObject(labeledObject->GetBlob(), *(confidentiality.get()), *(integrity.get()));

  return b->WrapObject(cx, nullptr); // TODO, acceptable with nullptr here?
}

} // namespace dom
} // namespace mozilla