/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Privilege.h"
#include "mozilla/dom/PrivilegeBinding.h"
#include "mozilla/dom/COWLBinding.h"
#include "nsContentUtils.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIPrincipal.h"

#include "nsLabelService.h"
#include "StructuredClonetags.h"

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Privilege)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Privilege)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Privilege)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Privilege)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Privilege::Privilege()
{
}

Privilege::Privilege(mozilla::dom::Label &label)
  : mLabel(label)
{
}

Privilege::~Privilege()
{
}

JSObject*
Privilege::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PrivilegeBinding::Wrap(aCx, this, aGivenProto);
}

Privilege*
Privilege::GetParentObject() const
{
  return nullptr; //TODO: return something sensible here
}

already_AddRefed<Privilege>
Privilege::Constructor(const GlobalObject& global,
                       ErrorResult& aRv)
{
  RefPtr<Privilege> priv = new Privilege();
  if (aRv.Failed()) return nullptr;

  return priv.forget();
}

already_AddRefed<Privilege>
Privilege::FreshPrivilege(const GlobalObject& global, ErrorResult& aRv)
{
  nsString uniquePrincipal;
  GenerateUniquePrincipal(uniquePrincipal, aRv);
  if (aRv.Failed()) return nullptr;

  RefPtr<Label> label = Label::Constructor(global, uniquePrincipal, aRv);
  if (aRv.Failed()) return nullptr;

  RefPtr<Privilege> priv = new Privilege(*label);
  if (!priv) return nullptr;

  return priv.forget();
}

void
Privilege::GenerateUniquePrincipal(nsAString& principal, ErrorResult& aRv)
{
  nsresult rv;
  // create a fresh principal
  nsCOMPtr<nsIPrincipal> prin =
    do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
  // baseDomain should become {UUID}, might be better to bypass nullprincipal and use UUID service directly
  nsAutoCString baseDomain;
  prin->GetBaseDomain(baseDomain);

  // For some reason the UUID is wrapped in {UUID}, remove braces
  // TODO, oneliner with substring
  baseDomain.Cut(0, 1); baseDomain.Cut(baseDomain.Length() - 1, 1);

  nsAutoCString uuidPrincipal = NS_LITERAL_CSTRING("unique:") +  baseDomain;
  // assign to principal...
  principal.Assign(NS_ConvertUTF8toUTF16(uuidPrincipal));
}

bool
Privilege::Equals(mozilla::dom::Privilege& other)
{
  return mLabel.Equals(*(other.DirectLabel()));
}

bool
Privilege::Subsumes(mozilla::dom::Privilege& other)
{
  return mLabel.Subsumes(*(other.DirectLabel()));
}

already_AddRefed<Privilege>
Privilege::Combine(mozilla::dom::Privilege& other)
{
  _And(other);
  RefPtr<Privilege> _this = this;
  return _this.forget();
}

already_AddRefed<Privilege>
Privilege::Delegate(JSContext* cx, Label& label, ErrorResult& aRv)
{
  aRv.MightThrowJSException();

  if ( !mLabel.Subsumes(label) ) {
    JSErrorResult(cx, aRv, "SecurityError: Earlier privilege does not subsume label");
    return nullptr;
  }

  RefPtr<Privilege> priv = new Privilege(label);
  if (!priv) return nullptr;

  return priv.forget();
}

void
Privilege::_And(mozilla::dom::Privilege& other)
{
  // And clones the label
  ErrorResult aRv;
  mLabel._And(*(other.DirectLabel()), aRv);
}

bool
Privilege::IsEmpty() const
{
  return mLabel.IsEmpty();
}

already_AddRefed<Label>
Privilege::AsLabel(ErrorResult& aRv)
{
  return mLabel.Clone(aRv);
}

already_AddRefed<Privilege>
Privilege::Clone(ErrorResult& aRv)
{
  RefPtr<Privilege> priv = new Privilege(mLabel);
  return priv.forget();
}

void
Privilege::Stringify(nsString& retval)
{
  retval = NS_LITERAL_STRING("Privilege(");
  nsAutoString str;
  mLabel.Stringify(str);
  retval.Append(str);
  retval.Append(NS_LITERAL_STRING(")"));
}

// TODO should use the one in COWL::JS... instead
void
Privilege::JSErrorResult(JSContext *cx, ErrorResult& aRv, const char *msg)
{
  JSString *err = JS_NewStringCopyZ(cx,msg);
  if (err) {
    JS::RootedValue errv(cx, JS::StringValue(err));
    aRv.ThrowJSException(cx,errv);
  } else {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
  }
}

bool
Privilege::WriteStructuredClone(JSContext* cx,
                                  JSStructuredCloneWriter* writer)
{
  nsresult rv;
  nsCOMPtr<nsILabelService> ilbs(do_GetService(LABELSERVICE_CID, &rv));
  if (NS_FAILED(rv)) {
    return false;
  }
  nsLabelService* ls = static_cast<nsLabelService*>(ilbs.get());
  if (!ls) {
    return false;
  }
  if (JS_WriteUint32Pair(writer, SCTAG_DOM_PRIVILEGE,
                                 ls->mLabelList.Length())) {
    ls->mLabelList.AppendElement(&mLabel);
    return true;
  }
  return false;
}

JSObject*
Privilege::ReadStructuredClone(JSContext* cx,
                                 JSStructuredCloneReader* reader, uint32_t idx)
{
  nsresult rv;
  nsCOMPtr<nsILabelService> ilbs(do_GetService(LABELSERVICE_CID, &rv));
  if (NS_FAILED(rv)) {
    return nullptr;
  }
  nsLabelService* ls = static_cast<nsLabelService*>(ilbs.get());
  if (!ls) {
    return nullptr;
  }

  if(idx >= ls->mLabelList.Length()) {
    return nullptr;
  }
  RefPtr<Label> label = ls->mLabelList[idx];


  // TODO: Removing the element will cause some problems when firing postMessage in rapid succession, as list is shifted and then leading to out of index etc. Not removing could cause a memory leak, will look into this
  /* ls->mLabelList.RemoveElementAt(idx); */

  ErrorResult aRv;
  label = label->Clone(aRv);
  if (aRv.Failed()) return nullptr;

  // Need to be a delegated or fresh privilege.
  DisjunctionSetArray* dsets = label->GetDirectRoles();
  for (unsigned i = 0; i < dsets->Length(); ++i) {
    DisjunctionSet& role = dsets->ElementAt(i);
    // if only size one and contains an origin principal return null
    if (role.Length() == 1 && role[0].IsOriginPrincipal()) {
      return nullptr;
    }
  }

  RefPtr<Privilege> privilege = new Privilege(*label);
  return privilege->WrapObject(cx, nullptr); // TODO, acceptable with nullptr here?
}

} // namespace dom
} // namespace mozilla
