/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/Assertions.h"
#include "xpcprivate.h"
#include "xpcpublic.h"
#include "jsfriendapi.h"
#include "mozilla/dom/COWL.h"
#include "mozilla/dom/Label.h"
#include "nsIContentSecurityPolicy.h"
#include "nsDocument.h"
#include "nsSandboxFlags.h"
#include "nsNetUtil.h"

using namespace xpc;
using namespace JS;
using namespace mozilla;
using namespace mozilla::dom;

namespace xpc {
namespace cowl {

#define COWL_CONFIG(compartment) \
    static_cast<CompartmentPrivate*>(JS_GetCompartmentPrivate((compartment)))->cowlConfig


static inline uint32_t cowlInitialSandboxFlags() {
  return SANDBOXED_DOMAIN | SANDBOXED_PLUGINS;
}

static inline uint32_t cowlConfinementSandboxFlags() {
  return SANDBOXED_ORIGIN | SANDBOXED_NAVIGATION | SANDBOXED_AUXILIARY_NAVIGATION;
}

NS_EXPORT_(void)
EnableCompartmentConfinement(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);

  if (IsCompartmentConfined(compartment))
    return;

  RefPtr<Label> confidentiality = new Label();
  MOZ_ASSERT(confidentiality);

  RefPtr<Label> integrity = new Label();
  MOZ_ASSERT(integrity);

  COWL_CONFIG(compartment).SetConfidentialityLabel(confidentiality);
  COWL_CONFIG(compartment).SetIntegrityLabel(integrity);

  // set privileges to compartment principal
  // we're not "copying" the principal since the principal may be a
  // null principal (iframe sandbox) and thus not a codebase principal
  nsCOMPtr<nsIPrincipal> privPrin = GetCompartmentPrincipal(compartment);

  ErrorResult aRv;
  RefPtr<Label> privileges  = new Label(privPrin, aRv);
  MOZ_ASSERT(privileges);

  COWL_CONFIG(compartment).SetPrivileges(privileges);

   // Get the compartment global
  nsCOMPtr<nsIGlobalObject> global =
    NativeGlobal(JS_GetGlobalForCompartmentOrNull(compartment));
  MOZ_ASSERT(global);

  // Get the underlying window, if it exists
  nsCOMPtr<nsPIDOMWindowInner> win(do_QueryInterface(global));

  nsCOMPtr<nsIDocument> doc;

  if (win) {
    // Get the window document
    doc = win->GetDoc();
  }

  // set initial flags...
  if (doc) {
#if COWL_DEBUG
    printf("Setting initial flags\n");
#endif
    doc->SetSandboxFlags(cowlInitialSandboxFlags());
  }

}


#define DEFINE_SET_LABEL(name)                                      \
  NS_EXPORT_(void)                                                  \
  SetCompartment##name(JSCompartment *compartment,                  \
                      mozilla::dom::Label *aLabel)                  \
  {                                                                 \
    MOZ_ASSERT(compartment);                                        \
    MOZ_ASSERT(aLabel);                                             \
                                                                    \
    NS_ASSERTION(IsCompartmentConfined(compartment),                \
                 "Must call EnableCompartmentConfinement() first"); \
    if (!IsCompartmentConfined(compartment))                        \
      return;                                                       \
                                                                    \
    ErrorResult aRv;                                                \
    RefPtr<Label> label = (aLabel)->Clone(aRv);                   \
                                                                    \
    MOZ_ASSERT(!(aRv).Failed());                                    \
    COWL_CONFIG(compartment).Set##name(label);                      \
  }

#define DEFINE_GET_LABEL(name)                                      \
  NS_EXPORT_(already_AddRefed<mozilla::dom::Label>)                 \
  GetCompartment##name(JSCompartment *compartment)                  \
  {                                                                 \
    MOZ_ASSERT(compartment);                                        \
    MOZ_ASSERT(cowl::IsCompartmentConfined(compartment));           \
    return COWL_CONFIG(compartment).Get##name();                    \
  }

// This function sets the compartment confidentiality label. It clones the given label.
// IMPORTANT: This function should not be exported to untrusted code.
// Untrusted code can only set the confidentiality label to a label that
// subsumes the "current label".
DEFINE_SET_LABEL(ConfidentialityLabel)
DEFINE_GET_LABEL(ConfidentialityLabel)

