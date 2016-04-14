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

LabeledObject::LabeledObject(JS::Handle<JS::Value> objVal, Label& confidentiality, Label& integrity)
  : mConfidentiality(&confidentiality)
  , mIntegrity(&integrity)
  , mObj(objVal)
{

}

already_AddRefed<LabeledObject>
LabeledObject::Constructor(const GlobalObject& global,
                         JSContext* cx,
                         JS::Handle<JSObject*> obj,
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

  JS::RootedValue objVal(cx, JS::ObjectValue(*obj));
  JS::RootedValue objectClone(cx);
  // TODO could possibly fail, handle that
  JS_StructuredClone(cx, objVal, &objectClone, nullptr, nullptr);

  // see label and so on which should be contained in CILABELS
  // perform a write check
  RefPtr<LabeledObject> labeledObject = new LabeledObject(objectClone, *confidentiality, *integrity);

  // RefPtr<Label> privacy = COWL::GetConfidentialityLabel(global, cx, aRv);
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
LabeledObject::Clone(JSContext* cx, const CILabel& labels, ErrorResult &aRv) const
{
  aRv.MightThrowJSException();
  JSCompartment *compartment = js::GetContextCompartment(cx);
  MOZ_ASSERT(compartment);

  if (MOZ_UNLIKELY(!xpc::cowl::IsCompartmentConfined(compartment)))
    xpc::cowl::EnableCompartmentConfinement(compartment);

  RefPtr<Label> privs = xpc::cowl::GetCompartmentPrivileges(compartment);

  RefPtr<Label> newConf;
  if (labels.mConfidentiality.WasPassed()) {
    newConf = labels.mConfidentiality.Value();
  } else {
    newConf = mConfidentiality;
  }

  RefPtr<Label> newInt;
  if (labels.mIntegrity.WasPassed()) {
    newInt = labels.mIntegrity.Value();
  } else {
    newInt = mIntegrity;
  }

  if (!newConf->Subsumes(*privs, *mConfidentiality)) {
    // throw security errorr...
    COWL::JSErrorResult(cx, aRv,
      "SecurityError: Confidentiality label needs to be more restrictive");
    return nullptr;
  }

  if (!mIntegrity->Subsumes(*privs, *newInt)) {
    COWL::JSErrorResult(cx, aRv,
      "SecurityError: Check integrity label");
    return nullptr;
  }

  // Ned to create a rooted value which is needed for JS::Handle used as function param?
  // JS::RootedValue objVal(cx, mObj);
  JS::RootedValue objVal(cx, mObj);
  JS::RootedValue objectClone(cx);
  // TODO could possibly fail, handle that
  JS_StructuredClone(cx, objVal, &objectClone, nullptr, nullptr);


  // TODO, make sure that mObj is actually copied?
  RefPtr<LabeledObject> labeledObject = new LabeledObject(objectClone, *newConf, *newInt);

  return labeledObject.forget();
}

void
LabeledObject::GetProtectedObject(JSContext* cx, JS::MutableHandle<JSObject*> retval, ErrorResult& aRv) const
{
  aRv.MightThrowJSException();
  JSCompartment *compartment = js::GetContextCompartment(cx);
  MOZ_ASSERT(compartment);

  if (MOZ_UNLIKELY(!xpc::cowl::IsCompartmentConfined(compartment)))
    xpc::cowl::EnableCompartmentConfinement(compartment);

  RefPtr<Label> privs = xpc::cowl::GetCompartmentPrivileges(compartment);

  // the current confidentiality label in context which called protectedObject
  RefPtr<Label> compConfidentiality = xpc::cowl::GetCompartmentConfidentialityLabel(compartment);
  RefPtr<Label> tmpConfLabel = compConfidentiality->And(*mConfidentiality, aRv);
  if (aRv.Failed()) return;

  RefPtr<Label> newConfLabel = tmpConfLabel->Downgrade(*privs);

  if (xpc::cowl::LabelRaiseWillResultInStuckContext(compartment, *newConfLabel, privs)) {
    COWL::JSErrorResult(cx, aRv,
      "SecurityError: Will result in stuck-context, please use an iFrame");
    return;
  }

  // set to new conf label
  xpc::cowl::SetCompartmentConfidentialityLabel(compartment, newConfLabel);

  RefPtr<Label> compIntegrity = xpc::cowl::GetCompartmentIntegrityLabel(compartment);
  RefPtr<Label> tmpIntLabel = compIntegrity->Or(*mIntegrity, aRv);
  RefPtr<Label> newIntLabel = tmpIntLabel->Downgrade(*privs);

  xpc::cowl::SetCompartmentIntegrityLabel(compartment, newIntLabel);

  retval.set(&mObj.toObject());
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
  RefPtr<Label> confidentiality   = labeledObject->Confidentiality();
  RefPtr<Label> integrity     = labeledObject->Integrity();

  confidentiality = confidentiality->Clone(aRv);
  if (aRv.Failed()) return nullptr;
  integrity = integrity->Clone(aRv);
  if (aRv.Failed()) return nullptr;

  // Ned to create a rooted value which is needed for JS::Handle used as function param?
  JS::RootedValue objVal(cx, labeledObject->GetObj());
  JS::RootedValue objectClone(cx);
  // TODO could possibly fail, handle that,
  JS_StructuredClone(cx, objVal, &objectClone, nullptr, nullptr);
  // could try do call constructor and do everything there instead, construct global?
  RefPtr<LabeledObject> b  =
    new LabeledObject(objectClone, *(confidentiality.get()), *(integrity.get()));

  return b->WrapObject(cx, nullptr); // TODO, acceptable with nullptr here?
}

} // namespace dom
} // namespace mozilla
