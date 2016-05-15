/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/COWLParser.h"

namespace mozilla {
namespace dom {

static const char16_t COLON        = ':';
static const char16_t SEMICOLON    = ';';
static const char16_t SLASH        = '/';
static const char16_t PLUS         = '+';
static const char16_t DASH         = '-';
static const char16_t DOT          = '.';
static const char16_t UNDERLINE    = '_';
static const char16_t TILDE        = '~';
static const char16_t WILDCARD     = '*';
static const char16_t WHITESPACE   = ' ';
static const char16_t SINGLEQUOTE  = '\'';
static const char16_t OPEN_CURL    = '{';
static const char16_t CLOSE_CURL   = '}';
static const char16_t NUMBER_SIGN  = '#';
static const char16_t QUESTIONMARK = '?';
static const char16_t PERCENT_SIGN = '%';
static const char16_t EXCLAMATION  = '!';
static const char16_t DOLLAR       = '$';
static const char16_t AMPERSAND    = '&';
static const char16_t OPENBRACE    = '(';
static const char16_t CLOSINGBRACE = ')';
static const char16_t EQUALS       = '=';
static const char16_t ATSYMBOL     = '@';

static const uint32_t kSubHostPathCharacterCutoff = 512;

PrincipalExpressionSplitter::PrincipalExpressionSplitter(const char16_t* aStart,
                               const char16_t* aEnd)
  : mCurChar(aStart)
  , mEndChar(aEnd)
{
}

PrincipalExpressionSplitter::~PrincipalExpressionSplitter()
{
}

void
PrincipalExpressionSplitter::generateNextToken(const nsAString& delim)
{
  int delimLength = delim.Length();

  skipWhiteSpace();
  bool done = false;

  nsAutoString s;


  while (!done && !atEnd()) {
    if (*mCurChar != ' ') s.Append(*mCurChar++);

    if (atEnd()) {
      done = true;
      break;
    }

    // collect whitespace?
    nsAutoString ws;
    while (*mCurChar == ' ') ws.Append(*mCurChar++);

    nsDependentSubstring sub = Substring(mCurChar, mCurChar + delimLength);
    // TODO, make case insensitive?
    if (sub.Equals(delim)) {
      done = true;
      mCurChar += delimLength;
    } else {
      s.Append(ws);
    }

  }

  mCurToken.Assign(s);
}

void
PrincipalExpressionSplitter::generateTokens(const nsAString& delim, nsTArray<nsString>& outTokens)
{
  while (!atEnd()) {
    generateNextToken(delim);
    outTokens.AppendElement(mCurToken);
  }
}

void
PrincipalExpressionSplitter::splitExpression(const nsAString &principalStr, const nsAString& delim, nsTArray<nsString>& outTokens)
{

  PrincipalExpressionSplitter tokenizer(principalStr.BeginReading(),
                           principalStr.EndReading());

  tokenizer.generateTokens(delim, outTokens);
}


COWLParser::COWLParser(const nsAString &principal)
  : mCurChar(nullptr)
  , mEndChar(nullptr)
{
  mCurToken = principal;
}

COWLParser::~COWLParser()
{

}

static bool
isCharacterToken(char16_t aSymbol)
{
  return (aSymbol >= 'a' && aSymbol <= 'z') ||
         (aSymbol >= 'A' && aSymbol <= 'Z');
}

static bool
isNumberToken(char16_t aSymbol)
{
  return (aSymbol >= '0' && aSymbol <= '9');
}

static bool
isValidHexDig(char16_t aHexDig)
{
  return (isNumberToken(aHexDig) ||
          (aHexDig >= 'A' && aHexDig <= 'F') ||
          (aHexDig >= 'a' && aHexDig <= 'f'));
}

bool
COWLParser::hexOctet()
{
  return accept(isValidHexDig) && accept(isValidHexDig);
}

void
COWLParser::resetCurChar(const nsAString& aToken)
{
  mCurChar = aToken.BeginReading();
  mEndChar = aToken.EndReading();
  resetCurValue();
}

// The path is terminated by the first question mark ("?") or
// number sign ("#") character, or by the end of the URI.
// http://tools.ietf.org/html/rfc3986#section-3.3
bool
COWLParser::atEndOfPath()
{
  return (atEnd() || peek(QUESTIONMARK) || peek(NUMBER_SIGN));
}

// unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
bool
COWLParser::atValidUnreservedChar()
{
  return (peek(isCharacterToken) || peek(isNumberToken) ||
          peek(DASH) || peek(DOT) ||
          peek(UNDERLINE) || peek(TILDE));
}

// sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                 / "*" / "+" / "," / ";" / "="
// Please note that even though ',' and ';' appear to be
// valid sub-delims according to the RFC production of paths,
// both can not appear here by itself, they would need to be
// pct-encoded in order to be part of the path.
bool
COWLParser::atValidSubDelimChar()
{
  return (peek(EXCLAMATION) || peek(DOLLAR) || peek(AMPERSAND) ||
          peek(SINGLEQUOTE) || peek(OPENBRACE) || peek(CLOSINGBRACE) ||
          peek(WILDCARD) || peek(PLUS) || peek(EQUALS));
}

// pct-encoded   = "%" HEXDIG HEXDIG
bool
COWLParser::atValidPctEncodedChar()
{
  const char16_t* pctCurChar = mCurChar;

  if ((pctCurChar + 2) >= mEndChar) {
    // string too short, can't be a valid pct-encoded char.
    return false;
  }

  // Any valid pct-encoding must follow the following format:
  // "% HEXDIG HEXDIG"
  if (PERCENT_SIGN != *pctCurChar ||
     !isValidHexDig(*(pctCurChar+1)) ||
     !isValidHexDig(*(pctCurChar+2))) {
    return false;
  }
  return true;
}

// pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
// http://tools.ietf.org/html/rfc3986#section-3.3
bool
COWLParser::atValidPathChar()
{
  return (atValidUnreservedChar() ||
          atValidSubDelimChar() ||
          atValidPctEncodedChar() ||
          peek(COLON) || peek(ATSYMBOL));
}

bool
COWLParser::hostChar()
{
  if (atEnd()) {
    return false;
  }
  return accept(isCharacterToken) ||
         accept(isNumberToken) ||
         accept(DASH);
}

// (ALPHA / DIGIT / "+" / "-" / "." )
bool
COWLParser::schemeChar()
{
  if (atEnd()) {
    return false;
  }
  return accept(isCharacterToken) ||
         accept(isNumberToken) ||
         accept(PLUS) ||
         accept(DASH) ||
         accept(DOT);
}

// port = ":" ( 1*DIGIT / "*" )
bool
COWLParser::port()
{
  // Consume the COLON we just peeked at in houstSource
  accept(COLON);

  // Resetting current value since we start to parse a port now.
  // e.g; "http://www.example.com:8888" then we have already parsed
  // everything up to (including) ":";
  resetCurValue();

  // Port might be "*"
  if (accept(WILDCARD)) {
    return true;
  }

  // Port must start with a number
  if (!accept(isNumberToken)) {
    return false;
  }
  // Consume more numbers and set parsed port to the nsCSPHost
  while (accept(isNumberToken)) { /* consume */ }
  return true;
}

bool
COWLParser::subPath()
{
  // Emergency exit to avoid endless loops in case a path in a CSP policy
  // is longer than 512 characters, or also to avoid endless loops
  // in case we are parsing unrecognized characters in the following loop.
  uint32_t charCounter = 0;

  while (!atEndOfPath()) {
    if (peek(SLASH)) {
      // Resetting current value since we are appending parts of the path
      // to aCspHost, e.g; "http://www.example.com/path1/path2" then the
      // first part is "/path1", second part "/path2"
      resetCurValue();
    }
    else if (!atValidPathChar()) {
      return false;
    }
    // potentially we have encountred a valid pct-encoded character in atValidPathChar();
    // if so, we have to account for "% HEXDIG HEXDIG" and advance the pointer past
    // the pct-encoded char.
    if (peek(PERCENT_SIGN)) {
      advance();
      advance();
    }
    advance();
    if (++charCounter > kSubHostPathCharacterCutoff) {
      return false;
    }
  }
  resetCurValue();
  return true;
}

bool
COWLParser::path()
{
  // Resetting current value and forgetting everything we have parsed so far
  // e.g. parsing "http://www.example.com/path1/path2", then
  // "http://www.example.com" has already been parsed so far
  // forget about it.
  resetCurValue();

  if (!accept(SLASH)) {
    return false;
  }
  if (atEndOfPath()) {
    // one slash right after host [port] is also considered a path, e.g.
    // www.example.com/ should result in www.example.com/
    // please note that we do not have to perform any pct-decoding here
    // because we are just appending a '/' and not any actual chars.
    return true;
  }
  // path can begin with "/" but not "//"
  // see http://tools.ietf.org/html/rfc3986#section-3.3
  if (peek(SLASH)) {
    return false;
  }
  return true;
}

bool
COWLParser::subHost()
{
  uint32_t charCounter = 0;

  while (!atEndOfPath() && !peek(COLON) && !peek(SLASH)) {
    ++charCounter;
    while (hostChar()) {
      /* consume */
      ++charCounter;
    }
    if (accept(DOT) && !hostChar()) {
      return false;
    }
    if (charCounter > kSubHostPathCharacterCutoff) {
      return false;
    }
  }
  return true;
}

bool COWLParser::schemeSource()
{
  if (!accept(isCharacterToken)) {
    return false;
  }
  while (schemeChar()) { /* consume */ }
  nsString scheme = mCurValue;

  // If the potential scheme is not followed by ":" - it's not a scheme
  if (!accept(COLON)) {
    return false;
  }

  return true;
}

// host = "*" / [ "*." ] 1*host-char *( "." 1*host-char )
bool COWLParser::host()
{
  // Check if the token starts with "*"; please remember that we handle
  // a single "*" as host in sourceExpression, but we still have to handle
  // the case where a scheme was defined, e.g., as:
  // "https://*", "*.example.com", "*:*", etc.
  if (accept(WILDCARD)) {
    // Might solely be the wildcard
    if (atEnd() || peek(COLON)) {
      return true;
    }
    // If the token is not only the "*", a "." must follow right after
    if (!accept(DOT)) {
      return false;
    }
  }

  // Expecting at least one host-char
  if (!hostChar()) {
    return false;
  }

  // There might be several sub hosts defined.
  if (!subHost()) {
    return false;
  }

  // Create a new nsCSPHostSrc with the parsed host.
  return true;
}

// host-source = [ scheme "://" ] host [ port ] [ path ]
bool COWLParser::hostSource()
{
  bool isHost  = host();
  if (!isHost) {
    // Error was reported in host()
    return false;
  }

  // Calling port() to see if there is a port to parse, if an error
  // occurs, port() reports the error, if port() returns true;
  // we have a valid port, so we add it to host.
  if (peek(COLON)) {
    if (!port()) {
      return false;
    }
  }

  if (atEndOfPath()) {
    return true;
  }

  // Calling path() to see if there is a path to parse, if an error
  // occurs, path() reports the error; handing host as an argument
  // which simplifies parsing of several paths.
  if (!path()) {
    // If the host [port] is followed by a path, it has to be a valid path,
    // otherwise we pass the nullptr, indicating an error, up the callstack.
    // see also http://www.w3.org/TR/CSP11/#source-list
    return false;
  }
  return true;
}

bool
COWLParser::checkHexOctets(uint32_t hexOctetNr)
{
  uint32_t count = 0;

  while (count < hexOctetNr && hexOctet()) { count++; }
  // TODO ternary?
  if (count == hexOctetNr) return true;
  else return false;
}

bool
COWLParser::uniquePrincipal()
{
  // check if starts with "unique:"
  if (!accept('u')) return false;
  if (!accept('n')) return false;
  if (!accept('i')) return false;
  if (!accept('q')) return false;
  if (!accept('u')) return false;
  if (!accept('e')) return false;
  if (!accept(COLON)) return false;

  // time-low
  if (!checkHexOctets(4)) return false;

  if (!accept(DASH)) return false;

  // time-mid
  if (!checkHexOctets(2)) return false;

  if (!accept(DASH)) return false;

  // time-high-and-version
  if (!checkHexOctets(2)) return false;

  if (!accept(DASH)) return false;

  // clock-seq-and-reserved
  if (!checkHexOctets(1)) return false;
  // clock-seq-low
  if (!checkHexOctets(1)) return false;

  if (!accept(DASH)) return false;

  // node
  if (!checkHexOctets(6)) return false;

  return atEnd();
}

bool
COWLParser::appPrincipal()
{
  // check if starts with "app:"
  if (!accept('a')) return false;
  if (!accept('p')) return false;
  if (!accept('p')) return false;
  if (!accept(COLON)) return false;

  // at least one host char
  if (!hostChar()) return false;
  // Consume all hostchars
  while (hostChar()) { /* consume */ }

  return atEnd();
}

COWLPrincipalType
COWLParser::principalExpression()
{

  // check if equals to self
  if (mCurToken.EqualsASCII("'self'")) return COWLPrincipalType::ORIGIN_PRINCIPAL;

  resetCurChar(mCurToken);

  if (uniquePrincipal()) {
    return COWLPrincipalType::UNIQUE_PRINCIPAL;
  }

  resetCurChar(mCurToken);

  if (appPrincipal()) {
    return COWLPrincipalType::APP_PRINCIPAL;
  }

  resetCurChar(mCurToken);

  // Calling resetCurChar allows us to use mCurChar and mEndChar
  // to parse mCurToken; e.g. mCurToken = "http://www.example.com", then
  // mCurChar = 'h'
  // mEndChar = points just after the last 'm'
  // mCurValue = ""
  resetCurChar(mCurToken);

  bool schemePrefix = schemeSource();
  if(schemePrefix) {
    if (atEnd()) {
      return COWLPrincipalType::INVALID_PRINCIPAL;
    }

    // If mCurToken provides not only a scheme, but also a host, we have to check
    // if two slashes follow the scheme.
    if (!accept(SLASH) || !accept(SLASH)) {
      return COWLPrincipalType::INVALID_PRINCIPAL;
    }
  }
  // Calling resetCurValue allows us to keep pointers for mCurChar and mEndChar
  // alive, but resets mCurValue; e.g. mCurToken = "http://www.example.com", then
  // mCurChar = 'w'
  // mEndChar = 'm'
  // mCurValue = ""
  resetCurValue();

  // Resetting internal helpers, because we might already have parsed some of the host
  // when trying to parse a scheme.
  if (!schemePrefix) resetCurChar(mCurToken);

   // At this point we are expecting a host to be parsed.
  // Trying to create a new nsCSPHost.
  if (hostSource()) {
    // Do not forget to set the parsed scheme.
    return COWLPrincipalType::ORIGIN_PRINCIPAL;
  }
  // Error was reported in hostSource()
  return COWLPrincipalType::INVALID_PRINCIPAL;
}

COWLPrincipalType
COWLParser::validateFormat(const nsAString& principal)
{
  COWLParser parser(principal);
  // construct new Labelparser
  // call parse and return result!
  return parser.principalExpression();
}

already_AddRefed<Label>
COWLParser::parsePrincipalExpression(const nsAString& principal, const nsACString& selfUrl)
{
  RefPtr<Label> label = new Label();

  // collapse etc

  /* // copy string and lowercase it? */
  /* nsAutoString tmpPrincipal(principal); */
  /* ToLowerCase(tmpPrincipal); */

  // TODO trim principal etc


  // chek if equal to none...
  if (principal.EqualsLiteral("'none'")) {
    return label.forget();
  }

  nsTArray<nsString> ands;
  PrincipalExpressionSplitter::splitExpression(principal, NS_LITERAL_STRING("AND"), ands);
  // TODO special case if only one AND

  ErrorResult aRv;

  for (nsString aAnd : ands) {
    RefPtr<Label> orExp = nullptr;

    if (ands.Length() > 1) {
      // should be wrapped in ( ), if not fail
      if (aAnd.First() != '(' || aAnd.Last() != ')') {
        return nullptr;
      }

      // remove first and last character... which should be ( and )
      aAnd.Cut(0, 1); aAnd.Cut(aAnd.Length() - 1, 1);
    }

    nsTArray<nsString> ors;
    PrincipalExpressionSplitter::splitExpression(aAnd, NS_LITERAL_STRING("OR"), ors);

    for (nsString prinTok : ors) {
      // perform or on orExpr?
      if (prinTok.EqualsLiteral("'self'")) {
        prinTok = NS_ConvertUTF8toUTF16(selfUrl);
      }

      if (!orExp) {
        orExp = new Label(prinTok, aRv);
      } else {
        orExp = orExp->Or(prinTok, aRv);
      }
      // did aRV fail?
      if (aRv.Failed()) {
      }
    }
    /* label = label->And(*orExp, aRv); */
    label = label->And(*orExp, aRv);
      if (aRv.Failed()) {
      }
    DisjunctionSetArray *otherRoles = label->GetDirectRoles();
    int length = otherRoles->Length();
  }

  return label.forget();
}

void
COWLParser::StrictSplit(const char* delim, const nsACString& expr, nsTArray<nsCString>& outTokens) {
  // Borrowed from https://dxr.mozilla.org/mozilla-central/source/dom/media/gmp/GMPUtils.cpp
  nsAutoCString str(expr);
  char* end = str.BeginWriting();
  const char* start = nullptr;
  while (!!(start = NS_strtok(delim, &end))) {
    outTokens.AppendElement(nsCString(start));
  }

}

// TODO could probably make more clean or add with the one below
void
COWLParser::parseLabeledContextHeader(const nsACString& expr, const nsACString& selfUrl, RefPtr<Label>* outConf, RefPtr<Label>* outInt, RefPtr<Label>* outPriv)
{
  RefPtr<Label> confidentiality = nullptr;
  RefPtr<Label> integrity = nullptr;
  RefPtr<Label> privilege = nullptr;

  nsTArray<nsCString> tokens;
  COWLParser::StrictSplit(";", expr, tokens);

  for (nsCString tok : tokens) {
    // make sure that TOK is not empty
    const char* start = tok.BeginReading();
    const char* end = tok.EndReading();

    // ship whitespace
    while (start < end && *start == ' ') start++;

    // collect sequence
    nsAutoCString directiveName;
    while (start < end && *start != ' ') directiveName.Append(*start++);
    // see if start is not at end.. and that is at space, then just skip ahead
    if (start < end && *start == ' ') start++;

    nsAutoCString directiveValue;
    // remaining characters should be the
    while (start < end) directiveValue.Append(*start++);

    // make use of parsellabel...
    RefPtr<Label> label = COWLParser::parsePrincipalExpression(NS_ConvertUTF8toUTF16(directiveValue), selfUrl);
    // look for failure, report to server

    // check if directive name equals data-confidentiality and conflabel null
    if (directiveName.EqualsLiteral("ctx-confidentiality") && !confidentiality) {
      confidentiality = label;
    } else if (directiveName.EqualsLiteral("ctx-integrity") && !integrity) {
      integrity = label;
    } else if (directiveName.EqualsLiteral("ctx-privilege") && !privilege) {
      // TODO should be a privilege with internal set to.....
      privilege = label;
    } else {
      // report an error?
      break;
    }

    printf("Result from split %s\n", ToNewCString(tok));
    printf("Directive name %s\n", ToNewCString(directiveName));
    printf("Directive value %s\n", ToNewCString(directiveValue));
  }

  // set out params
  *outConf = confidentiality;
  *outInt = integrity;
  *outPriv = privilege;


}

void
COWLParser::parseLabeledDataHeader(const nsACString& expr, const nsACString& selfUrl, RefPtr<Label>* outConf, RefPtr<Label>* outInt)
{
  RefPtr<Label> confidentiality = nullptr;
  RefPtr<Label> integrity = nullptr;

  nsTArray<nsCString> tokens;
  COWLParser::StrictSplit(";", expr, tokens);

  for (nsCString tok : tokens) {
    // make sure that TOK is not empty
    const char* start = tok.BeginReading();
    const char* end = tok.EndReading();

    // ship whitespace
    while (start < end && *start == ' ') start++;

    // collect sequence
    nsAutoCString directiveName;
    while (start < end && *start != ' ') directiveName.Append(*start++);
    // see if start is not at end.. and that is at space, then just skip ahead
    if (start < end && *start == ' ') start++;

    nsAutoCString directiveValue;
    // remaining characters should be the
    while (start < end) directiveValue.Append(*start++);

    // make use of parsellabel...
    RefPtr<Label> label = COWLParser::parsePrincipalExpression(NS_ConvertUTF8toUTF16(directiveValue), selfUrl);
    // look for failure, report to server

    // check if directive name equals data-confidentiality and conflabel null
    if (directiveName.EqualsLiteral("data-confidentiality") && !confidentiality) {
      confidentiality = label;
    } else if (directiveName.EqualsLiteral("data-integrity") && !integrity) {
      integrity = label;
    } else {
      // report an error?
      break;
    }

    printf("Result from split %s\n", ToNewCString(tok));
    printf("Directive name %s\n", ToNewCString(directiveName));
    printf("Directive value %s\n", ToNewCString(directiveValue));
  }

  // set out params
  *outConf = confidentiality;
  *outInt = integrity;
}

}
}