// This function sets the compartment integrity label. It clones the given label.
// IMPORTANT: This function should not be exported to untrusted code.
// Untrusted code can only set the integrity label to a label subsumed by
// the "current label".
DEFINE_SET_LABEL(IntegrityLabel)
DEFINE_GET_LABEL(IntegrityLabel)

DEFINE_SET_LABEL(Privileges)

#undef DEFINE_SET_LABEL
#undef DEFINE_GET_LABEL

// This function gets a copy of the compartment privileges.
NS_EXPORT_(already_AddRefed<mozilla::dom::Label>)
GetCompartmentPrivileges(JSCompartment*compartment)
{
  ErrorResult aRv;

  RefPtr<Label> privs;

  if (cowl::IsCompartmentConfined(compartment)) {
    privs = COWL_CONFIG(compartment).GetPrivileges();
    if (!privs)
      return nullptr;
    privs = privs->Clone(aRv);
  }

  if (!privs || aRv.Failed())
    return nullptr;

  return privs.forget();
}

NS_EXPORT_(bool)
IsCompartmentConfined(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);
  return COWL_CONFIG(compartment).isEnabled();
}

already_AddRefed<mozilla::dom::Label>
EffectiveConfidentialityLabel(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);

  RefPtr<Label> effectiveConf;

  if (IsCompartmentConfined(compartment)) {
    // get current confidentality label...
    RefPtr<Label> conf = GetCompartmentConfidentialityLabel(compartment);
    RefPtr<Label> privs = GetCompartmentPrivileges(compartment);

    effectiveConf = conf->Downgrade(*privs);
  } else {
    effectiveConf = new Label();
  }

  return effectiveConf.forget();
}

already_AddRefed<mozilla::dom::Label>
EffectiveIntegrityLabel(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);

  RefPtr<Label> effectiveIntegrity;

  if (IsCompartmentConfined(compartment)) {
    // get current confidentality label...
    RefPtr<Label> integrity = GetCompartmentIntegrityLabel(compartment);
    RefPtr<Label> privs = GetCompartmentPrivileges(compartment);

    effectiveIntegrity = integrity->Upgrade(*privs);
  } else {
    // Should be compartment principal
    nsCOMPtr<nsIPrincipal> privPrin = GetCompartmentPrincipal(compartment);
    ErrorResult aRv;
    effectiveIntegrity = new Label(privPrin, aRv);
    MOZ_ASSERT(effectiveIntegrity);
  }

  return effectiveIntegrity.forget();
}

already_AddRefed<mozilla::dom::Label>
UpgradedConfidentialityLabel(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);

  RefPtr<Label> upgradedConf;

  if (IsCompartmentConfined(compartment)) {
    // get current confidentality label...
    RefPtr<Label> conf = GetCompartmentConfidentialityLabel(compartment);
    RefPtr<Label> privs = GetCompartmentPrivileges(compartment);
    upgradedConf = conf->Upgrade(*privs);
  } else {
    // Should be compartment principal
    nsCOMPtr<nsIPrincipal> privPrin = GetCompartmentPrincipal(compartment);
    ErrorResult aRv;
    upgradedConf = new Label(privPrin, aRv);
    MOZ_ASSERT(upgradedConf);
  }

  return upgradedConf.forget();
}


NS_EXPORT_(bool)
LabelRaiseWillResultInStuckContext(JSCompartment *compartment,
                                    Label& confidentiality,
                                    Label* privs)
{
  // Get the compartment global
  nsCOMPtr<nsIGlobalObject> global =
    NativeGlobal(JS_GetGlobalForCompartmentOrNull(compartment));
  MOZ_ASSERT(global);

  // Get the underlying window, if it exists
  nsCOMPtr<nsPIDOMWindowInner> win(do_QueryInterface(global));
  // TODO, assert here, what if there is no window?

  nsCOMPtr<nsPIDOMWindowOuter> outer = win->GetOuterWindow();
  nsCOMPtr<nsPIDOMWindowOuter> outerParent = outer->GetScriptableTop();
  bool isTopLevelBrowsingContext = outer.get() == outerParent.get();

  if (!isTopLevelBrowsingContext) return false;
  // calculate effective label
  RefPtr<Label> effectiveLabel = confidentiality.Downgrade(*privs);

  // if not empty then considered stuck
  return !effectiveLabel->IsEmpty();
}

