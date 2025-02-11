/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FetchDriver_h
#define mozilla_dom_FetchDriver_h

#include "nsAutoPtr.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "mozilla/RefPtr.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/net/ReferrerPolicy.h"

class nsIDocument;
class nsIOutputStream;
class nsILoadGroup;
class nsIPrincipal;

namespace mozilla {
namespace dom {

class InternalRequest;
class InternalResponse;

class FetchDriverObserver
{
public:
  FetchDriverObserver() : mGotResponseAvailable(false)
  { }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FetchDriverObserver);
  void OnResponseAvailable(InternalResponse* aResponse)
  {
    MOZ_ASSERT(!mGotResponseAvailable);
    mGotResponseAvailable = true;
    OnResponseAvailableInternal(aResponse);
  }
  virtual void OnResponseEnd()
  { };

protected:
  virtual ~FetchDriverObserver()
  { };

  virtual void OnResponseAvailableInternal(InternalResponse* aResponse) = 0;

private:
  bool mGotResponseAvailable;
};

class FetchDriver final : public nsIStreamListener,
                          public nsIChannelEventSink,
                          public nsIInterfaceRequestor,
                          public nsIThreadRetargetableStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

  explicit FetchDriver(InternalRequest* aRequest, nsIPrincipal* aPrincipal,
                       nsILoadGroup* aLoadGroup);
  NS_IMETHOD Fetch(FetchDriverObserver* aObserver);

  void
  SetDocument(nsIDocument* aDocument);

private:
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsILoadGroup> mLoadGroup;
  RefPtr<InternalRequest> mRequest;
  RefPtr<InternalResponse> mResponse;
  nsCOMPtr<nsIOutputStream> mPipeOutputStream;
  RefPtr<FetchDriverObserver> mObserver;
  nsCOMPtr<nsIDocument> mDocument;

  DebugOnly<bool> mResponseAvailableCalled;
  DebugOnly<bool> mFetchCalled;

  FetchDriver() = delete;
  FetchDriver(const FetchDriver&) = delete;
  FetchDriver& operator=(const FetchDriver&) = delete;
  ~FetchDriver();

  bool DoCOWLCheck(InternalResponse* aResponse, nsIURI* aFinalURI);

  nsresult ContinueFetch();
  nsresult HttpFetch();
  // Returns the filtered response sent to the observer.
  // Callers who don't have access to a channel can pass null for aFinalURI.
  already_AddRefed<InternalResponse>
  BeginAndGetFilteredResponse(InternalResponse* aResponse, nsIURI* aFinalURI,
                              bool aFoundOpaqueRedirect);
  // Utility since not all cases need to do any post processing of the filtered
  // response.
  void FailWithNetworkError();

  void SetRequestHeaders(nsIHttpChannel* aChannel) const;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_FetchDriver_h
