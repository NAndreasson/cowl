/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/Role.h"
#include "mozilla/dom/RoleBinding.h"
#include "mozilla/dom/COWLParser.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsIURI.h"
#include "nsIStandardURL.h"
#include "nsCOMPtr.h"
#include "nsScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

//NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(Role, mPrincipals)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Role)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Role)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Role)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Role)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Role::Role()
{
}

Role::Role(const nsAString& principal, ErrorResult& aRv)
{
  PrincipalState principalState = COWLParser::validateFormat(principal);

  if (principalState == PrincipalState::INVALID_PRINCIPAL) {
    aRv.ThrowTypeError<MSG_INVALID_PRINCIPAL>(principal);
    // Do we need to return anything here, e.g. nullptr?
  }

  if (principalState != PrincipalState::ORIGIN_PRINCIPAL) {
    _Or(principal, false, aRv);
    return;
  }

  // If the principal matches an origin principal, we try to normalize it?
  nsAutoString retval;
  if (NormalizeOrigin(principal, retval)) {
    _Or(retval, true, aRv);
  } else {
    // if we failed to normalize it, then use passed principal
    // seems to happen if origin is missing a scheme..
    _Or(principal, true, aRv);
  }

}

bool
Role::NormalizeOrigin(const nsAString& principal, nsAString& retval) {
  nsresult rv;
  // Create URI
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), principal);

  if (NS_FAILED(rv)) return false;

  nsAutoCString origin;
  rv = uri->GetAsciiSpec(origin);
  CopyASCIItoUTF16(origin, retval);
  // NS_ENSURE_SUCCESS(rv, rv);
  return true;
}

Role::Role(nsIPrincipal* principal)
{
  // nsIURI uri;
  nsCOMPtr<nsIURI> uri;
  nsresult rv = principal->GetURI(getter_AddRefs(uri));
  // try fetching uri...


  nsAutoCString origin1;
  rv = uri->GetAsciiSpec(origin1);

  // nsresult rv = principal->GetOrigin(origin1);
  // NS_ASSERTION(NS_SUCCEEDED(rv), "nsIPrincipal::GetOrigin failed");
  ErrorResult aRv;

  _Or(NS_ConvertASCIItoUTF16(origin1), true, aRv);
}

Role::~Role()
{
}

JSObject*
Role::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return RoleBinding::Wrap(aCx, this, aGivenProto);
}

Role*
Role::GetParentObject() const
{
  return nullptr; //TODO: return something sensible here
}

already_AddRefed<Role>
Role::Constructor(const GlobalObject& global, const nsAString& principal,
                  ErrorResult& aRv)
{
  RefPtr<Role> role = new Role(principal, aRv);
  if (aRv.Failed())
    return nullptr;
  return role.forget();
}

// already_AddRefed<Role>
// Role::Constructor(const GlobalObject& global, const Sequence<nsString >& principals,
//                   ErrorResult& aRv)
// {
//   RefPtr<Role> role = new Role();
//   for (unsigned i = 0; i < principals.Length(); ++i) {
//     role->_Or(principals[i],aRv);
//     if (aRv.Failed())
//       return nullptr;
//   }
//   return role.forget();
// }

bool
Role::Equals(mozilla::dom::Role& other)
{
  // Break out early if the other and this are the same.
  if (&other == this)
    return true;

  PrincipalArray *otherPrincipals = other.GetDirectPrincipals();

  // The other role is of a different size, can't be equal.
  if (otherPrincipals->Length() != mPrincipals.Length())
    return false;

  PrincipalComparator cmp;
  for (unsigned i=0; i< mPrincipals.Length(); ++i) {
    /* This role contains a principal that the other role does not,
     * hence it cannot be equal to it. */
    if(!cmp.Equals(mPrincipals[i], (*otherPrincipals)[i]))
      return false;
  }

  return true;
}

bool
Role::Subsumes(mozilla::dom::Role& other)
{
  // Break out early if the other points to this
  if (&other == this)
    return true;

  PrincipalArray *otherPrincipals = other.GetDirectPrincipals();

  // The other role is smaller, this role cannot imply (subsume) it.
  if (otherPrincipals->Length() < mPrincipals.Length())
    return false;

  PrincipalComparator cmp;
  for (unsigned i=0; i< mPrincipals.Length(); ++i) {
    /* This role contains a principal that the other role does not,
     * hence it cannot imply (subsume) it. */
    if (!otherPrincipals->Contains(mPrincipals[i],cmp))
      return false;
  }

  return true;
}

// already_AddRefed<Role>
// Role::Or(const nsAString& principal, ErrorResult& aRv)
// {
//   _Or(principal, aRv);
//   if (aRv.Failed())
//     return nullptr;

//   RefPtr<Role> _this = this;
//   return _this.forget();
// }

already_AddRefed<Role>
Role::Or(nsIPrincipal* principal, ErrorResult& aRv)
{
  nsAutoCString origin1;
  nsresult rv = principal->GetOrigin(origin1);
  // NS_ASSERTION(NS_SUCCEEDED(rv), "nsIPrincipal::GetOrigin failed");

  _Or(NS_ConvertASCIItoUTF16(origin1), true, aRv);

  RefPtr<Role> _this = this;
  return _this.forget();
}

already_AddRefed<Role>
Role::Or(Role& other, ErrorResult& aRv)
{
  _Or(other);
  RefPtr<Role> _this = this;
  return _this.forget();
}

void
Role::Stringify(nsString& retval)
{
  retval = NS_LITERAL_STRING("");

  for (unsigned i=0; i < mPrincipals.Length(); ++i) {
    COWLPrincipal& principal = mPrincipals[i];
    nsAutoString principalString;
    principal.Stringify(principalString);

    retval.Append(principalString);

    if (i != (mPrincipals.Length() - 1))
      retval.Append(NS_LITERAL_STRING(" OR "));
  }

  retval.Append(NS_LITERAL_STRING(""));
}

already_AddRefed<Role>
Role::Clone(ErrorResult &aRv) const
{
  RefPtr<Role> role = new Role();

  if(!role) {
    aRv = NS_ERROR_OUT_OF_MEMORY;
    return nullptr;
  }

  PrincipalArray *newPrincipals = role->GetDirectPrincipals();
  for (unsigned i = 0; i < mPrincipals.Length(); ++i) {
    newPrincipals->InsertElementAt(i, mPrincipals[i]);
  }
  return role.forget();
}

//
// Internals
//
void
Role::_Or(const nsAString& principal, bool isOriginPrincipal, ErrorResult& aRv)
{
  COWLPrincipal cPrincipal(principal, isOriginPrincipal);
  _Or(cPrincipal);
}

void
Role::_Or(const COWLPrincipal& principal)
{
  PrincipalComparator cmp;
  if (!mPrincipals.Contains(principal, cmp))
    mPrincipals.InsertElementSorted(principal, cmp);
}

void
Role::_Or(Role& other)
{
  PrincipalArray *otherPrincipals = other.GetDirectPrincipals();
  for (unsigned i=0; i< otherPrincipals->Length(); ++i) {
    _Or(otherPrincipals->ElementAt(i));
  }
}

int
PrincipalComparator::Compare(const COWLPrincipal &p1,
                                const COWLPrincipal &p2) const
{
  // get their respective stirngs..
  nsAutoString p1String;
  p1.Stringify(p1String);
  nsAutoString p2String;
  p2.Stringify(p2String);


  bool res = strcmp(ToNewUTF8String(p1String), ToNewUTF8String(p2String));
  return res;
}

} // namespace dom
} // namespace mozilla