// Check if information can flow from the compartment to an object labeled with
// |confidentiality| and |integrity| into the compartment.
NS_EXPORT_(bool)
GuardWrite(JSCompartment *compartment,
           Label &confidentiality, Label &integrity, Label *aPrivs)
{
  ErrorResult aRv;

  if (!IsCompartmentConfined(compartment)) {
    NS_WARNING("Not in confined compartment");
    return false;
  }

  RefPtr<Label> privs = aPrivs ? aPrivs : new Label();
  RefPtr<Label> compConfidentiality, compIntegrity;
  compConfidentiality = GetCompartmentConfidentialityLabel(compartment);
  compIntegrity   = GetCompartmentIntegrityLabel(compartment);

  // If any of the labels are missing, don't allow the information flow
  if (!compConfidentiality || !compIntegrity) {
    NS_WARNING("Missing labels");
    return false;
  }


#if COWL_DEBUG
  {
    nsAutoString compConfidentialityStr, compIntegrityStr, confidentialityStr, integrityStr, privsStr;
    compConfidentiality->Stringify(compConfidentialityStr);
    compIntegrity->Stringify(compIntegrityStr);
    confidentiality.Stringify(confidentialityStr);
    integrity.Stringify(integrityStr);
    privs->Stringify(privsStr);

    printf("GuardWrite <%s,%s> to <%s,%s> | %s\n",
           NS_ConvertUTF16toUTF8(compConfidentialityStr).get(),
           NS_ConvertUTF16toUTF8(compIntegrityStr).get(),
           NS_ConvertUTF16toUTF8(confidentialityStr).get(),
           NS_ConvertUTF16toUTF8(integrityStr).get(),
           NS_ConvertUTF16toUTF8(privsStr).get());
  }
#endif


  // if not <compConfidentiality,compIntegrity> [=_privs <confidentiality,integrity>
  if (!(confidentiality.Subsumes(*privs, *compConfidentiality) && compIntegrity->Subsumes(*privs, integrity))) {
    NS_WARNING("Label not above current label");
    return false;
  }

  return true;
}

// Check if compartment can write to dst
NS_EXPORT_(bool)
GuardWrite(JSCompartment *compartment, JSCompartment *dst)
{
#if COWL_DEBUG
    {
        printf("GuardWrite :");
        {
            nsAutoCString origin;
            uint32_t status = 0;
            GetCompartmentPrincipal(compartment)->GetOrigin(origin);
            GetCompartmentPrincipal(compartment)->GetAppId(&status);
            printf(" %s [%x] to", origin.get(), status);
            // nsMemory::Free(origin);
        }
        {
            nsAutoCString origin;
            uint32_t status = 0;
            GetCompartmentPrincipal(dst)->GetOrigin(origin);
            GetCompartmentPrincipal(dst)->GetAppId(&status);
            printf("%s [%x] \n", origin.get(), status);
            // nsMemory::Free(origin);
        }
    }
#endif


  if (!IsCompartmentConfined(dst)) {
    NS_WARNING("Destination compartmetn is not confined");
    return false;
  }
  RefPtr<Label> confidentiality = GetCompartmentConfidentialityLabel(dst);
  RefPtr<Label> integrity   = GetCompartmentIntegrityLabel(dst);
  RefPtr<Label> privs   = GetCompartmentPrivileges(compartment);

  if (!confidentiality || !integrity || !privs) {
    NS_WARNING("Missing confidentiality or integrity labels");
    return false;
  }

  return GuardWrite(compartment, *confidentiality, *integrity, privs);
}

