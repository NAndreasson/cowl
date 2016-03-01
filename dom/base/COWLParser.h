/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_COWLParser_h
#define mozilla_dom_COWLParser_h

#include "nsString.h"

namespace mozilla {
namespace dom {

enum class PrincipalState
{
  APP_PRINCIPAL,
  UNIQUE_PRINCIPAL,
  ORIGIN_PRINCIPAL,
  INVALID_PRINCIPAL
};

class COWLParser
{

public:
  static PrincipalState validateFormat(const nsAString& principal);

private:
  COWLParser(const nsAString &principal);
  ~COWLParser();

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

}
}

#endif // mozilla_dom_COWLParser_h
