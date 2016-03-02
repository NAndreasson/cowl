/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_Role_h
#define mozilla_dom_Role_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsTArray.h"
#include "nsContentUtils.h"

struct JSContext;

namespace mozilla {
namespace dom {

// TODO
// subclass, split into origin etc, then perform normalization and so on here...
class COWLPrincipal
{
public:
  COWLPrincipal(const nsAString& principal, bool originPrincipal)
    : mPrincipal(principal), mOriginPrincipal(originPrincipal)
  {
  }

  void Stringify(nsString& retval) const
  {
    retval.Append(mPrincipal);
  }


  bool IsOriginPrincipal() const
  {
    return mOriginPrincipal;
  }

private:
  nsString mPrincipal;
  bool mOriginPrincipal;

};

typedef nsTArray<nsString> PrincipalsArray;
typedef nsTArray<COWLPrincipal> PrincipalArray;

class Role final
{
protected:
  ~Role();

public:
  Role();
  Role(const nsAString& principal, ErrorResult& aRv);
  Role(nsIPrincipal* principal);


  Role* GetParentObject() const; //FIXME

  bool Equals(const mozilla::dom::Role& other) const;

  bool Subsumes(const mozilla::dom::Role& other) const;

  Role* Or(nsIPrincipal* principal, ErrorResult& aRv);
  Role* Or(mozilla::dom::Role& other, ErrorResult& aRv);

  Role* Clone(ErrorResult &aRv) const;

  void Stringify(nsString& retval);

public: // C++ only:
  void _Or(const nsAString& principal, bool IsOriginPrincipal, ErrorResult& aRv);
  void _Or(const COWLPrincipal& principal);
  void _Or(mozilla::dom::Role& other);

  bool ContainsOriginPrincipal() const
  {
    bool foundOriginPrincipal = false;

    for (unsigned i = 0; i < mPrincipals.Length(); ++i) {
      if (mPrincipals[i].IsOriginPrincipal()) {
        foundOriginPrincipal = true;
        break;
      }
    }

    return foundOriginPrincipal;
  }

  bool IsEmpty() const
  {
    return !mPrincipals.Length();
  }

private:

public: // XXX TODO make private, unsafe
  PrincipalArray* GetDirectPrincipals()
  {
    return &mPrincipals;
  }

//  friend class Label;

private:
  bool NormalizeOrigin(const nsAString& principal, nsAString& retval);

  PrincipalArray mPrincipals;
  PrincipalsArray mmPrincipals;

};

// Comparator helpers

class PrincipalComparator {

public:
  int Compare(const COWLPrincipal& p1,
              const COWLPrincipal& p2) const;

  bool Equals(const COWLPrincipal& p1,
              const COWLPrincipal& p2) const
  {
    return Compare(p1,p2) == 0;
  }

  bool LessThan(const COWLPrincipal& p1,
                const COWLPrincipal& p2) const
  {
    return Compare(p1,p2) < 0;
  }

};

} // namespace dom
} // namespace mozilla

#endif