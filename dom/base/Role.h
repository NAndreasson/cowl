/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsTArray.h"
#include "nsContentUtils.h"

struct JSContext;

namespace mozilla {
namespace dom {

  enum class PrincipalState
  {
    APP_PRINCIPAL,
    UNIQUE_PRINCIPAL,
    ORIGIN_PRINCIPAL,
    INVALID_PRINCIPAL
  };

class PrincipalExpressionParser
{

public:
  static PrincipalState validateFormat(const nsAString& principal);

private:
  PrincipalExpressionParser(const nsAString &principal);
  ~PrincipalExpressionParser();

  bool hexOctet();
  bool hostChar();
  bool schemeChar();
  bool port();
  bool path();

  bool checkHexOctets(uint32_t hexOctetNr);

  bool schemeSource();
  bool subHost();
  bool host();
  bool hostSource();
  bool uniquePrincipal();
  bool appPrincipal();
  PrincipalState principalExpression();

  bool subPath();                 // helper function to parse paths

  bool atValidUnreservedChar();                         // helper function to parse unreserved
  bool atValidSubDelimChar();                           // helper function to parse sub-delims
  bool atValidPctEncodedChar();                         // helper function to parse pct-encoded



  inline bool atEnd()
  {
    return mCurChar >= mEndChar;
  }

  inline bool accept(char16_t aSymbol)
  {
    if (atEnd()) { return false; }
    return (*mCurChar == aSymbol) && advance();
  }

  inline bool accept(bool (*aClassifier) (char16_t))
  {
    if (atEnd()) { return false; }
    return (aClassifier(*mCurChar)) && advance();
  }

  inline bool peek(char16_t aSymbol)
  {
    if (atEnd()) { return false; }
    return *mCurChar == aSymbol;
  }

  inline bool peek(bool (*aClassifier) (char16_t))
  {
    if (atEnd()) { return false; }
    return aClassifier(*mCurChar);
  }

  inline bool advance()
  {
    if (atEnd()) { return false; }
    mCurValue.Append(*mCurChar++);
    return true;
  }

  inline void resetCurValue()
  {
    mCurValue.Truncate();
  }

  bool atEndOfPath();
  bool atValidPathChar();

  void resetCurChar(const nsAString& aToken);

  const char16_t*    mCurChar;
  const char16_t*    mEndChar;
  nsString           mCurValue;
  nsString           mCurToken;


};

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

class Role final : public nsISupports
                     , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Role)

protected:
  ~Role();

public:
  Role();
  Role(const nsAString& principal, ErrorResult& aRv);
  Role(nsIPrincipal* principal);


  Role* GetParentObject() const; //FIXME

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<Role> Constructor(const GlobalObject& global,
                                            const nsAString& principal,
                                            ErrorResult& aRv);
  // static already_AddRefed<Role> Constructor(const GlobalObject& global,
  //                                           const Sequence<nsString >& principals,
  //                                           ErrorResult& aRv);

  bool Equals(mozilla::dom::Role& other);

  bool Subsumes(mozilla::dom::Role& other);

  // already_AddRefed<Role> Or(const nsAString& principal, ErrorResult& aRv);
  already_AddRefed<Role> Or(nsIPrincipal* principal, ErrorResult& aRv);
  already_AddRefed<Role> Or(mozilla::dom::Role& other, ErrorResult& aRv);

  already_AddRefed<Role> Clone(ErrorResult &aRv) const;

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
