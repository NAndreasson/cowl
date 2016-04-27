/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/COWL.h"
#include "mozilla/dom/COWLBinding.h"
#include "mozilla/dom/LabelBinding.h"
#include "mozilla/dom/PrivilegeBinding.h"
#include "xpcprivate.h"

namespace mozilla {
namespace dom {

#define COWL_CONFIG(compartment) \
    static_cast<xpc::CompartmentPrivate*>(JS_GetCompartmentPrivate((compartment)))->cowlConfig


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(COWL)
NS_IMPL_CYCLE_COLLECTING_ADDREF(COWL)
NS_IMPL_CYCLE_COLLECTING_RELEASE(COWL)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(COWL)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

COWL::COWL()
{
}

COWL::~COWL()
{
}

JSObject*
COWL::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return COWLBinding::Wrap(aCx, this, aGivenProto);
}

COWL*
COWL::GetParentObject() const
{
  return nullptr; //TODO: return something sensible here
}

void
COWL::Enable(const GlobalObject& global, JSContext *cx)
{
  if (IsEnabled(global)) return;

  JSCompartment *compartment =
    js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);
  xpc::cowl::EnableCompartmentConfinement(compartment);
  // Start using COWL wrappers:
  js::RecomputeWrappers(cx, js::AllCompartments(), js::AllCompartments());
}

bool
COWL::IsEnabled(const GlobalObject& global)
{
  JSCompartment *compartment =
    js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);
  return xpc::cowl::IsCompartmentConfined(compartment);
}

void
COWL::GetPrincipal(const GlobalObject& global, JSContext* cx, nsString& retval)
{
  retval = NS_LITERAL_STRING("");
  JSCompartment *compartment =
    js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);

  nsIPrincipal* prin = xpc::GetCompartmentPrincipal(compartment);
  if (!prin) return;

  nsAutoCString origin;

  if (NS_FAILED(prin->GetOrigin(origin)))
    return;
  AppendASCIItoUTF16(origin, retval);
}

// Helper macro for retriveing the privacy/trust label/clearance
#define GET_LABEL(name)                                                  \
  do {                                                                   \
    JSCompartment *compartment = js::GetObjectCompartment(global.Get()); \
    MOZ_ASSERT(compartment);                                             \
    RefPtr<Label> l = xpc::cowl::GetCompartment##name(compartment);    \
                                                                         \
    if (!l) return nullptr;                                              \
    ErrorResult aRv;                                                     \
    RefPtr<Label> clone = l->Clone(aRv);                               \
    if (aRv.Failed()) return nullptr;                                    \
    return clone.forget();                                               \
  } while(0);

already_AddRefed<Label>
COWL::GetConfidentiality(const GlobalObject& global, JSContext* cx,
                      ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  if (!IsEnabled(global)) {
    JSErrorResult(cx, aRv, "COWL is not enabled.");
    return nullptr;
  }
  GET_LABEL(ConfidentialityLabel);
}

void
COWL::SetConfidentiality(const GlobalObject& global, JSContext* cx,
                    Label& aLabel, ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  Enable(global, cx);

  JSCompartment *compartment = js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);

  RefPtr<Label> privs = xpc::cowl::GetCompartmentPrivileges(compartment);

  RefPtr<Label> currentLabel = GetConfidentiality(global, cx, aRv);
  if (aRv.Failed()) return;
  if (!currentLabel) {
    JSErrorResult(cx, aRv, "Failed to get current confidentiality label.");
    return;
  }

  if (!aLabel.Subsumes(*privs, *currentLabel)) {
    JSErrorResult(cx, aRv, "Label is not above the current label.");
    return;
  }

  if (xpc::cowl::LabelRaiseWillResultInStuckContext(compartment, aLabel, privs)) {
    JSErrorResult(cx, aRv, "Sorry cant do that, create a frame");
    return;
  }

  xpc::cowl::SetCompartmentConfidentialityLabel(compartment, &aLabel);
  // This affects cross-compartment communication. Adjust wrappers:
  js::RecomputeWrappers(cx, js::AllCompartments(), js::AllCompartments());

  xpc::cowl::RefineCompartmentFlags(compartment);
}

already_AddRefed<Label>
COWL::GetIntegrity(const GlobalObject& global, JSContext* cx,
                    ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  if (!IsEnabled(global)) {
    JSErrorResult(cx, aRv, "COWL is not enabled.");
    return nullptr;
  }
  GET_LABEL(IntegrityLabel);
}

void
COWL::SetIntegrity(const GlobalObject& global, JSContext* cx,
                    Label& aLabel, ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  Enable(global, cx);

  JSCompartment *compartment = js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);

  RefPtr<Label> privs = xpc::cowl::GetCompartmentPrivileges(compartment);

  RefPtr<Label> currentLabel = GetIntegrity(global, cx, aRv);
  if (aRv.Failed()) return;
  if (!currentLabel) {
    JSErrorResult(cx, aRv, "Failed to get current trust label.");
    return;
  }

  if (!currentLabel->Subsumes(*privs, aLabel)) {
    JSErrorResult(cx, aRv, "Label is not below the current label.");
    return;
  }

  xpc::cowl::SetCompartmentIntegrityLabel(compartment, &aLabel);
  // Changing the trust/integrity label affects cross-compartment
  // communication. Adjust wrappers:
  js::RecomputeWrappers(cx, js::AllCompartments(), js::AllCompartments());

  xpc::cowl::RefineCompartmentFlags(compartment);
}

already_AddRefed<Privilege>
COWL::GetPrivilege(const GlobalObject& global, JSContext* cx, ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  if (!IsEnabled(global)) {
    JSErrorResult(cx, aRv, "COWL is not enabled.");
    return nullptr;
  }

  JSCompartment *compartment =
    js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);

  // copy compartment privileges
  RefPtr<Label> privL =
    xpc::cowl::GetCompartmentPrivileges(compartment);

  RefPtr<Privilege> privs;

  if (!privL)
    return nullptr;

  privs = new Privilege(*privL);

  return privs.forget();
}

void
COWL::SetPrivilege(const GlobalObject& global, JSContext* cx,
                    Privilege* priv, ErrorResult& aRv)
{
  Enable(global, cx);
  JSCompartment *compartment =
    js::GetObjectCompartment(global.Get());
  MOZ_ASSERT(compartment);

  if (!priv) return;

  // get current conf label...
  RefPtr<Label> currentLabel = GetConfidentiality(global, cx, aRv);
  if (aRv.Failed()) return;
  if (!currentLabel) {
    JSErrorResult(cx, aRv, "Failed to get current confidentiality label.");
    return;
  }

  RefPtr<Label> newPrivs = priv->GetAsLabel(aRv);
  if (xpc::cowl::LabelRaiseWillResultInStuckContext(compartment, *currentLabel, newPrivs)) {
    JSErrorResult(cx, aRv, "Sorry cant do that, create a frame");
    return;
  }


  if (aRv.Failed()) return;
  COWL_CONFIG(compartment).SetPrivileges(newPrivs);
}

// Helper for setting the ErrorResult to a string.  This function
// should only be called after MightThrowJSException() is called.
void
COWL::JSErrorResult(JSContext *cx, ErrorResult& aRv, const char *msg)
{
  JSString *err = JS_NewStringCopyZ(cx,msg);
  if (err) {
    JS::RootedValue errv(cx, JS::StringValue(err));
    aRv.ThrowJSException(cx,errv);
  } else {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
  }
}

#undef COWL_CONFIG
} // namespace dom
} // namespace mozilla