// Check if information can flow from an object labeled with |confidentiality|
// and |integrity| into the compartment. For this to hold, the compartment
// must preserve confidentiality, i.e., the compartment confidentiality label must
// subsume the object confidentiality label, and not be corrupted, i.e., the
// object integrity label must be at least as trustworthy as the
// compartment integrity label.
NS_EXPORT_(bool)
GuardRead(JSCompartment *compartment,
          Label &confidentiality, Label &integrity, Label *aPrivs, JSContext *cx)
{
  ErrorResult aRv;

  RefPtr<Label> privs = aPrivs ? aPrivs : new Label();
  RefPtr<Label> compConfidentiality, compIntegrity;

  if (IsCompartmentConfined(compartment)) {
    compConfidentiality = GetCompartmentConfidentialityLabel(compartment);
    compIntegrity   = GetCompartmentIntegrityLabel(compartment);
  } else {
    // compartment is not confined
    nsCOMPtr<nsIPrincipal> privPrin = GetCompartmentPrincipal(compartment);
    ErrorResult aRv;

    compConfidentiality = new Label(privPrin, aRv);
    if (aRv.Failed()) {
      printf("Err res failed\n");
      aRv.SuppressException();
      return false;
    }

    compIntegrity   = new Label();
  }

  // If any of the labels are missing, don't allow the information flow
  if (!compConfidentiality || !compIntegrity) {
    NS_WARNING("Missing labels!");
    return false;
  }


#if COWL_DEBUG
  {
    nsAutoString compConfidentialityStr, compIntegrityStr, confidentialityStr, integrityStr, privsStr;
    compConfidentiality->Stringify(compConfidentialityStr);
    compIntegrity->Stringify(compIntegrityStr);
    confidentiality.Stringify(confidentialityStr);
    integrity.Stringify(integrityStr);
    privs->Stringify(privsStr);

    printf("GuardRead <%s,%s> to <%s,%s> | %s\n",
           NS_ConvertUTF16toUTF8(confidentialityStr).get(),
           NS_ConvertUTF16toUTF8(integrityStr).get(),
           NS_ConvertUTF16toUTF8(compConfidentialityStr).get(),
           NS_ConvertUTF16toUTF8(compIntegrityStr).get(),
           NS_ConvertUTF16toUTF8(privsStr).get());
  }
#endif

  // <confidentiality,integrity> [=_privs <compConfidentiality,compIntegrity>
  if (compConfidentiality->Subsumes(*privs, confidentiality) &&
      integrity.Subsumes(*privs, *compIntegrity)) {
    return true;
  }

  NS_WARNING("Does not subsume");
  return false;
}

NS_EXPORT_(bool)
GuardRead(JSCompartment *source, const nsACString& aUri)
{
  printf("In guard read\n");
  MOZ_ASSERT(source);

  if (!IsCompartmentConfined(source)) {
    return true;
  }

  printf("The compartment is confined\n");

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUri);

  // print text uri ...
  nsAutoCString tmpOrigin;
  rv = uri->GetAsciiSpec(tmpOrigin);
  printf("Something %s\n", ToNewCString(tmpOrigin));

  RefPtr<Label> compConfidentiality = GetCompartmentConfidentialityLabel(source);
  RefPtr<Label> compIntegrity = GetCompartmentIntegrityLabel(source);
  RefPtr<Label> privs   = GetCompartmentPrivileges(source);

  // TODO, should maybe change these to nsresult?
  ErrorResult errRes;
  RefPtr<Label> uriLabel  = new Label(NS_ConvertASCIItoUTF16(tmpOrigin), errRes);
  if (errRes.Failed()) {
    printf("Err res failed\n");
    errRes.SuppressException();
    return false;
  }

  RefPtr<Label> effLabel = compConfidentiality->Downgrade(*privs);

  if (!uriLabel->Subsumes(*effLabel)) {
    printf("DOES NOT SUBSUME\n");
    return false;
  }

  nsAutoString compConfidentialityStr, compIntegrityStr;
  compConfidentiality->Stringify(compConfidentialityStr);
  compIntegrity->Stringify(compIntegrityStr);

  printf("Other GuardRead <%s,%s> \n",
      NS_ConvertUTF16toUTF8(compConfidentialityStr).get(),
      NS_ConvertUTF16toUTF8(compIntegrityStr).get());

  return true;
}

