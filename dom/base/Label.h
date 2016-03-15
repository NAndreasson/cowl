/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_Label_h
#define mozilla_dom_Label_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "mozilla/dom/COWLParser.h"

struct JSContext;

namespace mozilla {
namespace dom {

  class Privilege;

// TODO
// subclass, split into origin etc, then perform normalization and so on here...
class COWLPrincipal
{
public:
  COWLPrincipal(const nsAString& principal, COWLPrincipalType principalType)
    : mPrincipal(principal), mPrincipalType(principalType)
  {
  }

  void Stringify(nsString& retval) const
  {
    retval.Append(mPrincipal);
  }


  bool IsOriginPrincipal() const
  {
    return mPrincipalType == COWLPrincipalType::ORIGIN_PRINCIPAL;
  }

private:
  nsString mPrincipal;
  COWLPrincipalType mPrincipalType;

};

typedef nsTArray<COWLPrincipal> DisjunctionSet;
typedef nsTArray<DisjunctionSet> DisjunctionSetArray;

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
  Label(DisjunctionSet &dset, ErrorResult &aRv);


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
  already_AddRefed<Label> And(DisjunctionSet& role, ErrorResult& aRv);
  already_AddRefed<Label> And(mozilla::dom::Label& other, ErrorResult& aRv);

  already_AddRefed<Label> Or(const nsAString& principal, ErrorResult& aRv);
  already_AddRefed<Label> Or(DisjunctionSet& role, ErrorResult& aRv);
  already_AddRefed<Label> Or(mozilla::dom::Label& other, ErrorResult& aRv);

  // TODO: add a version that returns Label
  void Reduce(mozilla::dom::Label &label);

  bool IsEmpty() const;

  already_AddRefed<Label> Clone(ErrorResult &aRv) const;

  void Stringify(nsString& retval);

public: // C++ only:
  // bool Subsumes(nsIPrincipal* priv, const mozilla::dom::Label& other);
  bool Subsumes(const DisjunctionSet& role, const mozilla::dom::Label& other);
  bool Subsumes(const mozilla::dom::Label& privs, const mozilla::dom::Label& other);

  already_AddRefed<Label> Downgrade(mozilla::dom::Label& privilegeLabel);


  //TODO: const these:
  void _And(nsIPrincipal *p, ErrorResult& aRv);
  void _And(DisjunctionSet& role, ErrorResult& aRv);
  void _And(mozilla::dom::Label& label, ErrorResult& aRv);
  void _Or(DisjunctionSet& role, ErrorResult& aRv);
  void _Or(mozilla::dom::Label& label, ErrorResult& aRv);

  void Stringify(DisjunctionSet& dset, nsString& retval);

private:

  void InternalAnd(DisjunctionSet& role, ErrorResult* aRv = nullptr,
                   bool clone = false);

public: // XXX TODO make private, unsafe
  DisjunctionSetArray* GetDirectRoles()
  {
    return &mRoles;
  }

private:
  DisjunctionSetArray mRoles;

};

// Util functions
class DisjunctionSetUtils {

public:
  static DisjunctionSet ConstructDset();
  static DisjunctionSet ConstructDset(COWLPrincipal& principal);
  static DisjunctionSet CloneDset(const DisjunctionSet& dset);

  static void Or(DisjunctionSet& dset1, DisjunctionSet& dset2);

  static void Stringify(const DisjunctionSet& dset, nsString& retval);

  static bool ContainsOriginPrincipal(const DisjunctionSet& dset);

  static bool Subsumes(const DisjunctionSet& dset1, const DisjunctionSet& dset2);
  static bool Equals(const DisjunctionSet& dset1, const DisjunctionSet& dset2);

};

class COWLPrincipalUtils {

public:
  static COWLPrincipal ConstructPrincipal(const nsAString& principal, ErrorResult& aRv);
  static COWLPrincipal ConstructPrincipal(nsIPrincipal *principal, ErrorResult& aRv);

private:
  static COWLPrincipal ConstructOriginPrincipal(const nsAString& principal);

};

// Comparator helpers

class RoleComparator {

public:
  bool Equals(const DisjunctionSet& r1,
              const DisjunctionSet& r2) const
  {
    return DisjunctionSetUtils::Equals(r1, r2);
  }
};

class RoleSubsumeComparator {

public:
  bool Equals(const DisjunctionSet& r1,
              const DisjunctionSet& r2) const
  {
    return DisjunctionSetUtils::Subsumes(r1, r2);
  }
};

class RoleSubsumeInvComparator {

public:
  bool Equals(const DisjunctionSet& r1,
              const DisjunctionSet& r2) const
  {
    return DisjunctionSetUtils::Subsumes(r2, r1);
  }
};

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
