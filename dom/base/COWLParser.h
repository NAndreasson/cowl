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

class Label;

enum class COWLPrincipalType
{
  APP_PRINCIPAL,
  UNIQUE_PRINCIPAL,
  ORIGIN_PRINCIPAL,
  SENSITIVE_PRINCIPAL,
  INVALID_PRINCIPAL
};

typedef nsTArray< nsTArray<nsString> > principalExpressions;

class PrincipalExpressionSplitter {
  public:
    static void splitExpression(const nsAString& labelString, const nsAString& delim, nsTArray<nsString>& outTokens);

  private:
    PrincipalExpressionSplitter(const char16_t* aStart, const char16_t* aEnd);
    ~PrincipalExpressionSplitter();

    inline bool atEnd()
    {
      return mCurChar >= mEndChar;
    }

    inline void skipWhiteSpace()
    {
      while (mCurChar < mEndChar && *mCurChar == ' ') {
        mCurToken.Append(*mCurChar++);
      }
      mCurToken.Truncate();
    }

    inline bool accept(char16_t aChar)
    {
      NS_ASSERTION(mCurChar < mEndChar, "Trying to dereference mEndChar");
      if (*mCurChar == aChar) {
        mCurToken.Append(*mCurChar++);
        return true;
      }
      return false;
    }

    void generateNextToken(const nsAString& delim);
    void generateTokens(const nsAString& delim, nsTArray<nsString>& outTokens);

    const char16_t* mCurChar;
    const char16_t* mEndChar;
    nsString        mCurToken;
};

class COWLParser
{

public:
  static COWLPrincipalType validateFormat(const nsAString& principal);
  static already_AddRefed<Label> parsePrincipalExpression(const nsAString& principal, const nsACString& selfUrl);
  static void parseLabeledDataHeader(const nsACString& expr, const nsACString& selfUrl, RefPtr<Label>* outConf, RefPtr<Label>* outInt);
  static void parseLabeledContextHeader(const nsACString& expr, const nsACString& selfUrl, RefPtr<Label>* outConf, RefPtr<Label>* outInt, RefPtr<Label>* outPriv);
  static void StrictSplit(const char* delim, const nsACString& expr, nsTArray<nsCString>& outTokens);

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
  bool sensitivePrincipal();
  COWLPrincipalType principalExpression();

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