NS_EXPORT_(bool)
CanFlowTo(JSCompartment *fromComp, JSCompartment *toComp)
{
  RefPtr<Label> fromEffectiveConf = EffectiveConfidentialityLabel(fromComp);
  RefPtr<Label> fromEffectiveInt = EffectiveIntegrityLabel(fromComp);

  nsAutoString fromConfStr, fromIntStr;
  fromEffectiveConf->Stringify(fromConfStr);
  fromEffectiveInt->Stringify(fromIntStr);

  printf("From compartment conf and int <%s,%s> \n",
      NS_ConvertUTF16toUTF8(fromConfStr).get(),
      NS_ConvertUTF16toUTF8(fromIntStr).get());

  RefPtr<Label> toConf = UpgradedConfidentialityLabel(toComp);
  /* RefPtr<Label> toEffectiveInt = EffectiveIntegrityLabel(toComp); */
  RefPtr<Label> toEffectiveInt;
  if (IsCompartmentConfined(toComp)) {
    toEffectiveInt = GetCompartmentIntegrityLabel(toComp);
  } else {
    toEffectiveInt = new Label();
  }

  nsAutoString toConfStr, toIntStr;
  toConf->Stringify(toConfStr);
  toEffectiveInt->Stringify(toIntStr);

  printf("To compartment conf and int <%s,%s> \n",
      NS_ConvertUTF16toUTF8(toConfStr).get(),
      NS_ConvertUTF16toUTF8(toIntStr).get());

  if (!toConf->Subsumes(*fromEffectiveConf) || !fromEffectiveInt->Subsumes(*toEffectiveInt)) return false;

  return true;
}
// Check if information can flow from compartment |source| to
// compartment |compartment|.
NS_EXPORT_(bool)
GuardRead(JSCompartment *compartment, JSCompartment *source, bool isGET)
{
  //isGET = true:  compartment is reading from source
  //               use compartment privs
  //isGET = false: source is writing to compartment
  //               use source privs
#if COWL_DEBUG
    {
        printf("GuardRead %s :", isGET ? "GET" : "SET");
        {
            nsAutoCString origin;
            uint32_t status = 0;
            GetCompartmentPrincipal(source)->GetOrigin(origin);
            GetCompartmentPrincipal(source)->GetAppId(&status);
            printf("%s [%x]", origin.get(), status);
            // nsMemory::Free(origin);
        }
        {
            nsAutoCString origin;
            uint32_t status = 0;
            GetCompartmentPrincipal(compartment)->GetOrigin(origin);
            GetCompartmentPrincipal(compartment)->GetAppId(&status);
            printf(" to %s [%x]\n", origin.get(), status);
            // nsMemory::Free(origin);
        }
    }
#endif



  RefPtr<Label> confidentiality, integrity;

  if (IsCompartmentConfined(source)) {
    confidentiality = GetCompartmentConfidentialityLabel(source);
    integrity   = GetCompartmentIntegrityLabel(source);
  } else {
    confidentiality = new Label();
    integrity   = new Label();
  }

  RefPtr<Label> privs = isGET ? GetCompartmentPrivileges(compartment)
                                : GetCompartmentPrivileges(source);


  if (!confidentiality || !integrity) {
    NS_WARNING("Missing confidentiality or integrity labels");
    return false;
  }

  return GuardRead(compartment, *confidentiality, *integrity, privs);
}

NS_EXPORT_(void)
RefineCompartmentFlags(JSCompartment *compartment)
{
    // Get the compartment global
  nsCOMPtr<nsIGlobalObject> global =
    NativeGlobal(JS_GetGlobalForCompartmentOrNull(compartment));
  MOZ_ASSERT(global);

  // Get the underlying window, if it exists
  nsCOMPtr<nsPIDOMWindowInner> win(do_QueryInterface(global));
  nsCOMPtr<nsIDocument> doc;
  if (win) {
    doc = win->GetDoc();
  }

  // clear flags, as integrity can be set down to empty label
  // TODO should potentially revert to flags set before enabling COWL
  if (doc) {
#if COWL_DEBUG
    printf("Clearing sandbox flags\n");
#endif
    doc->SetSandboxFlags(0);
  }

  RefPtr<Label> effectiveConf = EffectiveConfidentialityLabel(compartment);
  RefPtr<Label> integrity = GetCompartmentIntegrityLabel(compartment);

  if (effectiveConf->IsEmpty() && integrity->IsEmpty()) {
#if COWL_DEBUG
    printf("Refine: confidentiality label is empty or integrity not set, do nothing\n");
#endif
    return;
  }

  // get privilege..
  RefPtr<Label> privilege  = COWL_CONFIG(compartment).GetPrivileges();
  RefPtr<Label> emptyLabel = new Label();

  // check effective label
  /* if (emptyLabel->Subsumes(*privilege, *confidentiality)) { */
/* #if COWL_DEBUG */
  /*   printf("Refine: Effective confidentiality label empty\n"); */
/* #endif */
  /*   return; */
  /* } */

  if (doc) {
#if COWL_DEBUG
    printf("Setting confinement flags\n");
#endif
    // keep the initial flags and shift in the confinement specific flags
    doc->SetSandboxFlags(cowlInitialSandboxFlags() | cowlConfinementSandboxFlags());
  }

}


#undef COWL_CONFIG

}; // cowl
}; // xpc
