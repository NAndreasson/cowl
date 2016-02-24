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
#include "mozilla/dom/Role.h"
#include "nsIContentSecurityPolicy.h"
#include "nsDocument.h"
#include "nsSandboxFlags.h"

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
  return SANDBOXED_ORIGIN | SANDBOXED_NAVIGATION;
}



NS_EXPORT_(void)
EnableCompartmentConfinement(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);

  if (IsCompartmentConfined(compartment))
    return;

  RefPtr<Label> privacy = new Label();
  MOZ_ASSERT(privacy);

  RefPtr<Label> trust = new Label();
  MOZ_ASSERT(trust);

  COWL_CONFIG(compartment).SetPrivacyLabel(privacy);
  COWL_CONFIG(compartment).SetTrustLabel(trust);

  // set privileges to compartment principal
  // we're not "copying" the principal since the principal may be a
  // null principal (iframe sandbox) and thus not a codebase principal
  nsCOMPtr<nsIPrincipal> privPrin = GetCompartmentPrincipal(compartment);
  RefPtr<Role> privRole = new Role(privPrin);
  ErrorResult aRv;
  RefPtr<Label> privileges = new Label(*privRole, aRv);
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

NS_EXPORT_(bool)
IsCompartmentConfined(JSCompartment *compartment)
{
  MOZ_ASSERT(compartment);
  return COWL_CONFIG(compartment).isEnabled();
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

// This function sets the compartment privacy label. It clones the given label.
// IMPORTANT: This function should not be exported to untrusted code.
// Untrusted code can only set the privacy label to a label that
// subsumes the "current label".
DEFINE_SET_LABEL(PrivacyLabel)
DEFINE_GET_LABEL(PrivacyLabel)

// This function sets the compartment trust label. It clones the given label.
// IMPORTANT: This function should not be exported to untrusted code.
// Untrusted code can only set the trust label to a label subsumed by
// the "current label".
DEFINE_SET_LABEL(TrustLabel)
DEFINE_GET_LABEL(TrustLabel)

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

// Check if information can flow from the compartment to an object labeled with
// |privacy| and |trust| into the compartment.
NS_EXPORT_(bool)
GuardWrite(JSCompartment *compartment,
           Label &privacy, Label &trust, Label *aPrivs)
{
  ErrorResult aRv;

  if (!IsCompartmentConfined(compartment)) {
    NS_WARNING("Not in confined compartment");
    return false;
  }

  RefPtr<Label> privs = aPrivs ? aPrivs : new Label();
  RefPtr<Label> compPrivacy, compTrust;
  compPrivacy = GetCompartmentPrivacyLabel(compartment);
  compTrust   = GetCompartmentTrustLabel(compartment);

  // If any of the labels are missing, don't allow the information flow
  if (!compPrivacy || !compTrust) {
    NS_WARNING("Missing labels");
    return false;
  }


#if COWL_DEBUG
  {
    nsAutoString compPrivacyStr, compTrustStr, privacyStr, trustStr, privsStr;
    compPrivacy->Stringify(compPrivacyStr);
    compTrust->Stringify(compTrustStr);
    privacy.Stringify(privacyStr);
    trust.Stringify(trustStr);
    privs->Stringify(privsStr);

    printf("GuardWrite <%s,%s> to <%s,%s> | %s\n",
           NS_ConvertUTF16toUTF8(compPrivacyStr).get(),
           NS_ConvertUTF16toUTF8(compTrustStr).get(),
           NS_ConvertUTF16toUTF8(privacyStr).get(),
           NS_ConvertUTF16toUTF8(trustStr).get(),
           NS_ConvertUTF16toUTF8(privsStr).get());
  }
#endif


  // if not <compPrivacy,compTrust> [=_privs <privacy,trust>
  if (!(privacy.Subsumes(*privs, *compPrivacy) && compTrust->Subsumes(*privs, trust))) {
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
  RefPtr<Label> privacy = GetCompartmentPrivacyLabel(dst);
  RefPtr<Label> trust   = GetCompartmentTrustLabel(dst);
  RefPtr<Label> privs   = GetCompartmentPrivileges(compartment);

  if (!privacy || !trust || !privs) {
    NS_WARNING("Missing privacy or trust labels");
    return false;
  }

  return GuardWrite(compartment, *privacy, *trust, privs);
}

// Check if information can flow from an object labeled with |privacy|
// and |trust| into the compartment. For this to hold, the compartment
// must preserve privacy, i.e., the compartment privacy label must
// subsume the object privacy label, and not be corrupted, i.e., the
// object trust label must be at least as trustworthy as the
// compartment trust label.
NS_EXPORT_(bool)
GuardRead(JSCompartment *compartment,
          Label &privacy, Label &trust, Label *aPrivs, JSContext *cx)
{
  ErrorResult aRv;

  RefPtr<Label> privs = aPrivs ? aPrivs : new Label();
  RefPtr<Label> compPrivacy, compTrust;

  if (IsCompartmentConfined(compartment)) {
    compPrivacy = GetCompartmentPrivacyLabel(compartment);
    compTrust   = GetCompartmentTrustLabel(compartment);
  } else {
    // compartment is not confined
    nsCOMPtr<nsIPrincipal> privPrin = GetCompartmentPrincipal(compartment);
    RefPtr<Role> privRole = new Role(privPrin);
    compPrivacy = new Label(*privRole, aRv);
    compTrust   = new Label();
    if (aRv.Failed()) return false;
  }

  // If any of the labels are missing, don't allow the information flow
  if (!compPrivacy || !compTrust) {
    NS_WARNING("Missing labels!");
    return false;
  }


#if COWL_DEBUG
  {
    nsAutoString compPrivacyStr, compTrustStr, privacyStr, trustStr, privsStr;
    compPrivacy->Stringify(compPrivacyStr);
    compTrust->Stringify(compTrustStr);
    privacy.Stringify(privacyStr);
    trust.Stringify(trustStr);
    privs->Stringify(privsStr);

    printf("GuardRead <%s,%s> to <%s,%s> | %s\n",
           NS_ConvertUTF16toUTF8(privacyStr).get(),
           NS_ConvertUTF16toUTF8(trustStr).get(),
           NS_ConvertUTF16toUTF8(compPrivacyStr).get(),
           NS_ConvertUTF16toUTF8(compTrustStr).get(),
           NS_ConvertUTF16toUTF8(privsStr).get());
  }
#endif

  // <privacy,trust> [=_privs <compPrivacy,compTrust>
  if (compPrivacy->Subsumes(*privs, privacy) &&
      trust.Subsumes(*privs, *compTrust)) {
    return true;
  }

  NS_WARNING("Does not subsume");
  return false;
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



  RefPtr<Label> privacy, trust;

  if (IsCompartmentConfined(source)) {
    privacy = GetCompartmentPrivacyLabel(source);
    trust   = GetCompartmentTrustLabel(source);
  } else {
    privacy = new Label();
    trust   = new Label();
  }

  RefPtr<Label> privs = isGET ? GetCompartmentPrivileges(compartment)
                                : GetCompartmentPrivileges(source);


  if (!privacy || !trust) {
    NS_WARNING("Missing privacy or trust labels");
    return false;
  }

  return GuardRead(compartment, *privacy, *trust, privs);
}

static inline uint32_t cowlSandboxFlags() {
  // Sandbox flags
  nsAttrValue sandboxAttr;
  sandboxAttr.ParseAtomArray(NS_LITERAL_STRING("allow-scripts allow-forms"));
  return nsContentUtils::ParseSandboxAttributeToFlags(&sandboxAttr);
}

NS_EXPORT_(void)
RefineCompartmentFlags(JSCompartment *compartment)
{
  RefPtr<Label> privacy = GetCompartmentPrivacyLabel(compartment);

  // TODO, take privilege in consideration.... Should extract to label downgrade algo...
  if (privacy->IsEmpty()) {
#if COWL_DEBUG
    printf("Refine: Privacy label is empty, do nothing\n");
#endif
    return;
  }

  // get privilege..
  RefPtr<Label> privilege  = COWL_CONFIG(compartment).GetPrivileges();
  RefPtr<Label> emptyLabel = new Label();

  // check effective label
  if (emptyLabel->Subsumes(*privilege, *privacy)) {
#if COWL_DEBUG
    printf("Effective label empty\n");
#endif
    return;
  }

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

  if (doc) {
#if COWL_DEBUG
    printf("Setting confinement flags\n");
#endif
    // keep the initial flags and shift in the confinement specific flags
    doc->SetSandboxFlags(cowlInitialSandboxFlags() | cowlConfinementSandboxFlags());
  }

}


// This function adjusts the "security permieter".
// Specifically, it adjusts:
// 1. The CSP policy to restrict with whom the current compartment may
// network-communicate with.
// 2. The compartment principal to restrict writing to storage
// cnannels.
//
// NS_EXPORT_(void)
// RefineCompartmentCSP(JSCompartment *compartment)
// {
//   nsresult rv;

//   // Get the compartment privacy label:
//   RefPtr<Label> privacy = GetCompartmentPrivacyLabel(compartment);

//   // Get the compartment principal
//   nsCOMPtr<nsIPrincipal> compPrincipal = GetCompartmentPrincipal(compartment);

//   // Get underlying CSP
//   nsCOMPtr<nsIContentSecurityPolicy> csp;
//   rv = compPrincipal->GetCsp(getter_AddRefs(csp));
//   MOZ_ASSERT(NS_SUCCEEDED(rv));

//   // If there is a CSP and we set it before, remove our existing policy
//   if (csp) {
//     uint32_t numPolicies = 0;
//     rv = csp->GetPolicyCount(&numPolicies);
//     MOZ_ASSERT(NS_SUCCEEDED(rv));
//     uint32_t cowlCSPPolicy = COWL_CONFIG(compartment).mCSPIndex ;
//     if (cowlCSPPolicy > 0 && cowlCSPPolicy <= numPolicies) {
//       rv = csp->RemovePolicy(cowlCSPPolicy-1);
//       MOZ_ASSERT(NS_SUCCEEDED(rv));
//       COWL_CONFIG(compartment).mCSPIndex = 0;
//     }
//   }

//   // Get the compartment global
//   nsCOMPtr<nsIGlobalObject> global =
//     NativeGlobal(JS_GetGlobalForCompartmentOrNull(compartment));
//   MOZ_ASSERT(global);

//   // Get the underlying window, if it exists
//   nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(global));
//   nsCOMPtr<nsIDocument> doc;

//   // Channel for reporting CSP violations, if there is an underlying
//   // document:
//   nsCOMPtr<nsIChannel> reportChan;

//   if (win) {
//     // Get the window document
//     doc = win->GetDoc();

//     MOZ_ASSERT(doc);

//     // Set the report channel to the document channel
//     reportChan = doc->GetChannel();

//     // Reset sandbox flags, if set
//     if (doc->GetSandboxFlags() == cowlSandboxFlags() &&
//         COWL_CONFIG(compartment).SetSandboxFlags()) {
//       doc->SetSandboxFlags(COWL_CONFIG(compartment).GetSandboxFlags());
//       COWL_CONFIG(compartment).ClearSandboxFlags();
//     }
//   }

//   // Case 1: Empty/public label, don't loosen/impose new restrictions
//   if (privacy->IsEmpty()) {
// #if COWL_DEBUG
//     printf("Refine: Privacy label is empty, do nothing\n");
// #endif
//     return;
//   }

//   nsString policy;
//   PrincipalsArray* labelPrincipals = privacy->GetPrincipalsIfSingleton();

//   if (labelPrincipals && labelPrincipals->Length() > 0) {
//     // Case 2: singleton disjunctive role
//     // Allow network access to all the origins in the list,
//     // but disable storage access.

//     // Create list of origins
//     nsString origins;
//     for (unsigned i = 0; i < labelPrincipals->Length(); ++i) {
//       nsAutoCString origin;
//       rv = labelPrincipals->ElementAt(i)->GetOrigin(origin);
//       MOZ_ASSERT(NS_SUCCEEDED(rv));
//       AppendASCIItoUTF16(origin, origins);
//       // NS_Free(origin);
//       origins.Append(NS_LITERAL_STRING(" "));
//     }

//     policy = NS_LITERAL_STRING("default-src 'unsafe-inline' 'unsafe-eval' ")  + origins
//            + NS_LITERAL_STRING(";script-src 'unsafe-inline' 'unsafe-eval' ")  + origins
//            + NS_LITERAL_STRING(";object-src ")                  + origins
//            + NS_LITERAL_STRING(";style-src 'unsafe-inline' ")   + origins
//            + NS_LITERAL_STRING(";img-src ")                     + origins
//            + NS_LITERAL_STRING(";media-src ")                   + origins
//            + NS_LITERAL_STRING(";frame-src ")                   + origins
//            + NS_LITERAL_STRING(";font-src ")                    + origins
//            + NS_LITERAL_STRING(";connect-src ")                 + origins
//            + NS_LITERAL_STRING(";");
// #if COWL_DEBUG
//     printf("Refine: Privacy label is disjunctive\n");
// #endif

//   } else {
//     // Case 3: not the empty label or singleton disjunctive role
//     // Disable all network and storage access.

//     // Policy to disable all communication
//     policy = NS_LITERAL_STRING("default-src 'none' 'unsafe-inline';")
//            + NS_LITERAL_STRING("script-src  'none' 'unsafe-inline' 'unsafe-eval';")
//            + NS_LITERAL_STRING("object-src  'none';")
//            + NS_LITERAL_STRING("style-src   'none' 'unsafe-inline';")
//            + NS_LITERAL_STRING("img-src     'none';")
//            + NS_LITERAL_STRING("media-src   'none';")
//            + NS_LITERAL_STRING("frame-src   'none';")
//            + NS_LITERAL_STRING("font-src    'none';")
//            + NS_LITERAL_STRING("connect-src 'none';");
// #if COWL_DEBUG
//     printf("Refine: Privacy label is conjunctive\n");
// #endif
//   }

//   // A. Set sandbox flags to disallow storage access

//   if (doc) {
//     // Save current flags:
//     COWL_CONFIG(compartment).SetSandboxFlags(doc->GetSandboxFlags());
//     doc->SetSandboxFlags(cowlSandboxFlags());
//   }

//   // B. Set new CSP policy

//   // Principal doesn't have a CSP, create it:
//   if (!csp) {
//     csp = do_CreateInstance("@mozilla.org/cspcontext;1", &rv);
//     MOZ_ASSERT(NS_SUCCEEDED(rv) && csp);
//     // Set the csp since we create a new principal
//     // rv = compPrincipal->EnsureCSP(doc, csp);
//     // MOZ_ASSERT(NS_SUCCEEDED(rv));

//     // Get the principal URI
//     // nsCOMPtr<nsIURI> baseURI;
//     // rv = compPrincipal->GetURI(getter_AddRefs(baseURI));
//     // MOZ_ASSERT(NS_SUCCEEDED(rv));

//     // // rv = csp->SetRequestContext(baseURI, nullptr, reportChan);
//     // MOZ_ASSERT(NS_SUCCEEDED(rv));
//   }

//   // Append policy
//   rv = csp->AppendPolicy(policy, false, false);
//   MOZ_ASSERT(NS_SUCCEEDED(rv));

//   // Track which policy is COWLs
//   rv = csp->GetPolicyCount(&COWL_CONFIG(compartment).mCSPIndex);
//   MOZ_ASSERT(NS_SUCCEEDED(rv));

//  #ifdef COWL_DEBUG
//   printf("Refine: appended policy [%d] to principal %p [csp=%p]: %s\n", COWL_CONFIG(compartment).mCSPIndex, compPrincipal.get(), csp.get(), NS_ConvertUTF16toUTF8(policy).get());
//  #endif


// #ifdef COWL_DEBUG
//   {
//     unsigned numPolicies = 0;
//     if (csp) {
//       nsresult rv = csp->GetPolicyCount(&numPolicies);
//       MOZ_ASSERT(NS_SUCCEEDED(rv));
//       printf("Refine: Number of CSP policies: %d\n", numPolicies);
//       for (unsigned i=0; i<numPolicies; i++) {
//         nsAutoString policy;
//         csp->GetPolicy(i, policy);
//         printf("Refine: Principal has CSP[%d]: %s\n", i,
//             NS_ConvertUTF16toUTF8(policy).get());
//       }
//     }
//   }
// #endif

// }



#undef COWL_CONFIG

}; // cowl
}; // xpc
