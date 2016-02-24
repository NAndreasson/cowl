/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/Role.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDocument.h"

struct JSContext;

namespace mozilla {
namespace dom {

  class Privilege;

typedef nsTArray<RefPtr<mozilla::dom::Role> > RoleArray;

class Label final : public nsISupports
                      , public nsWrapperCache
{
  friend class Privilege;
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Label)

protected:
  ~Label();

public:
  Label();
  Label(mozilla::dom::Role &role, ErrorResult &aRv);


  Label* GetParentObject() const;//FIXME

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<Label> Constructor(const GlobalObject& global,
                                             ErrorResult& aRv);
  static already_AddRefed<Label> Constructor(const GlobalObject& global,
                                             const nsAString& principal,
                                             ErrorResult& aRv);

  bool Equals(mozilla::dom::Label& other);

  bool Subsumes(mozilla::dom::Label& other, const Optional<NonNull<Privilege>>& priv) {
      return Subsumes(*static_cast<const mozilla::dom::Label*>(&other), priv);
  }
  bool Subsumes(const mozilla::dom::Label& other, const Optional<NonNull<Privilege>>& priv);
  bool Subsumes(const mozilla::dom::Label& other);

  already_AddRefed<Label> And(const nsAString& principal, ErrorResult& aRv);
  already_AddRefed<Label> And(mozilla::dom::Role& role, ErrorResult& aRv);
  already_AddRefed<Label> And(mozilla::dom::Label& other, ErrorResult& aRv);

  already_AddRefed<Label> Or(const nsAString& principal, ErrorResult& aRv);
  already_AddRefed<Label> Or(mozilla::dom::Role& role, ErrorResult& aRv);
  already_AddRefed<Label> Or(mozilla::dom::Label& other, ErrorResult& aRv);

  // TODO: add a version that returns Label
  void Reduce(mozilla::dom::Label &label);

  bool IsEmpty() const;

  already_AddRefed<Label> Clone(ErrorResult &aRv) const;

  void Stringify(nsString& retval);

public: // C++ only:
  // bool Subsumes(nsIPrincipal* priv, const mozilla::dom::Label& other);
  bool Subsumes(const mozilla::dom::Role& role, const mozilla::dom::Label& other);
  bool Subsumes(const mozilla::dom::Label& privs, const mozilla::dom::Label& other);

  already_AddRefed<Label> Downgrade(mozilla::dom::Label& privilegeLabel);


  //TODO: const these:
  void _And(nsIPrincipal *p, ErrorResult& aRv);
  void _And(mozilla::dom::Role& role, ErrorResult& aRv);
  void _And(mozilla::dom::Label& label, ErrorResult& aRv);
  void _Or(mozilla::dom::Role& role, ErrorResult& aRv);
  void _Or(mozilla::dom::Label& label, ErrorResult& aRv);


  // Get principal if label is singleton
  // already_AddRefed<nsIPrincipal> GetPrincipalIfSingleton() const;
  PrincipalArray* GetPrincipalsIfSingleton() const;

private:

  void InternalAnd(mozilla::dom::Role& role, ErrorResult* aRv = nullptr,
                   bool clone = false);

public: // XXX TODO make private, unsafe
  RoleArray* GetDirectRoles()
  {
    return &mRoles;
  }

private:
  RoleArray mRoles;

};

// Comparator helpers

class RoleComparator {

public:
  bool Equals(const RefPtr<Role> &r1,
              const RefPtr<Role> &r2) const
  {
    return r1->Equals(*r2);
  }
};

class RoleSubsumeComparator {

public:
  bool Equals(const RefPtr<Role> &r1,
              const RefPtr<Role> &r2) const
  {
    return r1->Subsumes(*r2);
  }
};

class RoleSubsumeInvComparator {

public:
  bool Equals(const RefPtr<Role> &r1,
              const RefPtr<Role> &r2) const
  {
    return r2->Subsumes(*r1);
  }
};

} // namespace dom
} // namespace mozilla
